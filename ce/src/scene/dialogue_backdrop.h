#ifndef FALLOUT_SCENE_DIALOGUE_BACKDROP_H
#define FALLOUT_SCENE_DIALOGUE_BACKDROP_H

namespace fallout {
namespace scene {

int createDialogueBackgroundWindow(int x, int y, int width, int height);
void destroyDialogueBackgroundWindow(int window);
void drawDialogueBackdrop();

} // namespace scene
} // namespace fallout

#endif
