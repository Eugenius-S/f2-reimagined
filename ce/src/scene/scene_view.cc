#include "scene_view.h"
#include "camera_turn.h"

#include "../dev_output.h"

#include <algorithm>
#include <ctype.h>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <vector>
#include <stdio.h>
#include <string.h>

#include <SDL.h>

#include "preview_policy.h"
#include "scene_build.h"
#include "scene_settings.h"
#include "scene_lights.h"
#include "scene_objects.h"
#include "menu_text.h"
#include "../art.h"
#include "../devtools/devtools.h"
#include "../dialog.h"
#include "../display_monitor.h"
#include "../map_edge.h"
#include "../object.h"
#include "../scene_renderer/game_view.h"
#include "../scene_renderer/perf.h"
#include "../scene_renderer/ground_pick.h"
#include "../scene_renderer/picking.h"
#include "../combat.h"
#include "../game.h"
#include "../interface.h"
#include "../map.h"
#include "../movie.h"
#include "../platform_compat.h"
#include "../scene_renderer/camera.h"
#include "../scene_renderer/live_objects.h"
#include "../scene_renderer/map_clip.h"
#include "../scene_renderer/build_wait.h"
#include "../scene_renderer/static_scene.h"
#include "../scene_renderer/sprite_card.h"
#include "../settings.h"
#include "../tile.h"
#include "../window_manager.h"

