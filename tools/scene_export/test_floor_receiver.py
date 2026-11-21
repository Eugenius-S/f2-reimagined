import json
import tempfile
import unittest
from pathlib import Path
from types import SimpleNamespace

from floor_receiver import FILENAME, HEADER, floor_receiver, write_floor_receiver

LAYOUT = json.loads(Path(__file__).with_name("floor_receivers.json").read_text(encoding="utf-8"))


class FloorReceiverTests(unittest.TestCase):
    def setUp(self):
        spec = LAYOUT["artemple:0"]
        self.objects = [SimpleNamespace(fid=spec["artFile"], tile=spec["tile"])]
        self.floor = [0] * 10000
        for u, v, tile in spec["floorGuards"]:
            self.floor[v * 100 + u] = tile

    def encode(self, map_name="artemple", elevation=0):
        return floor_receiver(LAYOUT, map_name, elevation, self.objects, self.floor)

    def test_matches_the_real_art_and_excludes_nearby_sand(self):
        payload = self.encode()
        magic, art, tile, x, y, w, h = HEADER.unpack_from(payload)
        self.assertEqual((magic, art, tile, x, y, w, h),
                         (b"F2FR", 0x0200055D, 16491, 3694, 170, 740, 340))
        self.assertEqual(len(payload), HEADER.size + w*h)
        coverage = lambda x, y: payload[HEADER.size + y*w + x]
        self.assertGreater(coverage(380, 221), 240)  # stair
        self.assertGreater(coverage(168, 239), 240)  # left foot
        self.assertGreater(coverage(542, 176), 240)  # right foot
        self.assertEqual(coverage(333, 308), 0)      # sand in front
        self.assertEqual(coverage(668, 179), 0)      # sand at the side

    def test_incompatible_reexport_removes_the_old_receiver_only(self):
        with tempfile.TemporaryDirectory() as temp:
            out = Path(temp)
            (out / "floor.bin").write_bytes(b"floor is separate")
            write_floor_receiver(out, self.encode())
            self.assertTrue((out / FILENAME).is_file())
            self.floor[42*100+47] ^= 1
            write_floor_receiver(out, self.encode())
            self.assertFalse((out / FILENAME).exists())
            self.assertEqual((out / "floor.bin").read_bytes(), b"floor is separate")

    def test_unowned_or_changed_tiles_never_receive_a_group(self):
        self.assertIsNone(self.encode("arcaves"))
        self.assertIsNone(self.encode(elevation=1))
        self.objects.append(self.objects[0])
        self.assertIsNone(self.encode())
        self.objects.pop()
        self.objects[0].tile -= 1
        self.assertIsNone(self.encode())
        self.objects[0].tile += 1
        self.floor[42*100+47] ^= 1
        self.assertIsNone(self.encode())


if __name__ == "__main__":
    unittest.main()
