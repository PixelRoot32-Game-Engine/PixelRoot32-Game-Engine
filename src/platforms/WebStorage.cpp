/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#include "platforms/WebStorage.h"

#if PIXELROOT32_PLATFORM_WEB

#include <emscripten.h>
#include <emscripten/em_js.h>
#include <emscripten/html5.h>

#include <cstdint>
#include <cstdio>

namespace pixelroot32::platforms::web {
namespace {

enum class State : uint8_t {
    Idle,         ///< Nothing mounted yet.
    Loading,      ///< Mounted; waiting for the saved data to arrive.
    Ready,        ///< Load finished (or gave up): stores may be read and written.
    Unavailable,  ///< No IDBFS in this build, or the mount failed.
};

State gState = State::Idle;
bool gHooksInstalled = false;
double gLoadStartMs = 0.0;

// Mounts IDBFS at `mountPoint`, makes it the working directory and starts
// loading it. Returns 0 when the build has no IDBFS (linked without
// -lidbfs.js) or the mount fails, so the C side degrades to "no storage"
// instead of pretending every read is a missing record.
EM_JS(int, mountPersistentFs, (const char* mountPoint), {
    if (typeof FS === 'undefined' || typeof IDBFS === 'undefined') {
        console.warn('[PR32] persistence needs IndexedDB: link the web build with -lidbfs.js');
        return 0;
    }
    const dir = UTF8ToString(mountPoint);
    try {
        FS.mkdirTree(dir);
        FS.mount(IDBFS, {}, dir);
        FS.chdir(dir);
    } catch (e) {
        console.error('[PR32] persistent file system unavailable: ' + e);
        return 0;
    }
    Module['__pr32StorageReady'] = false;
    FS.syncfs(true, function (err) {
        if (err) {
            console.error('[PR32] loading saved data failed: ' + err);
        }
        Module['__pr32StorageReady'] = true;
    });
    return 1;
});

// Polled, not called back: a C callback invoked from JS would need an exported
// symbol and a function pointer table for one flag the frame loop can read.
EM_JS(int, savedDataLoaded, (), {
    return (typeof Module !== 'undefined' && Module['__pr32StorageReady']) ? 1 : 0;
});

EM_JS(void, syncFsToBrowser, (), {
    if (typeof FS === 'undefined' || typeof FS.syncfs !== 'function') {
        return;
    }
    FS.syncfs(false, function (err) {
        if (err) {
            console.error('[PR32] saving to IndexedDB failed: ' + err);
        }
    });
});

bool onVisibilityChange(int, const EmscriptenVisibilityChangeEvent* event, void*) {
    if (event != nullptr && event->hidden) {
        flushPersistentStorage();
    }
    // false: no preventDefault; the browser keeps its own behaviour.
    return false;
}

const char* onBeforeUnload(int, const void*, void*) {
    flushPersistentStorage();
    return nullptr;
}

}  // namespace

void initPersistentStorage() {
    if (gState != State::Idle) {
        return;
    }

    if (!gHooksInstalled) {
        // Both hooks, because either can be the last one a browser delivers:
        // switching tabs fires visibilitychange, closing the page fires both in
        // no guaranteed order.
        emscripten_set_visibilitychange_callback(nullptr, EM_FALSE, onVisibilityChange);
        emscripten_set_beforeunload_callback(nullptr, onBeforeUnload);
        gHooksInstalled = true;
    }

    if (mountPersistentFs(kPersistentRoot) == 0) {
        gState = State::Unavailable;
        return;
    }

    gState = State::Loading;
    gLoadStartMs = emscripten_get_now();
}

bool isStorageReady() {
    switch (gState) {
        case State::Ready:
        case State::Unavailable:
            return true;
        case State::Idle:
            // Never mounted: treat as unavailable rather than hanging a caller
            // that forgot initPersistentStorage().
            return true;
        case State::Loading:
            break;
    }

    if (savedDataLoaded() == 1) {
        gState = State::Ready;
        return true;
    }

    if (emscripten_get_now() - gLoadStartMs > kStorageLoadTimeoutMs) {
        std::printf("[PR32] saved data did not load within %.0f ms; starting without it\n",
                    kStorageLoadTimeoutMs);
        gState = State::Ready;
        return true;
    }

    return false;
}

void flushPersistentStorage() {
    // Before the initial load finishes there is nothing to write: the tree is
    // either empty or about to be replaced by what IndexedDB holds.
    if (gState != State::Ready) {
        return;
    }
    syncFsToBrowser();
}

}  // namespace pixelroot32::platforms::web

#endif  // PIXELROOT32_PLATFORM_WEB
