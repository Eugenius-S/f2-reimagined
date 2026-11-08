#ifndef FALLOUT_SCENE_RENDERER_GL_API_H
#define FALLOUT_SCENE_RENDERER_GL_API_H

// Minimal OpenGL 4.1 core loader. Only the entry points this module uses are
// resolved (via SDL_GL_GetProcAddress); no third-party loader dependency and
// no GL headers: signatures are declared in gl_procs.h.

#include <SDL.h>

namespace fallout {
namespace scene_renderer {

bool glLoad();

#define GL_PROC(name, args, ret) extern ret (*gl##name) args;
#include "gl_procs.h"
#undef GL_PROC

// A few constants we need (mirroring GL headers).
constexpr unsigned int GL_TEXTURE_2D_ARRAY = 0x8C1A;

// glGetError answers only after the driver has caught up with the queued
// commands, so a check in a hot path serializes the CPU against the whole
// GPU frame queue (157: the hero card's upload stalled every frame by the
// scene's GPU time). The checks run only under F2CE_GL_DEBUG; a normal play
// takes the uploads on faith and a real GL failure shows as missing art.
bool glErrorChecksEnabled();

} // namespace scene_renderer
} // namespace fallout

#endif /* FALLOUT_SCENE_RENDERER_GL_API_H */
