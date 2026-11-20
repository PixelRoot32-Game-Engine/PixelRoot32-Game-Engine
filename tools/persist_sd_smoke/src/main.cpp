/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */

/**
 * @file main.cpp
 * @brief Board smoke for the persistence module's microSD adapter (T2).
 *
 * Prints, once, over serial at 115200:
 *
 * - `SIZE`: sizeof of SdSlotStore.
 * - `INIT`: the mount result for the CYD SD pins (CS=5, SCK=18, MISO=19,
 *   MOSI=23, 4 MHz) and the result of calling init() again.
 * - `CARD`: card capacity, total and used bytes, when a card is mounted.
 * - `SLOT0`: one write -> query -> read -> erase round trip, with every
 *   StoreResult.
 *
 * Safety: the firmware mounts the card without formatting it and writes only
 * under `/pr32smk/s` (slot 0), which it erases again. Nothing else on the card
 * is touched.
 *
 * Built with `pio test -e smoke --without-testing` this file is not part of
 * the test firmware (`test_build_src = no`); the shared contract suite is.
 */

#include <Arduino.h>
#include <SD.h>
#include <SPI.h>

#include <cstring>

#include <drivers/esp32/SdConfig.h>
#include <drivers/esp32/SdStore.h>

using pixelroot32::persistence::SdConfig;
using pixelroot32::persistence::SdSlotStore;
using pixelroot32::persistence::StoreResult;

namespace {

constexpr uint16_t kSlots = 3;
constexpr size_t kRecordBytes = 64;
constexpr char kRootDir[] = "/pr32smk";
constexpr char kPrefix[] = "s";

// CYD microSD wiring (SPI/VSPI).
constexpr int kCs = 5;
constexpr int kSck = 18;
constexpr int kMiso = 19;
constexpr int kMosi = 23;
constexpr uint32_t kFrequencyHz = 4000000;

// Static, as a game would hold it: `SIZE` reports this object.
SdSlotStore gSlots(kRootDir, kPrefix, kSlots, kRecordBytes);

int gFailures = 0;

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

void verdict(const char* what, bool pass) {
    Serial.printf("%s %s\n", pass ? "PASS" : "FAIL", what);
    if (!pass) {
        ++gFailures;
    }
}

void printCard() {
    const uint64_t cardSize = SD.cardSize();
    const uint64_t total = SD.totalBytes();
    const uint64_t used = SD.usedBytes();
    const uint64_t free = total >= used ? total - used : 0;
    Serial.printf("CARD cardSize=%llu total=%llu used=%llu free=%llu bytes\n",
                  static_cast<unsigned long long>(cardSize),
                  static_cast<unsigned long long>(total),
                  static_cast<unsigned long long>(used),
                  static_cast<unsigned long long>(free));
}

void roundTrip() {
    uint8_t data[kRecordBytes];
    for (size_t i = 0; i < sizeof(data); ++i) {
        data[i] = static_cast<uint8_t>(0x40u + i);
    }

    const StoreResult w = gSlots.write(0, data, sizeof(data));
    size_t queried = 0;
    const StoreResult q = gSlots.query(0, queried);
    uint8_t back[kRecordBytes] = {};
    size_t got = 0;
    const StoreResult r = gSlots.read(0, back, sizeof(back), got);
    const StoreResult e = gSlots.erase(0);
    size_t after = 123;
    const StoreResult q2 = gSlots.query(0, after);
    Serial.printf("SLOT0 write=%s query=%s(%u) read=%s(%u) erase=%s query_after=%s(%u)\n",
                  name(w), name(q), static_cast<unsigned>(queried), name(r),
                  static_cast<unsigned>(got), name(e), name(q2),
                  static_cast<unsigned>(after));
    verdict("slot 0 write/query/read/erase round trip",
            w == StoreResult::Ok && q == StoreResult::Ok && queried == sizeof(data) &&
                r == StoreResult::Ok && got == sizeof(data) &&
                memcmp(data, back, sizeof(data)) == 0 && e == StoreResult::Ok &&
                q2 == StoreResult::NotFound && after == 0);
}

}  // namespace

void setup() {
    Serial.begin(115200);
    delay(1500);
    Serial.printf("\n# persist_sd_smoke\n");
    Serial.printf("SIZE SdSlotStore=%u\n", static_cast<unsigned>(sizeof(SdSlotStore)));

    SdConfig cfg;
    cfg.spi = &SPI;
    cfg.sck = kSck;
    cfg.miso = kMiso;
    cfg.mosi = kMosi;
    cfg.cs = kCs;
    cfg.frequencyHz = kFrequencyHz;

    const StoreResult init = gSlots.init(cfg);
    Serial.printf("INIT root=%s prefix=%s cs=%d sck=%d miso=%d mosi=%d hz=%u -> %s\n",
                  kRootDir, kPrefix, kCs, kSck, kMiso, kMosi,
                  static_cast<unsigned>(kFrequencyHz), name(init));
    verdict("init() Ok", init == StoreResult::Ok);

    if (init == StoreResult::Ok) {
        Serial.printf("INIT again=%s\n", name(gSlots.init(cfg)));
        printCard();
        roundTrip();
    } else {
        Serial.printf("# no card mounted: insert a FAT32 card and reset\n");
    }

    Serial.printf("# done failures=%d\n", gFailures);
}

void loop() {
    delay(1000);
}
