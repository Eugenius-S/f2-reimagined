"""Retain parity between baked and direct floor lighting."""
from pathlib import Path
import unittest


class GroundBakeLightingTest(unittest.TestCase):
    def test_bake_uses_its_local_origin_and_restores_letterbox_origin(self):
        path = Path(__file__).resolve().parents[1] / "ce/src/scene_renderer/scene_draw.cc"
        source = path.read_text(encoding="utf-8")
        bake = source.split("if (bakeGround) {", 1)[1].split("// 172: the load chain", 1)[0]
        self.assertIn("glViewport(0, 0, width / 2, height / 2)", bake)
        self.assertIn("glUniform4f(gSceneViewportLocation, 0.0f, 0.0f", bake)
        self.assertIn("glViewport(x, y, width, height)", bake)
        self.assertIn("glUniform4f(gSceneViewportLocation, static_cast<float>(x)", bake)

    def test_bake_draw_uses_floor_light_mode_before_drawing(self):
        path = Path(__file__).resolve().parents[1] / "ce/src/scene_renderer/scene_draw.cc"
        source = path.read_text(encoding="utf-8")
        bake = source.split("if (bakeGround) {", 1)[1].split("glBindFramebuffer(0x8D40 /* GL_FRAMEBUFFER */, previousFbo);", 1)[0]
        floor_light = "glUniform1i(gSceneLightModeLocation, 1)"
        self.assertIn(floor_light, bake, "bake otherwise writes full white instead of scene lighting")
        self.assertLess(bake.index(floor_light), bake.index("glDrawArrays("))
        self.assertIn("glUniform1i(gSceneGroundBakeModeLocation, 1)", bake)


if __name__ == "__main__":
    unittest.main()
