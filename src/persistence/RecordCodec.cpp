/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#include "persistence/RecordCodec.h"

#if PIXELROOT32_ENABLE_PERSISTENCE

namespace pixelroot32::persistence {

namespace {

// CRC-32/IEEE remainders for every 4-bit value, reflected polynomial
// 0xEDB88320. Processing a byte as two nibbles costs 64 bytes of flash instead
// of the usual 1 KB byte table.
constexpr uint32_t kCrcNibbleTable[16] = {
    0x00000000u, 0x1DB71064u, 0x3B6E20C8u, 0x26D930ACu,
    0x76DC4190u, 0x6B6B51F4u, 0x4DB26158u, 0x5005713Cu,
    0xEDB88320u, 0xF00F9344u, 0xD6D6A3E8u, 0xCB61B38Cu,
    0x9B64C2B0u, 0x86D3D2D4u, 0xA00AE278u, 0xBDBDF21Cu};

constexpr uint8_t kMagic[4] = {'P', 'R', 'S', 'T'};

// Multi-byte fields are assembled by hand instead of by copying a struct, so
// neither padding nor host endianness can reach the stored record.
void writeU16(uint8_t* out, uint16_t value) {
    out[0] = static_cast<uint8_t>(value);
    out[1] = static_cast<uint8_t>(value >> 8);
}

void writeU32(uint8_t* out, uint32_t value) {
    out[0] = static_cast<uint8_t>(value);
    out[1] = static_cast<uint8_t>(value >> 8);
    out[2] = static_cast<uint8_t>(value >> 16);
    out[3] = static_cast<uint8_t>(value >> 24);
}

uint16_t readU16(const uint8_t* in) {
    return static_cast<uint16_t>(static_cast<uint16_t>(in[0]) |
                                 (static_cast<uint16_t>(in[1]) << 8));
}

uint32_t readU32(const uint8_t* in) {
    return static_cast<uint32_t>(in[0]) |
           (static_cast<uint32_t>(in[1]) << 8) |
           (static_cast<uint32_t>(in[2]) << 16) |
           (static_cast<uint32_t>(in[3]) << 24);
}

// Offset of the CRC field; the CRC covers every header byte before it.
constexpr size_t kCrcOffset = 12;

}  // namespace

uint32_t crc32(const uint8_t* data, size_t size, uint32_t crc) {
    crc = ~crc;
    for (size_t i = 0; i < size; ++i) {
        crc ^= data[i];
        crc = (crc >> 4) ^ kCrcNibbleTable[crc & 0x0Fu];
        crc = (crc >> 4) ^ kCrcNibbleTable[crc & 0x0Fu];
    }
    return ~crc;
}

StoreResult RecordCodec::encode(uint16_t schema, const uint8_t* payload, size_t size,
                                uint8_t* out, size_t capacity, size_t& written) {
    written = 0;
    if (out == nullptr || (payload == nullptr && size > 0) ||
        static_cast<uint64_t>(size) > UINT32_MAX) {
        return StoreResult::InvalidArgument;
    }
    // Written as a subtraction so a huge size cannot overflow the sum.
    if (capacity < kHeaderSize || capacity - kHeaderSize < size) {
        return StoreResult::TooLarge;
    }

    for (size_t i = 0; i < sizeof(kMagic); ++i) {
        out[i] = kMagic[i];
    }
    out[4] = kEnvelopeVersion;
    out[5] = 0;
    writeU16(out + 6, schema);
    writeU32(out + 8, static_cast<uint32_t>(size));
    for (size_t i = 0; i < size; ++i) {
        out[kHeaderSize + i] = payload[i];
    }
    writeU32(out + kCrcOffset, crc32(out + kHeaderSize, size, crc32(out, kCrcOffset)));

    written = kHeaderSize + size;
    return StoreResult::Ok;
}

StoreResult RecordCodec::decode(const uint8_t* in, size_t size, const uint8_t*& payload,
                                RecordInfo& info) {
    if (in == nullptr) {
        return StoreResult::InvalidArgument;
    }
    if (size < kHeaderSize) {
        return StoreResult::Corrupt;
    }
    for (size_t i = 0; i < sizeof(kMagic); ++i) {
        if (in[i] != kMagic[i]) {
            return StoreResult::Corrupt;
        }
    }
    if (in[4] != kEnvelopeVersion || in[5] != 0) {
        return StoreResult::Corrupt;
    }
    const size_t payloadSize = size - kHeaderSize;
    if (static_cast<uint64_t>(readU32(in + 8)) != static_cast<uint64_t>(payloadSize)) {
        return StoreResult::Corrupt;
    }
    const uint32_t expected = crc32(in + kHeaderSize, payloadSize, crc32(in, kCrcOffset));
    if (readU32(in + kCrcOffset) != expected) {
        return StoreResult::Corrupt;
    }

    payload = in + kHeaderSize;
    info.schemaVersion = readU16(in + 6);
    info.payloadSize = payloadSize;
    return StoreResult::Ok;
}

}  // namespace pixelroot32::persistence

#endif  // PIXELROOT32_ENABLE_PERSISTENCE
