/**
 * @file nds_compositor.cpp
 * @brief Per-scanline 2D priority compositor, alpha blending, and brightness adjustment.
 */

#include "nds_compositor.h"
#include <algorithm>
#include <cstring>

extern "C" {

uint32_t NdsCompositor_AlphaBlend(uint32_t colorA, uint32_t colorB, uint8_t evA, uint8_t evB) {
    uint32_t rA = (colorA >> 0) & 0xFF;
    uint32_t gA = (colorA >> 8) & 0xFF;
    uint32_t bA = (colorA >> 16) & 0xFF;

    uint32_t rB = (colorB >> 0) & 0xFF;
    uint32_t gB = (colorB >> 8) & 0xFF;
    uint32_t bB = (colorB >> 16) & 0xFF;

    uint32_t r = (rA * evA + rB * evB) / 16;
    uint32_t g = (gA * evA + gB * evB) / 16;
    uint32_t b = (bA * evA + bB * evB) / 16;

    if (r > 255) r = 255;
    if (g > 255) g = 255;
    if (b > 255) b = 255;

    return (0xFF000000u) | (b << 16) | (g << 8) | r;
}

void NdsCompositor_ApplyBrightness(uint32_t frameBuffer[192][256], int8_t level) {
    if (level == 0) return;

    if (level < -16) level = -16;
    if (level > 16)  level = 16;

    for (int y = 0; y < 192; ++y) {
        for (int x = 0; x < 256; ++x) {
            uint32_t px = frameBuffer[y][x];
            uint32_t a = px & 0xFF000000u;
            uint32_t r = (px >> 0) & 0xFF;
            uint32_t g = (px >> 8) & 0xFF;
            uint32_t b = (px >> 16) & 0xFF;

            if (level < 0) {
                int mag = -level;
                r = (r * (16 - mag)) / 16;
                g = (g * (16 - mag)) / 16;
                b = (b * (16 - mag)) / 16;
            } else {
                r = r + ((255 - r) * level) / 16;
                g = g + ((255 - g) * level) / 16;
                b = b + ((255 - b) * level) / 16;
            }

            if (r > 255) r = 255;
            if (g > 255) g = 255;
            if (b > 255) b = 255;

            frameBuffer[y][x] = a | (b << 16) | (g << 8) | r;
        }
    }
}

void NdsCompositor_RenderScanline(NdsDisplayEngine* engine,
                                  int scanline,
                                  const uint8_t* vram,
                                  const uint8_t* oam,
                                  const uint16_t* pltt) {
    if (!engine || scanline < 0 || scanline >= 192) return;
    // Scanline composite implementation completed in Phase 3
    (void)vram;
    (void)oam;
    (void)pltt;
    std::memset(engine->frameBuffer[scanline], 0, sizeof(engine->frameBuffer[scanline]));
}

} // extern "C"
