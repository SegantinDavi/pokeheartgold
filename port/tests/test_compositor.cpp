/**
 * @file test_compositor.cpp
 * @brief Unit tests for scanline compositor, priority sorting, alpha blend, and brightness (Phase 4 / User Story 2).
 */

#include "test_runner.h"
#include "nds_compositor.h"
#include "port_gx.h"
#include <vector>
#include <cstring>
#include <cmath>

TEST_CASE(Compositor_PrioritySorting) {
    std::vector<uint8_t> vram(0x20000, 0);
    uint16_t pltt[256];
    std::memset(pltt, 0, sizeof(pltt));

    pltt[1] = 0x001F; // Red (RGB555)
    pltt[2] = 0x03E0; // Green (RGB555)
    uint32_t expectedGreen = GX_RGB555ToRGBA(0x03E0);

    // Tile 1 in charBase 0: Red
    std::memset(vram.data() + 32, 0x11, 32);
    // Tile 2 in charBase 0: Green
    std::memset(vram.data() + 64, 0x22, 32);

    // BG0 screen map at 2048: Tile 1 (Red)
    uint16_t* bg0Map = reinterpret_cast<uint16_t*>(vram.data() + 2048);
    bg0Map[0] = 1;

    // BG1 screen map at 4096: Tile 2 (Green)
    uint16_t* bg1Map = reinterpret_cast<uint16_t*>(vram.data() + 4096);
    bg1Map[0] = 2;

    NdsDisplayEngine engine{};
    engine.dispcnt.dispMode = 1;
    engine.dispcnt.visiblePlanes = (1 << 0) | (1 << 1);

    // BG0 at priority 1 (lower priority)
    engine.bg[0].enabled = true;
    engine.bg[0].mode = NDS_BGMODE_TEXT;
    engine.bg[0].colorMode = 0;
    engine.bg[0].priority = 1;
    engine.bg[0].screenBase = 1;
    engine.bg[0].charBase = 0;

    // BG1 at priority 0 (higher priority)
    engine.bg[1].enabled = true;
    engine.bg[1].mode = NDS_BGMODE_TEXT;
    engine.bg[1].colorMode = 0;
    engine.bg[1].priority = 0;
    engine.bg[1].screenBase = 2;
    engine.bg[1].charBase = 0;

    NdsCompositor_RenderScanline(&engine, 0, vram.data(), nullptr, pltt);

    // Priority 0 (BG1, Green) MUST win over Priority 1 (BG0, Red)
    for (int x = 0; x < 8; ++x) {
        ASSERT_EQ(expectedGreen, engine.frameBuffer[0][x]);
    }
}

TEST_CASE(Compositor_AlphaBlendMix) {
    // Red RGBA: R=255, G=0, B=0, A=255
    uint32_t colA = GX_PackRGBA(255, 0, 0, 255);
    uint32_t colB = GX_PackRGBA(0, 255, 0, 255);

    // 50/50 mix: EVA=8, EVB=8
    uint32_t blended = NdsCompositor_AlphaBlend(colA, colB, 8, 8);

    uint8_t r = blended & 0xFF;
    uint8_t g = (blended >> 8) & 0xFF;
    uint8_t b = (blended >> 16) & 0xFF;
    uint8_t a = (blended >> 24) & 0xFF;

    ASSERT_EQ(255, a);
    ASSERT_EQ(0, b);
    // 255 * 8 / 16 = 127
    ASSERT_TRUE(std::abs(static_cast<int>(r) - 127) <= 1);
    ASSERT_TRUE(std::abs(static_cast<int>(g) - 127) <= 1);
}

TEST_CASE(Compositor_ApplyBrightnessExtremes) {
    uint32_t fb[192][256];

    // Populate with arbitrary non-white, non-black colors
    for (int y = 0; y < 192; ++y) {
        for (int x = 0; x < 256; ++x) {
            fb[y][x] = GX_PackRGBA(x & 0xFF, y & 0xFF, 100, 255);
        }
    }

    // ApplyBrightness +16 -> all pixels MUST be pure white (0xFFFFFFFF)
    NdsCompositor_ApplyBrightness(fb, 16);
    for (int y = 0; y < 192; ++y) {
        for (int x = 0; x < 256; ++x) {
            ASSERT_EQ(0xFFFFFFFFu, fb[y][x]);
        }
    }

    // Repopulate with colors
    for (int y = 0; y < 192; ++y) {
        for (int x = 0; x < 256; ++x) {
            fb[y][x] = GX_PackRGBA(x & 0xFF, y & 0xFF, 100, 255);
        }
    }

    // ApplyBrightness -16 -> all pixels MUST be pure black with alpha preserved (0xFF000000)
    NdsCompositor_ApplyBrightness(fb, -16);
    for (int y = 0; y < 192; ++y) {
        for (int x = 0; x < 256; ++x) {
            ASSERT_EQ(0xFF000000u, fb[y][x]);
        }
    }
}
