#include "gl_api.h"

namespace fallout {
namespace scene_renderer {

#define GL_PROC(name, args, ret) ret (*gl##name) args = nullptr;
#include "gl_procs.h"
#undef GL_PROC

bool glLoad()
{
#define GL_PROC(name, args, ret) \
    gl##name = reinterpret_cast<decltype(gl##name)>(SDL_GL_GetProcAddress("gl" #name)); \
    if (gl##name == nullptr) { \
        return false; \
    }
#include "gl_procs.h"
#undef GL_PROC
    return true;
}

bool glErrorChecksEnabled()
{
    static const bool on = SDL_getenv("F2CE_GL_DEBUG") != nullptr;
    return on;
}

} // namespace scene_renderer
} // namespace fallout
