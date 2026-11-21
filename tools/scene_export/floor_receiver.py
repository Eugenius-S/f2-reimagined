"""Optional authored native floor pixels that receive a sprite group's sun shade.

Inputs contain only geometry. An exact object and floor-ART guards keep the
patch off incompatible map variants. Ordinary floor never receives a group.
"""
import struct
from PIL import Image, ImageDraw, ImageFilter

HEADER = struct.Struct("<4sIHhhHH")
FILENAME = "floor-receiver.bin"


def floor_receiver(layout, map_name, elevation, objects, floor_ids):
    spec = layout.get(f"{map_name}:{elevation}")
    if spec is None:
        return None
    art, tile = spec["artFile"], spec["tile"]
    x, y, width, height = spec["bounds"]
    if (art < 0 or art & 0x0F000FFF != art or not 0 <= tile < 40000
            or not -32768 <= x < 32768 or not -32768 <= y < 32768
            or not 0 < width <= 2048 or not 0 < height <= 2048):
        raise ValueError("invalid floor receiver identity or bounds")
    if sum((int(obj.fid) & 0x0F000FFF) == art and obj.tile == tile for obj in objects) != 1:
        return None
    if not spec["floorGuards"]:
        raise ValueError("floor receiver needs at least one floor-art guard")
    for u, v, expected in spec["floorGuards"]:
        if not (0 <= u < 100 and 0 <= v < 100 and 0 <= expected <= 4095):
            raise ValueError("invalid floor-art guard")
        if int(floor_ids[v * 100 + u]) != expected:
            return None
    mask = Image.new("L", (width, height))
    brush = ImageDraw.Draw(mask)
    for polygon in spec["polygons"]:
        if len(polygon) < 3 or any(not (0 <= px < width and 0 <= py < height) for px, py in polygon):
            raise ValueError("invalid floor receiver polygon")
        brush.polygon([tuple(point) for point in polygon], fill=255)
    if mask.getbbox() is None:
        raise ValueError("empty floor receiver")
    mask = mask.filter(ImageFilter.GaussianBlur(1.0))
    return HEADER.pack(b"F2FR", art, tile, x, y, width, height) + mask.tobytes()


def write_floor_receiver(out_dir, payload):
    path = out_dir / FILENAME
    if payload is None:
        path.unlink(missing_ok=True)
    else:
        path.write_bytes(payload)
