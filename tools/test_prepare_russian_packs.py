import tempfile
import unittest
from unittest.mock import patch
from pathlib import Path

from prepare_russian_packs import convert_text, main, normalize_fargus, select, validate_msg, validate_sve


class SelectionTest(unittest.TestCase):
    def test_wrong_reference_creates_no_output(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            reference = root / "bad.dat"
            reference.write_bytes(b"not official")
            output = root / "output"
            with patch("sys.argv", ["prepare", str(root), str(output), "--rpu-reference", str(reference)]):
                with self.assertRaisesRegex(SystemExit, "SHA256"):
                    main()
            self.assertFalse(output.exists())

    def test_encodings_and_corrupt_source(self):
        phrase = "Вы действительно хотите выйти?"
        raw = phrase.encode("cp866")
        levy = bytes(b - 48 if 0xE0 <= b <= 0xEF else b for b in raw)
        self.assertEqual(convert_text(raw, False).decode("cp1251"), phrase)
        self.assertEqual(convert_text(levy, True).decode("cp1251"), phrase)
        with self.assertRaises(ValueError):
            convert_text(b"bad\0data", True)
        with self.assertRaises(UnicodeEncodeError):
            convert_text(b"\xb0", False)

    def test_message_validation(self):
        validate_msg(b"comment\n{1382}{}{Hello}\n")
        for content in (b"", b"comment", b"{1382{}{text}", b"{1}{}{x", b"{1}{}{" + b"x" * 1024 + b"}"):
            with self.assertRaises(ValueError):
                validate_msg(content)

    def test_subtitle_validation(self):
        validate_sve(b"\r\n1:hello\r\n\r\n200:world\r\n\r\n")
        for content in (b"", b"\r\n\r\n", b"\0binary", b"broken", b"2:a\n\n1:b", b"1:" + b"x" * 256):
            with self.assertRaises(ValueError):
                validate_sve(content)

    def test_fargus_single_row_normalization(self):
        row = "{1086}{}{Объявление}".encode("cp866")
        broken = row.replace(b"\xea", b"\xba")
        raw = broken + b"\n{1087}{}{\xba}\n"
        self.assertEqual(normalize_fargus("fio-fargus-text", "text/russian/game/scrname.msg", raw), row + b"\n{1087}{}{\xba}\n")
        self.assertEqual(normalize_fargus("fio-levy-text", "text/russian/game/scrname.msg", raw), raw)
        self.assertEqual(normalize_fargus("fio-fargus-text", "text/russian/game/other.msg", raw), raw)

    def test_language_paths_and_exclusions(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            for name in ("TEXT/ENGLISH/GAME/A.MSG", "FONT0.AAF", "SOUND/SPEECH/A/A.ACM",
                         "SOUND/SPEECH/A/A.LIP", "SOUND/SPEECH/A/A.TXT", "scripts/a.int",
                         "text/english/b.sve", "maps/test.map"):
                path = root / name
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_bytes(b"data")
            self.assertEqual(set(select(root, "text")), {"text/russian/game/a.msg", "text/russian/b.sve"})
            self.assertEqual(set(select(root, "voice")), {"sound/speech/a/a.acm", "sound/speech/a/a.lip"})


if __name__ == "__main__":
    unittest.main()
