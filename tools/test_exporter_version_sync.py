"""273: the game refuses scene caches from another exporter version, so the
version it expects (ce/src/scene/scene_build.cc) and the version the exporter
writes (tools/scene_export/export.py) must be one number. A drift here means
every shipped cache looks stale and every map rebuilds on entry, or worse,
the game accepts a cache it cannot read."""

import re
import unittest
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent

PY_DECLARATION = re.compile(r'^EXPORTER_VERSION = "([^"]+)"', re.MULTILINE)
CPP_DECLARATION = re.compile(r'const char kExpectedExporterVersion\[\] = "([^"]+)"')


def declared_versions(export_py_text: str, scene_build_text: str):
    """The two version strings; None where the declaration is missing."""
    py = PY_DECLARATION.search(export_py_text)
    cpp = CPP_DECLARATION.search(scene_build_text)
    return (py.group(1) if py else None), (cpp.group(1) if cpp else None)


def in_sync(export_py_text: str, scene_build_text: str) -> bool:
    py, cpp = declared_versions(export_py_text, scene_build_text)
    return py is not None and py == cpp and py != ""


class ExporterVersionSync(unittest.TestCase):
    def test_game_expects_the_exporters_version(self):
        export_py = (REPO / "tools" / "scene_export" / "export.py").read_text(encoding="utf-8")
        scene_build = (REPO / "ce" / "src" / "scene" / "scene_build.cc").read_text(encoding="utf-8")
        self.assertTrue(in_sync(export_py, scene_build),
            "the game's expected exporter version drifted from export.py EXPORTER_VERSION")

    def test_drift_is_detected(self):
        self.assertFalse(in_sync('EXPORTER_VERSION = "1.13.0"\n',
            'const char kExpectedExporterVersion[] = "1.12.0";'))

    def test_missing_declaration_is_detected(self):
        self.assertFalse(in_sync('EXPORTER_VERSION = "1.13.0"\n', 'no constant here'))
        self.assertFalse(in_sync('no declaration', 'const char kExpectedExporterVersion[] = "1.13.0";'))

    def test_empty_version_is_detected(self):
        self.assertFalse(in_sync('EXPORTER_VERSION = ""\n',
            'const char kExpectedExporterVersion[] = "";'))


if __name__ == "__main__":
    unittest.main()
