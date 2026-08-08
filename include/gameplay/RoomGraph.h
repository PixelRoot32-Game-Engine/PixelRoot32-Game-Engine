/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once

#include "platforms/PlatformDefaults.h"

#if PIXELROOT32_ENABLE_GAMEPLAY_ROOM

#include "math/Scalar.h"
#include "graphics/Camera2D.h"
#include <cstdint>

namespace pixelroot32::gameplay {

/**
 * @brief Cardinal direction enum for room connections.
 *
 * Maps connection slots by array index:
 *   - Up    (0) = North
 *   - Down  (1) = South
 *   - Left  (2) = West
 *   - Right (3) = East
 */
enum class RoomDir : uint8_t {
    Up    = 0,  ///< North
    Down  = 1,  ///< South
    Left  = 2,  ///< West
    Right = 3   ///< East
};

/**
 * @brief POD describing a single room: camera rect + optional tile window
 *        + fixed-size connection list.
 *
 * A Room is always copyable and trivially constructible. All fields have
 * sensible zero/default initializers so a default-constructed Room is safe
 * to inspect (hasTileWindow == false, no connections).
 *
 * @note Connections are stored by direction index (0 = Up, 1 = Down, 2 = Left,
 *       3 = Right). An unused slot holds INVALID_ROOM (0xFFFF).
 */
struct Room {
    math::Scalar cameraMinX = math::toScalar(0);  ///< Left camera bound (world units)
    math::Scalar cameraMinY = math::toScalar(0);  ///< Top camera bound (world units)
    math::Scalar cameraMaxX = math::toScalar(0);  ///< Right camera bound (world units)
    math::Scalar cameraMaxY = math::toScalar(0);  ///< Bottom camera bound (world units)

    int16_t tileOriginCol = -1;  ///< Tile window origin column (-1 = unused)
    int16_t tileOriginRow = -1;  ///< Tile window origin row    (-1 = unused)
    int16_t tileCols      =  0;  ///< Tile window width in tiles
    int16_t tileRows      =  0;  ///< Tile window height in tiles
    bool   hasTileWindow  = false;  ///< True if tile window fields are valid

    int     connections_[4]    = {};  ///< Target room indices by direction (Up/Down/Left/Right)
    uint8_t connectionCount_   = 0;   ///< Number of valid connections

    static constexpr int INVALID_ROOM = 0xFFFF;  ///< Sentinel for unused connection slots

    /// Default constructor: marks all connection slots as INVALID_ROOM.
    Room() {
        for (int i = 0; i < 4; ++i) {
            connections_[i] = INVALID_ROOM;
        }
    }
};

/**
 * @brief Fixed-capacity graph of rooms with camera rects and connections.
 *
 * @tparam N Max number of rooms (compile-time constant, must be >= 1).
 *
 * Zero-heap, zero-allocation. All arrays are fixed-size. The graph is built
 * once in Scene::init() and is read-only thereafter.
 *
 * Entering a room updates the camera bounds via Camera2D::setBounds /
 * Camera2D::setVerticalBounds and fires an optional onEnter callback.
 * Entity movement across rooms is the game's responsibility — RoomGraph
 * only provides data and camera management.
 */
template <uint16_t N>
class RoomGraph {
public:
    static_assert(N >= 1, "RoomGraph must have at least one room");

    static constexpr uint16_t kMaxRooms      = N;
    static constexpr uint8_t  kMaxConnections = 4;

    RoomGraph() = default;

    /**
     * @brief Add a room to the graph.
     * @param cameraMinX Left camera bound (world units)
     * @param cameraMinY Top camera bound (world units)
     * @param cameraMaxX Right camera bound (world units)
     * @param cameraMaxY Bottom camera bound (world units)
     * @return Room index (0 .. N-1), or 0xFFFF if full.
     */
    uint16_t addRoom(math::Scalar cameraMinX, math::Scalar cameraMinY,
                     math::Scalar cameraMaxX, math::Scalar cameraMaxY) {
        if (roomCount_ >= N) {
            return 0xFFFF;
        }
        Room& room = rooms_[roomCount_];
        room.cameraMinX = cameraMinX;
        room.cameraMinY = cameraMinY;
        room.cameraMaxX = cameraMaxX;
        room.cameraMaxY = cameraMaxY;
        return roomCount_++;
    }

