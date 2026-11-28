"""The minimal synthetic game tree the fixture export runs against (297).

Everything here is generated from the f2lib writers: no game data, no art
from the owner, nothing copyrighted - a grey ramp palette, one 80x36 tile
FRM of deterministic pixels, and a floor-only map whose every square uses
that one tile (the raw square value 0 is tile 0; the exporter treats a raw
value of 1 as NO_TILE, so zero is the right full-floor fill).

Layout (all loose files; the two DAT archives exist only because the CE
resource order requires them and carry nothing):

    fallout2.cfg                    system master_dat/critter_dat point at the archives
    master.dat, critter.dat         minimal empty DAT2 archives
    color.pal                       768-byte palette + the 32768-byte RGB555 table
    scripts/scripts.lst             empty (the map has no script; game.cc-style lookup
                                    happens through this list)
    art/tiles/tiles.lst             one entry: fixture_tile.frm
    art/tiles/fixture_tile.frm      the tile
    maps/fixture.map                elevation 0, SQUARE_COUNT squares all tile 0, no objects

check_fixture() is the bundled-resource precondition the smoke run asserts
before any export: a missing file here fails with its path instead of a
halfway export.
"""

import numpy as np

from f2lib import dat2, frm as frm_lib, geometry, map as map_lib

FIXTURE_MAP_NAME = "fixture"
MAP_FILE_NAME = "maps/fixture.map"
TILE_ART_NAME = "fixtile.frm"  # art LST entries keep at most 12 characters
TILE_ART_DIR = "art/tiles"

# Every file a complete fixture carries (relative to the game root). The
# smoke run checks this list before it exports anything.
BUNDLED_RESOURCES = (
    "fallout2.cfg",
    "master.dat",
    "critter.dat",
    "color.pal",
    "scripts/scripts.lst",
    "art/tiles/tiles.lst",
    f"{TILE_ART_DIR}/{TILE_ART_NAME}",
    MAP_FILE_NAME,
)

CFG_BODY = (
    "[system]\n"
    "master_dat=master.dat\n"
    "critter_dat=critter.dat\n"
    "critter_patches=\n"
    "master_patches=\n"
    "language=english\n"
)


def _tile_frm_bytes(width=80, height=36):
    """One-direction, one-frame tile FRM of deterministic grey pixels."""
    image = frm_lib.Frm()
    image.stored = [[frm_lib.Frame(width, height, bytes((i * 5) % 251 for i in range(width * height)))]]
    image.direction_map = [0] * 6
    return image.to_bytes()


def _color_pal_bytes():
    """768 palette bytes (all channels 0..63, a grey ramp) + the empty LUT."""
    palette = bytes((i * 7) % 64 for i in range(768))
    return palette + bytes(32768)


def build_fixture(root):
    """Create the whole synthetic game tree under `root` (created if needed)."""
    root.mkdir(parents=True, exist_ok=True)
    (root / "fallout2.cfg").write_text(CFG_BODY, encoding="ascii")
    # The two required CE DAT sources: minimal valid, entirely empty archives.
    dat2.write_dat2({}, str(root / "master.dat"))
    dat2.write_dat2({}, str(root / "critter.dat"))
    (root / "color.pal").write_bytes(_color_pal_bytes())

    (root / "scripts").mkdir(exist_ok=True)
    (root / "scripts" / "scripts.lst").write_bytes(b"")

    (root / TILE_ART_DIR).mkdir(parents=True, exist_ok=True)
    (root / TILE_ART_DIR / "tiles.lst").write_text(TILE_ART_NAME + "\n", encoding="ascii")
    (root / TILE_ART_DIR / TILE_ART_NAME).write_bytes(_tile_frm_bytes())

    (root / "maps").mkdir(exist_ok=True)
    game_map = map_lib.MapFile()
    game_map.name_raw = b"FIXTURE.MAP".ljust(16)
    game_map.entering_tile = 20100
    game_map.entering_elevation = 0
    game_map.script_index = 0
    game_map.tiles[0] = np.zeros(geometry.SQUARE_COUNT, dtype=np.uint32)
    game_map.save(str(root / MAP_FILE_NAME))
    return root


def check_fixture(root):
    """Failures for an incomplete fixture: one line per missing resource."""
    failures = []
    for relative in BUNDLED_RESOURCES:
        if not (root / relative).is_file():
            failures.append(f"missing fixture resource: {relative}")
    return failures
