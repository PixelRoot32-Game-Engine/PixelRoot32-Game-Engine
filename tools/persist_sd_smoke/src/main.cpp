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
 * - `HOTSWAP`: the interactive stage. It seeds slot 1, asks for the card to be
 *   taken out (Enter in the terminal), checks that an access and a write on
 *   the dead volume fail visibly and that init() answers Unavailable, then
 *   asks for the card back (Enter again), checks that init() mounts again
 *   without a reboot, that the seeded record still reads back byte for byte
 *   and that slot 1 erases. Each prompt waits kHotSwapTimeoutMs; a missing
 *   answer prints `SKIP` and is counted in `skipped`, not in `failures`.
 *
 * Safety: the firmware mounts the card without formatting it and writes only
 * under `/pr32smk/s` (slots 0 and 1), which it erases again. Nothing else on
 * the card is touched.
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

// The hot swap keeps its record out of slot 0 so the round trip above stays
// self-contained.
constexpr uint16_t kHotSwapSlot = 1;
constexpr uint32_t kHotSwapTimeoutMs = 180000;
// Pressing Enter before the card is actually out (or back) is not a failure:
// the stage says so and asks again, this many times.
constexpr int kHotSwapAttempts = 3;

// CYD microSD wiring (SPI/VSPI).
constexpr int kCs = 5;
constexpr int kSck = 18;
constexpr int kMiso = 19;
constexpr int kMosi = 23;
constexpr uint32_t kFrequencyHz = 4000000;

// Static, as a game would hold it: `SIZE` reports this object.
SdSlotStore gSlots(kRootDir, kPrefix, kSlots, kRecordBytes);

int gFailures = 0;
int gSkipped = 0;

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

/**
 * Waits for any byte from the serial port, repeating @p what every 15 s so the
 * prompt survives a monitor opened mid-wait. Returns false on timeout.
 */
bool waitForEnter(uint32_t timeoutMs, const char* what) {
    const uint32_t start = millis();
    uint32_t lastReminder = start;
    for (;;) {
        if (Serial.available() > 0) {
            while (Serial.available() > 0) {
                (void)Serial.read();
            }
            Serial.println();
            return true;
        }
        const uint32_t now = millis();
        if (now - start >= timeoutMs) {
            Serial.println();
            return false;
        }
        if (now - lastReminder >= 15000) {
            lastReminder = now;
            Serial.printf(">>> waiting: %s, then Enter (%u s left)\n", what,
                          static_cast<unsigned>((timeoutMs - (now - start)) / 1000));
        }
        delay(100);
    }
}

