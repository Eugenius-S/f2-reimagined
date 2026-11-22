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
    def test_rp_voice_recovery_when_excluded_or_unsupported(self):
        packs = Path(ROOT)
        for label in ('1c', 'levy'):
            for situation in ('excluded', 'unsupported'):
                with self.subTest(label=label, situation=situation), tempfile.TemporaryDirectory(prefix='f2r-rp-voice-') as temp:
                    root = Path(temp)
                    mods = root / 'mods'
                    mods.mkdir()
                    for name in ('master.dat', 'critter.dat'):
                        (root / name).write_bytes(b'fixture')
                    (root / 'fallout2.cfg').write_bytes(b'[system]\r\nlanguage=english\r\n')
                    shutil.copyfile(packs / 'proof/rpu-2.4.34/mods/rpu.dat', mods / 'rpu.dat')
                    if situation == 'unsupported':
                        with (mods / 'rpu.dat').open('r+b') as file:
                            first = file.read(1)
                            file.seek(0)
                            file.write(bytes([first[0] ^ 1]))
                    voice = mods / ('f2r-voice-rp-' + label + '.dat')
                    shutil.copyfile(packs / 'for-google-drive-voice' / (label + '-voice.dat'), voice)
                    voice_hash = hashlib.sha256(voice.read_bytes()).hexdigest()
                    (mods / 'f2r-text-choice.txt').write_text('original\n')
                    (mods / 'f2r-voice-choice.txt').write_text('rp-' + label + '\nhash=' + voice_hash + '\n')
                    manual = mods / 'upu_russian_sound.dat'
                    if label == '1c' and situation == 'excluded':
                        shutil.copyfile(packs / 'for-google-drive-voice/1c-voice.dat', manual)
                    else:
                        manual.write_bytes(b'foreign voice')
                    manual_hash = hashlib.sha256(manual.read_bytes()).hexdigest()
                    order = mods / 'mods_order.txt'
                    order.write_bytes((b'rpu.dat\r\n' if situation == 'unsupported' else b'') +
                                      b'./' + voice.name.encode() + b'\r\n.\\' + voice.name.encode() + b'\r\nupu_russian_sound.dat\r\nplayer.dat\r\n')
                    proc = subprocess.run([str(EXE), '--game-root', str(root), '--recover-text'], capture_output=True, timeout=30)
                    self.assertEqual(proc.returncode, 0, proc.stdout.decode('utf8', errors='replace'))
                    self.assertFalse(voice.exists())
                    self.assertEqual((mods / 'f2r-voice-choice.txt').read_text().strip(), 'original')
                    self.assertNotIn(voice.name.encode(), order.read_bytes())
                    self.assertIn(b'upu_russian_sound.dat\r\nplayer.dat\r\n', order.read_bytes())
                    self.assertEqual(hashlib.sha256(manual.read_bytes()).hexdigest(), manual_hash)
                    voice.write_bytes(b'foreign replacement')
                    (mods / 'f2r-voice-choice.txt').write_text('rp-' + label + '\nhash=' + voice_hash + '\n')
                    order.write_bytes(voice.name.encode() + b'\r\nplayer.dat\r\n')
                    proc = subprocess.run([str(EXE), '--game-root', str(root), '--recover-text'], capture_output=True, timeout=30)
                    self.assertNotEqual(proc.returncode, 0)
                    self.assertEqual(voice.read_bytes(), b'foreign replacement')
                    self.assertEqual(order.read_bytes(), voice.name.encode() + b'\r\nplayer.dat\r\n')

    def test_relative_rpu_order_paths_keep_priority(self):
        packs = Path(ROOT)
        with tempfile.TemporaryDirectory(prefix='f2r-relative-rp-') as temp:
            root = Path(temp)
            mods = root / 'mods'
            mods.mkdir()
            for name in ('master.dat', 'critter.dat'):
                (root / name).write_bytes(b'fixture')
            (root / 'fallout2.cfg').write_bytes(b'[system]\r\nlanguage=english\r\n')
            shutil.copyfile(packs / 'proof/rpu-2.4.34/mods/rpu.dat', mods / 'rpu.dat')
            shutil.copyfile(packs / 'proof/rpu-2.4.34/mods/rpu_russian.dat', mods / 'rpu_russian.dat')
            order = mods / 'mods_order.txt'
            for rpu_line, official_line in ((b'./rpu.dat\n', b'./rpu_russian.dat\n'), (b'.\\rpu.dat\n', b'.\\rpu_russian.dat\n')):
                order.write_bytes(b'# player comment\n' + rpu_line + official_line + b'player.dat\n')
                proc = subprocess.run([str(EXE), '--game-root', str(root), '--text-translation', '1c'], capture_output=True, timeout=120)
                self.assertEqual(proc.returncode, 0, proc.stdout.decode('utf8', errors='replace'))
                result = order.read_bytes()
                self.assertTrue(result.startswith(b'# player comment\n' + rpu_line + b'rpu_russian.dat\r\n'))
                self.assertLess(result.index(b'rpu_russian.dat'), result.index(b'steam-1c-text.dat'))
                self.assertLess(result.index(b'steam-1c-text.dat'), result.index(b'player.dat'))
                self.assertEqual(result.count(b'rpu_russian.dat'), 1)
                proc = subprocess.run([str(EXE), '--game-root', str(root), '--recover-text'], capture_output=True, timeout=30)
                self.assertEqual(proc.returncode, 0, proc.stdout.decode('utf8', errors='replace'))
                self.assertEqual((mods / 'f2r-text-choice.txt').read_text().splitlines()[0], '1c')
                result = result.replace(b'rpu_russian.dat\r\n', official_line + b'./rpu_russian.dat\n')
                result = result.replace(b'steam-1c-text.dat\r\n', b'./steam-1c-text.dat\n.\\steam-1c-text.dat\n')
                result = result.replace(b'shared-legacy-fonts.dat\r\n', b'./shared-legacy-fonts.dat\n')
                order.write_bytes(result)
                proc = subprocess.run([str(EXE), '--game-root', str(root), '--text-translation', 'original'], capture_output=True, timeout=30)
                self.assertEqual(proc.returncode, 0, proc.stdout.decode('utf8', errors='replace'))
                self.assertEqual(order.read_bytes(), b'# player comment\n' + rpu_line + b'player.dat\n')

    def test_manual_rp_removal_is_noop_and_excluded_rp_is_rejected(self):
        packs = Path(ROOT)
        with tempfile.TemporaryDirectory(prefix='f2r-manual-rp-') as temp:
            root = Path(temp)
            mods = root / 'mods'
            mods.mkdir()
            for name in ('master.dat', 'critter.dat'):
                (root / name).write_bytes(b'fixture')
            (root / 'fallout2.cfg').write_bytes(b'[system]\r\nlanguage=russian\r\n')
            shutil.copyfile(packs / 'proof/rpu-2.4.34/mods/rpu.dat', mods / 'rpu.dat')
            shutil.copyfile(packs / 'for-google-drive-rp-text/rp-1c-text.dat', mods / 'steam-1c-text.dat')
            shutil.copyfile(packs / 'prepared-v4/shared-legacy-fonts.dat', mods / 'shared-legacy-fonts.dat')
            (mods / 'f2r-text-choice.txt').write_text('1c\n')
            order = mods / 'mods_order.txt'
            order.write_bytes(b'rpu.dat\r\nsteam-1c-text.dat\r\n')
            proc = subprocess.run([str(EXE), '--game-root', str(root), '--remove-restoration-project'], capture_output=True, timeout=20)
            self.assertEqual(proc.returncode, 0, proc.stdout.decode('utf8', errors='replace'))
            self.assertTrue((mods / 'rpu.dat').exists())
            self.assertEqual(order.read_bytes(), b'rpu.dat\r\nsteam-1c-text.dat\r\n')
            order.write_bytes(b'steam-1c-text.dat\r\nplayer.dat\r\n')
            proc = subprocess.run([str(EXE), '--game-root', str(root), '--text-translation', '1c'], capture_output=True, timeout=20)
            self.assertNotEqual(proc.returncode, 0)
            self.assertIn(b'excluded from mods_order.txt', proc.stdout)
            self.assertEqual(order.read_bytes(), b'steam-1c-text.dat\r\nplayer.dat\r\n')
            proc = subprocess.run([str(EXE), '--game-root', str(root), '--voice-translation', 'rp-1c'], capture_output=True, timeout=20)
            self.assertNotEqual(proc.returncode, 0)
            self.assertEqual(order.read_bytes(), b'steam-1c-text.dat\r\nplayer.dat\r\n')
            proc = subprocess.run([str(EXE), '--game-root', str(root), '--recover-text'], capture_output=True, timeout=20)
            self.assertEqual(proc.returncode, 0, proc.stdout.decode('utf8', errors='replace'))
            self.assertEqual(order.read_bytes(), b'player.dat\r\n')
            self.assertIn(b'language=english', (root / 'fallout2.cfg').read_bytes())
            proc = subprocess.run([str(EXE), '--game-root', str(root), '--voice-translation', 'rp-1c'], capture_output=True, timeout=20)
            self.assertNotEqual(proc.returncode, 0)
            self.assertEqual(order.read_bytes(), b'player.dat\r\n')
            (mods / 'steam-1c-text.dat').write_bytes(b'player file')
            (mods / 'f2r-text-choice.txt').write_text('1c\n')
            order.write_bytes(b'steam-1c-text.dat\r\nplayer.dat\r\n')
            proc = subprocess.run([str(EXE), '--game-root', str(root), '--recover-text'], capture_output=True, timeout=20)
            self.assertNotEqual(proc.returncode, 0)
            self.assertEqual((mods / 'steam-1c-text.dat').read_bytes(), b'player file')
            self.assertEqual(order.read_bytes(), b'steam-1c-text.dat\r\nplayer.dat\r\n')

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
