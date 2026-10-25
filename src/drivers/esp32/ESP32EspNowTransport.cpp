/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#include "drivers/esp32/ESP32EspNowTransport.h"

#include "platforms/EngineConfig.h"

#if PIXELROOT32_ENABLE_NETWORKING && defined(ESP32)

#include <Arduino.h>
#include <WiFi.h>
#include <esp_arduino_version.h>
#include <esp_now.h>
#include <esp_wifi.h>

#include <cstring>

namespace pixelroot32::networking {

// The radio and its callbacks are process-global, so exactly one transport may
// claim them. Every callback entry point dereferences this pointer and drops
// the frame when it is null, which is also what makes shutdown() safe.
ESP32EspNowTransport* ESP32EspNowTransport::instance_ = nullptr;

ESP32EspNowTransport::~ESP32EspNowTransport() {
    // The object owns process-global callbacks while it is alive; letting it
    // die without unregistering would leave the Wi-Fi task calling into freed
    // memory. shutdown() is idempotent, so this is also safe after release().
    shutdown();
}

NetResult ESP32EspNowTransport::init(const NetTransportConfig& newConfig) {
    // Validate before claiming the radio: an unusable channel must not leave
    // this instance marked as the owner of a half-configured link.
    if (newConfig.channel < kMinChannel || newConfig.channel > kMaxChannel) {
        return NetResult::InvalidArgument;
    }

    // Callbacks can only name one owner, so a second transport is refused
    // before it can touch Wi-Fi or ESP-NOW state that is not its own.
    if (instance_ != nullptr && instance_ != this) {
        return NetResult::Busy;
    }

    if (ready) {
        // Re-init reconfigures the live link instead of cycling it: tearing the
        // radio down would drop frames the session is still waiting on.
        config = newConfig;
        if (newConfig.channel != channel) {
            return setChannel(newConfig.channel);
        }
        return NetResult::Ok;
    }

    instance_ = this;
    config = newConfig;
    clearRings();

    if (config.manageWifi) {
        // Station without a router: ESP-NOW needs the interface started, not an
        // association, and joining an access point would move the channel under
        // the link. Only this transport's own bring-up is undone in release().
        if (!WiFi.mode(WIFI_STA)) {
            release();
            return NetResult::TransportError;
        }
        ownsWifi = true;
        WiFi.disconnect();
    }

    // The channel must be set once the interface exists; before that the call
    // reports "not initialized" and every peer would stay on the default one.
    if (esp_wifi_set_channel(config.channel, WIFI_SECOND_CHAN_NONE) != ESP_OK) {
        release();
        return NetResult::TransportError;
    }
    channel = config.channel;

    if (esp_now_init() != ESP_OK) {
        release();
        return NetResult::TransportError;
    }
    linkUp = true;

    // Arduino core 3.x changed the receive callback to carry a full
    // esp_now_recv_info_t. The source MAC is the only field this backend reads,
    // so the shim only changes where that MAC comes from.
    esp_err_t callbackResult;
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
    callbackResult = esp_now_register_recv_cb(
        [](const esp_now_recv_info_t* info, const uint8_t* data, int length) {
            if (instance_ != nullptr && info != nullptr) {
                instance_->stageReceived(info->src_addr, data, length);
            }
        });
#else
    callbackResult = esp_now_register_recv_cb(
        [](const uint8_t* mac, const uint8_t* data, int length) {
            if (instance_ != nullptr) {
                instance_->stageReceived(mac, data, length);
            }
        });
#endif
    if (callbackResult != ESP_OK) {
        release();
        return NetResult::TransportError;
    }

    callbackResult = esp_now_register_send_cb(
        [](const uint8_t* mac, esp_now_send_status_t status) {
            if (instance_ != nullptr) {
                instance_->stageSendResult(mac, status == ESP_NOW_SEND_SUCCESS);
            }
        });
    if (callbackResult != ESP_OK) {
        release();
        return NetResult::TransportError;
    }

    if (esp_wifi_get_mac(WIFI_IF_STA, localMac.bytes) != ESP_OK) {
        release();
        return NetResult::TransportError;
    }

    // Discovery may be disabled, but HELLO and BYE are broadcasts, so the
    // broadcast peer must exist before the session can announce itself.
    NetPeerConfig broadcastPeer;
    std::memset(broadcastPeer.address.bytes, 0xFF, kNetAddressSize);
    if (addPeer(broadcastPeer) != NetResult::Ok) {
        release();
        return NetResult::TransportError;
    }

    ready = true;
    return NetResult::Ok;
}

void ESP32EspNowTransport::shutdown() {
    if (instance_ != this) {
        // Never owned the radio, or a failed init() already unwound everything.
        // Touching the callbacks here would deinitialize another instance's
        // link, so this path only clears the local flag.
        ready = false;
        return;
    }
    release();
}

NetResult ESP32EspNowTransport::addPeer(const NetPeerConfig& peer) {
    if (instance_ != this || !linkUp) {
        return NetResult::NotInitialized;
    }
    if (peer.address.isZero()) {
        // A zero MAC is not a deliverable identity; ESP-NOW would register a
        // peer that can never be reached.
        return NetResult::InvalidArgument;
    }

    esp_now_peer_info_t info{};
    std::memcpy(info.peer_addr, peer.address.bytes, kNetAddressSize);
    // Channel 0 means "follow the interface", so a later setChannel moves this
    // peer with the link instead of leaving it stranded on the old frequency.
    info.channel = 0;
    info.ifidx = WIFI_IF_STA;
    // v1 ships no key exchange: NetPeerConfig::encrypted is ignored on purpose,
    // because honouring it without an LMK would silently make the peer
    // unreachable instead of reporting a configuration error.
    info.encrypt = false;
    info.priv = nullptr;

    const esp_err_t result = esp_now_add_peer(&info);
    if (result == ESP_OK || result == ESP_ERR_ESPNOW_EXIST) {
        // Already registered is the state the caller asked for.
        return NetResult::Ok;
    }
    if (result == ESP_ERR_ESPNOW_FULL) {
        return NetResult::PeerTableFull;
    }
    if (result == ESP_ERR_ESPNOW_ARG) {
        return NetResult::InvalidArgument;
    }
    return NetResult::TransportError;
}

NetResult ESP32EspNowTransport::removePeer(const NetAddress& address) {
    if (instance_ != this || !linkUp) {
        return NetResult::NotInitialized;
    }

    const esp_err_t result = esp_now_del_peer(address.bytes);
    if (result == ESP_OK || result == ESP_ERR_ESPNOW_NOT_FOUND) {
        // Both outcomes mean the address is no longer registered.
        return NetResult::Ok;
    }
    if (result == ESP_ERR_ESPNOW_ARG) {
        return NetResult::InvalidArgument;
    }
    return NetResult::TransportError;
}

NetResult ESP32EspNowTransport::send(const NetAddress& to, const uint8_t* frame,
                                     uint16_t length) {
    if (!ready) {
        return NetResult::NotInitialized;
    }
    if (frame == nullptr && length != 0) {
        return NetResult::InvalidArgument;
    }
    if (length > kNetTransportFrameSize) {
        return NetResult::PayloadTooLarge;
    }

    const esp_err_t result = esp_now_send(to.bytes, frame, length);
    if (result == ESP_OK) {
        return NetResult::Ok;
    }
    if (result == ESP_ERR_ESPNOW_NO_MEM) {
        // The radio is momentarily out of buffers; the session keeps the
        // message queued and retries on a later update.
        return NetResult::Busy;
    }
    if (result == ESP_ERR_ESPNOW_NOT_FOUND) {
        return NetResult::PeerNotFound;
    }
    if (result == ESP_ERR_ESPNOW_ARG) {
        return NetResult::InvalidArgument;
    }
    return NetResult::TransportError;
}

bool ESP32EspNowTransport::pollRx(NetRxInfo& out) {
    const uint16_t tail = rxTail.load(std::memory_order_relaxed);
    // Acquire pairs with the producer's release store, so the entry contents
    // are visible before the index advance is.
    const uint16_t head = rxHead.load(std::memory_order_acquire);
    if (tail == head) {
        return false;
    }

    out = rxRing[tail % kRxRingCapacity];
    rxTail.store(static_cast<uint16_t>(tail + 1), std::memory_order_release);
    return true;
}

bool ESP32EspNowTransport::pollTxResult(NetTxResult& out) {
    const uint16_t tail = txTail.load(std::memory_order_relaxed);
    const uint16_t head = txHead.load(std::memory_order_acquire);
    if (tail == head) {
        return false;
    }

    out = txRing[tail % kTxResultRingCapacity];
    txTail.store(static_cast<uint16_t>(tail + 1), std::memory_order_release);
    return true;
}

NetAddress ESP32EspNowTransport::localAddress() const {
    return localMac;
}

NetResult ESP32EspNowTransport::setChannel(uint8_t newChannel) {
    if (newChannel < kMinChannel || newChannel > kMaxChannel) {
        return NetResult::InvalidArgument;
    }
    if (!ready) {
        // Without a live interface the radio call would fail with a less
        // specific error; the caller gets the real reason instead.
        return NetResult::NotInitialized;
    }

    const esp_err_t result =
        esp_wifi_set_channel(newChannel, WIFI_SECOND_CHAN_NONE);
    if (result == ESP_ERR_INVALID_ARG) {
        return NetResult::InvalidArgument;
    }
    if (result != ESP_OK) {
        return NetResult::TransportError;
    }

    channel = newChannel;
    return NetResult::Ok;
}

bool ESP32EspNowTransport::isReady() const {
    return ready;
}

void ESP32EspNowTransport::release() noexcept {
    if (linkUp) {
        // Unregister first so the Wi-Fi task stops calling into this object
        // before the rings are reset and the instance pointer is cleared.
        esp_now_unregister_recv_cb();
        esp_now_unregister_send_cb();
        esp_now_deinit();
        linkUp = false;
    }

    clearRings();
    localMac = NetAddress{};
    channel = kMinChannel;

    if (ownsWifi) {
        // Only undo the bring-up this transport performed: an application that
        // passed manageWifi = false keeps its own Wi-Fi state untouched.
        WiFi.disconnect();
        WiFi.mode(WIFI_OFF);
        ownsWifi = false;
    }

    ready = false;
    if (instance_ == this) {
        instance_ = nullptr;
    }
}

void ESP32EspNowTransport::clearRings() noexcept {
    rxHead.store(0, std::memory_order_relaxed);
    rxTail.store(0, std::memory_order_relaxed);
    txHead.store(0, std::memory_order_relaxed);
    txTail.store(0, std::memory_order_relaxed);
    rxDropped.store(0, std::memory_order_relaxed);
    txDropped.store(0, std::memory_order_relaxed);
}

void ESP32EspNowTransport::stageReceived(const uint8_t* mac, const uint8_t* data,
                                         int length) noexcept {
    // A partial copy would hand the session a frame whose length field and
    // bytes disagree, so anything outside the frame window is dropped whole.
    if (mac == nullptr || data == nullptr || length <= 0 ||
        length > static_cast<int>(kNetTransportFrameSize)) {
        rxDropped.fetch_add(1, std::memory_order_relaxed);
        return;
    }

    const uint16_t head = rxHead.load(std::memory_order_relaxed);
    const uint16_t tail = rxTail.load(std::memory_order_acquire);
    if (static_cast<uint16_t>(head - tail) >= kRxRingCapacity) {
        // Drop the newest frame: the ones already staged arrived first and are
        // more likely to be the retransmissions the peer is waiting on.
        rxDropped.fetch_add(1, std::memory_order_relaxed);
        return;
    }

    NetRxInfo& entry = rxRing[head % kRxRingCapacity];
    std::memcpy(entry.from.bytes, mac, kNetAddressSize);
    std::memcpy(entry.data, data, static_cast<size_t>(length));
    entry.length = static_cast<uint16_t>(length);
    rxHead.store(static_cast<uint16_t>(head + 1), std::memory_order_release);
}

void ESP32EspNowTransport::stageSendResult(const uint8_t* mac,
                                           bool success) noexcept {
    if (mac == nullptr) {
        txDropped.fetch_add(1, std::memory_order_relaxed);
        return;
    }

    const uint16_t head = txHead.load(std::memory_order_relaxed);
    const uint16_t tail = txTail.load(std::memory_order_acquire);
    if (static_cast<uint16_t>(head - tail) >= kTxResultRingCapacity) {
        // Losing a link-level result is harmless: the application-level ACK is
        // what the session trusts, so the newest result is the one to drop.
        txDropped.fetch_add(1, std::memory_order_relaxed);
        return;
    }

    NetTxResult& entry = txRing[head % kTxResultRingCapacity];
    std::memcpy(entry.to.bytes, mac, kNetAddressSize);
    entry.success = success;
    txHead.store(static_cast<uint16_t>(head + 1), std::memory_order_release);
}

}  // namespace pixelroot32::networking

#endif  // PIXELROOT32_ENABLE_NETWORKING && defined(ESP32)
