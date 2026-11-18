#include "build_wait.h"

#include "scene_internal.h"
#include "camera.h"
#include "gl_api.h"
#include "sky.h"
#include "static_scene.h"

#include <SDL.h>
#include <vector>

#include "../geometry.h"
#include "../map.h"
#include "../window_manager.h"

namespace fallout {
namespace scene_renderer {

using namespace internal;

namespace {

bool gWaitOn = false;
bool gWaitCaptured = false;
float gPlaneDepth = 75.0f;
unsigned int gWaitTexture = 0;
unsigned int gWaitVao = 0;
unsigned int gWaitVbo = 0;
int gWaitFrameWidth = 0;
int gWaitFrameHeight = 0;

// The map's ground square: 100 tiles across, the picture keeps the screen's
// 4:3 shape inside it (the logical frame is 640x480).
constexpr float kPlaneWidth = 100.0f;

void ensureQuad()
{
    if (gWaitVao != 0) return;
    // FloorVertex {x, y, z, u, v}: the frame's top edge lies to the north
    // (-z), so the fixed preview camera (from the south) reads it like a
    // poster on the floor. Cull is off in the draw path.
    const float hx = kPlaneWidth / 2.0f;
    const float hz = gPlaneDepth / 2.0f;
    const float quad[] = {
        -hx, 0.0f, -hz, 0.0f, 0.0f,
        hx, 0.0f, -hz, 1.0f, 0.0f,
        hx, 0.0f, hz, 1.0f, 1.0f,
        -hx, 0.0f, -hz, 0.0f, 0.0f,
        hx, 0.0f, hz, 1.0f, 1.0f,
        -hx, 0.0f, hz, 0.0f, 1.0f,
    };
    glGenVertexArrays(1, &gWaitVao);
    glBindVertexArray(gWaitVao);
    glGenBuffers(1, &gWaitVbo);
    glBindBuffer(kArrayBuffer, gWaitVbo);
    glBufferData(kArrayBuffer, static_cast<long long>(sizeof(quad)), quad, kStaticDraw);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, 0x1406 /* GL_FLOAT */, 0, sizeof(float) * 5, reinterpret_cast<const void*>(0));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, 0x1406, 0, sizeof(float) * 5, reinterpret_cast<const void*>(3 * sizeof(float)));
    glBindVertexArray(0);
}

} // namespace

bool waitPlaneActive()
{
    return gWaitOn;
}

void setWaitPlane(bool on)
{
    gWaitOn = on;
    if (on) {
        // The first frame after the build starts still holds the game's full
        // 2D map picture (the game stops redrawing it once the 3D view
        // presents); it is frozen as the waiting art.
        gWaitCaptured = false;
    } else {
        gWaitFrameWidth = gWaitFrameHeight = 0;
    }
}

void uploadWaitFrame(SDL_Surface* indexed)
{
    if (!gWaitOn || gWaitCaptured || indexed == nullptr || indexed->format == nullptr
        || indexed->format->palette == nullptr || indexed->pixels == nullptr) {
        return;
    }
    // The map's 2D art lives in the iso window's own buffer: the screen
    // surface only receives it in the 2D present path, which the waiting
    // state replaces. The buffer already holds the whole map.
    Rect isoRect;
    if (windowGetRect(gIsoWindow, &isoRect) != 0) return;
    const unsigned char* isoBuffer = windowGetBuffer(gIsoWindow);
    if (isoBuffer == nullptr) return;
    const int width = rectGetWidth(&isoRect);
    const int height = rectGetHeight(&isoRect);
    if (width <= 0 || height <= 0) return;
    const int pitch = rectGetWidth(&isoRect);
    // The tick leaves the buffer empty while the 3D view presents, and the
    // build can start while the map data is still loading: paint the map
    // again until it really shows up, and only then freeze it.
    mapRenderWholeIsoWindow();
    // A loading screen or a not-yet-rendered map leaves the buffer uniform:
    // keep waiting for the real map art instead of freezing that.
    size_t artPixels = 0;
    for (size_t at = 0; at < static_cast<size_t>(width) * height; at += 7) {
        if (isoBuffer[at] != 0) artPixels++;
    }
    if (artPixels * 7 < static_cast<size_t>(width) * height / 20) return;

    std::vector<unsigned char> rgba(static_cast<size_t>(width) * height * 4);
    const SDL_Palette* palette = indexed->format->palette;
    for (int y = 0; y < height; y++) {
        const unsigned char* row = isoBuffer + static_cast<size_t>(y) * pitch;
        unsigned char* out = rgba.data() + static_cast<size_t>(y) * width * 4;
        for (int x = 0; x < width; x++) {
            const unsigned char index = row[x];
            if (index >= palette->ncolors) continue;
            const SDL_Color& color = palette->colors[index];
            out[x * 4 + 0] = color.r;
            out[x * 4 + 1] = color.g;
            out[x * 4 + 2] = color.b;
            out[x * 4 + 3] = 255;
        }
    }
    if (gWaitTexture == 0) glGenTextures(1, &gWaitTexture);
    glBindTexture(kTexture2D, gWaitTexture);
    glPixelStorei(kUnpackRowLength, 0);
    glTexImage2D(kTexture2D, 0, kRgba8, width, height, 0, kRGBA, kUnsignedByte, rgba.data());
    glTexParameteri(kTexture2D, kTextureMinFilter, kLinear);
    glTexParameteri(kTexture2D, kTextureMagFilter, kLinear);
    glTexParameteri(kTexture2D, kTextureWrapS, kClampToEdge);
    glTexParameteri(kTexture2D, kTextureWrapT, kClampToEdge);
    gWaitFrameWidth = width;
    gWaitFrameHeight = height;
    // The plane keeps the picture's aspect on the map square's width.
    gPlaneDepth = kPlaneWidth * static_cast<float>(height) / static_cast<float>(width);
    {
        ensureQuad(); // creates the VAO/VBO when the plane never drew yet
        const float hx = kPlaneWidth / 2.0f;
        const float hz = gPlaneDepth / 2.0f;
        const float quad[] = {
            -hx, 0.0f, -hz, 0.0f, 0.0f,
            hx, 0.0f, -hz, 1.0f, 0.0f,
            hx, 0.0f, hz, 1.0f, 1.0f,
            -hx, 0.0f, -hz, 0.0f, 0.0f,
            hx, 0.0f, hz, 1.0f, 1.0f,
            -hx, 0.0f, hz, 0.0f, 1.0f,
        };
        glBindVertexArray(gWaitVao);
        glBindBuffer(kArrayBuffer, gWaitVbo);
        glBufferData(kArrayBuffer, static_cast<long long>(sizeof(quad)), quad, kStaticDraw);
        glBindVertexArray(0);
    }
    gWaitCaptured = true;
}