namespace fallout {
namespace scene {

namespace {

View gView = View::Off;
bool gOriginalShown = false; // Ctrl+F10 asked for the game as it was
bool gCameraGlide = true;
// The wheel sets a target zoom (settings.scene3d.zoom) and the shown zoom
// eases there, so the picture glides instead of jumping a step at a time.
constexpr double kZoomGlideSeconds = 0.2;
double gZoomShown = 0.0; // the zoom applied last frame (0: nothing applied yet)
bool gZoomGliding = false;
double gZoomGlideFrom = 1.0;
double gZoomGlideStartVelocity = 0.0;
double gZoomGlideVelocity = 0.0;
Uint64 gZoomGlideStarted = 0;
// The map elevation whose scene the renderer holds, and the last one that
// could not be loaded (not tried again until the game leaves it).
std::string gLoadedMap;
int gLoadedElevation = -1;
View gLoadedView = View::Off;
std::string gFailedMap;
int gFailedElevation = -1;
std::string gFailure;
PreviewBlockReason gLastBlockReason = PreviewBlockReason::None;
bool gPolicyLogged = false;
std::uint64_t gCleanEdgesRevision = static_cast<std::uint64_t>(-1);
std::string gClipMap;
int gClipElevation = -1;
bool gClipBottomExtended = false;
unsigned gClipEdgeSerial = 0;
std::string gClipFailureMap;
int gClipFailureElevation = -1;
std::uint64_t gClipFailureRevision = static_cast<std::uint64_t>(-1);
double cameraTurnDegrees();

void stripMapExtension(const char* name, char* out, size_t size)
{
    size_t len = strlen(name);
    if (len > 4
        && (compat_stricmp(name + len - 4, ".map") == 0
            || compat_stricmp(name + len - 4, ".sav") == 0)) {
        len -= 4;
    }
    if (len >= size) {
        len = size - 1;
    }
    memcpy(out, name, len);
    out[len] = '\0';
}

std::filesystem::path gameRootPath()
{
    if (settings.system.master_dat_path.empty()) {
        return std::filesystem::current_path();
    }
    std::filesystem::path datPath(settings.system.master_dat_path);
    if (!datPath.is_absolute()) {
        datPath = std::filesystem::current_path() / datPath;
    }
    std::filesystem::path root = datPath.parent_path();
    if (compat_stricmp(root.filename().string().c_str(), "data") == 0) {
        root = root.parent_path();
    }
    return root;
}

// The current map's name as the exporter writes it (kladwtwn), and elevation.
std::string currentMapName()
{
    char base[64];
    stripMapExtension(gMapHeader.name, base, sizeof(base));
    std::string name = base;
    for (char& ch : name) ch = static_cast<char>(tolower(static_cast<unsigned char>(ch)));
    return name;
}

scene_renderer::MapClip authoredMapClip()
{
    using namespace scene_renderer;
    const MapClip floorClip = staticFloorMapClip();
    if (!mapEdgeIsLoaded()) return floorClip;
    const MapElevation elevation = static_cast<MapElevation>(gElevation);

    if (mapEdgeHasSquareRect(elevation)) {
        Rect square;
        mapEdgeGetSquareRect(elevation, &square);
        if (square.left < 0 || square.left > 99 || square.right < 0 || square.right > 99
            || square.top < 0 || square.top > 99 || square.bottom < 0 || square.bottom > 99) return floorClip;
        const double left = std::min(square.left, square.right) - 50.0;
        const double right = std::max(square.left, square.right) + 1.0 - 50.0;
        const double top = std::min(square.top, square.bottom) - 50.0;
        const double bottom = std::max(square.top, square.bottom) + 1.0 - 50.0;
        return makeMapClipRect(left, top, right, bottom, MapClipSource::Authored);
    }

    const EdgeElevationData& data = mapEdgeGetElevationData(elevation);
    if (data.zones.empty()) return floorClip;
    int minX = 0, maxX = 0, minY = 0, maxY = 0;
    bool haveZone = false;
    for (const EdgeZone& zone : data.zones) {
        const int left = std::min(zone.pixelRect.left, zone.pixelRect.right);
        const int right = std::max(zone.pixelRect.left, zone.pixelRect.right);
        const int top = std::min(zone.pixelRect.top, zone.pixelRect.bottom);
        const int bottom = std::max(zone.pixelRect.top, zone.pixelRect.bottom);
        if (left == right || top == bottom) continue;
        if (!haveZone) {
            minX = left; maxX = right; minY = top; maxY = bottom;
            haveZone = true;
        } else {
            minX = std::min(minX, left); maxX = std::max(maxX, right);
            minY = std::min(minY, top); maxY = std::max(maxY, bottom);
        }
    }
    if (!haveZone) return floorClip;

    // EDG v1 offsets are a different map coordinate origin and have opposite X
    // handedness from game pixels. Anchor at the same known hex, then apply the
    // game's own projection inverse to ground. Envelope zones before conversion
    // so walking across a zone never changes the cut.
    int anchorEdgeX = 0, anchorEdgeY = 0;
    tileToPixelOffset(0, anchorEdgeX, anchorEdgeY);
    double anchorGame[2];
    gameViewTilePixel(0, anchorGame);
    const double edgeCorners[4][2] = {
        { static_cast<double>(minX), static_cast<double>(minY) },
        { static_cast<double>(maxX), static_cast<double>(minY) },
        { static_cast<double>(maxX), static_cast<double>(maxY) },
        { static_cast<double>(minX), static_cast<double>(maxY) },
    };
    SpriteCardProjection projection;
    if (!scene_renderer::currentGameProjection(&projection)) return floorClip;
    MapClipPoint points[4];
    for (int index = 0; index < 4; index++) {
        double gameX = 0.0, gameY = 0.0;
        edgeOffsetToGamePixel(edgeCorners[index][0], edgeCorners[index][1], anchorEdgeX, anchorEdgeY,
            anchorGame[0], anchorGame[1], &gameX, &gameY);
        if (!spritePixelToGround(projection, gameX, gameY, &points[index].x, &points[index].z)) return floorClip;
    }
    const MapClip authored = makeMapClip(points, MapClipSource::Authored);
    return authored.enabled ? authored : floorClip;
}

// Keep the lower viewport edge on drawn floor. EDG clips are authored in the
// game map, but the exported floor hull can extend farther at the bottom.
// Expand only plane 2 (the bottom edge in authoredMapClip's perimeter order)
// to cover the floor hull; all other edges stay authored. This also handles
// squareRect clips whose axis-aligned edge normals do not match the floor hull.
bool extendAuthoredBottomToFloor(scene_renderer::MapClip& clip)
{
    using namespace scene_renderer;
    if (!clip.enabled || clip.source != MapClipSource::Authored) return false;
    const MapClip floor = staticFloorMapClip();
    if (!floor.enabled) return false;
    return extendMapClipPlaneToFloor(clip, floor, 2);
}

void updateMapClip()
{
    if (gLoadedMap.empty() || gLoadedMap != currentMapName() || gLoadedElevation != static_cast<int>(gElevation)) {
        scene_renderer::setStaticMapClip({});
        gClipMap.clear();
        gClipElevation = -1;
        gClipBottomExtended = false;
        return;
    }
    const std::uint64_t revision = cleanEdgesRevision();
    const bool extendBottom = cleanEdges() && tileSceneOverlayActive() && mapEdgeIsLoaded()
        && mapEdgeTilePastBottomStop(gCenterTile);
    if (gClipMap == gLoadedMap && gClipElevation == gLoadedElevation
        && gCleanEdgesRevision == revision && gClipBottomExtended == extendBottom
        && gClipEdgeSerial == mapEdgeSerial()) return;
    scene_renderer::MapClip clip;
    if (cleanEdges()) {
        const char* requested = devtools::isEnabled() ? SDL_getenv("F2CE_DEV_CLEAN_EDGES") : nullptr;
        if (requested != nullptr && compat_stricmp(requested, "floor") == 0) {
            clip = scene_renderer::staticFloorMapClip();
        } else if (requested != nullptr && compat_stricmp(requested, "off") == 0) {
            clip = {};
        } else {
            clip = authoredMapClip();
        }
    }
    if (extendBottom) extendAuthoredBottomToFloor(clip);
    scene_renderer::setStaticMapClip(clip);
    if (cleanEdges() && !clip.enabled
        && (gClipFailureMap != gLoadedMap || gClipFailureElevation != gLoadedElevation
            || gClipFailureRevision != revision)) {
        const char* reason = "no valid authored edge rectangle or floor envelope";
        printf("scene: %s elevation %d clean map edges disabled: %s\n",
            gLoadedMap.c_str(), gLoadedElevation, reason);
        fflush(stdout);
        if (FILE* log = fopen((dev_output::outputDirectory("logs") / "scene.log").string().c_str(), "a")) {
            fprintf(log, "scene: %s elevation %d clean map edges disabled: %s\n",
                gLoadedMap.c_str(), gLoadedElevation, reason);
            fclose(log);
        }
        gClipFailureMap = gLoadedMap;
        gClipFailureElevation = gLoadedElevation;
        gClipFailureRevision = revision;
    }
    gClipMap = gLoadedMap;
    gClipElevation = gLoadedElevation;
    gClipBottomExtended = extendBottom;
    gClipEdgeSerial = mapEdgeSerial();
    gCleanEdgesRevision = revision;
}

// scene.json's `"daylight": true`: the map elevation is under the sky. An
// export without the key (or without the file) has no sun.
bool readDaylight(const std::filesystem::path& cacheDirectory)
{
    std::ifstream file(cacheDirectory / "scene.json", std::ios::binary);
    if (!file) return false;
    std::ostringstream text;
    text << file.rdbuf();
    const std::string json = text.str();
    const size_t key = json.find("\"daylight\"");
    if (key == std::string::npos) return false;
    size_t value = json.find(':', key);
    if (value == std::string::npos) return false;
    value = json.find_first_not_of(" \t\r\n", value + 1);
    return value != std::string::npos && json.compare(value, 4, "true") == 0;
}

// The map's scene cache directory: the dev cache override when the dev
// tools run, the game's mod-cache otherwise.
static std::filesystem::path scenesRootPath()
{
    // F2CE_SCENE_CACHE (dev tools only): a worktree's own scene cache, so a test
    // run of a new exporter leaves the cache of a game running alongside alone.
    const char* devCache = devtools::isEnabled() ? SDL_getenv("F2CE_SCENE_CACHE") : nullptr;
    if (devCache != nullptr && devCache[0] != '\0') {
        return std::filesystem::path(devCache);
    }
    return gameRootPath() / "mod-cache" / "scene";
}

static std::filesystem::path sceneCacheDirectory(const std::string& map, int elevation)
{
    return scenesRootPath() / map / std::to_string(elevation);
}

// The 3D toggle's refusal (211) goes to the game's status line, in the
// game's language: which map has no scene, or what failed. A refused
// enable that stays silent reads to the player as a button that does not
// work. Dev panels keep their own status line on top of this.
static void reportToggleRefusal(const std::string& rawMessage)
{
    if (gameGetState() != GameState::Normal || rawMessage.empty()) return;
    const char* reason = nullptr;
    std::string map;
    int elevation = 0;
    if (rawMessage == "3D scene is disabled in settings") {
        reason = "disabled";
    } else if (rawMessage == "no map is loaded") {
        reason = "no-map";
    } else {
        map = currentMapName();
        elevation = static_cast<int>(gElevation);
        // A map that was never exported has no cache directory at all; every
        // other failure means the scene exists but did not load.
        const bool sceneless = !std::filesystem::exists(sceneCacheDirectory(map, elevation));
        reason = sceneless ? "no-scene" : "load-failed";
    }
    const std::string text = toggleRefusalText(reason, map, elevation, settings.system.language);
    displayMonitorAddMessage(text.c_str());
}

// Loads what `view` needs of the current map's scene.
bool loadScene(View view, const std::string& map, int elevation, std::string* message)
{
    const std::filesystem::path cacheDirectory = sceneCacheDirectory(map, elevation);
    printf("scene: cache %s\n", cacheDirectory.string().c_str());
    fflush(stdout);
    std::string error;
    scene_renderer::PerfScope total("scene_total");
    {
        scene_renderer::PerfScope stage("floor");
        if (!scene_renderer::loadStaticFloor(cacheDirectory, &error)) {
            *message = "floor preview unavailable: " + error;
            return false;
        }
    }
    setSceneDaylight(readDaylight(cacheDirectory));
    if (view != View::Floor) {
        scene_renderer::PerfScope stage("walls");
        if (!scene_renderer::loadStaticWalls(cacheDirectory, &error)) {
            *message = "wall preview unavailable: " + error;
            return false;
        }
    }
    if (view == View::Full && !scene_renderer::loadStaticProjection(cacheDirectory, &error)) {
        *message = "hero projection unavailable: " + error;
        return false;
    }
    if (view == View::Full) {
        scene_renderer::SpriteCardProjection projection;
        if (scene_renderer::currentGameProjection(&projection)) {
            scene_renderer::setStaticFloorMapProjection(projection);
        }
    }
    gClipMap.clear(); // Full loads add the projection needed to convert legacy EDG pixels.
    gClipElevation = -1;
    gCleanEdgesRevision = static_cast<std::uint64_t>(-1);
    scene_renderer::perfLoadMark("scene_loaded");
    return true;
}

// Roofs as the game shows them now: hidden over the building the hero is in
// (tile_fill_roof), all of them off with the roof toggle; and the hero's
// see-through circle over them (the egg, as tileRenderRoof uses it).
void updateRoofsFromGame()
{
    static unsigned char visible[SQUARE_GRID_SIZE];
    const bool roofsOn = tileRoofIsVisible() != 0;
    const TileData* squares = _square[gElevation];
    for (int index = 0; index < SQUARE_GRID_SIZE; index++) {
        // The upper 16 bits: the roof's 12-bit frame and 4 flag bits.
        const unsigned int roof = static_cast<unsigned int>(squares->tileFid[index]) >> 16;
        const unsigned int flags = (roof >> 12) & 0xF;
        visible[index] = roofsOn && (flags & 0x3 /* TemporarilyHidden | AlwaysHidden */) == 0 ? 1 : 0;
    }
    scene_renderer::setStaticRoofVisibility(visible);

    static std::vector<unsigned char> eggMask;
    static int eggWidth = 0, eggHeight = 0, eggX = 0, eggY = 0;
    if (eggMask.empty() && gEgg != nullptr) {
        FrmImage image;
        if (image.lock(FrmId(gEgg)) && image.getData() != nullptr) {
            eggWidth = image.getWidth();
            eggHeight = image.getHeight();
            eggX = image.getXOffset();
            eggY = image.getYOffset();
            eggMask.assign(image.getData(), image.getData() + eggWidth * eggHeight);
        }
    }
    double pixel[2], x, y;
    if (eggMask.empty() || gEgg == nullptr || !hexGridTileIsValid(gEgg->tile)) {
        scene_renderer::clearStaticEgg();
        return;
    }
    scene_renderer::gameViewTilePixel(gEgg->tile, pixel);
    if (!scene_renderer::staticScreenFromGamePixel(pixel[0] + eggX + gEgg->x, pixel[1] + eggY + gEgg->y, &x, &y)) {
        scene_renderer::clearStaticEgg();
        return;
    }
    scene_renderer::setStaticEgg(eggMask.data(), eggWidth, eggHeight,
        static_cast<int>(std::lround(x)) - eggWidth / 2, static_cast<int>(std::lround(y)) - eggHeight + 1);
}

PreviewRuntimeState runtimeState()
{
    return {
        gameGetState() == GameState::Normal,
        gameModeShowsMap(static_cast<unsigned int>(GameMode::getCurrentGameMode())),
        !isoIsDisabled(),
        _moviePlaying() == 0,
    };
}

const char* blockReasonName(PreviewBlockReason reason)
{
    switch (reason) {
    case PreviewBlockReason::None: return "available";
    case PreviewBlockReason::GameState: return "game-state";
    case PreviewBlockReason::GameMode: return "game-mode";
    case PreviewBlockReason::IsoDisabled: return "iso-disabled";
    case PreviewBlockReason::Movie: return "movie";
    }
    return "unknown";
}

PreviewBlockReason currentBlockReason()
{
    const PreviewBlockReason reason = previewBlockReason(runtimeState());
    if (!gPolicyLogged || reason != gLastBlockReason) {
        printf("scene: preview policy=%s gameState=%d gameMode=0x%x uiDisabled=%d isoDisabled=%d combat=%d dialog=%d movie=%d\n",
            blockReasonName(reason), static_cast<int>(gameGetState()),
            static_cast<unsigned int>(GameMode::getCurrentGameMode()), gameUiIsDisabled() ? 1 : 0,
            isoIsDisabled() ? 1 : 0, isInCombat() ? 1 : 0, _inDialog != 0 ? 1 : 0,
            _moviePlaying() != 0 ? 1 : 0);
        fflush(stdout);
        gLastBlockReason = reason;
        gPolicyLogged = true;
    }
    return reason;
}

} // namespace

// 273: the one on-entry exporter run. The game keeps drawing while it works.
scene_build::SceneBuild gSceneBuild;
// The map whose scene cache is being built on entry; the 3D view stays up
// and the map's 2D art lies on the ground plane until it finishes.
std::string gBuildWaitingMap;

bool showsCurrentMap()
{
    return !gLoadedMap.empty() && gLoadedMap == currentMapName() && gLoadedElevation == static_cast<int>(gElevation);
}

// 273: the current map's cache is being built and the waiting state (its 2D
// art on the ground plane) stands in for the scene.
bool buildWaiting()
{
    return !gBuildWaitingMap.empty() && gBuildWaitingMap == currentMapName();
}

bool bottomViewHasDrawnGround(int tile)
{
    if (!mapEdgeTilePastBottomStop(tile) || !showsCurrentMap() || activeView() != View::Full
        || (scene_renderer::staticFlyCameraActive() && !cameraTurned())) return false;

    scene_renderer::GameView baseView;
    if (!scene_renderer::gameViewAtCenterTile(tile, &baseView)) return false;
    Rect mapWindow;
    if (windowGetRect(gIsoWindow, &mapWindow) != 0) return false;
    Rect interfaceBar;
    const int visibleBottom = windowGetRect(gInterfaceBarWindow, &interfaceBar) == 0
        ? std::min(mapWindow.bottom + 1, interfaceBar.top) : mapWindow.bottom + 1;
    const int left = std::max(0, mapWindow.left);
    const int right = std::min(baseView.logicalWidth, mapWindow.right + 1);
    const int bottom = std::min(baseView.logicalHeight, visibleBottom);
    if (left >= right || bottom <= mapWindow.top) return false;

    baseView.screenCenterSet = true;
    baseView.screenCenter[0] = mapWindow.left + (rectGetWidth(&mapWindow) - 32) / 2.0
        + mapEdgeGetTileXAlignment();
    baseView.screenCenter[1] = mapWindow.top + (rectGetHeight(&mapWindow) - 16) / 2.0;
    scene_renderer::MapClip clip = scene_renderer::staticMapClip();
    if (!extendAuthoredBottomToFloor(clip)) return false;
    const bool turned = cameraTurned();
    const scene_renderer::GroundPickViewport pickViewport {
        baseView.logicalWidth, baseView.logicalHeight, baseView.logicalWidth, baseView.logicalHeight,
        baseView.logicalWidth, baseView.logicalHeight,
    };

    const scene_renderer::GameView& view = baseView;
    scene_renderer::GroundPickProjection projection {};
    scene_renderer::GroundPickCamera camera {};
    if (turned) {
        double eye[3], centerX, centerZ;
        double leftX, leftZ, rightX, rightZ;
        const double middleX = view.screenCenter[0];
        const double middleY = view.screenCenter[1];
        if (!scene_renderer::gameViewEye(view, eye)
            || !scene_renderer::gameViewPickGround(view, middleX, middleY, &centerX, &centerZ)
            || !scene_renderer::gameViewPickGround(view, 0.0, middleY, &leftX, &leftZ)
            || !scene_renderer::gameViewPickGround(view, view.logicalWidth, middleY, &rightX, &rightZ)) return false;
        const double to[3] = { centerX - eye[0], -eye[1], centerZ - eye[2] };
        const double length = std::sqrt(to[0] * to[0] + to[1] * to[1] + to[2] * to[2]);
        if (!(length > 0.0) || !(to[1] < -0.01)) return false;
        const scene_renderer::StaticPreviewCamera preview = scene_renderer::staticPreviewCamera(true);
        const double width = std::hypot(rightX - leftX, rightZ - leftZ);
        const double aspect = static_cast<double>(view.logicalWidth) / view.logicalHeight;
        const double distance = width / (2.0 * aspect * std::tan(preview.fovY * 3.14159265358979323846 / 360.0));
        const double yaw = std::atan2(to[2], to[0]) + cameraTurnDegrees() * 3.14159265358979323846 / 180.0;
        const double pitch = std::asin(to[1] / length);
        const double forward[3] = {
            std::cos(pitch) * std::cos(yaw), std::sin(pitch), std::cos(pitch) * std::sin(yaw),
        };
        camera = {
            { centerX - forward[0] * distance, -forward[1] * distance,
                centerZ - forward[2] * distance },
            { centerX - forward[0] * distance + forward[0], -forward[1] * distance + forward[1],
                centerZ - forward[2] * distance + forward[2] },
            preview.fovY, true,
        };
        scene_renderer::SpriteCardProjection spriteProjection;
        if (!scene_renderer::currentGameProjection(&spriteProjection)) return false;
        projection = { { spriteProjection.squ[0], spriteProjection.squ[1] },
            { spriteProjection.sqv[0], spriteProjection.sqv[1] },
            { spriteProjection.origin[0], spriteProjection.origin[1] } };
    }

    // The last viewport row above the interface must map to drawn floor, using
    // the active zoom, camera turn and clip. No color or brightness test.
    for (int x = left; x < right; x++) {
        double worldX, worldZ;
        bool picked;
        if (turned) {
            scene_renderer::GroundPickResult result {};
            picked = scene_renderer::pickGroundTile(projection, camera, pickViewport,
                x + 0.5, bottom - 0.5, &result);
            worldX = result.worldX;
            worldZ = result.worldZ;
        } else {
            picked = scene_renderer::gameViewPickGround(view, x + 0.5, bottom - 0.5, &worldX, &worldZ);
        }
        if (!picked || !scene_renderer::staticFloorHasGround(worldX, worldZ)
            || !scene_renderer::mapClipContains(clip, worldX, worldZ)) return false;
    }
    return true;
}

// One line to the console and scene.log, the way the load fallback lines go.
static void logSceneLine(const std::string& line)
{
    printf("%s\n", line.c_str());
    fflush(stdout);
    // A file too: the Windows build has no console.
    if (FILE* log = fopen((dev_output::outputDirectory("logs") / "scene.log").string().c_str(), "a")) {
        fprintf(log, "%s\n", line.c_str());
        fclose(log);
    }
}

void syncToMap()
{
    const std::string map = currentMapName();
    const int elevation = static_cast<int>(gElevation);
    // 273: an on-entry build that finished clears the failure so the load
    // below retries with the fresh cache in the same frame.
    if (gSceneBuild.status() == scene_build::SceneBuild::Status::Running) {
        const scene_build::SceneBuild::Status built = gSceneBuild.poll();
        if (built == scene_build::SceneBuild::Status::Succeeded) {
            logSceneLine("scene: " + gSceneBuild.map() + " scene data built in "
                + std::to_string(gSceneBuild.elapsedMs()) + " ms; loading it");
            gFailedMap.clear();
            gFailedElevation = -1;
            // The waiting plane yields to the real scene this frame; if the
            // fresh cache still fails to load below, the rebuild that starts
            // then puts it back up.
            gBuildWaitingMap.clear();
            scene_renderer::setWaitPlane(false);
        } else if (built == scene_build::SceneBuild::Status::Failed) {
            logSceneLine("scene: " + gSceneBuild.map() + " scene data could not be built; it stays 2D");
            gBuildWaitingMap.clear();
            scene_renderer::setWaitPlane(false);
        }
    }
    if (!gLoadedMap.empty() && (gLoadedMap != map || gLoadedElevation != elevation)) {
        scene_renderer::unloadStaticScene();
        gLoadedMap.clear();
        gLoadedElevation = -1;
    }
    if (!gBuildWaitingMap.empty() && gBuildWaitingMap != map) {
        // The player went elsewhere mid-build: the waiting state belongs to
        // the map it was started for. The run itself keeps going; entering
        // that map again finds the cache ready.
        gBuildWaitingMap.clear();
        scene_renderer::setWaitPlane(false);
    }
    if (gFailedMap != map || gFailedElevation != elevation) {
        gFailedMap.clear();
        gFailedElevation = -1;
    }
    // Floor < Walls < Full: a view needs what the smaller ones load, and more.
    const bool loaded = !gLoadedMap.empty() && static_cast<int>(gLoadedView) >= static_cast<int>(gView);
    if (gView != View::Off && !map.empty() && gFailedMap.empty() && !loaded) {
        std::string message;
        // 273: a cache written by another exporter version is rebuilt, not
        // loaded: the load attempts are skipped for one, so its old floor
        // never shows while the fresh scene is built.
        const bool staleVersion = scene_build::cacheState(sceneCacheDirectory(map, elevation))
            == scene_build::CacheState::StaleVersion;
        if (staleVersion) {
            message = "scene data is from another exporter version";
        }
        bool sceneLoaded = false;
        if (!staleVersion) {
            sceneLoaded = loadScene(gView, map, elevation, &message);
        }
        if (sceneLoaded) {
            gLoadedMap = map;
            gLoadedElevation = elevation;
            gLoadedView = gView;
            gFailure.clear();
        } else {
            scene_renderer::unloadStaticScene();
            std::string floorError;
            // Keep a usable 3D ground plane when a map's wall or projection
            // sidecar is missing or damaged. This is also the fallback for
            // incomplete exports; it must not silently turn the whole map 2D.
            // A stale-version cache takes neither: it is rebuilt whole.
            if (!staleVersion && gView != View::Floor
                && loadScene(View::Floor, map, elevation, &floorError)) {
                gLoadedMap = map;
                gLoadedElevation = elevation;
                gLoadedView = View::Floor;
                gFailedMap = map;
                gFailedElevation = elevation;
                gFailure = message;
                const std::string line = "scene: " + map + " elevation " + std::to_string(elevation)
                    + " uses 3D ground plane: " + message;
                printf("%s\n", line.c_str());
                fflush(stdout);
                if (FILE* log = fopen((dev_output::outputDirectory("logs") / "scene.log").string().c_str(), "a")) {
                    fprintf(log, "%s\n", line.c_str());
                    fclose(log);
                }
            } else {
                // 273: with the shipped exporter beside the game, a missing
                // or stale cache is built on entry instead of staying 2D. A
                // cache whose provenance still matches but whose files do
                // not load is damaged: only --force rewrites it. While the
                // build runs the map keeps drawing as it does now; the
                // waiting state on the 3D ground plane is the visual step.
                const scene_build::CacheState state = scene_build::cacheState(sceneCacheDirectory(map, elevation));
                // Stale-version caches need --force too: the exporter's own
                // staleness reads fingerprints, not the version field, so a
                // cache whose files match its tampered provenance would come
                // back "current" and never rewrite the version (a rebuild
                // loop).
                const bool damaged = state != scene_build::CacheState::Missing;
                const char* why = state == scene_build::CacheState::Missing ? "has no scene data"
                    : state == scene_build::CacheState::StaleVersion ? "has scene data from another exporter version"
                    : "has damaged scene data";
                if (gSceneBuild.start(map, scene_build::exporterPath(), gameRootPath(),
                        scenesRootPath(), damaged)) {
                    logSceneLine("scene: " + map + " elevation " + std::to_string(elevation) + " "
                        + why + " (" + message + "); building it with the shipped exporter");
                    gLoadedMap.clear();
                    gLoadedElevation = -1;
                    gFailedMap = map;
                    gFailedElevation = elevation;
                    gFailure = message;
                    // 273: the 3D view stays up; the map's 2D art lies on the
                    // ground plane until the build finishes.
                    gBuildWaitingMap = map;
                    scene_renderer::setWaitPlane(true);
                } else {
                    gLoadedMap.clear();
                    gLoadedElevation = -1;
                    gFailedMap = map;
                    gFailedElevation = elevation;
                    gFailure = message;
                    printf("scene: %s elevation %d stays 2D: %s\n", map.c_str(), elevation, message.c_str());
                    fflush(stdout);
                    // A file too: the Windows build has no console.
                    if (FILE* log = fopen((dev_output::outputDirectory("logs") / "scene.log").string().c_str(), "a")) {
                        fprintf(log, "scene: %s elevation %d stays 2D: %s\n", map.c_str(), elevation, message.c_str());
                        fclose(log);
                    }
                }
            }
        }
    }
    updateMapClip();
}

void syncSettings(bool suppressInitialEnable)
{
    // Wait until gameplay has a real map; settings must not activate a scene
    // from the main menu or while a map transition is still being installed.
    if (gMapHeader.name[0] == '\0' || gameGetState() != GameState::Normal) return;

    static SettingsLifecycle lifecycle;
    const SettingsAction action = lifecycle.update(enabled(), settingsRevision(), suppressInitialEnable);
    if (action == SettingsAction::EnableFull) {
        std::string error;
        enable(View::Full, &error);
    } else if (action == SettingsAction::Disable) {
        disable();
    }
}

bool enable(View view, std::string* message)
{
    if (view == View::Off) {
        disable();
        return true;
    }
    if (!settings.scene3d.enabled) {
        *message = "3D scene is disabled in settings";
        reportToggleRefusal(*message);
        return false;
    }
    gView = view;
    gOriginalShown = false;
    // 273: a running build keeps its failure bookkeeping; a re-enable while
    // it works must not log a bogus stays-2D line or refuse the toggle.
    if (!buildWaiting()) {
        gFailedMap.clear();
        gFailedElevation = -1;
    }
    syncToMap();
    if (!showsCurrentMap() && !buildWaiting()) {
        *message = gFailure.empty() ? "no map is loaded" : gFailure;
        reportToggleRefusal(*message);
        return false;
    }
    message->clear();
    return true;
}

bool disable()
{
    if (gView == View::Off) return false;
    gView = View::Off;
    tileSetFreeScroll(false);
    resetHeroLog();
    scene_renderer::clearStaticHero();
    scene_renderer::clearStaticObjects();
    scene_renderer::setStaticCameraCenterTile(-1);
    return true;
}

bool showOriginal()
{
    if (!disable()) return false;
    gOriginalShown = true;
    return true;
}

bool sharpTextWanted()
{
    return settings.scene3d.sharp_text && !(gOriginalShown && gView == View::Off);
}

View requestedView()
{
    return gView;
}

View activeView()
{
    if (!settings.scene3d.enabled || gView == View::Off) return View::Off;
    if (buildWaiting()) return gView; // 273: the waiting plane stands in for the scene
    return showsCurrentMap() ? gLoadedView : View::Off;
}

bool shouldPresent()
{
    return activeView() != View::Off && currentBlockReason() == PreviewBlockReason::None;
}

const char* availability()
{
    if (activeView() == View::Off) return "not-requested";
    return blockReasonName(previewBlockReason(runtimeState()));
}

void setCameraGlide(bool glide)
{
    gCameraGlide = glide;
}

namespace {

double gTurnAt = 0.0; // degrees round, where the camera is
double gTurnTo = 0.0; // and where it goes
double gTurnSpeed = 0.0; // degrees a second, signed, of the eased turn
bool gTurnOwnsCamera = false;
bool gTurnDevOverride = false;
Uint64 gTurnLast = 0;
scene_renderer::StaticFlyCamera gTurnPose = {};

double cameraTurnDegrees()
{
    return gTurnAt;
}

void landCameraTurn()
{
    gTurnAt = gTurnTo = gTurnSpeed = 0.0;
    if (gTurnOwnsCamera) scene_renderer::setStaticFlyCamera(nullptr);
    gTurnOwnsCamera = false;
    gTurnDevOverride = false;
    gTurnLast = 0;
}

} // namespace

bool turnCamera(int steps)
{
    if (!settings.scene3d.camera_rotation || steps == 0) return false;
    if (activeView() != View::Full || !shouldPresent()) return false;
    if (scene_renderer::staticFlyCameraActive() && !gTurnOwnsCamera) return false; // the dev fly camera
    gTurnDevOverride = false;
    if (!settings.scene3d.camera_rotation_unlimited) {
        gTurnAt = limitedCameraTurn(gTurnAt);
        gTurnTo = limitedCameraTurn(gTurnTo);
    }
    gTurnTo = cameraTurnTarget(gTurnTo, steps, settings.scene3d.camera_rotation_unlimited);
    return true;
}

void setCameraTurnNow(double degrees)
{
    gTurnDevOverride = true;
    gTurnAt = gTurnTo = degrees;
    gTurnSpeed = 0.0;
    gTurnLast = 0;
}

bool cameraTurned()
{
    return gTurnOwnsCamera;
}

double cameraTurnAngle()
{
    return gTurnOwnsCamera ? gTurnAt : 0.0;
}

void cameraTurnScrollStepNow(int dx, int dy, int* outDx, int* outDy)
{
    *outDx = dx;
    *outDy = dy;
    if (!gTurnOwnsCamera || gTurnAt == 0.0) return;
    ScrollGroundBasis basis {};
    scene_renderer::StaticFlyCamera pose;
    if (!scene_renderer::gameViewGroundStep(kScrollStepX, kScrollStepY, basis.horizontal, basis.vertical)
        || !scene_renderer::staticGameViewCamera(&pose)) return;
    basis.yaw = pose.yaw;
    basis.pitch = pose.pitch;
    cameraTurnScrollStep(dx, dy, gTurnAt, basis, outDx, outDy);
}

void updateCameraTurn()
{
    if (!gTurnOwnsCamera && gTurnAt == 0.0 && gTurnTo == 0.0) return;
    if (activeView() != View::Full || !shouldPresent()
        || (!gTurnDevOverride && !settings.scene3d.camera_rotation)
        || (scene_renderer::staticFlyCameraActive() && !gTurnOwnsCamera)) {
        landCameraTurn();
        return;
    }
    if (!gTurnDevOverride && !settings.scene3d.camera_rotation_unlimited) {
        gTurnAt = limitedCameraTurn(gTurnAt);
        gTurnTo = limitedCameraTurn(gTurnTo);
    }
    const Uint64 now = SDL_GetPerformanceCounter();
    const double seconds = gTurnLast == 0 ? 0.0
        : static_cast<double>(now - gTurnLast) / static_cast<double>(SDL_GetPerformanceFrequency());
    gTurnLast = now;
    // The step eases in and out; a press mid-turn keeps the speed and bends
    // the turn into the new target.
    const bool landed = cameraTurnEaseStep(gTurnAt, gTurnTo, gTurnSpeed, seconds);
    if (landed && std::fmod(gTurnTo, 360.0) == 0.0) {
        landCameraTurn();
        return;
    }
    // The game's view, circled round the floor point it looks at.
    scene_renderer::StaticFlyCamera pose;
    if (!scene_renderer::staticGameViewCamera(&pose)) return;
    float forward[3], right[3];
    scene_renderer::staticFlyCameraAxes(pose, forward, right);
    if (!(forward[1] < -0.01f)) return;
    const float reach = -pose.eye[1] / forward[1];
    const float target[3] = { pose.eye[0] + forward[0] * reach, 0.0f, pose.eye[2] + forward[2] * reach };
    pose.yaw += static_cast<float>(gTurnAt * 3.14159265358979323846 / 180.0);
    scene_renderer::staticFlyCameraAxes(pose, forward, right);
    for (int i = 0; i < 3; i++) pose.eye[i] = target[i] - forward[i] * reach;
    gTurnPose = pose;
    scene_renderer::setStaticFlyCamera(&gTurnPose);
    gTurnOwnsCamera = true;
}

void adjustZoom(int steps)
{
    if (steps == 0) return;
    if (activeView() != View::Full || !shouldPresent()) return;
    double low = settings.scene3d.zoom_min;
    double high = settings.scene3d.zoom_max;
    if (low > high) std::swap(low, high);
    double zoom = settings.scene3d.zoom;
    zoom *= std::pow(1.25, static_cast<double>(steps));
    if (zoom < low) zoom = low;
    if (zoom > high) zoom = high;
    settings.scene3d.zoom = zoom; // written back on save, kept between sessions
    // Re-anchor on the visible zoom and carry its current velocity into the
    // next glide, so a wheel event during motion does not jump the speed.
    if (gZoomShown <= 0.0) gZoomShown = zoom;
    gZoomGlideStartVelocity = gZoomGliding ? gZoomGlideVelocity : 0.0;
    gZoomGlideFrom = gZoomShown;
    gZoomGlideStarted = SDL_GetPerformanceCounter();
    gZoomGliding = true;
}

// The shown zoom for this frame: the stored one, or the glide toward it.
double shownZoom()
{
    double zoom = settings.scene3d.zoom;
    if (gZoomGliding) {
        const double seconds = static_cast<double>(SDL_GetPerformanceCounter() - gZoomGlideStarted)
            / static_cast<double>(SDL_GetPerformanceFrequency());
        if (seconds < kZoomGlideSeconds) {
            const double t = seconds / kZoomGlideSeconds;
            const double t2 = t * t;
            const double t3 = t2 * t;
            const double h00 = 2.0 * t3 - 3.0 * t2 + 1.0;
            const double h10 = t3 - 2.0 * t2 + t;
            const double h01 = -2.0 * t3 + 3.0 * t2;
            zoom = h00 * gZoomGlideFrom + h10 * kZoomGlideSeconds * gZoomGlideStartVelocity
                + h01 * settings.scene3d.zoom;
            const double dh00 = 6.0 * t2 - 6.0 * t;
            const double dh10 = 3.0 * t2 - 4.0 * t + 1.0;
            const double dh01 = -6.0 * t2 + 6.0 * t;
            gZoomGlideVelocity = (dh00 * gZoomGlideFrom + dh10 * kZoomGlideSeconds * gZoomGlideStartVelocity
                + dh01 * settings.scene3d.zoom) / kZoomGlideSeconds;
        } else {
            gZoomGliding = false;
            gZoomGlideVelocity = 0.0;
        }
    }
    return zoom;
}

RefreshResult refreshForPresent()
{
    if (activeView() != View::Full) return { false, 0.0 };
    // 273: while the cache builds there is no scene to refresh; the waiting
    // plane draws the game's own picture instead of the hero and the walls.
    if (buildWaiting()) return { false, 0.0 };
    const Uint64 start = SDL_GetPerformanceCounter();
    // Follow the stored zoom every frame, so a changed setting applies live;
    // a wheel step glides there instead (see adjustZoom).
    gZoomShown = shownZoom();
    scene_renderer::setCameraZoom(gZoomShown);
    // The game scrolls (screen edges, arrow keys, following the hero); the 3D
    // view goes with it.
    if (!gCameraGlide) scene_renderer::setStaticCameraCenterTile(-1);
    scene_renderer::setStaticCameraCenterTile(gCenterTile);
    const Uint64 cameraDone = SDL_GetPerformanceCounter();
    scene_renderer::perfCpuMark("r_camera");
    // 172: the chain's split of the first refresh (the between-presents gap).
    scene_renderer::perfLoadMark("ff_refresh_camera");
    const bool heroUpdated = updateObjectsFromGame();
    scene_renderer::perfLoadMark("ff_refresh_objects");
    const Uint64 objectsDone = SDL_GetPerformanceCounter();
    updateRoofsFromGame();
    scene_renderer::perfCpuMark("r_roofs");
    updateLightsFromGame();
    scene_renderer::perfCpuMark("r_lights");
    updateSunFromGame();
    scene_renderer::perfCpuMark("r_sun");
    scene_renderer::perfLoadMark("ff_refresh_rest");
    const Uint64 end = SDL_GetPerformanceCounter();
    static int reports = 0;
    if (getenv("F2CE_PERF") != nullptr && ++reports % 120 == 0) {
        const double toMs = 1000.0 / static_cast<double>(SDL_GetPerformanceFrequency());
        printf("perf refresh camera=%.2f objects=%.2f rest=%.2f\n",
            static_cast<double>(cameraDone - start) * toMs,
            static_cast<double>(objectsDone - cameraDone) * toMs,
            static_cast<double>(end - objectsDone) * toMs);
    }
    const double seconds = static_cast<double>(end - start) / SDL_GetPerformanceFrequency();
    return { heroUpdated, seconds };
}

} // namespace scene
} // namespace fallout
