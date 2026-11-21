"""The 271 table parser self-test: rows, sorting, flags, one-build rule."""

import io
import json
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import map_health_table as mht  # noqa: E402

try:
    from PIL import Image
except ImportError:
    Image = None


def write_state(rows):
    handle = tempfile.NamedTemporaryFile('w', suffix='.jsonl', delete=False, encoding='utf-8')
    for row in rows:
        handle.write(json.dumps(row) + '\n')
    handle.close()
    return handle.name


SNAPSHOT = None


def make_snapshot():
    """A real tiny PNG: the evidence check looks at the file itself."""
    global SNAPSHOT
    if SNAPSHOT is None or not Path(SNAPSHOT).exists():
        from PIL import Image
        p = Path(tempfile.gettempdir()) / 'mht-snapshot.png'
        Image.new('RGB', (4, 4), (10, 10, 10)).save(p)
        SNAPSHOT = str(p)
    return SNAPSHOT


class LoadRowsTest(unittest.TestCase):
    def test_last_line_per_row_wins(self):
        path = write_state([
            {'map': 'kladwtwn', 'elevation': 0, 'status': 'failed', 'attempts': 1, 'exe_sha256': 'a'},
            {'map': 'kladwtwn', 'elevation': 0, 'status': 'ok', 'attempts': 2, 'oneLowFps': 70.0, 'exe_sha256': 'a'},
            {'map': 'newr1', 'elevation': 0, 'status': 'ok', 'attempts': 1, 'oneLowFps': 90.0, 'exe_sha256': 'a'},
        ])
        rows = mht.load_rows(path)
        self.assertEqual(len(rows), 2)
        self.assertEqual(rows[0]['status'], 'ok')
        self.assertEqual(rows[0]['attempts'], 2)
        self.assertEqual(rows[1]['map'], 'newr1')

    def test_utf8_bom_state_file_reads(self):
        # PowerShell's Add-Content may open the state file with a BOM.
        handle = tempfile.NamedTemporaryFile('w', suffix='.jsonl', delete=False, encoding='utf-8-sig')
        handle.write(json.dumps({'map': 'kladwtwn', 'elevation': 0, 'status': 'ok',
                                 'attempts': 1, 'oneLowFps': 70.0, 'exe_sha256': 'a'}) + '\n')
        handle.close()
        rows = mht.load_rows(handle.name)
        self.assertEqual(rows[0]['map'], 'kladwtwn')


class FlagsTest(unittest.TestCase):
    def test_card_flags_in_order(self):
        row = {'map': 'newr1', 'elevation': 0, 'status': 'ok', 'loadMs': 2500.0,
               'oneLowFps': 55.0, 'fallback': ['scene: NEWR1 elevation 0 stays 2D: no cache'],
               'scene_3d_enabled': False}
        found = mht.flags(row, 25.0)
        self.assertEqual(len(found), 5)
        self.assertIn('over 2000', found[0])
        self.assertIn('under 80', found[1])
        self.assertIn('stays 2D', found[2])
        self.assertIn('black 25.0%', found[3])
        self.assertIn('not 3D', found[4])

    def test_clean_row_has_no_flags(self):
        row = {'map': 'kladwtwn', 'elevation': 0, 'status': 'ok', 'loadMs': 800.0,
               'oneLowFps': 120.0, 'fallback': [], 'scene_3d_enabled': True}
        self.assertEqual(mht.flags(row, 2.0), [])

    def test_na_black_never_flags(self):
        row = {'map': 'kladwtwn', 'elevation': 0, 'status': 'ok', 'loadMs': 100.0,
               'oneLowFps': 120.0, 'fallback': []}
        self.assertEqual(mht.flags(row, None), [])


class SortKeyTest(unittest.TestCase):
    def test_invalid_then_noisy_then_weakest(self):
        timeout = {'map': 'zz', 'elevation': 0, 'status': 'timeout', 'attempts': 2}
        noisy = {'map': 'aa', 'elevation': 0, 'status': 'ok', 'noisy': True, 'oneLowFps': 10.0}
        weak = {'map': 'mm', 'elevation': 0, 'status': 'ok', 'oneLowFps': 40.0}
        strong = {'map': 'bb', 'elevation': 0, 'status': 'ok', 'oneLowFps': 130.0}
        ordered = sorted([strong, weak, noisy, timeout],
                         key=lambda row: mht.sort_key(row, None))
        # Invalid rows (timeout, noisy) all lead the table, in map-name
        # order; the valid rows follow by the weakest 1% low.
        self.assertEqual([row['map'] for row in ordered], ['aa', 'zz', 'mm', 'bb'])