void drawWaitPlane()
{
    if (!gWaitOn || !gInitialized || gWaitTexture == 0 || gWaitFrameWidth <= 0) return;
    ensureQuad();
    int windowWidth = 0, windowHeight = 0;
    SDL_GL_GetDrawableSize(gWindow, &windowWidth, &windowHeight);
    if (windowWidth <= 0 || windowHeight <= 0) return;
    glViewport(0, 0, windowWidth, windowHeight);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(kColorBufferBit | kDepthBufferBit);
    int x, y, width, height;
    if (!fitRect(gLogicalWidth, gLogicalHeight, windowWidth, windowHeight, &x, &y, &width, &height)) return;
    glViewport(x, y, width, height);

    drawSceneSky(static_cast<float>(width) / height, width, height, false);

    // The scene shader is normally created with the first floor load; the
    // waiting state runs before any of it (273).
    if (gSceneProgram == 0) {
        std::string shaderError;
        if (!createSceneShader(&shaderError)) return;
    }

    float mvp[16];
    makeFloorMvp(static_cast<float>(width) / height, staticPreviewCamera(false), mvp);
    glEnable(kDepthTest);
    glDepthFunc(kLess);
    glDepthMask(1);
    glDisable(kCullFace);
    glDisable(kBlend);
    glUseProgram(gSceneProgram);
    glUniformMatrix4fv(gSceneMvpLocation, 1, 0, mvp);
    glUniform1i(gSceneTextureLocation, 0);
    glUniform1i(gSceneTexturedLocation, 1);
    glUniform1i(gSceneCloseupFilterLocation, 0);
    glUniform1i(gSceneDepthAtLocation, 0);
    glUniform4f(gSceneViewportLocation, static_cast<float>(x), static_cast<float>(y),
        static_cast<float>(width), static_cast<float>(height));
    glUniform1i(gSceneTranslucentLocation, 0);
    glUniform1i(gScenePickLocation, 0);
    glUniform1i(gSceneSeeThroughLocation, 0);
    glUniform1i(gSceneEggPassLocation, 0);
    glUniform2f(gSceneRoofClipLocation, 0.0f, 0.0f);
    glUniform1i(gSceneCloudOnLocation, 0);
    glUniform1i(gSceneShadowOnLocation, 0);
    glUniform1i(gSceneShadowSelfFromVertexLocation, 0);
    glUniform1i(gSceneLightModeLocation, 0);
    glUniform1i(gSceneShadowGroupLocation, -1);
    bindStaticMapClipUniforms();
    glActiveTexture(kTexture0);
    glBindVertexArray(gWaitVao);
    glBindTexture(kTexture2D, gWaitTexture);
    glDrawArrays(kTriangles, 0, 6);
    glBindVertexArray(0);
}

void destroyWaitPlane()
{
    if (gContext != nullptr) {
        if (gWaitTexture != 0) glDeleteTextures(1, &gWaitTexture);
        if (gWaitVbo != 0) glDeleteBuffers(1, &gWaitVbo);
        if (gWaitVao != 0) glDeleteVertexArrays(1, &gWaitVao);
    }
    gWaitTexture = 0;
    gWaitVbo = 0;
    gWaitVao = 0;
    gWaitOn = false;
    gWaitFrameWidth = gWaitFrameHeight = 0;
}

} // namespace scene_renderer
} // namespace fallout
