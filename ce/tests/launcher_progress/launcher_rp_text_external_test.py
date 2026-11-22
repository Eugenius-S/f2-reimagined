"""Disposable RP install switching test; uses pinned external DAT fixtures."""
import hashlib
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest

EXE = Path(sys.argv.pop(1)).resolve()
ROOT = os.environ.get('F2R_RP_TEXT_PACKS')


@unittest.skipUnless(ROOT, 'external DAT fixtures not supplied')
class RpTextSwitch(unittest.TestCase):
    def test_unsupported_rp_drops_adapted_text(self):
        packs = Path(ROOT)
        with tempfile.TemporaryDirectory(prefix='f2r-rp-unsupported-') as temp:
            root = Path(temp)
            mods = root / 'mods'
            mods.mkdir()
            for name in ('master.dat', 'critter.dat'):
                (root / name).write_bytes(b'fixture')
            (root / 'fallout2.cfg').write_bytes(b'[system]\r\nlanguage=russian\r\n')
            (mods / 'rpu.dat').write_bytes(b'unsupported future RP')
            shutil.copyfile(packs / 'proof/rpu-2.4.34/mods/rpu_russian.dat', mods / 'rpu_russian.dat')
            shutil.copyfile(packs / 'prepared-v4/shared-legacy-fonts.dat', mods / 'shared-legacy-fonts.dat')
            shutil.copyfile(packs / 'for-google-drive-rp-text/rp-1c-text.dat', mods / 'steam-1c-text.dat')
            (mods / 'f2r-text-choice.txt').write_text('1c\n')
            (mods / 'mods_order.txt').write_bytes(b'rpu.dat\r\nrpu_russian.dat\r\nshared-legacy-fonts.dat\r\nsteam-1c-text.dat\r\nplayer.dat\r\n')
            proc = subprocess.run([str(EXE), '--game-root', str(root), '--recover-text'], capture_output=True, timeout=20)
            self.assertEqual(proc.returncode, 0, proc.stdout.decode('utf8', errors='replace'))
            self.assertEqual((mods / 'mods_order.txt').read_text().splitlines(), ['rpu.dat', 'player.dat'])
            self.assertTrue((mods / 'rpu_russian.dat').exists())
            self.assertIn(b'language=english', (root / 'fallout2.cfg').read_bytes())

    def test_every_choice_and_original(self):
        packs = Path(ROOT)
        with tempfile.TemporaryDirectory(prefix='f2r-rp-text-') as temp:
            root = Path(temp)
            mods = root / 'mods'
            mods.mkdir()
            for name in ('master.dat', 'critter.dat'):
                (root / name).write_bytes(b'fixture')
            (root / 'fallout2.cfg').write_bytes(b'[system]\r\nlanguage=english\r\n')
            (mods / 'player.dat').write_bytes(b'player')
            shutil.copyfile(packs / 'proof/rpu-2.4.34/mods/rpu.dat', mods / 'rpu.dat')
            shutil.copyfile(packs / 'proof/rpu-2.4.34/mods/rpu_russian.dat', mods / 'rpu_russian.dat')
            (mods / 'mods_order.txt').write_bytes(b'rpu.dat\r\nrpu_russian.dat\r\nplayer.dat\r\n')
            for choice, filename in [('official', None), ('1c', 'steam-1c-text.dat'),
                                     ('fargus', 'fio-fargus-text.dat'), ('levy', 'fio-levy-text.dat'), ('original', None)]:
                with self.subTest(choice=choice):
                    proc = subprocess.run([str(EXE), '--game-root', str(root), '--text-translation', choice],
                                          capture_output=True, timeout=120)
                    self.assertEqual(proc.returncode, 0, proc.stdout.decode('utf8', errors='replace'))
                    order = (mods / 'mods_order.txt').read_text().splitlines()
                    self.assertIn('player.dat', order)
                    if filename:
                        expected = packs / 'for-google-drive-rp-text' / ('rp-' + choice + '-text.dat')
                        self.assertEqual(hashlib.sha256((mods / filename).read_bytes()).digest(), hashlib.sha256(expected.read_bytes()).digest())
                        self.assertLess(order.index('rpu.dat'), order.index('rpu_russian.dat'))
                        self.assertLess(order.index('rpu_russian.dat'), order.index(filename))
                        self.assertLess(order.index(filename), order.index('player.dat'))
                    if choice == 'original':
                        self.assertNotIn('rpu_russian.dat', order)
                        self.assertIn(b'language=english', (root / 'fallout2.cfg').read_bytes())


if __name__ == '__main__':
    unittest.main()
