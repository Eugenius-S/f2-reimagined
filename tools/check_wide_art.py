"""Validate every required wide-art file of a release payload before upload (299).

The required set is the actual consumers and the existing permitted release
payload (the owner's wide-art.zip release), nothing invented:

- menu: the hires menu PNG the main menu and the launcher both load
  (ce/src/mainmenu.cc, ce/src/launcher_uninstall.h);
- death: the death-screen PNG (ce/src/main.cc);
- loading splash, English and Russian variants already shipped:
  data/art/splash/splash0..6.rix and data/art/russian/splash/splash0..6.rix
  (ce/src/game.cc reads splash<N>.rix, ten are tried, the payload ships seven
  per language and the index wraps, so seven is complete for play).

Every file must exist and decode: a PNG through Pillow, a RIX through the
game's own format (RIX3 magic, short width and height, 768-byte palette at
offset 10, width*height indexed pixels). All failures are reported with
their relative paths; the exit code is nonzero when anything fails.
"""

import argparse
import struct
import sys
from pathlib import Path

WIDE_ART_FILES = (
    "user-assets/fullscreen-20261028/wallpapers-selected/hires/menu/"
    "art__intrface__hr_mainmenu__d0_f0_21-9.png",
    "user-assets/fullscreen-20261028/wallpapers-selected/common/death/death.png",
) + tuple(
    f"data/art/splash/splash{i}.rix" for i in range(7)
) + tuple(
    f"data/art/russian/splash/splash{i}.rix" for i in range(7)
)

RIX_MAGIC = b"RIX3"
RIX_HEADER_SIZE = 10
RIX_PALETTE_SIZE = 768


def decode_png(path):
    """Decode a PNG through Pillow; returns an error string or None."""
    try:
        from PIL import Image
    except ImportError:
        return "Pillow is not available"
    try:
        with Image.open(path) as image:
            image.verify()
        with Image.open(path) as image:
            image.load()
    except Exception as error:  # noqa: BLE001 - any decode failure is the row's message
        return f"PNG does not decode: {error}"
    return None


def decode_rix(path):
    """Decode a RIX3 splash the way the game does; returns an error or None."""
    data = Path(path).read_bytes()
    if len(data) < RIX_HEADER_SIZE or data[:4] != RIX_MAGIC:
        return "not a RIX3 file (bad magic or truncated header)"
    width, height = struct.unpack_from("<hh", data, 4)
    if width <= 0 or height <= 0:
        return f"bad dimensions {width}x{height}"
    needed = RIX_HEADER_SIZE + RIX_PALETTE_SIZE + width * height
    if len(data) < needed:
        return f"truncated: {len(data)} bytes, the {width}x{height} image needs {needed}"
    palette = data[RIX_HEADER_SIZE:RIX_HEADER_SIZE + RIX_PALETTE_SIZE]
    pixels = data[RIX_HEADER_SIZE + RIX_PALETTE_SIZE:needed]
    if len(pixels) != width * height:
        return "pixel data does not match the dimensions"
    # A full decode: indexed pixels through the palette into RGBA bytes.
    rgba = bytearray(width * height * 4)
    for i, index in enumerate(pixels):
        rgba[i * 4:i * 4 + 3] = palette[index * 3:index * 3 + 3]
        rgba[i * 4 + 3] = 255
    if len(rgba) != width * height * 4:
        return "decode produced the wrong size"
    return None


def validate_payload(payload_root):
    """The failure list for a payload directory, one string per bad file."""
    failures = []
    root = Path(payload_root)
    for relative in WIDE_ART_FILES:
        path = root / relative
        if not path.is_file():
            failures.append(f"missing: {relative}")
            continue
        error = decode_png(path) if path.suffix.lower() == ".png" else decode_rix(path)
        if error:
            failures.append(f"undecodable: {relative} ({error})")
    return failures


def make_fixture(payload_root):
    """A minimal complete payload: real PNGs and real RIX3 splash files."""
    from PIL import Image

    root = Path(payload_root)
    for relative in WIDE_ART_FILES:
        path = root / relative
        path.parent.mkdir(parents=True, exist_ok=True)
        if path.suffix.lower() == ".png":
            Image.new("RGB", (16, 9), (32, 32, 32)).save(path)
        else:
            width, height = 8, 6
            palette = bytes(range(256)) * 3
            pixels = bytes((i * 7) % 256 for i in range(width * height))
            path.write_bytes(RIX_MAGIC + struct.pack("<hh", width, height)
                             + b"\x00\x00" + palette + pixels)


def self_test():
    import tempfile

    failed = False

    def check(ok, what):
        nonlocal failed
        if not ok:
            print(f"selftest: FAIL {what}")
            failed = True

    base = Path(tempfile.mkdtemp(prefix="f2r-wide-art-selftest-"))

    # The complete fixture passes.
    payload = base / "full"
    make_fixture(payload)
    check(validate_payload(payload) == [], "the complete fixture passes")

    # Deleting one required file from each group fails with its path.
    partial = base / "partial"
    make_fixture(partial)
    removed = [
        WIDE_ART_FILES[0],
        WIDE_ART_FILES[1],
        WIDE_ART_FILES[5],                       # an English splash
        WIDE_ART_FILES[len(WIDE_ART_FILES) - 1],  # a Russian splash
    ]
    for relative in removed:
        (partial / relative).unlink()
    failures = validate_payload(partial)
    check(len(failures) == len(removed), f"exactly the removed files fail, got {failures}")
    for relative in removed:
        check(any(relative in f for f in failures), f"the failure names {relative}")

    # A truncated RIX and a truncated PNG fail with their paths.
    truncated = base / "truncated"
    make_fixture(truncated)
    rix = truncated / WIDE_ART_FILES[2]
    rix.write_bytes(rix.read_bytes()[:RIX_HEADER_SIZE + 100])
    png = truncated / WIDE_ART_FILES[0]
    png.write_bytes(png.read_bytes()[:20])
    failures = validate_payload(truncated)
    check(len(failures) == 2, f"exactly the truncated files fail, got {failures}")
    check(all(WIDE_ART_FILES[0] in f or WIDE_ART_FILES[2] in f for f in failures),
          "the truncation failures name their paths")

    # A wrong-magic RIX is undecodable, not silently accepted.
    magic = base / "magic"
    make_fixture(magic)
    bad = magic / WIDE_ART_FILES[3]
    data = bytearray(bad.read_bytes())
    data[0:4] = b"XXXX"
    bad.write_bytes(bytes(data))
    failures = validate_payload(magic)
    check(len(failures) == 1 and WIDE_ART_FILES[3] in failures[0],
          f"a wrong magic fails with its path, got {failures}")

    import shutil
    shutil.rmtree(base, ignore_errors=True)
    if failed:
        return 1
    print("selftest: ok")
    return 0


def main(argv=None):
    parser = argparse.ArgumentParser(description="Validate the release's required wide art (299).")
    parser.add_argument("--payload", help="the directory the wide-art archive was unpacked into")
    parser.add_argument("--self-test", action="store_true", help="run the synthetic fixture tests")
    args = parser.parse_args(argv)
    if args.self_test:
        return self_test()
    if not args.payload:
        parser.error("--payload is required (or run --self-test)")
    failures = validate_payload(args.payload)
    for failure in failures:
        print(f"WIDE_ART {failure}")
    if failures:
        print(f"WIDE_ART FAIL {len(failures)} of {len(WIDE_ART_FILES)} required files")
        return 1
    print(f"WIDE_ART PASS all {len(WIDE_ART_FILES)} required files present and decoded")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
