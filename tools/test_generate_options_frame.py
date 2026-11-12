import tempfile
import unittest
from pathlib import Path

from tools.generate_options_frame import NAMES, build_header, write_header


class GenerateOptionsFrameTests(unittest.TestCase):
    def test_embeds_all_source_bytes_in_stable_groups(self):
        with tempfile.TemporaryDirectory() as temporary:
            source = Path(temporary) / 'source'
            source.mkdir()
            for index, name in enumerate(NAMES):
                (source / (name + '.png')).write_bytes(bytes((index, 255, 7)) * 9)

            header = build_header(source)

            self.assertIn('static constexpr unsigned char topLeft[] = {\n    0,255,7,0,255,7,0,255,7,0,255,7,0,255,7,0,255,7,0,255,7,0,255,7,\n    0,255,7,\n};', header)
            self.assertIn('static constexpr unsigned char bottomRight[] = {\n    7,255,7,7,255,7,7,255,7,7,255,7,7,255,7,7,255,7,7,255,7,7,255,7,\n    7,255,7,\n};', header)
            self.assertEqual(8, header.count('static constexpr unsigned char '))
            self.assertTrue(header.endswith('} } }\n#endif\n'))

    def test_missing_source_does_not_replace_existing_header(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            source = root / 'source'
            source.mkdir()
            (source / (NAMES[0] + '.png')).write_bytes(b'frame')
            output = root / 'options_frame_owner.h'
            output.write_text('previous header\n', encoding='utf-8')

            with self.assertRaises(FileNotFoundError):
                write_header(source, output)

            self.assertEqual('previous header\n', output.read_text(encoding='utf-8'))


if __name__ == '__main__':
    unittest.main()