void printCard() {
    Serial.printf("CARD cardSize=%llu total=%llu used=%llu bytes\n",
                  static_cast<unsigned long long>(SD.cardSize()),
                  static_cast<unsigned long long>(SD.totalBytes()),
                  static_cast<unsigned long long>(SD.usedBytes()));
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

/**
 * Card out and back in, with the board running the whole time: the record
 * seeded before the removal proves the volume really came back, and the
 * uptime proves no reset happened in between.
 */
void hotSwap(const SdConfig& cfg) {
    uint8_t seed[kRecordBytes];
    for (size_t i = 0; i < sizeof(seed); ++i) {
        seed[i] = static_cast<uint8_t>(0xA0u + i);
    }

    const StoreResult seeded = gSlots.write(kHotSwapSlot, seed, sizeof(seed));
    Serial.printf("HOTSWAP seed slot=%u write=%s\n", static_cast<unsigned>(kHotSwapSlot),
                  name(seeded));
    verdict("hot swap seed write", seeded == StoreResult::Ok);
    if (seeded != StoreResult::Ok) {
        return;
    }

    bool cardOut = false;
    for (int attempt = 1; attempt <= kHotSwapAttempts && !cardOut; ++attempt) {
        Serial.println();
        Serial.printf(">>> Take the microSD card OUT of the CYD, then send Enter in this terminal.%s\n",
                      attempt > 1 ? " Retrying." : "");
        if (!waitForEnter(kHotSwapTimeoutMs, "REMOVE the card")) {
            ++gSkipped;
            Serial.println("SKIP hot swap: no key from the serial port (card still in?)");
            return;
        }
        size_t removedSize = 123;
        const StoreResult outQuery = gSlots.query(kHotSwapSlot, removedSize);
        if (outQuery == StoreResult::Ok) {
            Serial.printf("HOTSWAP card out: query=Ok(%u) - the card is still in; pull it out, "
                          "then Enter\n",
                          static_cast<unsigned>(removedSize));
            continue;
        }
        cardOut = true;
        Serial.printf("HOTSWAP card out: query=%s(%u)\n", name(outQuery),
                      static_cast<unsigned>(removedSize));
        verdict("hot swap: with the card out an access fails visibly (NotFound/IoError)",
                outQuery == StoreResult::NotFound || outQuery == StoreResult::IoError);
    }
    if (!cardOut) {
        ++gSkipped;
        Serial.println("SKIP hot swap: the card never left the reader");
        return;
    }

    const StoreResult outInit = gSlots.init(cfg);
    Serial.printf("HOTSWAP card out: init=%s\n", name(outInit));
    verdict("hot swap: init() without the card is Unavailable",
            outInit == StoreResult::Unavailable);

    const StoreResult outWrite = gSlots.write(kHotSwapSlot, seed, sizeof(seed));
    Serial.printf("HOTSWAP card out: write=%s\n", name(outWrite));
    verdict("hot swap: an unmounted store answers Unavailable",
            outWrite == StoreResult::Unavailable);

    bool mounted = false;
    for (int attempt = 1; attempt <= kHotSwapAttempts && !mounted; ++attempt) {
        Serial.println();
        Serial.printf(">>> Put the card BACK IN, then send Enter in this terminal.%s\n",
                      attempt > 1 ? " Retrying." : "");
        if (!waitForEnter(kHotSwapTimeoutMs, "INSERT the card again")) {
            ++gSkipped;
            Serial.println("SKIP hot swap: no key from the serial port (card left out)");
            return;
        }
        const uint32_t started = millis();
        const StoreResult backInit = gSlots.init(cfg);
        const uint32_t took = millis() - started;
        if (backInit != StoreResult::Ok) {
            Serial.printf("HOTSWAP card back: init=%s - is the card in? insert it fully, "
                          "then Enter\n",
                          name(backInit));
            continue;
        }
        mounted = true;
        Serial.printf("HOTSWAP card back: init=%s in %u ms uptime=%u ms\n", name(backInit),
                      static_cast<unsigned>(took), static_cast<unsigned>(millis()));
        verdict("hot swap: init() mounts again without a reboot", true);
    }
    if (!mounted) {
        ++gSkipped;
        Serial.println("SKIP hot swap: the card never came back");
        return;
    }

    size_t backSize = 0;
    const StoreResult backQuery = gSlots.query(kHotSwapSlot, backSize);
    uint8_t back[kRecordBytes] = {};
    size_t got = 0;
    const StoreResult backRead = gSlots.read(kHotSwapSlot, back, sizeof(back), got);
    const bool match = backRead == StoreResult::Ok && got == sizeof(seed) &&
                       memcmp(seed, back, sizeof(seed)) == 0;
    Serial.printf("HOTSWAP card back: query=%s(%u) read=%s(%u) same=%s\n", name(backQuery),
                  static_cast<unsigned>(backSize), name(backRead),
                  static_cast<unsigned>(got), match ? "yes" : "no");
    verdict("hot swap: the record survived the reinsert",
            backQuery == StoreResult::Ok && backSize == sizeof(seed) && match);

    const StoreResult erased = gSlots.erase(kHotSwapSlot);
    size_t after = 7;
    const StoreResult afterQuery = gSlots.query(kHotSwapSlot, after);
    Serial.printf("HOTSWAP card back: erase=%s query_after=%s(%u)\n", name(erased),
                  name(afterQuery), static_cast<unsigned>(after));
    verdict("hot swap: slots work again after the re-mount",
            erased == StoreResult::Ok && afterQuery == StoreResult::NotFound && after == 0);
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
        hotSwap(cfg);
    } else {
        Serial.printf("# no card mounted: insert a FAT32 card and reset\n");
    }

    Serial.printf("# done failures=%d skipped=%d\n", gFailures, gSkipped);
}

void loop() {
    delay(1000);
}
