"""External archive regression: F2R_RP_TEXT_PACKS points at the pack root."""
import os
import re
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
sys.path.insert(0, str(Path(__file__).resolve().parent / 'scene_export' / 'vendor'))
from f2lib.dat2 import Dat2
from prepare_rp_text import build

ROW = re.compile(rb'\{([+-]?\d+)\}\s*\{([^{}]*)\}\s*\{([^{}]*)\}')
MARKER = re.compile(rb'%(?:\d+\$)?[-+#0 .\d*hlL]*[a-zA-Z%]|\\[a-zA-Z]|\|[a-zA-Z0-9_]+')


class SyntheticRegression(unittest.TestCase):
    def test_preserves_changed_and_new_rp_messages_and_voice(self):
        class Archive(dict):
            def names(self): return list(self)
            def has(self, name): return name in self
            def read(self, name): return self[name]
        ru, en = 'text/russian/game/a.msg', 'text/english/game/a.msg'
        base = Archive({en: b'{1}{base}{Hello %s}\n{2}{}{Old}\n'})
        rp = Archive({en: b'{1}{rp}{Hello %s}\n{2}{}{Changed}\n{3}{}{New}\n'})
        official = Archive({ru: b'{1}{voice}{Official %s}\n{2}{}{RP translation}\n{3}{}{New translation}\n'})
        source = Archive({ru: b'{1}{wrong}{Alternative %s}\n{2}{}{Alternative old}\n{3}{}{Unsafe}\n'})
        result, counts, _ = build(base, rp, official, source)
        self.assertEqual(result[ru], b'{1}{voice}{Alternative %s}\n{2}{}{RP translation}\n{3}{}{New translation}\n')
        self.assertEqual(counts['replaced'], 1)


@unittest.skipUnless(os.environ.get('F2R_RP_TEXT_PACKS'), 'external data not supplied')
class RpTextRegression(unittest.TestCase):
    def test_current_messages_and_english_proof(self):
        root = Path(os.environ['F2R_RP_TEXT_PACKS'])
        proof = root / 'proof'
        with Dat2(proof / 'vanilla/master.dat') as base, Dat2(proof / 'rpu-2.4.34/mods/rpu.dat') as rp, Dat2(proof / 'rpu-2.4.34/mods/rpu_russian.dat') as official:
            for label, source in [('1c', 'steam-1c-text.dat'), ('fargus', 'fio-fargus-text.dat'), ('levy', 'fio-levy-text.dat')]:
                with self.subTest(label=label), Dat2(root / 'prepared-v4' / source) as source_dat, Dat2(root / 'for-google-drive-rp-text' / ('rp-' + label + '-text.dat')) as result:
                    names = {n for n in official.names() if n.startswith('text/russian/')}
                    self.assertEqual(set(result.names()), names)
                    for name in names:
                        before, after = official.read(name), result.read(name)
                        if before == after:
                            continue
                        self.assertTrue(name.endswith('.msg'))
                        def rows(data):
                            found = ROW.findall(data)
                            self.assertEqual(len(found), len({int(r[0]) for r in found}))
                            return {int(r[0]): (r[1], r[2]) for r in found}
                        original, actual = rows(before), rows(after)
                        self.assertEqual(original.keys(), actual.keys())
                        en = name.replace('text/russian/', 'text/english/', 1)
                        vanilla = rows(base.read(en))
                        effective = rows(rp.read(en)) if rp.has(en) else vanilla
                        translated = rows(source_dat.read(name))
                        for number in original:
                            self.assertEqual(original[number][0], actual[number][0])
                            if original[number][1] != actual[number][1]:
                                self.assertEqual(vanilla[number][1], effective[number][1])
                                self.assertEqual(translated[number][1], actual[number][1])
                                for reference in (original[number][1], effective[number][1]):
                                    self.assertEqual(MARKER.findall(reference), MARKER.findall(actual[number][1]))


if __name__ == '__main__':
    unittest.main()
