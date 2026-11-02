/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */

/**
 * @file main.cpp
 * @brief Board smoke for the persistence module's NVS adapter (T6).
 *
 * Prints, once, over serial at 115200:
 *
 * - `S13`: a blob written by Arduino `Preferences::putBytes` in namespace
 *   `pr32pref`, key `cfg`, read back through `Esp32NvsKeyValueStore::getBlob`
 *   (and the other way round), byte for byte.
 * - `IDF`: what the real NVS does when one key is written under two types,
 *   with the raw `nvs_*` calls (the behaviour the adapter's one-value-per-key
 *   rule is built on), then the adapter's own type replacement check.
 * - `TIME`: microseconds of `setBlob` and of `commit()` for 8 B and 64 B blobs,
 *   min / mean / max over kIterations writes - the flash stall a game pays at a
 *   save point.
 * - `STATS`: `nvs_get_stats` (used / free / total entries) before and after.
 * - `SIZE`: sizeof of both store classes.
 *
 * Safety: the firmware writes only to its own namespaces (`pr32pref`,
 * `pr32smk`) and erases only those, through handles of their own, at the end.
 * It never opens namespace `doom`, never erases the partition and never
 * erases flash.
 *
 * Built with -D PERSIST_SMOKE_LINK_STORES=0 (env smoke_nostores) every
 * Esp32Nvs* object and call is compiled out and the rest stays; the .data +
 * .bss difference between the two images is the adapter's static RAM (N3).
 */

#include <Arduino.h>
#include <Preferences.h>
#include <esp_idf_version.h>
#include <esp_timer.h>
#include <nvs.h>
#include <nvs_flash.h>

#include <cstring>

#ifndef PERSIST_SMOKE_LINK_STORES
#define PERSIST_SMOKE_LINK_STORES 1
#endif

#if PERSIST_SMOKE_LINK_STORES
#include <drivers/esp32/Esp32NvsStore.h>
using pixelroot32::persistence::Esp32NvsKeyValueStore;
using pixelroot32::persistence::Esp32NvsSlotStore;
using pixelroot32::persistence::StoreResult;
#endif

