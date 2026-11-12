"""Embed the owner-supplied settings frame PNGs without changing the originals."""
from pathlib import Path


NAMES = ('topLeft', 'top', 'topRight', 'left', 'right', 'bottomLeft', 'bottom', 'bottomRight')


def build_header(source: Path) -> str:
    lines = ['// Generated from the owner supplied PNG files in assets/options-frame-owner-133.',
             '// Regenerate with tools/generate_options_frame.py.',
             '#ifndef FALLOUT_SCENE_OPTIONS_FRAME_OWNER_H',
             '#define FALLOUT_SCENE_OPTIONS_FRAME_OWNER_H',
             'namespace fallout { namespace scene { namespace owner_frame {']
    for name in NAMES:
        data = (source / (name + '.png')).read_bytes()
        lines.append('static constexpr unsigned char ' + name + '[] = {')
        lines.extend('    ' + ','.join(str(x) for x in data[i:i + 24]) + ','
                     for i in range(0, len(data), 24))
        lines.append('};')
    lines.extend(['} } }', '#endif'])
    return '\n'.join(lines) + '\n'


def write_header(source: Path, output: Path) -> None:
    output.write_text(build_header(source), encoding='utf-8')


if __name__ == '__main__':
    root = Path(__file__).resolve().parents[1]
    write_header(root / 'assets/options-frame-owner-133',
                 root / 'ce/src/scene/options_frame_owner.h')
