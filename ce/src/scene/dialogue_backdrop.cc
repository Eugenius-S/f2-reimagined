#include "dialogue_backdrop.h"
#include "scene_view.h"
#include "dialogue_fade.h"

#include "../debug.h"
#include "../svga.h"
#include "../window_manager.h"
#include "../scene_renderer/context.h"

#include <SDL.h>

namespace fallout {
namespace scene {
namespace {

int backdropWindow = -1;
bool fullDialogue = false;
DialogueFade fade;

void destroyBackdrop()
{
    if (backdropWindow != -1) {
        windowDestroy(backdropWindow);
        backdropWindow = -1;
    }
}

} // namespace

int createDialogueBackgroundWindow(int x, int y, int width, int height)
{
    // In the classic view an opaque window hides the world,
    // leaving all dialogue windows, barter and the cursor above it.
    destroyBackdrop();
    if (activeView() != View::Full) {
        backdropWindow = windowCreate(0, 0, screenGetWidth(), screenGetHeight(),
            static_cast<ColorWithFlags>(0), WINDOW_DONT_MOVE_TOP);
        if (backdropWindow == -1) {
            debugPrint("Dialogue backdrop disabled: window creation failed.\n");
        }
    }

    const int window = windowCreate(x, y, width, height,
        static_cast<ColorWithFlags>(256), WINDOW_DONT_MOVE_TOP | WINDOW_MODAL);
    if (window == -1) {
        destroyBackdrop();
    } else {
        fullDialogue = activeView() == View::Full;
        if (fullDialogue) {
            fade.setOpening(true, SDL_GetTicks());
        } else if (backdropWindow != -1) {
            windowRefresh(backdropWindow);
        }
    }
    return window;
}

void destroyDialogueBackgroundWindow(int window)
{
    if (fullDialogue) {
        fade.setOpening(false, SDL_GetTicks());
        fullDialogue = false;
    }
    windowDestroy(window);
    // Destroying the full-screen window refreshes the exposed world now.
    destroyBackdrop();
}

void drawDialogueBackdrop()
{
    fade.advance(SDL_GetTicks());
    if (fade.value <= 0.0f) return;
    const unsigned char pixel[4] = { 0, 0, 0, static_cast<unsigned char>(fade.opacity() * 255.0f) };
    scene_renderer::drawOverlay(pixel, 1, 1, 4, 0, 0, screenGetWidth(), screenGetHeight());
}

} // namespace scene
} // namespace fallout
