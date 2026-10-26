/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#include "networking/NetProtocol.h"

#if PIXELROOT32_ENABLE_NETWORKING

namespace pixelroot32::networking {

namespace {

// Multi-byte fields are assembled by hand instead of by copying a struct, so
// neither padding nor host endianness can reach the wire. ESP32 (RISC-V) and
// native hosts are both little-endian today; doing it explicitly keeps the
// wire format a property of this file rather than of the compiler.
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

void writeU16(uint8_t* out, uint16_t value) {
    out[0] = static_cast<uint8_t>(value & 0xFFu);
    out[1] = static_cast<uint8_t>(value >> 8);
}

void writeU32(uint8_t* out, uint32_t value) {
    out[0] = static_cast<uint8_t>(value & 0xFFu);
    out[1] = static_cast<uint8_t>((value >> 8) & 0xFFu);
    out[2] = static_cast<uint8_t>((value >> 16) & 0xFFu);
    out[3] = static_cast<uint8_t>((value >> 24) & 0xFFu);
}

bool hasReservedFlags(uint8_t flags) {
    // Bit 0 is the only defined flag; anything else means the sender speaks a
    // newer dialect than this build and its frames must not be interpreted.
    return (flags & static_cast<uint8_t>(~kNetFlagReliable)) != 0;
}

// Single validation path shared by both decoders, so decodeHeader and
// decodeFrame can never disagree about what a valid frame is. The check order
// is part of the public contract documented on NetParseResult.
NetParseResult validateHeader(const uint8_t* frame, uint16_t frameLength,
                              NetPacketHeader& out) {
    if (frameLength < kNetHeaderSize) {
        return NetParseResult::TooShort;
    }

    // The magic is the ASCII tag 'P','R' in reading order, so it is read
    // high-byte first while every numeric field below is little-endian.
    const uint16_t magic = static_cast<uint16_t>(
        (static_cast<uint16_t>(frame[0]) << 8) | static_cast<uint16_t>(frame[1]));
    if (magic != kNetMagic) {
        return NetParseResult::BadMagic;
    }

    out.version = frame[2];
    out.type = static_cast<NetPacketType>(frame[3]);
    out.flags = frame[4];
    out.sourceNodeId = frame[5];
    out.sourceBootId = readU16(frame + 6);
    out.sequence = readU32(frame + 8);
    out.ackSequence = readU16(frame + 12);
    out.payloadLength = readU16(frame + 14);

    if (out.version != platforms::config::NetProtocolVersion) {
        return NetParseResult::BadVersion;
    }
    if (!isValidPacketType(out.type)) {
        return NetParseResult::BadType;
    }
    if (hasReservedFlags(out.flags)) {
        return NetParseResult::BadFlags;
    }
    if (out.payloadLength > platforms::config::NetMaxPayload) {
        return NetParseResult::PayloadTooLarge;
    }
    if (frameLength != static_cast<uint16_t>(kNetHeaderSize + out.payloadLength)) {
        return NetParseResult::LengthMismatch;
    }
    return NetParseResult::Ok;
}

}  // namespace

bool isValidPacketType(NetPacketType type) noexcept {
    switch (type) {
        case NetPacketType::Data:
        case NetPacketType::Hello:
        case NetPacketType::Bye:
        case NetPacketType::Ack:
        case NetPacketType::Snapshot:
            return true;
    }
    return false;
}

NetParseResult decodeHeader(const uint8_t* frame, uint16_t frameLength,
                            NetPacketHeader& out) {
    if (frame == nullptr) {
        return NetParseResult::InvalidArgument;
    }
    return validateHeader(frame, frameLength, out);
}

NetParseResult decodeFrame(const uint8_t* frame, uint16_t frameLength,
                           NetPacketHeader& out, const uint8_t** payload) {
    if (frame == nullptr || payload == nullptr) {
        return NetParseResult::InvalidArgument;
    }

    // Reset before validating so a rejected frame never leaves the caller
    // holding a pointer into the buffer it just tried to decode.
    *payload = nullptr;

    const NetParseResult result = validateHeader(frame, frameLength, out);
    if (result != NetParseResult::Ok) {
        return result;
    }

    if (out.payloadLength > 0) {
        *payload = frame + kNetHeaderSize;
    }
    return NetParseResult::Ok;
}

uint16_t encodeHeader(const NetPacketHeader& header, uint8_t* out,
                      uint16_t capacity) {
    if (out == nullptr) {
        return 0;
    }
    if (capacity < kNetHeaderSize) {
        return 0;
    }
    // Encode applies the same rules as decode: a node running this build must
    // never put a frame on the wire that it would itself reject.
    if (header.version != platforms::config::NetProtocolVersion) {
        return 0;
    }
    if (!isValidPacketType(header.type)) {
        return 0;
    }
    if (hasReservedFlags(header.flags)) {
        return 0;
    }
    if (header.payloadLength > platforms::config::NetMaxPayload) {
        return 0;
    }

    out[0] = static_cast<uint8_t>(kNetMagic >> 8);  // 0x50 'P'
    out[1] = static_cast<uint8_t>(kNetMagic & 0xFFu);  // 0x52 'R'
    out[2] = header.version;
    out[3] = static_cast<uint8_t>(header.type);
    out[4] = header.flags;
    out[5] = header.sourceNodeId;
    writeU16(out + 6, header.sourceBootId);
    writeU32(out + 8, header.sequence);
    writeU16(out + 12, header.ackSequence);
    writeU16(out + 14, header.payloadLength);
    return kNetHeaderSize;
}

uint16_t encodeFrame(const NetPacketHeader& header, const uint8_t* payload,
                     uint8_t* out, uint16_t capacity) {
    if (out == nullptr) {
        return 0;
    }
    if (header.payloadLength > 0 && payload == nullptr) {
        return 0;
    }
    if (header.payloadLength > platforms::config::NetMaxPayload) {
        return 0;
    }

    // kNetHeaderSize + payloadLength cannot overflow: the ceiling above keeps
    // payloadLength <= 234 for any legal configuration.
    const uint16_t frameLength =
        static_cast<uint16_t>(kNetHeaderSize + header.payloadLength);
    if (capacity < frameLength) {
        return 0;
    }

    if (encodeHeader(header, out, capacity) == 0) {
        return 0;
    }

    for (uint16_t i = 0; i < header.payloadLength; ++i) {
        out[kNetHeaderSize + i] = payload[i];
    }
    return frameLength;
}

}  // namespace pixelroot32::networking

#endif  // PIXELROOT32_ENABLE_NETWORKING
