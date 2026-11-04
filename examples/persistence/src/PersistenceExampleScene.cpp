#include "PersistenceExampleScene.h"

#include <cstdarg>
#include <cstdio>

#include <core/Engine.h>
#include <input/InputManager.h>

namespace pr32 = pixelroot32;

extern pr32::core::Engine engine;

namespace persistenceexample {

namespace persist = pr32::persistence;
namespace gfx = pr32::graphics;

namespace {

/// InputManager order: Up, Down, Left, Right, A, B. A is Space on native and
/// the board's A pin (13) on hardware.
constexpr uint8_t kButtonConfirm = 4;

// The schema version is the game's, not the engine's: RecordCodec stores it and
// reports it back, and the loader below decides what to do with it.
constexpr uint16_t kSettingsSchema = 1;
constexpr uint16_t kSlotSchema = 1;

constexpr const char* kSettingsKey = "cfg";
constexpr uint16_t kSlotIndex = 0;

// Field-by-field layouts, not struct images: padding and byte order of a
// struct differ between the PC and the ESP32.
constexpr size_t kSettingsPayloadSize = 3;  // volume, brightness, invertY
constexpr size_t kSlotPayloadSize = 3;      // level, presses low byte, presses high byte

// The service writes header + payload into this caller-owned buffer. A stack
// array is the intended use; the module allocates nothing.
constexpr size_t kScratchSize = persist::RecordCodec::kHeaderSize + 32;

/** @brief The StoreResult's own name, so a transcript never shows a bare bool. */
const char* resultName(persist::StoreResult result) {
    switch (result) {
        case persist::StoreResult::Ok: return "Ok";
        case persist::StoreResult::NotFound: return "NotFound";
        case persist::StoreResult::TooLarge: return "TooLarge";
        case persist::StoreResult::NoSpace: return "NoSpace";
        case persist::StoreResult::Corrupt: return "Corrupt";
        case persist::StoreResult::IoError: return "IoError";
        case persist::StoreResult::Unavailable: return "Unavailable";
        case persist::StoreResult::InvalidKey: return "InvalidKey";
        case persist::StoreResult::InvalidArgument: return "InvalidArgument";
    }
    return "Unknown";
}

/**
 * @brief Prints one stable, greppable line.
 *
 * On the PC the console is stdout, so a transcript can be captured or piped; on
 * the board it is the UART through `Serial`.
 */
void logLine(const char* format, ...) {
    char line[192];
    va_list args;
    va_start(args, format);
    std::vsnprintf(line, sizeof(line), format, args);
    va_end(args);

#if defined(PLATFORM_NATIVE)
    std::fputs(line, stdout);
    std::fputc('\n', stdout);
    std::fflush(stdout);
#else
    Serial.println(line);
#endif
}

uint16_t readU16(const uint8_t* bytes) {
    return static_cast<uint16_t>(bytes[0] | (static_cast<uint16_t>(bytes[1]) << 8));
}

}  // namespace

void PersistenceExampleScene::loadOrUseDefaults() {
#if defined(ESP32)
    logLine("PERSIST,platform,esp32");
#else
    logLine("PERSIST,platform,native");
#endif

    const persist::StoreResult settingsInit = settingsStore_.init();
    const persist::StoreResult savesInit = saveStore_.init();
    logLine("PERSIST,boot,settings_store_init,%s", resultName(settingsInit));
    logLine("PERSIST,boot,saves_store_init,%s", resultName(savesInit));

    // A store that fails to initialise stays unbound: its role then answers
    // Unavailable to every call and the scene keeps running on its defaults.
    if (settingsInit == persist::StoreResult::Ok) {
        persistence_.bindSettings(&settingsStore_);
    }
    if (savesInit == persist::StoreResult::Ok) {
        persistence_.bindSaves(&saveStore_);
    }
    logLine("PERSIST,boot,bound,settings=%u,saves=%u,slots=%u,max_record=%u",
            unsigned(persistence_.hasSettings()), unsigned(persistence_.hasSaves()),
            unsigned(persistence_.saves().slotCount()),
            unsigned(persistence_.saves().maxRecordSize()));

    // --- Settings record -----------------------------------------------------
    uint8_t settingsScratch[kScratchSize];
    const uint8_t* settingsPayload = nullptr;
    persist::RecordInfo settingsInfo{};
    const persist::StoreResult settingsResult = persistence_.loadSettingRecord(
        kSettingsKey, settingsScratch, sizeof(settingsScratch), settingsPayload, settingsInfo);

    if (settingsResult == persist::StoreResult::Ok) {
        logLine("PERSIST,boot,settings_load,Ok,schema=%u,payload=%u",
                unsigned(settingsInfo.schemaVersion), unsigned(settingsInfo.payloadSize));
        // RecordCodec reports the version, it never rejects one. Accepting the
        // layout is this game's decision, made here.
        if (settingsInfo.schemaVersion == kSettingsSchema &&
            settingsInfo.payloadSize == kSettingsPayloadSize) {
            volume_ = settingsPayload[0];
            brightness_ = settingsPayload[1];
            invertY_ = settingsPayload[2] != 0;
            logLine("PERSIST,boot,settings_values,volume=%u,brightness=%u,invert_y=%u",
                    unsigned(volume_), unsigned(brightness_), unsigned(invertY_ ? 1 : 0));
        } else {
            logLine("PERSIST,boot,settings_defaults,reason=schema_or_size");
        }
    } else {
        logLine("PERSIST,boot,settings_load,%s", resultName(settingsResult));
        logLine("PERSIST,boot,settings_defaults,reason=%s", resultName(settingsResult));
    }

    // --- Save-slot record ----------------------------------------------------
    uint8_t slotScratch[kScratchSize];
    const uint8_t* slotPayload = nullptr;
    persist::RecordInfo slotInfo{};
    const persist::StoreResult slotResult = persistence_.loadRecord(
        kSlotIndex, slotScratch, sizeof(slotScratch), slotPayload, slotInfo);

    if (slotResult == persist::StoreResult::Ok) {
        logLine("PERSIST,boot,slot_load,Ok,schema=%u,payload=%u",
                unsigned(slotInfo.schemaVersion), unsigned(slotInfo.payloadSize));
        if (slotInfo.schemaVersion == kSlotSchema &&
            slotInfo.payloadSize == kSlotPayloadSize) {
            level_ = slotPayload[0];
            presses_ = readU16(slotPayload + 1);
            logLine("PERSIST,boot,slot_values,level=%u,presses=%u",
                    unsigned(level_), unsigned(presses_));
        } else {
            logLine("PERSIST,boot,slot_defaults,reason=schema_or_size");
        }
    } else {
        logLine("PERSIST,boot,slot_load,%s", resultName(slotResult));
        logLine("PERSIST,boot,slot_defaults,reason=%s", resultName(slotResult));
    }

    // --- What the envelope itself reports -----------------------------------
    // A direct encode/decode pair shows the 16-byte header's promise without a
    // medium in between: the schema version and payload length come back
    // exactly as written, and a truncated record is Corrupt rather than data.
    const uint8_t probePayload[kSettingsPayloadSize] = {
        volume_, brightness_, static_cast<uint8_t>(invertY_ ? 1 : 0)};
    uint8_t probe[persist::RecordCodec::kHeaderSize + kSettingsPayloadSize];
    size_t probeWritten = 0;
    const persist::StoreResult encodeResult = persist::RecordCodec::encode(
        kSettingsSchema, probePayload, kSettingsPayloadSize, probe, sizeof(probe),
        probeWritten);
    logLine("PERSIST,codec,encode,%s,bytes=%u", resultName(encodeResult),
            unsigned(probeWritten));

    if (encodeResult == persist::StoreResult::Ok) {
        const uint8_t* decodedPayload = nullptr;
        persist::RecordInfo decodedInfo{};
        const persist::StoreResult decodeResult =
            persist::RecordCodec::decode(probe, probeWritten, decodedPayload, decodedInfo);
        if (decodeResult == persist::StoreResult::Ok) {
            logLine("PERSIST,codec,decode,Ok,schema=%u,payload=%u",
                    unsigned(decodedInfo.schemaVersion), unsigned(decodedInfo.payloadSize));
        } else {
            logLine("PERSIST,codec,decode,%s", resultName(decodeResult));
        }

        // One byte short of the record size: the length field disagrees and
        // the CRC never matches, so the codec reports Corrupt.
        const persist::StoreResult truncated =
            persist::RecordCodec::decode(probe, probeWritten - 1, decodedPayload, decodedInfo);
        logLine("PERSIST,codec,truncated,%s", resultName(truncated));
    }
}

void PersistenceExampleScene::saveOnInput() {
    logLine("PERSIST,input,A,action=save");

    volume_ = static_cast<uint8_t>((volume_ + 1) % 11);
    brightness_ = static_cast<uint8_t>((brightness_ + 1) % 16);
    invertY_ = !invertY_;
    ++presses_;
    level_ = static_cast<uint8_t>(1 + presses_ / 5);

    // Settings: one setBlob, then one commit at this safe point. On the board
    // the set is the expensive part and commit() is nearly free.
    const uint8_t settingsPayload[kSettingsPayloadSize] = {
        volume_, brightness_, static_cast<uint8_t>(invertY_ ? 1 : 0)};
    uint8_t settingsScratch[kScratchSize];
    const persist::StoreResult settingsWrite = persistence_.saveSettingRecord(
        kSettingsKey, kSettingsSchema, settingsPayload, sizeof(settingsPayload),
        settingsScratch, sizeof(settingsScratch));
    logLine("PERSIST,save,settings_write,%s", resultName(settingsWrite));

    const persist::StoreResult settingsCommit = persistence_.settings().commit();
    logLine("PERSIST,save,settings_commit,%s", resultName(settingsCommit));

    // Save slots have no separate commit: on the board the write already
    // reached flash, and on native the file was moved into place.
    const uint8_t slotPayload[kSlotPayloadSize] = {
        level_, static_cast<uint8_t>(presses_ & 0xFF),
        static_cast<uint8_t>((presses_ >> 8) & 0xFF)};
    uint8_t slotScratch[kScratchSize];
    const persist::StoreResult slotWrite = persistence_.saveRecord(
        kSlotIndex, kSlotSchema, slotPayload, sizeof(slotPayload), slotScratch,
        sizeof(slotScratch));
    logLine("PERSIST,save,slot_write,%s,slot=0,bytes=%u", resultName(slotWrite),
            unsigned(persist::RecordCodec::kHeaderSize + kSlotPayloadSize));

    lastResult_ = (settingsWrite == persist::StoreResult::Ok &&
                   settingsCommit == persist::StoreResult::Ok &&
                   slotWrite == persist::StoreResult::Ok)
                      ? "Ok"
                      : "error";

    // Read both back through the service. This is what a restart does, so one
    // press proves the write reached the medium rather than the members.
    uint8_t settingsReadScratch[kScratchSize];
    const uint8_t* loadedSettings = nullptr;
    persist::RecordInfo loadedSettingsInfo{};
    const persist::StoreResult settingsReload = persistence_.loadSettingRecord(
        kSettingsKey, settingsReadScratch, sizeof(settingsReadScratch), loadedSettings,
        loadedSettingsInfo);
    if (settingsReload == persist::StoreResult::Ok) {
        logLine("PERSIST,save,settings_reload,Ok,schema=%u,payload=%u",
                unsigned(loadedSettingsInfo.schemaVersion),
                unsigned(loadedSettingsInfo.payloadSize));
        if (loadedSettingsInfo.schemaVersion == kSettingsSchema &&
            loadedSettingsInfo.payloadSize == kSettingsPayloadSize) {
            volume_ = loadedSettings[0];
            brightness_ = loadedSettings[1];
            invertY_ = loadedSettings[2] != 0;
            logLine("PERSIST,save,settings_now,volume=%u,brightness=%u,invert_y=%u",
                    unsigned(volume_), unsigned(brightness_), unsigned(invertY_ ? 1 : 0));
        }
    } else {
        logLine("PERSIST,save,settings_reload,%s", resultName(settingsReload));
    }

    uint8_t slotReadScratch[kScratchSize];
    const uint8_t* loadedSlot = nullptr;
    persist::RecordInfo loadedSlotInfo{};
    const persist::StoreResult slotReload = persistence_.loadRecord(
        kSlotIndex, slotReadScratch, sizeof(slotReadScratch), loadedSlot, loadedSlotInfo);
    if (slotReload == persist::StoreResult::Ok) {
        logLine("PERSIST,save,slot_reload,Ok,schema=%u,payload=%u",
                unsigned(loadedSlotInfo.schemaVersion), unsigned(loadedSlotInfo.payloadSize));
        if (loadedSlotInfo.schemaVersion == kSlotSchema &&
            loadedSlotInfo.payloadSize == kSlotPayloadSize) {
            level_ = loadedSlot[0];
            presses_ = readU16(loadedSlot + 1);
            logLine("PERSIST,save,slot_now,level=%u,presses=%u", unsigned(level_),
                    unsigned(presses_));
        }
    } else {
        logLine("PERSIST,save,slot_reload,%s", resultName(slotReload));
    }
}

void PersistenceExampleScene::init() {
    Scene::init();

    volume_ = 8;
    brightness_ = 12;
    invertY_ = false;
    level_ = 1;
    presses_ = 0;
    lastResult_ = "none";

    loadOrUseDefaults();
    logLine("PERSIST,ready,input=A");
}

void PersistenceExampleScene::update(unsigned long deltaTime) {
    Scene::update(deltaTime);

    if (engine.getInputManager().isButtonPressed(kButtonConfirm)) {
        saveOnInput();
    }
}

void PersistenceExampleScene::draw(gfx::Renderer& renderer) {
    const int width = renderer.getLogicalWidth();
    const int height = renderer.getLogicalHeight();
    renderer.drawFilledRectangle(0, 0, width, height, gfx::Color::Black);
    Scene::draw(renderer);

    renderer.drawText("Persistence", 4, 4, gfx::Color::Cyan, 1);

    char line[48];
    std::snprintf(line, sizeof(line), "volume %u  brightness %u", unsigned(volume_),
                  unsigned(brightness_));
    renderer.drawText(line, 4, 20, gfx::Color::White, 1);

    std::snprintf(line, sizeof(line), "invert Y %u", unsigned(invertY_ ? 1 : 0));
    renderer.drawText(line, 4, 34, gfx::Color::White, 1);

    std::snprintf(line, sizeof(line), "slot %u  level %u  presses %u", unsigned(kSlotIndex),
                  unsigned(level_), unsigned(presses_));
    renderer.drawText(line, 4, 52, gfx::Color::White, 1);

    renderer.drawText("A: change + save", 4, 74, gfx::Color::Yellow, 1);
    std::snprintf(line, sizeof(line), "last write %s", lastResult_);
    renderer.drawText(line, 4, 88, gfx::Color::Gray, 1);
    renderer.drawText("Restart to reload", 4, 104, gfx::Color::Gray, 1);
}

}  // namespace persistenceexample
