#ifndef FALLOUT_SCENE_RENDERER_LIVE_OBJECTS_H
#define FALLOUT_SCENE_RENDERER_LIVE_OBJECTS_H

#include <cstddef>
#include <string>

namespace fallout {
namespace scene_renderer {

struct StaticHeroFrame {
    int tile;
    int elevation;
    int fid;
    int frame;
    int rotation;
    int objectX;
    int objectY;
    int artXOffset;
    int artYOffset;
    int width;
    int height;
    const unsigned char* rgba;
    size_t rgbaSize;
    int stack = 0; // sprites the game paints on the same hex before this one
    int pickId = -1; // the caller's id for pickStaticObjects
};

// One live map object (critter, item, scenery, door) in its current frame.
// `rgba` may be null when staticObjectArtCached() says the frame is uploaded.
struct StaticObjectFrame {
    int tile;
    int fid;
    int frame;
    int rotation;
    int objectX;
    int objectY;
    int artXOffset;
    int artYOffset;
    int width;
    int height;
    bool flat;
    const unsigned char* rgba;
    size_t rgbaSize;
    int stack = 0; // sprites the game paints on the same hex before this one
    // The engine's translucent look (_dark_translucent_trans_buf_to_buf): the
    // tint's RGB, and whether the glass gray table is used; tint < 0: opaque.
    float tint[3] = { -1.0f, -1.0f, -1.0f };
    bool glassGray = false;
    // The engine paints it at full light, whatever its hex (OBJECT_TRANS_WALL).
    bool fullLight = false;
    int pickId = -1; // the caller's id for pickStaticObjects, -1: never picked
    // Drawn by walls.bin (a wall, furniture made solid): its card is only
    // picked. Seen from the game's camera a solid covers its sprite's pixels.
    bool pickOnly = false;
    // The art variant: 1 = the shadow-free sprite of a painted-shadow board
    // (shadows.bin), so its texture caches apart from the original's.
    int variant = 0;
    // A wall or scenery the hero stands behind: the egg around him cuts it.
    bool seeThrough = false;
    // OBJECT_NO_BLOCK: ground clutter (scrub, weeds) whatever its height; its
    // card reads the sun shadow as the ground does (109).
    bool noBlock = false;
    // 241: the art set of a scenery piece (its file name without the
    // trailing number, TEMPLE01..temple08), interned; 0: none.
    int artSet = 0;
};

struct StaticHeroInfo {
    int tile;
    int fid;
    int frame;
    int rotation;
    double worldU;
    double worldV;
};

bool updateStaticHero(const StaticHeroFrame& frame, std::string* error);
void clearStaticHero();
bool staticHeroInfo(StaticHeroInfo* info);

// The rotation whose art shows the side of a sprite in `rotation` that the
// fly or turned camera sees (087); the game's own view sees `rotation`.
// Updated by beginStaticObjects.
int staticShownRotation(int rotation);

// Live objects are rebuilt every presented frame between begin and end.
// Objects walls.bin already draws (static_scene.h) are only picked.
bool staticObjectArtCached(int fid, int frame, int rotation, int variant = 0);
void beginStaticObjects();
bool addStaticObject(const StaticObjectFrame& frame, std::string* error);
void endStaticObjects();
void clearStaticObjects();
unsigned int staticObjectCount();

// 157: the cheap state a built card depends on, read before any art lookup.
// An object whose snapshot is unchanged reuses the card built before: the
// game view's card geometry comes from these fields and the frame-wide modes
// alone, so the art locks, the decodes and the projection rebuild are
// skipped for the thousands of things that did not move.
struct StaticObjectSnapshot {
    unsigned int fid;
    int frame;
    int rotation;
    int tile;
    int x;
    int y;
    int pid;
    unsigned int flags;
    int stack;
    bool seeThrough;
    bool flat;
    bool flyActive;
    int turnOffset;
    bool shadowsOn;
    bool spriteSidesOn;
    bool clipEnabled;
};

// True when the card built for `key` still answers this snapshot. The caller
// then appends it (appendStaticObjectCard) instead of building the frame.
bool staticObjectCardFresh(const void* key, const StaticObjectSnapshot& snapshot);
// Appends the cached card for `key` with this frame's pick id.
void appendStaticObjectCard(const void* key, int pickId);
// Builds the card for `key` from the frame and stores it under the snapshot.
bool addStaticObjectCached(const void* key, const StaticObjectSnapshot& snapshot,
    const StaticObjectFrame& frame, std::string* error);

} // namespace scene_renderer
} // namespace fallout

#endif /* FALLOUT_SCENE_RENDERER_LIVE_OBJECTS_H */
