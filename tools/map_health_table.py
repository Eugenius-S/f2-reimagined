"""The all-map health table from a map-health.ps1 state file (271).

Reads the sweep's state.jsonl (one JSON line per finished attempt, last line
per map elevation wins) and writes the table: every map and elevation on one
build, sorted worst first, with the flag lists the task card asks for (rows
over the 2000 ms load gate, under the 80 fps 1% low bar, with a 2D fallback
line, over 20% black pixels). Rows that are not valid results stay in their
own section: TIMEOUT, FAILED and NOISY rows never count as the sweep's
overall numbers.

The black share is computed from the row's snapshot PNG (pixels whose
brightest channel is under 8 in 0..255); a row without a readable snapshot
says n/a instead of a made-up number. All rows must carry the same
exe_sha256: a table mixing builds lies about the game, so it refuses.
"""

import argparse
import json
import sys
from pathlib import Path

try:
    from PIL import Image
except ImportError:  # pragma: no cover - the black share needs Pillow
    Image = None

LOAD_GATE_MS = 2000.0
ONE_LOW_BAR_FPS = 80.0
BLACK_FLAG_PERCENT = 20.0
BLACK_CHANNEL_MAX = 8


def load_rows(path):
    """The final row per map elevation, in file order (later lines win)."""
    rows = {}
    order = []
    text = Path(path).read_text(encoding='utf-8-sig')
    for line in text.splitlines():
        line = line.strip()
        if not line:
            continue
        row = json.loads(line)
        key = (row['map'], int(row['elevation']))
        if key not in rows:
            order.append(key)
        rows[key] = row
    return [rows[key] for key in order]


def compute_black_share(png_path):
    """The share of near-black pixels in 0..100, or None without a PNG."""
    if Image is None or not png_path or not Path(png_path).exists():
        return None
    # max(r, g, b) per pixel, at C speed: lighten the three channels into
    # one, then read its 256-bin histogram.
    with Image.open(png_path) as image:
        red, green, blue = image.convert('RGB').split()
        import PIL.ImageChops as ImageChops
        brightest = ImageChops.lighter(ImageChops.lighter(red, green), blue)
        histogram = brightest.histogram()
    black = sum(histogram[:BLACK_CHANNEL_MAX])
    total = sum(histogram)
    return round(100.0 * black / max(total, 1), 1)


def flags(row, black_share):
    """The reason strings a row earns, in card order."""
    found = []
    if row.get('loadMs') is not None and float(row['loadMs']) > LOAD_GATE_MS:
        found.append(f"load {row['loadMs']} ms over {LOAD_GATE_MS:.0f}")
    if row.get('oneLowFps') is not None and float(row['oneLowFps']) < ONE_LOW_BAR_FPS:
        found.append(f"1% low {row['oneLowFps']} fps under {ONE_LOW_BAR_FPS:.0f}")
    if row.get('fallback'):
        found.append('2D fallback line: ' + '; '.join(row['fallback']))
    if black_share is not None and black_share > BLACK_FLAG_PERCENT:
        found.append(f"black {black_share}% over {BLACK_FLAG_PERCENT:.0f}%")
    if row.get('scene_3d_enabled') is False:
        found.append('the captured frame is not 3D (scene_3d_enabled false)')
    return found


def sort_key(row, black_share):
    """Worst first: invalid rows (timeout, failed, noisy) never play the
    numbers game, so they lead the table; valid rows follow by the weakest
    1% low, ties by the blackest frame, then the longest load."""
    valid = row.get('status') == 'ok' and not row.get('noisy')
    if not valid:
        return (0, float('inf'), -1.0, 0.0, row['map'], row['elevation'])
    one_low = float(row['oneLowFps']) if row.get('oneLowFps') is not None else float('inf')
    black = black_share if black_share is not None else 0.0
    return (1, one_low, -black, -(row.get('loadMs') or 0), row['map'], row['elevation'])


