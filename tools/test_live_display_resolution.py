"""The close/apply path must resize physical output without rebuilding local 3D."""
from pathlib import Path
import unittest

SRC = Path(__file__).resolve().parents[1] / "ce/src"


class LiveDisplayResolutionTest(unittest.TestCase):
    def test_resolution_changes_on_close_but_scale_keeps_restart(self):
        settings = (SRC / "settings.cc").read_text(encoding="utf-8")
        self.assertIn("resolutionModeDescriptor.applyPolicy = SettingApplyPolicy::OnClose", settings)
        self.assertIn("scaleFactorDescriptor.applyPolicy = SettingApplyPolicy::Restart", settings)
        self.assertIn("if (!onlyAdd && settings.screen.resolution_mode > 0)", settings)

    def test_existing_close_path_preserves_opengl_state(self):
        preferences = (SRC / "preferences.cc").read_text(encoding="utf-8")
        renderer = (SRC / "svga.cc").read_text(encoding="utf-8")
        self.assertIn("applyWindowModeLive();", preferences)
        live = renderer.split("void applyWindowModeLive()", 1)[1].split("static void renderPresentBody", 1)[0]
        self.assertIn("SDL_SetWindowDisplayMode", live)
        self.assertIn("SDL_SetWindowSize", live)
        resize = renderer.split("void handleWindowSizeChanged()", 1)[1].split("void applyWindowModeLive()", 1)[0]
        self.assertLess(resize.index("return;"), resize.index("destroyRenderer();"))
        self.assertIn("mouseDeviceRefreshWindowMapping();", resize)


if __name__ == "__main__":
    unittest.main()
