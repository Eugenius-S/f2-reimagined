"""VSync must reach both Windows presentation paths without capping benchmarks."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]


class VSyncSettingsTest(unittest.TestCase):
    def test_screen_setting_is_default_on_and_registered(self):
        settings = (ROOT / "ce/src/settings.h").read_text(encoding="utf-8")
        registry = (ROOT / "ce/src/settings.cc").read_text(encoding="utf-8")
        self.assertIn("bool vsync = true;", settings)
        self.assertIn("SETTING(vsync);", registry)
        self.assertIn("vsyncDescriptor.applyPolicy = SettingApplyPolicy::Restart", registry)

    def test_both_renderers_consume_the_same_setting(self):
        opengl = (ROOT / "ce/src/scene_renderer/context.cc").read_text(encoding="utf-8")
        fallback = (ROOT / "ce/src/svga.cc").read_text(encoding="utf-8")
        self.assertIn("settings.screen.vsync ? 1 : 0", opengl)
        self.assertIn("settings.screen.vsync ? SDL_RENDERER_PRESENTVSYNC : 0", fallback)
        self.assertIn("gSdlRenderer == nullptr && flags != 0", fallback)

    def test_benchmark_uses_uncapped_swap(self):
        config = (ROOT / "tools/test-config/fallout2.cfg").read_text(encoding="utf-8")
        screen = config.split("[screen]", 1)[1].split("[", 1)[0]
        self.assertIn("vsync=0", screen)


if __name__ == "__main__":
    unittest.main()
