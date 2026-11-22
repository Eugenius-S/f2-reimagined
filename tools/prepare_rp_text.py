"""Build RP 2.4.34 text variants locally; never commit the resulting DATs."""
import argparse
import json
import re
from collections import Counter
from pathlib import Path

from prepare_russian_packs import Dat2, digest, validate_msg, write_checked

RP_HASH = "27db830e0fb67d02b60e166e2acb772a92292f5f4dd88f733b740b406a0864ee"
RU_HASH = "f32d9d0b2696a5066a05a13e431c8dfc9d0772c058932121209049c374d8a8c7"
BASE_HASH = "9b096d3035edafd4077deeb8ee7877a803b9db98497aa4596624b5c058a84711"
SOURCES = {
    'steam-1c-text.dat': 'a89666c64983553b55d042b8a24b46f07ebc8ab782e7e6b1514d902a71d9d5f6',
    'fio-fargus-text.dat': 'f6ad417e5f78c6498c22458fed0d14720fefc2a5ded5edc2575b826aa1d439de',
    'fio-levy-text.dat': '1b735481a1c82a1c4ef320ed34c25e1b937ecfe218ce6ace0389efad3a572b9b',
}
ENTRY = re.compile(rb"\{([+-]?\d+)\}\s*\{([^{}]*)\}\s*\{([^{}]*)\}")
CONTROL = re.compile(rb"%(?:\d+\$)?[-+#0 .\d*hlL]*[a-zA-Z%]|\\[a-zA-Z]|\|[a-zA-Z0-9_]+")


def entries(raw):
    validate_msg(raw)
    matches = list(ENTRY.finditer(raw))
    values = {}
    for m in matches:
        key = int(m.group(1))
        if key in values:
            raise ValueError("duplicate message ID")
        values[key] = m
    if not matches or len(matches) * 3 != len(re.findall(rb"\{[^{}]*\}", raw)):
        raise ValueError("ambiguous message format")
    return values


def controls(text):
    return CONTROL.findall(text)


def build(base, rp, official, variant):
    result = {n: official.read(n) for n in official.names() if n.startswith("text/russian/")}
    counts = Counter()
    coverage = {}
    for name, raw in list(result.items()):
        if not name.endswith('.msg') or not variant.has(name):
            continue
        en = name.replace('text/russian/', 'text/english/', 1)
        if not base.has(en):
            continue
        try:
            current, translated = entries(raw), entries(variant.read(name))
            vanilla = entries(base.read(en))
            effective = entries(rp.read(en)) if rp.has(en) else vanilla
        except ValueError:
            counts['ambiguous_files'] += 1
            continue
        replacements = []
        ids = []
        for key, dest in current.items():
            v, e, t = vanilla.get(key), effective.get(key), translated.get(key)
            if not v or not e or not t:
                continue
            if e.group(3) != v.group(3):
                counts['changed_english'] += 1
                continue
            if controls(dest.group(3)) != controls(t.group(3)) or controls(e.group(3)) != controls(t.group(3)):
                counts['control_mismatch'] += 1
                continue
            if dest.group(3) == t.group(3):
                continue
            replacements.append((dest.start(3), dest.end(3), t.group(3)))
            ids.append(key)
        for start, end, value in reversed(replacements):
            raw = raw[:start] + value + raw[end:]
        validate_msg(raw)
        result[name] = raw
        if ids:
            coverage[name] = ids
            counts['replaced'] += len(ids)
    return result, counts, coverage


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('root', type=Path, help='translation-packs directory')
    p.add_argument('output', type=Path, help='new directory outside git')
    args = p.parse_args()
    if args.output.exists() or args.output.resolve().is_relative_to(Path(__file__).resolve().parent.parent):
        raise SystemExit('Output must be a new directory outside the repository')
    proof = args.root / 'proof'
    rp = proof / 'rpu-2.4.34/mods/rpu.dat'
    ru = proof / 'rpu-2.4.34/mods/rpu_russian.dat'
    if digest(rp) != RP_HASH or digest(ru) != RU_HASH:
        raise SystemExit('RP 2.4.34 reference hash mismatch')
    if digest(proof / 'vanilla/master.dat') != BASE_HASH or any(
            digest(args.root / 'prepared-v4' / name) != sha for name, sha in SOURCES.items()):
        raise SystemExit('Vanilla or translation reference hash mismatch')
    args.output.mkdir(parents=True)
    report = {'rp_sha256': RP_HASH, 'official_sha256': RU_HASH, 'variants': {}}
    with Dat2(proof / 'vanilla/master.dat') as base, Dat2(rp) as rpu, Dat2(ru) as official:
        for label, source in [('1c', 'steam-1c-text.dat'), ('fargus', 'fio-fargus-text.dat'), ('levy', 'fio-levy-text.dat')]:
            with Dat2(args.root / 'prepared-v4' / source) as variant:
                result, counts, coverage = build(base, rpu, official, variant)
            info = write_checked(result, args.output / ('rp-' + label + '-text.dat'))
            del info['members_sha256']
            report['variants'][label] = {'source_sha256': digest(args.root / 'prepared-v4' / source), **info,
                                         'counts': dict(counts), 'replaced_ids': coverage}
    (args.output / 'coverage.json').write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    print(json.dumps({k: {x: v[x] for x in ('size', 'sha256', 'files', 'counts')} for k, v in report['variants'].items()}, indent=2))


if __name__ == '__main__':
    main()