namespace {

constexpr char kPartition[] = "nvs";
constexpr char kPrefNamespace[] = "pr32pref";
constexpr char kSmokeNamespace[] = "pr32smk";
constexpr int kIterations = 50;
constexpr size_t kMaxBlob = 64;

#if PERSIST_SMOKE_LINK_STORES
// Static, as a game would hold them: these objects are what N3 measures.
Esp32NvsKeyValueStore gPrefStore(kPrefNamespace, kMaxBlob);
Esp32NvsKeyValueStore gSmokeStore(kSmokeNamespace, kMaxBlob);

const char* name(StoreResult r) {
    switch (r) {
        case StoreResult::Ok: return "Ok";
        case StoreResult::NotFound: return "NotFound";
        case StoreResult::TooLarge: return "TooLarge";
        case StoreResult::NoSpace: return "NoSpace";
        case StoreResult::Corrupt: return "Corrupt";
        case StoreResult::IoError: return "IoError";
        case StoreResult::Unavailable: return "Unavailable";
        case StoreResult::InvalidKey: return "InvalidKey";
        case StoreResult::InvalidArgument: return "InvalidArgument";
    }
    return "?";
}
#endif

int gFailures = 0;

void verdict(const char* what, bool pass) {
    Serial.printf("%s %s\n", pass ? "PASS" : "FAIL", what);
    if (!pass) {
        ++gFailures;
    }
}

void printStats(const char* when) {
    nvs_stats_t s = {};
    const esp_err_t err = nvs_get_stats(kPartition, &s);
    Serial.printf("STATS %s err=%s used=%u free=%u total=%u namespaces=%u\n", when,
                  esp_err_to_name(err), static_cast<unsigned>(s.used_entries),
                  static_cast<unsigned>(s.free_entries), static_cast<unsigned>(s.total_entries),
                  static_cast<unsigned>(s.namespace_count));
}

// Erases every key of one of this firmware's own namespaces.
void wipeOwnNamespace(const char* ns) {
    nvs_handle_t h = 0;
    esp_err_t err = nvs_open_from_partition(kPartition, ns, NVS_READWRITE, &h);
    if (err == ESP_OK) {
        err = nvs_erase_all(h);
        if (err == ESP_OK) {
            err = nvs_commit(h);
        }
        nvs_close(h);
    }
    Serial.printf("CLEAN %s %s\n", ns, esp_err_to_name(err));
}

void fill(uint8_t* data, size_t size, uint8_t seed) {
    for (size_t i = 0; i < size; ++i) {
        data[i] = static_cast<uint8_t>(seed + i * 13u);
    }
}

// --- IDF behaviour: one key, two types, raw nvs_* calls ----------------------
void probeIdfTypes() {
    nvs_handle_t h = 0;
    esp_err_t err = nvs_open_from_partition(kPartition, kSmokeNamespace, NVS_READWRITE, &h);
    if (err != ESP_OK) {
        Serial.printf("IDF open %s\n", esp_err_to_name(err));
        ++gFailures;
        return;
    }
    const uint8_t blob[3] = {1, 2, 3};
    int32_t i = 0;
    uint32_t u = 0;
    size_t len = 0;
    Serial.printf("IDF set_i32(t)=%s\n", esp_err_to_name(nvs_set_i32(h, "t", -5)));
    Serial.printf("IDF set_blob(t) on i32 key=%s\n",
                  esp_err_to_name(nvs_set_blob(h, "t", blob, sizeof(blob))));
    Serial.printf("IDF get_i32(t)=%s\n", esp_err_to_name(nvs_get_i32(h, "t", &i)));
    Serial.printf("IDF get_u32(t)=%s\n", esp_err_to_name(nvs_get_u32(h, "t", &u)));
    Serial.printf("IDF get_blob(t) len=%s\n",
                  esp_err_to_name(nvs_get_blob(h, "t", nullptr, &len)));
    Serial.printf("IDF set_u32(t) on i32+blob key=%s\n",
                  esp_err_to_name(nvs_set_u32(h, "t", 7)));
    int erased = 0;
    while (erased < 8 && nvs_erase_key(h, "t") == ESP_OK) {
        ++erased;
    }
    Serial.printf("IDF erase_key(t) removed %d item(s)\n", erased);
    Serial.printf("IDF set_blob(z) nullptr len 0=%s\n",
                  esp_err_to_name(nvs_set_blob(h, "z", nullptr, 0)));
    nvs_erase_key(h, "z");
    Serial.printf("IDF set_blob(e) len 0=%s\n", esp_err_to_name(nvs_set_blob(h, "e", blob, 0)));
    len = 99;
    Serial.printf("IDF get_blob(e) len=%s size=%u\n",
                  esp_err_to_name(nvs_get_blob(h, "e", nullptr, &len)),
                  static_cast<unsigned>(len));
    nvs_erase_key(h, "e");
    nvs_commit(h);
    nvs_close(h);
}

// Preferences side of S13, compiled in both images so the N3 difference is
// the adapter alone.
size_t putWithPreferences(const char* key, const uint8_t* data, size_t size) {
    Preferences prefs;
    const bool ok = prefs.begin(kPrefNamespace, false);
    const size_t put = ok ? prefs.putBytes(key, data, size) : 0;
    prefs.end();
    Serial.printf("S13 Preferences.putBytes(%s/%s) begin=%d wrote=%u\n", kPrefNamespace, key,
                  ok, static_cast<unsigned>(put));
    return put;
}

size_t getWithPreferences(const char* key, uint8_t* out, size_t capacity) {
    Preferences prefs;
    const bool ok = prefs.begin(kPrefNamespace, true);
    const size_t n = ok ? prefs.getBytes(key, out, capacity) : 0;
    prefs.end();
    return n;
}

// 8 bytes, the size of the DOOM settings record.
constexpr size_t kCfgBytes = 8;

#if PERSIST_SMOKE_LINK_STORES
// --- S13: Preferences::putBytes -> Esp32NvsKeyValueStore::getBlob -------------
void checkPreferencesCompat() {
    uint8_t written[kCfgBytes];
    fill(written, sizeof(written), 0x31);
    const size_t put = putWithPreferences("cfg", written, sizeof(written));

    StoreResult r = gPrefStore.init();
    Serial.printf("S13 Esp32NvsKeyValueStore(%s).init=%s\n", kPrefNamespace, name(r));
    uint8_t read[kMaxBlob];
    memset(read, 0xEE, sizeof(read));
    size_t stored = 0;
    r = gPrefStore.getBlob("cfg", read, sizeof(read), stored);
    Serial.printf("S13 getBlob=%s stored=%u\n", name(r), static_cast<unsigned>(stored));
    verdict("S13 Preferences.putBytes -> getBlob byte-identical",
            put == sizeof(written) && r == StoreResult::Ok && stored == sizeof(written) &&
                memcmp(read, written, sizeof(written)) == 0 && read[sizeof(written)] == 0xEE);

    // The other way round: the adapter writes, Preferences reads.
    uint8_t back[12];
    fill(back, sizeof(back), 0x77);
    r = gPrefStore.setBlob("cfg2", back, sizeof(back));
    const StoreResult c = gPrefStore.commit();
    uint8_t got[sizeof(back)] = {};
    const size_t n = getWithPreferences("cfg2", got, sizeof(got));
    verdict("S13 setBlob -> Preferences.getBytes byte-identical",
            r == StoreResult::Ok && c == StoreResult::Ok && n == sizeof(back) &&
                memcmp(got, back, sizeof(back)) == 0);
}

// --- Adapter: last write wins across types, on the real IDF -------------------
void checkTypeReplacement() {
    const uint8_t blob[5] = {5, 4, 3, 2, 1};
    StoreResult a = gSmokeStore.setI32("k", 1234);
    StoreResult b = gSmokeStore.setBlob("k", blob, sizeof(blob));
    int32_t i = 42;
    StoreResult g = gSmokeStore.getI32("k", i);
    uint8_t out[8] = {};
    size_t stored = 0;
    StoreResult gb = gSmokeStore.getBlob("k", out, sizeof(out), stored);
    Serial.printf("TYPE setI32=%s setBlob=%s getI32=%s(i=%ld) getBlob=%s stored=%u\n", name(a),
                  name(b), name(g), static_cast<long>(i), name(gb),
                  static_cast<unsigned>(stored));
    verdict("TYPE i32 then blob: getI32 NotFound, blob intact",
            a == StoreResult::Ok && b == StoreResult::Ok && g == StoreResult::NotFound &&
                i == 42 && gb == StoreResult::Ok && stored == sizeof(blob) &&
                memcmp(out, blob, sizeof(blob)) == 0);

    a = gSmokeStore.setU32("k", 0xCAFEF00Du);
    uint32_t u = 0;
    g = gSmokeStore.getU32("k", u);
    stored = 77;
    gb = gSmokeStore.getBlob("k", out, sizeof(out), stored);
    verdict("TYPE blob then u32: getU32 Ok, getBlob NotFound",
            a == StoreResult::Ok && g == StoreResult::Ok && u == 0xCAFEF00Du &&
                gb == StoreResult::NotFound && stored == 77);
    verdict("TYPE erase removes the key", gSmokeStore.erase("k") == StoreResult::Ok &&
                                              gSmokeStore.erase("k") == StoreResult::NotFound);
}

// --- Write cost at a save point ----------------------------------------------
void timeWrites(size_t size) {
    uint8_t data[kMaxBlob];
    int64_t setMin = INT64_MAX, setMax = 0, setSum = 0;
    int64_t comMin = INT64_MAX, comMax = 0, comSum = 0;
    int failures = 0;
    for (int n = 0; n < kIterations; ++n) {
        fill(data, size, static_cast<uint8_t>(n));  // A real change every time.
        const int64_t t0 = esp_timer_get_time();
        const StoreResult s = gSmokeStore.setBlob("tm", data, size);
        const int64_t t1 = esp_timer_get_time();
        const StoreResult c = gSmokeStore.commit();
        const int64_t t2 = esp_timer_get_time();
        if (s != StoreResult::Ok || c != StoreResult::Ok) {
            ++failures;
        }
        const int64_t ds = t1 - t0, dc = t2 - t1;
        setMin = ds < setMin ? ds : setMin;
        setMax = ds > setMax ? ds : setMax;
        setSum += ds;
        comMin = dc < comMin ? dc : comMin;
        comMax = dc > comMax ? dc : comMax;
        comSum += dc;
    }
    Serial.printf("TIME setBlob %2u B us min=%lld mean=%lld max=%lld | commit us min=%lld "
                  "mean=%lld max=%lld | n=%d failures=%d\n",
                  static_cast<unsigned>(size), setMin, setSum / kIterations, setMax, comMin,
                  comSum / kIterations, comMax, kIterations, failures);
    if (failures != 0) {
        ++gFailures;
    }
}
#endif  // PERSIST_SMOKE_LINK_STORES

}  // namespace

