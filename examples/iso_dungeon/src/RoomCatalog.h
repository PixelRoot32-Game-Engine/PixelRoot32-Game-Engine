#pragma once

#include <cstdint>

#include "gameplay/RoomGraph.h"
#include "gameplay/RoomLayout.h"
#include "graphics/Renderer.h"

#include "IsoDungeonConstants.h"
#include "assets/DungeonProps.h"

/**
 * @file RoomCatalog.h
 * @brief The three rooms this dungeon is made of, and the doors between them.
 *
 * `gameplay::RoomGraph` stores the topology -- which room a door leads to --
 * and nothing else. It deliberately knows no tiles, no sprites and no spawn
 * points, because those differ for every game that has rooms. This file is
 * where THIS game supplies them, and the two are joined in IsoDungeonScene by
 * indexing both with the same room index.
 *
 * ### The dungeon is a graph, not a map
 *
 * Worth being blunt about, because the art makes it unavoidable. Doors exist
 * only for the two BACK walls (see DOOR_NE_SPRITE and DOOR_NW_SPRITE): the
 * front edges of a room are open, and they have to stay open, because a wall
 * drawn along the front edge has a HIGHER screen depth than the tiles behind
 * it and would paint over the hero standing there. That is the isometric
 * front-wall problem, and it has no cheap fix.
 *
 * So every door in this dungeon is carved into a back wall, and a room's exit
 * and the matching entrance in the next room do not line up on any plane. Walk
 * up-right out of room 0 and you arrive beside room 1's up-left door. That is
 * a topological connection, not a spatial one -- which is exactly what
 * `RoomGraph` models, and why its `RoomDir` slots are keys rather than
 * geometry. A game that needs its rooms to tile a plane has to supply front
 * walls, and therefore art this example does not have.
 *
 * ### Camera rects are degenerate here
 *
 * `RoomGraph::addRoom` takes a camera rect because its usual job is clamping a
 * scrolling camera (see examples/legend_of_clone). One room of this dungeon is
 * 224 px wide inside a 240 px display, so nothing scrolls, this scene passes
 * `camera = nullptr` to every `enterRoom()`, and no code here ever reads the
 * rect back. The graph still earns its place: the connections, the
 * current-room index and the onEnter callback are what drive the transition.
 *
 * The rect is nonetheless filled in with something true rather than zeroed --
 * see kCellWorldWidth for which true thing, and why an isometric room cannot
 * have both an exact rect and an exact tile window.
 */