    /**
     * @brief Set the tile window for a room.
     * @param roomIdx   Room index
     * @param originCol Tile window origin column
     * @param originRow Tile window origin row
     * @param cols      Tile window width in tiles
     * @param rows      Tile window height in tiles
     *
     * No-op when roomIdx is out of range.
     */
    void setTileWindow(uint16_t roomIdx, int16_t originCol, int16_t originRow,
                       int16_t cols, int16_t rows) {
        if (roomIdx >= roomCount_) return;
        Room& room = rooms_[roomIdx];
        room.tileOriginCol = originCol;
        room.tileOriginRow = originRow;
        room.tileCols = cols;
        room.tileRows = rows;
        room.hasTileWindow = true;
    }

    /**
     * @brief Connect two rooms in a cardinal direction.
     * @param fromIdx Source room index
     * @param toIdx   Target room index
     * @param dir     Direction from source to target
     *
     * No-op when either index is out of range. Re-connecting an already-
     * occupied direction slot does not double-count the connection.
     */
    void connect(uint16_t fromIdx, uint16_t toIdx, RoomDir dir) {
        if (fromIdx >= roomCount_ || toIdx >= roomCount_) return;
        Room& room = rooms_[fromIdx];
        const int dirIdx = static_cast<int>(dir);
        if (room.connections_[dirIdx] == Room::INVALID_ROOM) {
            room.connections_[dirIdx] = toIdx;
            ++room.connectionCount_;
        }
    }

    /**
     * @brief Enter a room by index.
     *
     * Updates currentRoomIndex_, clamps the camera to the room's rect
     * (if a non-null Camera2D* is provided), and fires the onEnter
     * callback if one is registered.
     *
     * @param idx    Room index
     * @param camera Pointer to Camera2D (may be nullptr)
     *
     * No-op when idx is out of range.
     */
    void enterRoom(uint16_t idx, graphics::Camera2D* camera) {
        if (idx >= roomCount_) return;
        const uint16_t fromIdx = currentRoomIndex_;
        currentRoomIndex_ = idx;
        const Room& room = rooms_[idx];
        if (camera != nullptr) {
            camera->setBounds(room.cameraMinX, room.cameraMaxX);
            camera->setVerticalBounds(room.cameraMinY, room.cameraMaxY);
        }
        if (onEnter_ != nullptr) {
            onEnter_(fromIdx, idx, userData_);
        }
    }

    /**
     * @brief Register an onEnter callback.
     *
     * The callback signature is:
     * @code void (*fn)(int fromIdx, int toIdx, void* userData) @endcode
     *
     * @param fn       Plain function pointer (not std::function — zero heap)
     * @param userData Context pointer passed through to the callback
     */
    void setOnEnter(void (*fn)(int fromIdx, int toIdx, void* userData),
                    void* userData = nullptr) {
        onEnter_  = fn;
        userData_ = userData;
    }

    /**
     * @brief Get the current room index.
     * @return 0xFFFF if enterRoom() has never been called.
     */
    uint16_t currentRoomIndex() const {
        return currentRoomIndex_;
    }

    /**
     * @brief Get a read-only reference to a room by index.
     *
     * The caller is responsible for passing a valid index (this method
     * does NOT bounds-check — use isValidIdx() first if needed).
     */
    const Room& getRoom(uint16_t idx) const {
        return rooms_[idx];
    }

    /**
     * @brief Number of rooms currently in the graph.
     */
    uint16_t roomCount() const {
        return roomCount_;
    }

    /**
     * @brief Check if a room index is valid (idx < roomCount()).
     */
    bool isValidIdx(uint16_t idx) const {
        return idx < roomCount_;
    }

    /**
     * @brief Copy up to maxOut connection target indices into a
     *        caller-provided buffer.
     *
     * @param idx    Room index (no-op when out of range)
     * @param out    Output buffer — must be non-null when maxOut > 0
     * @param maxOut Maximum number of connections to copy
     * @return Number of connections actually copied (≤ maxOut).
     *
     * Truncation is documented, not an error: when the room has more
     * connections than maxOut, only maxOut entries are written.
     */
    uint8_t getConnections(uint16_t idx, int out[], uint8_t maxOut) const {
        if (idx >= roomCount_) return 0;
        const Room& room = rooms_[idx];
        uint8_t count = 0;
        for (int i = 0; i < 4 && count < maxOut; ++i) {
            if (room.connections_[i] != Room::INVALID_ROOM) {
                out[count++] = room.connections_[i];
            }
        }
        return count;
    }

private:
    Room     rooms_[N];
    uint16_t roomCount_        = 0;
    uint16_t currentRoomIndex_ = 0xFFFF;
    void   (*onEnter_)(int fromIdx, int toIdx, void* userData) = nullptr;
    void*   userData_ = nullptr;
};

} // namespace pixelroot32::gameplay

#endif // PIXELROOT32_ENABLE_GAMEPLAY_ROOM