class BuildTableTest(unittest.TestCase):
    def test_mixed_builds_refuse(self):
        path = write_state([
            {'map': 'kladwtwn', 'elevation': 0, 'status': 'ok', 'exe_sha256': 'a', 'oneLowFps': 90.0},
            {'map': 'newr1', 'elevation': 0, 'status': 'ok', 'exe_sha256': 'b', 'oneLowFps': 90.0},
        ])
        out = Path(tempfile.gettempdir()) / 'mht-mixed.md'
        with self.assertRaises(SystemExit):
            mht.build_table(mht.load_rows(path), 'deadbeef', out)

    def test_mixed_commits_refuse(self):
        # 516 P2: the commit is identity; rows recorded on two commits do not
        # build one table.
        path = write_state([
            {'map': 'kladwtwn', 'elevation': 0, 'status': 'ok', 'exe_sha256': 'a', 'commit': 'aaa', 'oneLowFps': 90.0},
            {'map': 'newr1', 'elevation': 0, 'status': 'ok', 'exe_sha256': 'a', 'commit': 'bbb', 'oneLowFps': 90.0},
        ])
        out = Path(tempfile.gettempdir()) / 'mht-commits.md'
        with self.assertRaises(SystemExit):
            mht.build_table(mht.load_rows(path), '', out)

    def test_relabeling_commit_refused(self):
        # A --commit that contradicts the rows is refused, not stamped over.
        path = write_state([
            {'map': 'kladwtwn', 'elevation': 0, 'status': 'ok', 'exe_sha256': 'a', 'commit': 'aaa', 'oneLowFps': 90.0},
        ])
        out = Path(tempfile.gettempdir()) / 'mht-relabel.md'
        with self.assertRaises(SystemExit):
            mht.build_table(mht.load_rows(path), 'bbb', out)

    def test_row_without_evidence_is_not_valid(self):
        # 516 P2: an ok row without its capture evidence counts nowhere.
        path = write_state([
            {'map': 'x', 'elevation': 0, 'status': 'ok', 'exe_sha256': 'a', 'oneLowFps': 90.0},
        ])
        out = Path(tempfile.gettempdir()) / 'mht-evidence.md'
        valid, flagged = mht.build_table(mht.load_rows(path), '', out)
        self.assertEqual((valid, flagged), (0, 0))
        text = out.read_text(encoding='utf-8')
        self.assertIn('no capture evidence', text)
        self.assertIn('incomplete', text)

    def test_missing_snapshot_file_is_not_valid(self):
        # 516 P2: a snapshot path to nothing is not visual evidence.
        path = write_state([
            {'map': 'x', 'elevation': 0, 'status': 'ok', 'exe_sha256': 'a', 'oneLowFps': 90.0,
             'avgFps': 100.0, 'worstMs': 20.0, 'loadMs': 100.0, 'scene_3d_enabled': True,
             'snapshot': 'does-not-exist.png'},
        ])
        out = Path(tempfile.gettempdir()) / 'mht-missing-png.md'
        valid, flagged = mht.build_table(mht.load_rows(path), '', out)
        self.assertEqual((valid, flagged), (0, 0))
        self.assertIn('snapshot file', out.read_text(encoding='utf-8'))


    def test_commit_verification_is_reported(self):
        # 516 P2: a build-receipt-verified row says so; a row without the
        # verification is an unverified label, in the headline.
        path = write_state([
            {'map': 'x', 'elevation': 0, 'status': 'ok', 'exe_sha256': 'a', 'oneLowFps': 90.0,
             'avgFps': 100.0, 'worstMs': 20.0, 'loadMs': 100.0, 'scene_3d_enabled': True,
             'snapshot': make_snapshot(), 'commit': 'aaa', 'commit_verified': True},
        ])
        out = Path(tempfile.gettempdir()) / 'mht-verified.md'
        mht.build_table(mht.load_rows(path), 'aaa', out)
        self.assertIn('verified by the build receipt', out.read_text(encoding='utf-8'))
        path2 = write_state([
            {'map': 'x', 'elevation': 0, 'status': 'ok', 'exe_sha256': 'a', 'oneLowFps': 90.0,
             'avgFps': 100.0, 'worstMs': 20.0, 'loadMs': 100.0, 'scene_3d_enabled': True,
             'snapshot': make_snapshot(), 'commit': 'aaa', 'commit_verified': False},
        ])
        out2 = Path(tempfile.gettempdir()) / 'mht-unverified.md'
        mht.build_table(mht.load_rows(path2), 'aaa', out2)
        self.assertIn('unverified label', out2.read_text(encoding='utf-8'))


    def test_coverage_claim_needs_the_expected_list(self):
        # Without --expected the table never claims the whole game was walked.
        path = write_state([
            {'map': 'x', 'elevation': 0, 'status': 'ok', 'exe_sha256': 'a', 'oneLowFps': 90.0,
             'avgFps': 100.0, 'worstMs': 20.0, 'loadMs': 100.0, 'scene_3d_enabled': True,
             'snapshot': 'missing.png'},
        ])
        out = Path(tempfile.gettempdir()) / 'mht-coverage-none.md'
        mht.build_table(mht.load_rows(path), '', out)
        self.assertIn('Coverage: partial', out.read_text(encoding='utf-8'))
        # With an expected list that the rows do not cover: partial too.
        out2 = Path(tempfile.gettempdir()) / 'mht-coverage-partial.md'
        mht.build_table(mht.load_rows(path), '', out2, expected=[('x', 0), ('y', 1)])
        self.assertIn('partial, 1 of 2 expected', out2.read_text(encoding='utf-8'))
        # Full coverage says so, and only then.
        out3 = Path(tempfile.gettempdir()) / 'mht-coverage-full.md'
        mht.build_table(mht.load_rows(path), '', out3, expected=[('x', 0)])
        text3 = out3.read_text(encoding='utf-8')
        self.assertIn('Coverage: every one of the 1 expected', text3)
        self.assertIn('walked every playable map and elevation', text3)
        self.assertNotIn('walked the map elevations this table carries', text3)
        # A partial table never makes the whole-game claim.
        out4 = Path(tempfile.gettempdir()) / 'mht-coverage-partial-head.md'
        mht.build_table(mht.load_rows(path), '', out4, expected=[('x', 0), ('y', 1)])
        self.assertIn('walked the map elevations this table carries', out4.read_text(encoding='utf-8'))

    def test_table_marks_invalid_rows_and_counts(self):
        path = write_state([
            {'map': 'kladwtwn', 'elevation': 0, 'status': 'ok', 'attempts': 1, 'exe_sha256': 'a',
             'avgFps': 100.0, 'oneLowFps': 90.0, 'worstMs': 33.0, 'loadMs': 900.0, 'cpu': 12,
             'fallback': [], 'scene_3d_enabled': True, 'snapshot': make_snapshot()},
            {'map': 'gstcav2', 'elevation': 1, 'status': 'timeout', 'attempts': 2, 'exe_sha256': 'a',
             'error': 'No snapshot within 300 s.'},
            {'map': 'newr1', 'elevation': 0, 'status': 'ok', 'attempts': 1, 'exe_sha256': 'a',
             'avgFps': 60.0, 'oneLowFps': 45.0, 'worstMs': 90.0, 'loadMs': 2600.0, 'cpu': 10,
             'fallback': ['scene: NEWR1 elevation 0 stays 2D: walls.bin is missing'],
             'scene_3d_enabled': True, 'snapshot': make_snapshot()},
        ])
        out = Path(tempfile.gettempdir()) / 'mht-basic.md'
        valid, flagged = mht.build_table(mht.load_rows(path), 'de6d5267', out)
        self.assertEqual((valid, flagged), (2, 1))
        text = out.read_text(encoding='utf-8')
        self.assertIn('2 of 3 rows are valid results', text)
        self.assertIn('| newr1 | 0 | 60.0 | 45.0', text)
        self.assertIn('Not valid results', text)
        self.assertIn('gstcav2:e1 timeout after 2 attempts: No snapshot within 300 s.', text)
        self.assertIn('with a 2D fallback line: newr1:e0', text)
        self.assertIn('load over 2000 ms: newr1:e0', text)
        self.assertIn('1% low under 80 fps: newr1:e0', text)

    def test_invalid_rows_sort_before_valid(self):
        path = write_state([
            {'map': 'aaa', 'elevation': 0, 'status': 'ok', 'exe_sha256': 'a',
             'avgFps': 100.0, 'oneLowFps': 30.0, 'worstMs': 20.0, 'loadMs': 100.0, 'fallback': [],
             'scene_3d_enabled': True, 'snapshot': make_snapshot()},
            {'map': 'zzz', 'elevation': 0, 'status': 'failed', 'attempts': 2, 'exe_sha256': 'a',
             'error': 'crash'},
        ])
        out = Path(tempfile.gettempdir()) / 'mht-order.md'
        mht.build_table(mht.load_rows(path), 'de6d5267', out)
        text = out.read_text(encoding='utf-8')
        self.assertLess(text.index('| zzz | 0 |'), text.index('| aaa | 0 |'))


@unittest.skipUnless(Image is not None, 'Pillow is needed for the black share')
class BlackShareTest(unittest.TestCase):
    def test_black_share_counts_dark_pixels(self):
        image = Image.new('RGB', (10, 10), (255, 255, 255))
        for x in range(10):
            for y in range(5):
                image.putpixel((x, y), (0, 0, 0))
        handle = tempfile.NamedTemporaryFile(suffix='.png', delete=False)
        image.save(handle, 'PNG')
        handle.close()
        self.assertEqual(mht.compute_black_share(handle.name), 50.0)
        Path(handle.name).unlink()

    def test_missing_png_is_na(self):
        self.assertIsNone(mht.compute_black_share(None))
        self.assertIsNone(mht.compute_black_share('does-not-exist.png'))


if __name__ == '__main__':
    unittest.main()