def build_table(rows, commit, out):
    """The markdown table; returns (valid_rows, flagged_rows) counts."""
    builds = {row.get('exe_sha256') for row in rows}
    if len(builds) > 1:
        raise SystemExit('the state rows carry more than one exe_sha256: '
                         'a table must be one build. Sweep the missing rows '
                         'on the current build and drop the stale ones.')
    exe_sha = builds.pop() if builds else ''

    shares = {}
    for row in rows:
        if row.get('black_share') is not None:
            shares[(row['map'], row['elevation'])] = float(row['black_share'])
        else:
            share = compute_black_share(row.get('snapshot'))
            if share is not None:
                shares[(row['map'], row['elevation'])] = share

    ordered = sorted(rows, key=lambda row: sort_key(row, shares.get((row['map'], row['elevation']))))

    valid = [row for row in ordered if row.get('status') == 'ok' and not row.get('noisy')]
    invalid = [row for row in ordered if not (row.get('status') == 'ok' and not row.get('noisy'))]
    flagged = [row for row in valid if flags(row, shares.get((row['map'], row['elevation'])))]

    lines = []
    lines.append('# All-map health table (271)')
    lines.append('')
    lines.append(f'One command (tools/map-health.ps1) walked every playable map and elevation.')
    lines.append(f'Build: exe sha256 `{exe_sha[:16] if exe_sha else "?"}`, main commit `{commit or "?"}`.')
    lines.append('')
    valid_n = len(valid)
    clean = len([row for row in valid if not flags(row, shares.get((row['map'], row['elevation'])))])
    total = len(rows)
    lines.append(f'{valid_n} of {total} rows are valid results '
                 f'(one warmup and one measured run each); {clean} of them breach nothing. '
                 f'{len(invalid)} rows are not valid results and never count in the numbers above a table row.')
    lines.append('')
    lines.append('Sorted worst first: the weakest 1% low at the top; TIMEOUT, FAILED and NOISY '
                 'rows first of all, because they say nothing about the game.')
    lines.append('')
    lines.append('| map | elev | avg fps | 1% low | worst ms | load ms | black % | fallback | flags | cpu | status |')
    lines.append('|---|---|---|---|---|---|---|---|---|---|---|')
    for row in ordered:
        key = (row['map'], row['elevation'])
        share = shares.get(key)
        share_text = f"{share:.1f}" if share is not None else 'n/a'
        fallback = 'yes' if row.get('fallback') else ''
        reason = '; '.join(flags(row, shares.get(key)))
        cpu = row.get('cpu')
        cpu_text = str(cpu) if cpu is not None else '?'
        note = row.get('error', '')
        status = row.get('status', '?')
        if status == 'ok' and row.get('noisy'):
            status = 'noisy'
        lines.append('| {0} | {1} | {2} | {3} | {4} | {5} | {6} | {7} | {8} | {9} | {10} |'.format(
            row['map'], row['elevation'],
            row.get('avgFps', '?'), row.get('oneLowFps', '?'), row.get('worstMs', '?'),
            row.get('loadMs', '?'), share_text, fallback,
            reason if reason else note,
            cpu_text, status))
    lines.append('')

    lines.append('## Flagged valid rows')
    lines.append('')
    groups = [
        ('load over {0:.0f} ms'.format(LOAD_GATE_MS),
         lambda row, share: row.get('loadMs') is not None and float(row['loadMs']) > LOAD_GATE_MS),
        ('1% low under {0:.0f} fps'.format(ONE_LOW_BAR_FPS),
         lambda row, share: row.get('oneLowFps') is not None and float(row['oneLowFps']) < ONE_LOW_BAR_FPS),
        ('with a 2D fallback line', lambda row, share: bool(row.get('fallback'))),
        ('black over {0:.0f}%'.format(BLACK_FLAG_PERCENT),
         lambda row, share: share is not None and share > BLACK_FLAG_PERCENT),
    ]
    for title, test in groups:
        members = [row for row in valid if test(row, shares.get((row['map'], row['elevation'])))]
        if members:
            names = ', '.join('{0}:e{1}'.format(row['map'], row['elevation']) for row in members)
            lines.append(f'- {title}: {names}')
        else:
            lines.append(f'- {title}: none')
    if invalid:
        lines.append('')
        lines.append('## Not valid results')
        lines.append('')
        for row in invalid:
            why = row.get('error') or ('measured under CPU load over the noisy line' if row.get('noisy') else '')
            lines.append('- {0}:e{1} {2} after {3} attempts: {4}'.format(
                row['map'], row['elevation'], row.get('status', '?'), row.get('attempts', '?'), why))

    out.write_text('\n'.join(lines) + '\n', encoding='utf-8')
    return len(valid), len(flagged)


def main(argv=None):
    parser = argparse.ArgumentParser(description='Build the 271 all-map health table from a state file.')
    parser.add_argument('--state', required=True, help='map-health.ps1 state.jsonl')
    parser.add_argument('--out', required=True, help='the markdown table to write')
    parser.add_argument('--commit', default='', help='the main commit the build was made from')
    args = parser.parse_args(argv)
    rows = load_rows(args.state)
    valid, flagged = build_table(rows, args.commit, Path(args.out))
    print(f'map-health-table: {len(rows)} rows, {valid} valid, {flagged} flagged -> {args.out}')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
