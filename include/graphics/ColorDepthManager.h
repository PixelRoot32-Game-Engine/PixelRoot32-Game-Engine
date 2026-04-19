/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 *
 * ColorDepthManager: Runtime color depth selection and palette management.
 * Supports 24-bit, 16-bit (RGB565), 8-bit indexed, and 4-bit indexed modes.
 */

#pragma once

#include <cstddef>
#include <cstdint>

namespace pixelroot32::graphics {

/**
 * @class ColorDepthManager
 * @brief Manages color depth selection for display output.
 *
 * Provides runtime selection of color depth to reduce SPI transfer:
 * - 24-bit: RGB888 (not used in v1)
 * - 16-bit: RGB565 (default - uses existing paletteLUT)
 * - 8-bit: 256-color indexed (reduces bandwidth by 66%)
 * - 4-bit: 16-color indexed (reduces bandwidth by 83%)
 *
 * Forward declared for external palette LUT (managed elsewhere).
 */
class ColorDepthManager {
public:
    /**
     * @enum Depth
     * @brief Color depth options
     */
    enum class Depth : uint8_t {
        Depth24 = 24,  // RGB888 (not currently used for output)
        Depth16 = 16,  // RGB565 (default)
        Depth8 = 8,    // Indexed palette (256 colors)
        Depth4 = 4     // Indexed palette (16 colors)
    };

private:
    Depth currentDepth_ = Depth::Depth16;
    bool paletteDirty_ = true;

    // Custom palette (nullptr = use default)
    const uint16_t* customPalette_ = nullptr;
    bool usingCustomPalette_ = false;

    // Statistics tracking
    uint32_t totalBytesTransferred_ = 0;
    uint32_t frameCount_ = 0;

public:
    ColorDepthManager();

    /**
     * @brief Set color depth at runtime.
     * @param depth New color depth
     */
    void setDepth(Depth depth);

    /**
     * @brief Set color depth from integer value.
     * @param depthBits Number of bits (24, 16, 8, 4)
     */
    void setDepth(int depthBits) {
        switch (depthBits) {
            case 24: setDepth(Depth::Depth24); break;
            case 16: setDepth(Depth::Depth16); break;
            case 8:  setDepth(Depth::Depth8); break;
            case 4:  setDepth(Depth::Depth4); break;
            default: setDepth(Depth::Depth16); break;
        }
    }

    /**
     * @brief Get current depth.
     * @return Current depth
     */
    Depth getDepth() const { return currentDepth_; }

    /**
     * @brief Get current depth as integer bits.
     * @return 24, 16, 8, or 4
     */
    int getDepthBits() const { return static_cast<int>(currentDepth_); }

    /**
     * @brief Get bytes per pixel for current depth.
     * @return Bytes per pixel (1-3)
     */
    int getBytesPerPixel() const {
        switch (currentDepth_) {
            case Depth::Depth24: return 3;
            case Depth::Depth16: return 2;
            case Depth::Depth8:
            case Depth::Depth4:  return 1;
            default: return 2;
        }
    }

    /**
     * @brief Estimate transfer size for a region.
     * @param width Region width in pixels
     * @param height Region height in pixels
     * @return Estimated transfer size in bytes
     */
    size_t estimateTransferSize(int width, int height) const {
        return static_cast<size_t>(width) * height * getBytesPerPixel();
    }

    /**
     * @brief Check if current mode needs palette conversion.
     * @return true for 8-bit or 4-bit modes
     */
    bool needsPaletteConversion() const {
        return currentDepth_ == Depth::Depth8 ||
               currentDepth_ == Depth::Depth4;
    }

    /**
     * @brief Check if sprites can use 8-bit format directly.
     * @return true for 8-bit or 4-bit depth
     */
    bool canUse8BitSprites() const {
        return currentDepth_ == Depth::Depth8 ||
               currentDepth_ == Depth::Depth4;
    }

    /**
     * @brief Check if palette was changed (needs rebuild).
     * @return true if palette needs update
     */
    bool isPaletteDirty() const { return paletteDirty_; }

    /**
     * @brief Mark palette as clean after update.
     */
    void markPaletteClean() { paletteDirty_ = false; }

    /**
     * @brief Get transfer reduction ratio vs 24-bit.
     * @return Ratio as float (e.g., 0.67 for 16-bit)
     */
    float getTransferRatio() const {
        return static_cast<float>(getBytesPerPixel()) / 3.0f;
    }

    /**
     * @brief Get statistics: total bytes transferred.
     * @return Total bytes
     */
    uint32_t getTotalBytesTransferred() const { return totalBytesTransferred_; }

    /**
     * @brief Accumulate bytes transferred.
     * @param bytes Number of bytes sent
     */
    void addBytesTransferred(uint32_t bytes) {
        totalBytesTransferred_ += bytes;
    }

    /**
     * @brief Get statistics: frame count.
     * @return Frame count
     */
    uint32_t getFrameCount() const { return frameCount_; }

    /**
     * @brief Increment frame count.
     */
    void incrementFrameCount() { ++frameCount_; }

    /**
     * @brief Get average bytes per frame.
     * @return Average bytes, or 0 if no frames
     */
    uint32_t getAverageBytesPerFrame() const {
        if (frameCount_ == 0) return 0;
        return totalBytesTransferred_ / frameCount_;
    }

    /**
     * @brief Reset statistics.
     */
    void resetStatistics() {
        totalBytesTransferred_ = 0;
        frameCount_ = 0;
    }

    /**
     * @brief Check if using default 16-bit depth.
     * @return true if depth is Depth16
     */
    bool isDefaultDepth() const {
        return currentDepth_ == Depth::Depth16;
    }

    // ============================================================================
    // 8-bit Indexed Palette API
    // ============================================================================

    /**
     * @brief Set a custom 256-color palette for 8-bit indexed mode.
     *
     * The palette should be an array of 256 RGB565 color values (512 bytes total).
     * Use nullptr (default) to revert to the built-in default palette.
     *
     * The palette is stored by reference - the caller must ensure the array
     * remains valid for the lifetime of the ColorDepthManager. For ESP32,
     * store the palette in PROGMEM/flash to conserve RAM.
     *
     * @param palette256 Array of 256 RGB565 color values, or nullptr for default
     */
    void setCustomPalette(const uint16_t* palette256);

    /**
     * @brief Get the current palette.
     * @return Pointer to the 256-color palette (never null)
     */
    const uint16_t* getPalette() const;

    /**
     * @brief Check if using a custom palette vs default.
     * @return true if custom palette is set
     */
    bool isUsingCustomPalette() const { return usingCustomPalette_; }
};

// Inline implementation for Depth enum operators
inline ColorDepthManager::Depth operator+(ColorDepthManager::Depth a, int b) {
    return static_cast<ColorDepthManager::Depth>(
        static_cast<uint8_t>(a) + b
    );
}

} // namespace pixelroot32::graphics