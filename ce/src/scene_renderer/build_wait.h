#ifndef FALLOUT_SCENE_RENDERER_BUILD_WAIT_H
#define FALLOUT_SCENE_RENDERER_BUILD_WAIT_H

struct SDL_Surface;

namespace fallout {
namespace scene_renderer {

// 273: while a map's scene cache is being built on entry, the 3D view stays
// up and the map's 2D art lies on the ground plane, the way 173 keeps an
// unbuilt thing as a flat picture instead of switching back to 2D.

// True while the waiting plane should be shown (a build is running).
bool waitPlaneActive();
// Scene side says the wait begins or ends (a build started, finished or the
// map changed). The GL calls run on the present thread, where the scene
// state also changes.
void setWaitPlane(bool on);
// 275: the same, with the plane's picture taken from the given elevation's
// own 2D render (a nearly-empty upper floor shows the elevation below).
void setWaitPlaneAt(bool on, int artElevation);
// The game's current indexed frame as the plane's texture, uploaded before
// the scene draw while waiting. Cheap no-op when the wait is not on.
void uploadWaitFrame(SDL_Surface* indexed);
// The waiting state itself: sky, then the frame lying on the ground plane.
// Draws nothing when the wait is off or nothing was uploaded yet.
void drawWaitPlane();
void destroyWaitPlane();

} // namespace scene_renderer
} // namespace fallout

#endif
