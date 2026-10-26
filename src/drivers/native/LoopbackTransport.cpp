/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#include "platforms/EngineConfig.h"

#if PIXELROOT32_ENABLE_NETWORKING && defined(PLATFORM_NATIVE)

#include "drivers/native/LoopbackTransport.h"

namespace pixelroot32::networking {

// =============================================================================
// LoopbackBus
// =============================================================================

void LoopbackBus::setNowMs(uint32_t nowMs) noexcept {
    currentNowMs = nowMs;
}

uint32_t LoopbackBus::nowMs() const noexcept {
    return currentNowMs;
}

void LoopbackBus::setFaults(const LoopbackFaults& faults) noexcept {
    faultState = faults;
    reseed();
}

const LoopbackFaults& LoopbackBus::faults() const noexcept {
    return faultState;
}

void LoopbackBus::reset() noexcept {
    for (uint8_t i = 0; i < transportCount; ++i) {
        transports[i]->resetFromBus();
        transports[i] = nullptr;
    }
    transportCount = 0;
    reseed();
}

bool LoopbackBus::attach(LoopbackTransport& transport) noexcept {
    for (uint8_t i = 0; i < transportCount; ++i) {
        if (transports[i] == &transport) {
            return true;
        }
    }
    if (transportCount >= kMaxTransports) {
        return false;
    }
    transports[transportCount++] = &transport;
    return true;
}

void LoopbackBus::detach(LoopbackTransport& transport) noexcept {
    for (uint8_t i = 0; i < transportCount; ++i) {
        if (transports[i] != &transport) {
            continue;
        }
        for (uint8_t j = static_cast<uint8_t>(i + 1); j < transportCount; ++j) {
            transports[j - 1] = transports[j];
        }
        --transportCount;
        transports[transportCount] = nullptr;
        return;
    }
}

LoopbackTransport* LoopbackBus::findByAddress(
    const NetAddress& address) noexcept {
    for (uint8_t i = 0; i < transportCount; ++i) {
        if (transports[i] != nullptr && transports[i]->address == address) {
            return transports[i];
        }
    }
    return nullptr;
}

uint32_t LoopbackBus::nextRandom() noexcept {
    // xorshift32: small, deterministic and allocation-free. Zero is a fixed
    // point, so reseed() never installs it.
    uint32_t value = rngState;
    value ^= value << 13;
    value ^= value >> 17;
    value ^= value << 5;
    rngState = value;
    return value;
}

void LoopbackBus::reseed() noexcept {
    rngState = (faultState.seed != 0u) ? faultState.seed : 0x12345678u;
}

// =============================================================================
// LoopbackTransport
// =============================================================================

LoopbackTransport::LoopbackTransport(LoopbackBus& busRef,
                                     const NetAddress& localAddress)
    : bus(busRef), address(localAddress) {}

LoopbackTransport::~LoopbackTransport() {
    if (attached) {
        bus.detach(*this);
        attached = false;
    }
}

NetResult LoopbackTransport::init(const NetTransportConfig& newConfig) {
    if (newConfig.channel < 1 || newConfig.channel > 13) {
        return NetResult::InvalidArgument;
    }

    config = newConfig;
    channel = newConfig.channel;

    if (!attached) {
        if (!bus.attach(*this)) {
            return NetResult::Busy;
        }
        attached = true;
    }

    ready = true;
    return NetResult::Ok;
}

void LoopbackTransport::shutdown() {
    if (attached) {
        bus.detach(*this);
        attached = false;
    }
    ready = false;
    clearQueues();
}

NetResult LoopbackTransport::addPeer(const NetPeerConfig& peer) {
    // The loopback resolves an address against its attached transports, so a
    // peer registration has nothing to store.
    (void)peer;
    return NetResult::Ok;
}

NetResult LoopbackTransport::removePeer(const NetAddress& address) {
    (void)address;
    return NetResult::Ok;
}

NetResult LoopbackTransport::send(const NetAddress& to, const uint8_t* frame,
                                  uint16_t length) {
    if (!ready) {
        return NetResult::NotInitialized;
    }
    if (frame == nullptr && length > 0) {
        return NetResult::InvalidArgument;
    }
    if (length > kNetTransportFrameSize) {
        return NetResult::PayloadTooLarge;
    }
    if (bus.faultState.sendBusy) {
        return NetResult::Busy;
    }

    // One entry per destination that must produce a tx result. A plan with
    // zero copies is a destination that was reached but not delivered
    // (channel mismatch or injected loss).
    struct Plan {
        LoopbackTransport* dest = nullptr;
        uint8_t copies = 0;
        bool reorder = false;
    };

    Plan plans[LoopbackBus::kMaxTransports]{};
    uint8_t planCount = 0;

    if (to.isBroadcast()) {
        for (uint8_t i = 0; i < bus.transportCount; ++i) {
            LoopbackTransport* candidate = bus.transports[i];
            if (candidate == this || candidate->channel != channel) {
                continue;
            }
            Plan plan;
            plan.dest = candidate;
            if (!rollPercent(bus.faultState.lossPercent)) {
                plan.copies =
                    rollPercent(bus.faultState.duplicatePercent) ? 2u : 1u;
                plan.reorder = rollPercent(bus.faultState.reorderPercent);
            }
            plans[planCount++] = plan;
        }
        // A broadcast into an empty bus is still an accepted send; there is
        // simply nobody to report a result for.
        if (planCount == 0) {
            return NetResult::Ok;
        }
    } else {
        LoopbackTransport* dest = bus.findByAddress(to);
        if (dest == nullptr) {
            return NetResult::PeerNotFound;
        }
        Plan plan;
        plan.dest = dest;
        if (dest->channel == channel &&
            !rollPercent(bus.faultState.lossPercent)) {
            plan.copies =
                rollPercent(bus.faultState.duplicatePercent) ? 2u : 1u;
            plan.reorder = rollPercent(bus.faultState.reorderPercent);
        }
        plans[planCount++] = plan;
    }

    // Pre-flight both fixed queues. Busy must never leave a partial delivery
    // or a send without its matching result.
    if (static_cast<uint16_t>(txCount) + planCount > kTxResultCapacity) {
        return NetResult::Busy;
    }
    for (uint8_t i = 0; i < planCount; ++i) {
        const Plan& plan = plans[i];
        if (plan.copies == 0) {
            continue;
        }
        if (static_cast<uint16_t>(plan.dest->rxCount) + plan.copies >
            kRxQueueCapacity) {
            return NetResult::Busy;
        }
    }

    const uint32_t dueMs =
        bus.currentNowMs + static_cast<uint32_t>(bus.faultState.delayMs);

    for (uint8_t i = 0; i < planCount; ++i) {
        const Plan& plan = plans[i];

        if (plan.copies == 0) {
            txQueue[txCount] = NetTxResult{plan.dest->address, false};
            ++txCount;
            continue;
        }

        const uint8_t base = plan.dest->rxCount;
        for (uint8_t copy = 0; copy < plan.copies; ++copy) {
            plan.dest->pushRx(address, frame, length, dueMs);
        }

        if (plan.reorder && base > 0) {
            // Reordering means the new frame overtakes the most recent entry
            // already waiting for this destination.
            const uint8_t tail = static_cast<uint8_t>(plan.dest->rxCount - 1);
            const RxEntry previous = plan.dest->rxQueue[base - 1];
            plan.dest->rxQueue[base - 1] = plan.dest->rxQueue[tail];
            plan.dest->rxQueue[tail] = previous;
        }

        txQueue[txCount] = NetTxResult{plan.dest->address, true};
        ++txCount;
    }

    return NetResult::Ok;
}

bool LoopbackTransport::pollRx(NetRxInfo& out) {
    const uint32_t now = bus.currentNowMs;
    for (uint8_t i = 0; i < rxCount; ++i) {
        // Wrap-safe "now >= dueMs", so a 32-bit clock wrap does not freeze
        // queued frames forever.
        if (static_cast<int32_t>(now - rxQueue[i].dueMs) < 0) {
            continue;
        }
        out = rxQueue[i].info;
        for (uint8_t j = static_cast<uint8_t>(i + 1); j < rxCount; ++j) {
            rxQueue[j - 1] = rxQueue[j];
        }
        --rxCount;
        return true;
    }
    return false;
}

bool LoopbackTransport::pollTxResult(NetTxResult& out) {
    if (txCount == 0) {
        return false;
    }
    out = txQueue[0];
    for (uint8_t i = 1; i < txCount; ++i) {
        txQueue[i - 1] = txQueue[i];
    }
    --txCount;
    return true;
}

NetAddress LoopbackTransport::localAddress() const {
    return address;
}

NetResult LoopbackTransport::setChannel(uint8_t newChannel) {
    if (newChannel < 1 || newChannel > 13) {
        return NetResult::InvalidArgument;
    }
    channel = newChannel;
    return NetResult::Ok;
}

bool LoopbackTransport::isReady() const {
    return ready;
}

void LoopbackTransport::clearQueues() noexcept {
    rxCount = 0;
    txCount = 0;
}

void LoopbackTransport::resetFromBus() noexcept {
    attached = false;
    ready = false;
    clearQueues();
}

void LoopbackTransport::pushRx(const NetAddress& from, const uint8_t* frame,
                               uint16_t length, uint32_t dueMs) noexcept {
    RxEntry& entry = rxQueue[rxCount];
    entry.info = NetRxInfo{};
    entry.info.from = from;
    entry.info.length = length;
    for (uint16_t i = 0; i < length; ++i) {
        entry.info.data[i] = frame[i];
    }
    entry.dueMs = dueMs;
    ++rxCount;
}

bool LoopbackTransport::rollPercent(uint8_t percent) noexcept {
    return (bus.nextRandom() % 100u) < percent;
}

}  // namespace pixelroot32::networking

#endif  // PIXELROOT32_ENABLE_NETWORKING && defined(PLATFORM_NATIVE)
