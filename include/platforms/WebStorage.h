/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once

#include "platforms/PlatformSelect.h"

#if PIXELROOT32_PLATFORM_WEB

/**
 * @file WebStorage.h
 * @brief Makes the browser build's file system survive a page reload.
 *
 * Emscripten's file system starts as MEMFS: real fopen()/stat()/rename(), and
 * nothing on disk. `HostFileKeyValueStore` and `HostFileSlotStore` therefore run
 * on the web without a single change, and lose everything on reload unless the
 * tree is mounted over IndexedDB and flushed. That is all this module does:
 *
 * - `initPersistentStorage()` mounts IDBFS at `kPersistentRoot`, makes it the
 *   working directory (so a store's relative root like "save_data" ends up
 *   inside it) and starts loading what the previous session left behind.
 * - The load is asynchronous and cannot be awaited from C without Asyncify, so
 *   it completes after `main()` has been entered. A scene reads its records in
 *   `Scene::init()`, which runs as soon as it is set, so the game must not read
 *   them before `isStorageReady()` says so -- otherwise it sees an empty tree
 *   and may write defaults over the save. There is no callback on purpose: the
 *   engine already has a frame loop to poll from, and starting the loop from a
 *   JavaScript callback would break `emscripten_set_main_loop`'s unwind.
 * - `flushPersistentStorage()` copies the tree to IndexedDB. It is wired to
 *   `visibilitychange` (hidden) and `beforeunload`, the pair a browser
 *   reliably delivers before a tab goes away.
 *
 * The module degrades instead of failing: a web build linked without
 * `-lidbfs.js` has no IDBFS, so every call here turns into a no-op after one
 * console warning and the game runs without persistence.
 */
namespace pixelroot32::platforms::web {

/**
 * @brief Directory the browser build mounts its persistent file system on.
 *
 * Absolute on purpose: it is the one path that has to exist before any store is
 * created, and the working directory changes to it, so game code keeps using
 * relative roots on desktop and on the web alike.
 */
inline constexpr const char* kPersistentRoot = "/pr32-persist";

/**
 * @brief How long the saved data may take to load before reads are allowed anyway.
 *
 * IndexedDB can be blocked (private windows, another tab holding the database)
 * and its callback would never fire. Reading an empty tree loses a save;
 * refusing to run loses the game, so the timeout wins and the expiry is logged.
 */
inline constexpr double kStorageLoadTimeoutMs = 5000.0;

/**
 * @brief Mounts the persistent file system and starts loading it.
 *
 * Idempotent, and it returns immediately: poll `isStorageReady()` before the
 * first read. Registers the flush hooks on the first call.
 */
void initPersistentStorage();

/**
 * @brief True once the saved data has been loaded, or once there is no storage.
 *
 * Storage that is missing or broken counts as ready: waiting for it forever
 * would hang the page instead of running the game without persistence.
 */
bool isStorageReady();

/**
 * @brief Copies the persistent tree to IndexedDB.
 *
 * Asynchronous, no return value: the browser decides when it writes. A no-op
 * when storage is unavailable or the load has not finished (there is nothing
 * worth flushing yet). Safe to call from the visibility and unload hooks; call
 * it by hand after a save the player must not lose.
 */
void flushPersistentStorage();

}  // namespace pixelroot32::platforms::web

#endif  // PIXELROOT32_PLATFORM_WEB
