#pragma once

#include <platforms/EngineConfig.h>

#if !PIXELROOT32_ENABLE_PERSISTENCE
#error "This example requires PIXELROOT32_ENABLE_PERSISTENCE=1 in lib/platformio.ini"
#endif

#include <core/Scene.h>
#include <graphics/Renderer.h>
#include <persistence/PersistenceService.h>

#if defined(ESP32)
#include <drivers/esp32/Esp32NvsStore.h>
#if PIXELROOT32_ENABLE_SD_STORAGE
#include <drivers/esp32/SdConfig.h>
#include <drivers/esp32/SdStore.h>
#endif
#elif defined(PLATFORM_NATIVE)
#include <drivers/native/HostFileStore.h>
#endif

namespace persistenceexample {

/**
 * @class PersistenceExampleScene
 * @brief One settings record and one save slot, with every result printed.
 *
 * The scene owns the two stores and the service, loads both records at init,
 * and rewrites them only when the player presses A. Nothing reaches the medium
 * without that press: the module never saves on its own, and on the board a
 * flash write stalls both cores.
 *
 * The same code runs on the board and on a PC. Only the store types differ:
 * NVS through `Esp32NvsKeyValueStore` / `Esp32NvsSlotStore` on the ESP32, host
 * files through `HostFileKeyValueStore` / `HostFileSlotStore` on native.
 *
 * On the ESP32 the save slots are opt-in over microSD: build the `esp32sd`
 * environment (`PIXELROOT32_ENABLE_SD_STORAGE=1`) and the saves move to
 * `SdSlotStore` while the settings stay in NVS. The scene and the
 * `PersistenceService` calls do not change - only the saves store does, which
 * is the adapter pattern this example demonstrates.
 */
class PersistenceExampleScene : public pixelroot32::core::Scene {
public:
    void init() override;
    void update(unsigned long deltaTime) override;
    void draw(pixelroot32::graphics::Renderer& renderer) override;

private:
    /// Reads both records, reports every StoreResult, and falls back to
    /// defaults when a record is missing, damaged or from another schema.
    void loadOrUseDefaults();

    /// Mutates the in-memory records, writes both, commits settings, then reads
    /// both back so one press proves the whole round trip on either medium.
    void saveOnInput();

    pixelroot32::persistence::PersistenceService persistence_;

#if defined(ESP32)
    // Namespace "mygame" holds the settings blob (`cfg`). In the default NVS
    // build it also holds the slot blob (`save0`); the opt-in SD build moves
    // only the saves to the card.
    pixelroot32::persistence::Esp32NvsKeyValueStore settingsStore_{"mygame", 64};
#if PIXELROOT32_ENABLE_SD_STORAGE
    // `esp32sd`: saves live at /pr32demo/save/<i>.slot on the FAT card, while
    // settings still use NVS through the same PersistenceService.
    pixelroot32::persistence::SdConfig sdConfig_;
    pixelroot32::persistence::SdSlotStore saveStore_{"/pr32demo", "save", 3, 256};
#else
    pixelroot32::persistence::Esp32NvsSlotStore saveStore_{"mygame", "save", 3, 256};
#endif
#elif defined(PLATFORM_NATIVE)
    // Files land under save_data/mygame/ (cfg.kv) and save_data/save/
    // (0.slot), relative to the working directory the binary runs from.
    pixelroot32::persistence::HostFileKeyValueStore settingsStore_{"save_data", "mygame", 64};
    pixelroot32::persistence::HostFileSlotStore saveStore_{"save_data", "save", 3, 256};
#endif

    // Settings record: one byte each, written field by field. A struct memcpy
    // would store padding that differs between the PC and the ESP32.
    uint8_t volume_ = 8;
    uint8_t brightness_ = 12;
    bool invertY_ = false;

    // Save-slot record: the progress a "Continue" menu would read.
    uint8_t level_ = 1;
    uint16_t presses_ = 0;

    /// Result name of the last write pair, drawn on the status panel.
    const char* lastResult_ = "none";
};

}  // namespace persistenceexample