void setup() {
    Serial.begin(115200);
    delay(1500);
    Serial.printf("\n# persist_nvs_smoke stores=%d\n", PERSIST_SMOKE_LINK_STORES);
    Serial.printf("# chip=%s rev=%d mhz=%lu idf=%s arduino=%d.%d.%d\n", ESP.getChipModel(),
                  ESP.getChipRevision(), static_cast<unsigned long>(ESP.getCpuFreqMHz()),
                  esp_get_idf_version(), ESP_ARDUINO_VERSION_MAJOR, ESP_ARDUINO_VERSION_MINOR,
                  ESP_ARDUINO_VERSION_PATCH);
    printStats("before");

#if PERSIST_SMOKE_LINK_STORES
    Serial.printf("SIZE Esp32NvsKeyValueStore=%u Esp32NvsSlotStore=%u\n",
                  static_cast<unsigned>(sizeof(Esp32NvsKeyValueStore)),
                  static_cast<unsigned>(sizeof(Esp32NvsSlotStore)));
    // Before init() everything is Unavailable.
    int32_t v = 0;
    verdict("Unavailable before init", gSmokeStore.getI32("k", v) == StoreResult::Unavailable &&
                                           gSmokeStore.commit() == StoreResult::Unavailable);
    const StoreResult r = gSmokeStore.init();
    Serial.printf("INIT %s=%s (again=%s)\n", kSmokeNamespace, name(r), name(gSmokeStore.init()));
    if (r != StoreResult::Ok) {
        ++gFailures;
    }
    checkPreferencesCompat();
#else
    uint8_t cfg[kCfgBytes];
    fill(cfg, sizeof(cfg), 0x31);
    putWithPreferences("cfg", cfg, sizeof(cfg));
    Serial.printf("S13 skipped (stores not linked), read back %u B with Preferences\n",
                  static_cast<unsigned>(getWithPreferences("cfg", cfg, sizeof(cfg))));
#endif

    probeIdfTypes();

#if PERSIST_SMOKE_LINK_STORES
    if (r == StoreResult::Ok) {
        checkTypeReplacement();
        printStats("before-timing");
        timeWrites(8);
        timeWrites(64);
        printStats("after-timing");
    }
#endif

    wipeOwnNamespace(kSmokeNamespace);
    wipeOwnNamespace(kPrefNamespace);
    printStats("after");
    Serial.printf("# done failures=%d\n", gFailures);
}

void loop() {
    delay(1000);
}