namespace iso_dungeon {

namespace gfx = pixelroot32::graphics;

/// Number of rooms. Fixes the RoomGraph template parameter and the catalog
/// array length together, so adding a room to one without the other fails to
/// compile rather than reading past the end.
inline constexpr uint16_t kRoomCount = 3;

/// The largest prop count any single room has. Sizes the scene's prop pool.
inline constexpr uint8_t kMaxPropsPerRoom = 4;

/// The largest door count any single room has.
inline constexpr uint8_t kMaxDoorsPerRoom = 2;

// --- Direction convention ---------------------------------------------------

/**
 * @brief Which `RoomDir` slot a step along each CELL axis occupies.
 *
 * The mapping is on the cell axes, not on the screen: `-tileY` is `Up`,
 * `+tileY` is `Down`, `-tileX` is `Left`, `+tileX` is `Right`. That matches
 * HeroActor's button handling exactly (BTN_UP produces `dy = -1`), so a door
 * the player reaches by holding Up is stored in the `Up` slot and the two
 * cannot drift apart.
 *
 * It does NOT match the screen, and should not: under kTileProjection the cell
 * axes project to the four screen diagonals. Naming the slots after what the
 * player presses is the only convention that survives a change of basis.
 */
constexpr pixelroot32::gameplay::RoomDir dirFromCellStep(int dx, int dy) {
    return dy < 0   ? pixelroot32::gameplay::RoomDir::Up
         : dy > 0   ? pixelroot32::gameplay::RoomDir::Down
         : dx < 0   ? pixelroot32::gameplay::RoomDir::Left
                    : pixelroot32::gameplay::RoomDir::Right;
}

// --- Doors ------------------------------------------------------------------

/**
 * @struct DoorSpec
 * @brief One doorway: where it is, where it goes, and where the hero lands.
 *
 * `arriveTileX/Y` is a field rather than something derived from the target
 * room's door position, and that is deliberate. Deriving it would have to
 * guess which neighbouring tile is walkable, and guessing wrong puts the hero
 * inside a wall on a room the author never looked at. Writing it down means
 * the static_asserts below can check it instead.
 */
struct DoorSpec {
    int tileX;             ///< Door tile column, indexed [tileY][tileX].
    int tileY;             ///< Door tile row.
    uint16_t targetRoom;   ///< Room index this door leads to.
    int arriveTileX;       ///< Where the hero stands on arrival, in the TARGET room.
    int arriveTileY;       ///< Ditto.
};

// --- Props ------------------------------------------------------------------

/**
 * @struct PropSpec
 * @brief A block the depth sort has to order against the hero.
 *
 * The sprite is a pointer rather than a reference so the array is copyable and
 * a prop slot can be re-pointed when the room changes.
 */
struct PropSpec {
    const gfx::Sprite4bpp* sprite;  ///< Block bitmap.
    int footY;                      ///< Its *_FOOT_Y (see IsoDraw.h).
    int tileX;                      ///< Cell it stands on.
    int tileY;
};

// --- Layouts ----------------------------------------------------------------

/// Room 0: the ritual chamber the example has always opened on.
///
/// Two doors. `D` at (3, 0) is carved into the wall running along +x and is
/// reached by holding Up; `E` at (0, 3) is in the wall along +y and is reached
/// by holding Left.
inline constexpr char kLayoutRitual[kRoomTiles][kRoomTiles + 1] = {
    "WWWDWWW",  // y = 0
    "W......",  // y = 1
    "W.aaa..",  // y = 2
    "E.aAaP.",  // y = 3
    "W.aaa..",  // y = 4
    "W..P...",  // y = 5
    "W......",  // y = 6
};

/// Room 1: a pillar hall. Reached by holding Up in the ritual chamber; its
/// own `E` door leads back.
///
/// Four pillars rather than props on the floor, because the point of this room
/// is to show the depth sort doing real work: the hero passes behind the back
/// pair and in front of the front pair on the same walk.
inline constexpr char kLayoutPillarHall[kRoomTiles][kRoomTiles + 1] = {
    "WWWWWWW",  // y = 0
    "W......",  // y = 1
    "W.P.P..",  // y = 2
    "E......",  // y = 3
    "W.P.P..",  // y = 4
    "W......",  // y = 5
    "W......",  // y = 6
};

/// Room 2: a shrine. Reached by holding Left in the ritual chamber; its own
/// `D` door leads back.
inline constexpr char kLayoutShrine[kRoomTiles][kRoomTiles + 1] = {
    "WWWDWWW",  // y = 0
    "W......",  // y = 1
    "W..A...",  // y = 2
    "W......",  // y = 3
    "W.P.P..",  // y = 4
    "W......",  // y = 5
    "W......",  // y = 6
};

// --- Per-room tables --------------------------------------------------------

inline constexpr PropSpec kPropsRitual[] = {
    {&ALTAR_SPRITE,  ALTAR_FOOT_Y,  3, 3},
    {&PILLAR_SPRITE, PILLAR_FOOT_Y, 5, 3},
    {&PILLAR_SPRITE, PILLAR_FOOT_Y, 3, 5},
};

inline constexpr PropSpec kPropsPillarHall[] = {
    {&PILLAR_SPRITE, PILLAR_FOOT_Y, 2, 2},
    {&PILLAR_SPRITE, PILLAR_FOOT_Y, 4, 2},
    {&PILLAR_SPRITE, PILLAR_FOOT_Y, 2, 4},
    {&PILLAR_SPRITE, PILLAR_FOOT_Y, 4, 4},
};

inline constexpr PropSpec kPropsShrine[] = {
    {&ALTAR_SPRITE,  ALTAR_FOOT_Y,  3, 2},
    {&PILLAR_SPRITE, PILLAR_FOOT_Y, 2, 4},
    {&PILLAR_SPRITE, PILLAR_FOOT_Y, 4, 4},
};

/// Ritual chamber: Up through `D` to the pillar hall, Left through `E` to the
/// shrine. Both land the hero on the tile in front of the destination's own
/// door, never on the door itself -- arriving on a door would re-trigger the
/// transition and bounce the player straight back.
inline constexpr DoorSpec kDoorsRitual[] = {
    {3, 0, /*targetRoom=*/1, /*arrive=*/1, 3},
    {0, 3, /*targetRoom=*/2, /*arrive=*/3, 1},
};

inline constexpr DoorSpec kDoorsPillarHall[] = {
    {0, 3, /*targetRoom=*/0, /*arrive=*/3, 1},
};

inline constexpr DoorSpec kDoorsShrine[] = {
    {3, 0, /*targetRoom=*/0, /*arrive=*/1, 3},
};

/**
 * @struct RoomSpec
 * @brief Everything about one room that RoomGraph does not store.
 */
struct RoomSpec {
    const char (*layout)[kRoomTiles + 1];  ///< kRoomTiles rows of kRoomTiles chars.
    const PropSpec* props;
    uint8_t propCount;
    const DoorSpec* doors;
    uint8_t doorCount;
};

/// Indexed by room index -- the SAME index RoomGraph uses. The order here is
/// the order IsoDungeonScene calls addRoom() in, which is what keeps them
/// aligned.
inline constexpr RoomSpec kRooms[kRoomCount] = {
    {kLayoutRitual,     kPropsRitual,     3, kDoorsRitual,     2},
    {kLayoutPillarHall, kPropsPillarHall, 4, kDoorsPillarHall, 1},
    {kLayoutShrine,     kPropsShrine,     3, kDoorsShrine,     1},
};

// --- Cell helpers -----------------------------------------------------------

/// True when the hero cannot stand on this tile of the given room.
///
/// Outside the room counts as solid, so a room's edge holds without a ring of
/// blocker tiles around the layout.
///
/// Doorways are deliberately NOT solid. The hero has to be able to stand on
/// one, because standing on one is what the scene watches for to fire the
/// transition. Nothing walks past a door: the step that lands on it is the
/// step that leaves the room.
constexpr bool isSolidTile(const RoomSpec& room, int tileX, int tileY) {
    if (!containsTile(tileX, tileY)) {
        return true;
    }
    const char c = room.layout[tileY][tileX];
    return c == 'W' || c == 'A' || c == 'P';
}

/// The door standing on this tile, or nullptr when there is none.
constexpr const DoorSpec* doorAt(const RoomSpec& room, int tileX, int tileY) {
    for (uint8_t i = 0; i < room.doorCount; ++i) {
        if (room.doors[i].tileX == tileX && room.doors[i].tileY == tileY) {
            return &room.doors[i];
        }
    }
    return nullptr;
}

/**
 * @brief The step that points from a door into the room it opens onto.
 *
 * Used to face the hero when it arrives. Doors only ever sit on the two BACK
 * walls (tileX == 0 or tileY == 0), so "into the room" is always +x or +y --
 * both of which project toward the camera. That is why an arriving hero can
 * never end up with its back turned, and it is a property of where doors CAN
 * be rather than a rule applied on top: a door on a front wall would break it,
 * and a front wall is exactly what this art cannot draw (see the file comment).
 *
 * Derived rather than stored in DoorSpec. A stored facing would be a fourth
 * hand-written field that has to agree with the other three, and the one it
 * would disagree with first is the one nobody looks at.
 *
 * @return `{0, 0}` when the tile has no door beside it, which the caller reads
 *         as "keep whatever facing you had".
 */
struct CellStep {
    int dx;
    int dy;
};

constexpr CellStep inwardStepFromDoor(const RoomSpec& room, int arriveTileX, int arriveTileY) {
    if (doorAt(room, arriveTileX - 1, arriveTileY) != nullptr) {
        return {1, 0};   // came through the wall along +y; face down-right
    }
    if (doorAt(room, arriveTileX, arriveTileY - 1) != nullptr) {
        return {0, 1};   // came through the wall along +x; face down-left
    }
    return {0, 0};
}

/**
 * @brief The step that carries the hero ONTO a door and would carry it out of
 *        the room.
 *
 * A door always sits on a room edge, so the step that reaches it points from
 * the interior outward, and continues outward past it. One expression answers
 * two questions that must never diverge: which `RoomDir` slot the connection
 * to the next room occupies, and which tile has to be solid for the door to
 * be a dead end. It was written out twice before, in two files, and the second
 * copy is the one that would have been missed when a wall moved.
 *
 * Two placements would make the answer meaningless, and each is rejected
 * elsewhere rather than papered over here:
 *
 * - A door NOT on an edge returns `{0, 0}`, which names no direction at all.
 *   `everyDoorIsADeadEnd()` rejects it: the tile "beyond" such a door is the
 *   door itself, and a door is walkable.
 * - A door on a CORNER returns a diagonal, and `dirFromCellStep` resolves that
 *   by dropping the x component -- so a corner door would quietly be filed as
 *   Up or Down. `doorsAreWellFormed()` rejects it.
 */
constexpr CellStep outwardStepFromDoor(const DoorSpec& door) {
    return {door.tileX == 0 ? -1 : (door.tileX == kRoomTiles - 1 ? 1 : 0),
            door.tileY == 0 ? -1 : (door.tileY == kRoomTiles - 1 ? 1 : 0)};
}

/// How many tiles of a room carry a given layout char.
constexpr int countTiles(const RoomSpec& room, char kind) {
    int n = 0;
    for (int y = 0; y < kRoomTiles; ++y) {
        for (int x = 0; x < kRoomTiles; ++x) {
            if (room.layout[y][x] == kind) {
                ++n;
            }
        }
    }
    return n;
}

// --- Compile-time consistency ------------------------------------------------
//
// The catalog is hand-written in four places that have to agree -- layout
// chars, prop list, door list and arrival tiles -- and every disagreement is
// silent at runtime: a prop that draws with nothing under it, a door that
// leads nowhere, a hero spawned inside a wall. These asserts turn each one
// into a build failure at the line that caused it.

/// Every prop stands on a tile whose layout char declares it, and every such
/// char has a prop. Move a pillar in a layout and the build stops.
constexpr bool propsMatchLayout(const RoomSpec& room) {
    int altars = 0;
    int pillars = 0;
    for (uint8_t i = 0; i < room.propCount; ++i) {
        const PropSpec& p = room.props[i];
        if (!containsTile(p.tileX, p.tileY)) {
            return false;
        }
        const char c = room.layout[p.tileY][p.tileX];
        if (c == 'A') {
            ++altars;
        } else if (c == 'P') {
            ++pillars;
        } else {
            return false;  // a prop on a tile that does not declare one
        }
    }
    return altars == countTiles(room, 'A') && pillars == countTiles(room, 'P');
}

/// Every door sits on a doorway char, targets a room that exists, and lands
/// the hero somewhere walkable in THAT room that is not itself a door.
constexpr bool doorsAreWellFormed(const RoomSpec& room) {
    for (uint8_t i = 0; i < room.doorCount; ++i) {
        const DoorSpec& d = room.doors[i];
        if (!containsTile(d.tileX, d.tileY)) {
            return false;
        }
        const char c = room.layout[d.tileY][d.tileX];
        if (c != 'D' && c != 'E') {
            return false;
        }
        // A corner door leaves in two directions at once, and RoomDir has one
        // slot per direction -- so `dirFromCellStep` would have to pick, and it
        // picks the y axis. The connection would be filed under Up or Down with
        // nothing recording that Left or Right was equally true. Rejected here
        // rather than resolved, because there is no correct answer to give.
        const CellStep step = outwardStepFromDoor(d);
        if (step.dx != 0 && step.dy != 0) {
            return false;
        }
        if (d.targetRoom >= kRoomCount) {
            return false;
        }
        const RoomSpec& target = kRooms[d.targetRoom];
        if (isSolidTile(target, d.arriveTileX, d.arriveTileY)) {
            return false;
        }
        if (doorAt(target, d.arriveTileX, d.arriveTileY) != nullptr) {
            return false;  // arriving on a door would bounce the player back
        }
    }
    // Every doorway char in the layout is claimed by exactly one DoorSpec.
    return room.doorCount == countTiles(room, 'D') + countTiles(room, 'E');
}

/// Every door has a way back, and the way back lands you beside the door you
/// came out of.
///
/// The strong half is the second clause. A dungeon whose doors merely pair up
/// can still strand the player: walk through, walk back, and arrive somewhere
/// you have never been. Requiring the return trip to land ON the tile in front
/// of the original door makes the two rooms a genuine round trip, and makes a
/// mistyped arrival coordinate a build error instead of a lost player.
constexpr bool hasReturnDoor(const RoomSpec& room, const DoorSpec& out) {
    const RoomSpec& target = kRooms[out.targetRoom];
    for (uint8_t i = 0; i < target.doorCount; ++i) {
        const DoorSpec& back = target.doors[i];
        if (back.targetRoom != &room - &kRooms[0]) {
            continue;
        }
        // Landing beside the door we left by: same column or row, one step in.
        const bool besideOurDoor =
            (back.arriveTileX == out.tileX || back.arriveTileY == out.tileY);
        return besideOurDoor;
    }
    return false;
}

constexpr bool everyDoorIsTwoWay(const RoomSpec& room) {
    for (uint8_t i = 0; i < room.doorCount; ++i) {
        if (!hasReturnDoor(room, room.doors[i])) {
            return false;
        }
    }
    return true;
}

/// Every arrival tile sits directly inside a door, so inwardStepFromDoor()
/// always has an answer and an arriving hero always faces the camera. Without
/// this, a stray arrival coordinate would silently fall back to whatever
/// facing the hero walked into the previous door with -- which is its back.
constexpr bool everyArrivalFacesInward(const RoomSpec& room) {
    for (uint8_t i = 0; i < room.doorCount; ++i) {
        const DoorSpec& d = room.doors[i];
        const CellStep step =
            inwardStepFromDoor(kRooms[d.targetRoom], d.arriveTileX, d.arriveTileY);
        if (step.dx == 0 && step.dy == 0) {
            return false;
        }
    }
    return true;
}

/// No door can be walked straight off in the direction that reaches it.
///
/// This is load-bearing, not a tidiness check. IsoDungeonScene polls for the
/// hero standing on a door once per frame, and HeroActor runs several logic
/// steps on a long frame -- so a hero able to step onto a door and off it
/// between two polls would walk through it without the room changing.
///
/// This assert is what makes that impossible: the step that would carry the
/// hero off the door is the one isSolidTile refuses, and input is frame-cached
/// so no other direction is available until the next frame. See the proof at
/// the poll in IsoDungeonScene::update. Delete this assert and that poll
/// silently becomes wrong on slow frames only.
///
/// Every shipped door already satisfies it -- they all sit on a room edge, so
/// the step beyond leaves the room and isSolidTile() rejects it. That was true
/// by coincidence of the layouts until this assert; now it is a rule, and a
/// door placed mid-wall with floor behind it fails the build.
constexpr bool everyDoorIsADeadEnd(const RoomSpec& room) {
    for (uint8_t i = 0; i < room.doorCount; ++i) {
        const DoorSpec& d = room.doors[i];
        const CellStep step = outwardStepFromDoor(d);
        if (!isSolidTile(room, d.tileX + step.dx, d.tileY + step.dy)) {
            return false;
        }
    }
    return true;
}

static_assert(everyDoorIsADeadEnd(kRooms[0]), "Room 0 has a door the hero can walk straight through.");
static_assert(everyDoorIsADeadEnd(kRooms[1]), "Room 1 has a door the hero can walk straight through.");
static_assert(everyDoorIsADeadEnd(kRooms[2]), "Room 2 has a door the hero can walk straight through.");

static_assert(everyArrivalFacesInward(kRooms[0]), "Room 0 lands the hero with no door beside it.");
static_assert(everyArrivalFacesInward(kRooms[1]), "Room 1 lands the hero with no door beside it.");
static_assert(everyArrivalFacesInward(kRooms[2]), "Room 2 lands the hero with no door beside it.");

static_assert(everyDoorIsTwoWay(kRooms[0]), "Room 0 has a door with no way back.");
static_assert(everyDoorIsTwoWay(kRooms[1]), "Room 1 has a door with no way back.");
static_assert(everyDoorIsTwoWay(kRooms[2]), "Room 2 has a door with no way back.");

static_assert(propsMatchLayout(kRooms[0]), "Room 0's props and layout disagree.");
static_assert(propsMatchLayout(kRooms[1]), "Room 1's props and layout disagree.");
static_assert(propsMatchLayout(kRooms[2]), "Room 2's props and layout disagree.");

static_assert(doorsAreWellFormed(kRooms[0]), "Room 0 has a malformed door.");
static_assert(doorsAreWellFormed(kRooms[1]), "Room 1 has a malformed door.");
static_assert(doorsAreWellFormed(kRooms[2]), "Room 2 has a malformed door.");

static_assert(kRooms[0].propCount <= kMaxPropsPerRoom &&
              kRooms[1].propCount <= kMaxPropsPerRoom &&
              kRooms[2].propCount <= kMaxPropsPerRoom,
              "A room has more props than the scene's prop pool holds.");

static_assert(kRooms[0].doorCount <= kMaxDoorsPerRoom &&
              kRooms[1].doorCount <= kMaxDoorsPerRoom &&
              kRooms[2].doorCount <= kMaxDoorsPerRoom,
              "A room has more doors than kMaxDoorsPerRoom.");

static_assert(!isSolidTile(kRooms[0], kSpawnTileX, kSpawnTileY),
              "The hero spawns inside a wall.");
static_assert(doorAt(kRooms[0], kSpawnTileX, kSpawnTileY) == nullptr,
              "The hero spawns on a door and would leave the room immediately.");

// --- Room graph export -------------------------------------------------------
//
// Everything below turns the catalog above into `gameplay::RoomLayer`, the
// shape the PixelRoot32 Tilemap Editor emits and `gameplay::buildRoomGraph()`
// consumes. The scene then builds its RoomGraph with one call instead of two
// hand-written loops.
//
// The point is not the loops. It is that the topology now has an EXPORTED
// shape: a dungeon authored in the editor drops in by replacing this block
// with the generated header, and IsoDungeonScene::init() does not change.
//
// It is derived at compile time from kRooms rather than written out a second
// time, and that is the whole reason it is safe. A hand-written RoomData table
// would be a fourth copy of the door topology -- one the layouts, the doors
// and the renderer could all drift away from silently. Deriving it means the
// only way to change the graph is to change the doors the player walks
// through.

/**
 * @brief World size of one cell, for the camera rect `buildRoomGraph` derives.
 *
 * `RoomLayer` assumes what every rectangular tilemap assumes: that a room's
 * world rect is its tile rect times a tile size. An isometric room does not
 * satisfy that -- its cells are diamonds sheared by kTileProjection, so there
 * is no single (w, h) that reproduces where the room actually lands on screen.
 *
 * So the rect and the tile window cannot both be exact, and this picks the
 * tile window. `cols`/`rows` are the true 7x7 cell extent, and 32x16 is the
 * full width and height of one floor diamond, which makes the derived rect
 * 224x112 -- the exact size of the room's floor bounding box, though anchored
 * at the origin rather than at the (8, 80) where it is drawn. `originCol` and
 * `originRow` cannot express that offset: 8 is not a multiple of 32.
 *
 * That inexactness is affordable here for one reason, and it is worth stating
 * because it would NOT be affordable in a scrolling game: this scene passes
 * `camera = nullptr` to every `enterRoom()` and never reads the rect back. A
 * scrolling isometric dungeon would have to clamp its camera from the
 * projection instead -- `RoomLayer` cannot describe it.
 */
inline constexpr uint8_t kCellWorldWidth  = kTileHalfWidth * 2;   // 32
inline constexpr uint8_t kCellWorldHeight = kTileHalfHeight * 2;  // 16

/// Two doors of one room must leave in two different directions.
///
/// `RoomData` has exactly four connection slots, keyed by direction, so two
/// doors sharing a direction cannot both be stored -- the second would
/// overwrite the first and the dungeon would quietly lose an exit.
///
/// This is stricter than the code it replaces, on purpose. `RoomGraph::connect`
/// keeps the FIRST connection written to a slot and ignores later ones, so the
/// old hand-written loop had the same collision and resolved it silently the
/// other way. Neither resolution is right; the data being ambiguous is the
/// actual problem, and this is where it stops being tolerated.
constexpr bool doorsLeaveInDistinctDirections(const RoomSpec& room) {
    for (uint8_t i = 0; i < room.doorCount; ++i) {
        const CellStep a = outwardStepFromDoor(room.doors[i]);
        for (uint8_t j = static_cast<uint8_t>(i + 1); j < room.doorCount; ++j) {
            const CellStep b = outwardStepFromDoor(room.doors[j]);
            if (dirFromCellStep(a.dx, a.dy) == dirFromCellStep(b.dx, b.dy)) {
                return false;
            }
        }
    }
    return true;
}

static_assert(doorsLeaveInDistinctDirections(kRooms[0]),
              "Room 0 has two doors leaving in the same direction; one would be lost.");
static_assert(doorsLeaveInDistinctDirections(kRooms[1]),
              "Room 1 has two doors leaving in the same direction; one would be lost.");
static_assert(doorsLeaveInDistinctDirections(kRooms[2]),
              "Room 2 has two doors leaving in the same direction; one would be lost.");

/// One room's doors, in the four-slot form RoomLayer stores them in.
constexpr gameplay::RoomData makeRoomData(const RoomSpec& room) {
    gameplay::RoomData data{0, 0, kRoomTiles, kRoomTiles,
                            {gameplay::kNoRoomConnection, gameplay::kNoRoomConnection,
                             gameplay::kNoRoomConnection, gameplay::kNoRoomConnection}};
    for (uint8_t i = 0; i < room.doorCount; ++i) {
        const DoorSpec& door = room.doors[i];
        const CellStep step = outwardStepFromDoor(door);
        data.connections[static_cast<uint8_t>(dirFromCellStep(step.dx, step.dy))] =
            door.targetRoom;
    }
    return data;
}

/// Wraps the array so the whole thing is built by one loop over kRooms.
///
/// A plain `RoomData kExport[] = {makeRoomData(kRooms[0]), ...}` would need one
/// initializer per room, and a fourth room added to kRooms but forgotten here
/// would NOT fail to compile -- the missing entry would value-initialize, and
/// `connections` of all zeroes means "connected to room 0 in all four
/// directions". Silently. Looping is what makes that impossible.
struct RoomGraphExport {
    gameplay::RoomData rooms[kRoomCount];
};

constexpr RoomGraphExport makeRoomGraphExport() {
    RoomGraphExport out{};
    for (uint16_t i = 0; i < kRoomCount; ++i) {
        out.rooms[i] = makeRoomData(kRooms[i]);
    }
    return out;
}

inline constexpr RoomGraphExport kRoomGraphExport = makeRoomGraphExport();

/// What IsoDungeonScene::init() hands to `gameplay::buildRoomGraph()`.
inline constexpr gameplay::RoomLayer kRoomLayer{
    kRoomGraphExport.rooms, kRoomCount, kCellWorldWidth, kCellWorldHeight};

}  // namespace iso_dungeon
