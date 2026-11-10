/**
 * @file test_bg_renderer.cpp
 * @brief Unit tests for 2D background tilemap and affine rendering (Phase 3 / User Story 1).
 */

#include "test_runner.h"
#include "nds_bg_renderer.h"
#include "nds_compositor.h"
#include "gx_pipeline.h"
#include "port_gx.h"
#include <vector>
#include <cstring>

TEST_CASE(BgRenderer_4bppTextTileDecode) {
    std::vector<uint8_t> vram(0x20000, 0);
    uint16_t pltt[256];
    std::memset(pltt, 0, sizeof(pltt));

    // Palette:
    // Index 0: 0x0000 (transparent)
    // Index 1: 0x001F (Red)
    // Index 2: 0x03E0 (Green)
    // Index 3: 0x7C00 (Blue)
    pltt[0] = 0x0000;
    pltt[1] = 0x001F;
    pltt[2] = 0x03E0;
    pltt[3] = 0x7C00;

    uint32_t expectedRed = GX_RGB555ToRGBA(0x001F);
    uint32_t expectedGreen = GX_RGB555ToRGBA(0x03E0);
    uint32_t expectedBlue = GX_RGB555ToRGBA(0x7C00);

    // Tiles (4bpp: 32 bytes per 8x8 tile)
    // Tile 0 at offset 0: empty (all 0)
    // Tile 1 at offset 32: all nibbles = 1
    std::memset(vram.data() + 32, 0x11, 32);
    // Tile 2 at offset 64: all nibbles = 2
    std::memset(vram.data() + 64, 0x22, 32);
    // Tile 3 at offset 96: all nibbles = 3
    std::memset(vram.data() + 96, 0x33, 32);

    // Screen map at screenBase = 1 (offset 2048)
    uint16_t* screenMap = reinterpret_cast<uint16_t*>(vram.data() + 2048);
    screenMap[0] = 1; // Tile 1
    screenMap[1] = 2; // Tile 2
    screenMap[2] = 3; // Tile 3

    NdsBgLayer bg{};
    bg.enabled = true;
    bg.mode = NDS_BGMODE_TEXT;
    bg.colorMode = 0; // 4bpp
    bg.screenBase = 1;
    bg.charBase = 0;
    bg.screenSize = 0; // 256x256
    bg.scrollX = 0;
    bg.scrollY = 0;

    uint32_t outLine[256];
    std::memset(outLine, 0, sizeof(outLine));
    NdsBg_RenderText(&bg, 0, vram.data(), pltt, outLine);

    // Verify first 3 tiles (8 pixels each)
    for (int x = 0; x < 8; ++x) {
        ASSERT_EQ(expectedRed, outLine[x]);
    }
    for (int x = 8; x < 16; ++x) {
        ASSERT_EQ(expectedGreen, outLine[x]);
    }
    for (int x = 16; x < 24; ++x) {
        ASSERT_EQ(expectedBlue, outLine[x]);
    }
    // Remaining pixels in line are tile 0 -> transparent
    for (int x = 24; x < 256; ++x) {
        ASSERT_EQ(NDS_PIXEL_TRANSPARENT, outLine[x]);
    }

    // Verify full 256x192 scanline loop consistency
    for (int y = 0; y < 192; ++y) {
        NdsBg_RenderText(&bg, y, vram.data(), pltt, outLine);
        if (y < 8) {
            ASSERT_EQ(expectedRed, outLine[0]);
            ASSERT_EQ(expectedGreen, outLine[8]);
            ASSERT_EQ(expectedBlue, outLine[16]);
            ASSERT_EQ(NDS_PIXEL_TRANSPARENT, outLine[24]);
        } else {
            // Lines 8..191 are tile 0
            for (int x = 0; x < 256; ++x) {
                ASSERT_EQ(NDS_PIXEL_TRANSPARENT, outLine[x]);
            }
        }
    }
}

TEST_CASE(BgRenderer_ScrollWrapSeamless) {
    std::vector<uint8_t> vram(0x20000, 0);
    uint16_t pltt[256];
    std::memset(pltt, 0, sizeof(pltt));

    pltt[1] = 0x001F; // Red
    pltt[2] = 0x03E0; // Green
    pltt[3] = 0x7C00; // Blue

    uint32_t expectedRed = GX_RGB555ToRGBA(0x001F);
    uint32_t expectedGreen = GX_RGB555ToRGBA(0x03E0);
    uint32_t expectedBlue = GX_RGB555ToRGBA(0x7C00);

    std::memset(vram.data() + 32, 0x11, 32); // Tile 1
    std::memset(vram.data() + 64, 0x22, 32); // Tile 2
    std::memset(vram.data() + 96, 0x33, 32); // Tile 3

    uint16_t* screenMap = reinterpret_cast<uint16_t*>(vram.data() + 2048);
    screenMap[0] = 1; // Tile 1 at tile col 0 (pixels 0..7)
    screenMap[1] = 2; // Tile 2 at tile col 1 (pixels 8..15)
    screenMap[2] = 3; // Tile 3 at tile col 2 (pixels 16..23)

    NdsBgLayer bg{};
    bg.enabled = true;
    bg.mode = NDS_BGMODE_TEXT;
    bg.colorMode = 0;
    bg.screenBase = 1;
    bg.charBase = 0;
    bg.screenSize = 0; // 256x256
    bg.scrollX = 250; // Scroll offset wrapping around 256-pixel boundary
    bg.scrollY = 0;

    uint32_t outLine[256];
    NdsBg_RenderText(&bg, 0, vram.data(), pltt, outLine);

    // x=0..5 corresponds to effX=250..255 (tile 31, which is empty -> transparent)
    for (int x = 0; x < 6; ++x) {
        ASSERT_EQ(NDS_PIXEL_TRANSPARENT, outLine[x]);
    }

    // x=6 corresponds to effX=(6+250)%256 = 0 (start of Tile 1, Red)
    // x=6..13 (8 pixels) should be Tile 1 (Red)
    for (int x = 6; x < 14; ++x) {
        ASSERT_EQ(expectedRed, outLine[x]);
    }

    // x=14..21 (8 pixels) should be Tile 2 (Green)
    for (int x = 14; x < 22; ++x) {
        ASSERT_EQ(expectedGreen, outLine[x]);
    }

    // x=22..29 (8 pixels) should be Tile 3 (Blue)
    for (int x = 22; x < 30; ++x) {
        ASSERT_EQ(expectedBlue, outLine[x]);
    }

    // x=30..255 should be transparent
    for (int x = 30; x < 256; ++x) {
        ASSERT_EQ(NDS_PIXEL_TRANSPARENT, outLine[x]);
    }
}

TEST_CASE(BgRenderer_AffineIdentityMatrix) {
    std::vector<uint8_t> vram(0x20000, 0);
    uint16_t pltt[256];
    std::memset(pltt, 0, sizeof(pltt));

    pltt[10] = 0x001F; // Red
    pltt[20] = 0x03E0; // Green

    uint32_t expectedRed = GX_RGB555ToRGBA(0x001F);
    uint32_t expectedGreen = GX_RGB555ToRGBA(0x03E0);

    // 8bpp tiles (64 bytes per tile):
    // Tile 0 at offset 0: filled with palette index 10
    std::memset(vram.data() + 0, 10, 64);
    // Tile 1 at offset 64: filled with palette index 20
    std::memset(vram.data() + 64, 20, 64);

    // Affine screen map at screenBase = 1 (offset 2048): 1 byte per tile
    vram[2048 + 0] = 0; // Tile 0
    vram[2048 + 1] = 1; // Tile 1

    NdsBgLayer bg{};
    bg.enabled = true;
    bg.mode = NDS_BGMODE_AFFINE;
    bg.screenBase = 1;
    bg.charBase = 0;
    bg.screenSize = 0; // 128x128 (16x16 tiles)
    bg.affineA = 0x0100; // 1.0 in fx8.8
    bg.affineB = 0;
    bg.affineC = 0;
    bg.affineD = 0x0100; // 1.0 in fx8.8
    bg.affineRefX = 0;
    bg.affineRefY = 0;
    bg.areaOver = false;

    uint32_t outLine[256];
    NdsBg_RenderAffine(&bg, 0, vram.data(), pltt, outLine);

    // Pixels 0..7 are Tile 0 (Red)
    for (int x = 0; x < 8; ++x) {
        ASSERT_EQ(expectedRed, outLine[x]);
    }
    // Pixels 8..15 are Tile 1 (Green)
    for (int x = 8; x < 16; ++x) {
        ASSERT_EQ(expectedGreen, outLine[x]);
    }
    // Pixels 128..255 are beyond dimension (128x128) with areaOver=false -> transparent
    for (int x = 128; x < 256; ++x) {
        ASSERT_EQ(NDS_PIXEL_TRANSPARENT, outLine[x]);
    }
}

TEST_CASE(BgRenderer_DisabledBgTransparent) {
    std::vector<uint8_t> vram(0x20000, 0xFF);
    uint16_t pltt[256];
    std::memset(pltt, 0x1F, sizeof(pltt));
    pltt[0] = 0x7C00; // Backdrop color: Blue

    NdsBgLayer bg{};
    bg.enabled = false;
    bg.mode = NDS_BGMODE_TEXT;

    uint32_t outLine[256];
    std::memset(outLine, 0xAA, sizeof(outLine));

    NdsBg_RenderText(&bg, 0, vram.data(), pltt, outLine);

    for (int x = 0; x < 256; ++x) {
        ASSERT_EQ(NDS_PIXEL_TRANSPARENT, outLine[x]);
    }

    // Compositor behavior when all planes disabled: outputs backdrop
    NdsDisplayEngine engine{};
    engine.dispcnt.dispMode = 1;
    engine.dispcnt.visiblePlanes = 0;

    uint32_t expectedBackdrop = GX_RGB555ToRGBA(0x7C00);
    NdsCompositor_RenderScanline(&engine, 0, vram.data(), nullptr, pltt);

    for (int x = 0; x < 256; ++x) {
        ASSERT_EQ(expectedBackdrop, engine.frameBuffer[0][x]);
    }

    // When display mode is 0 (off), outputs black
    engine.dispcnt.dispMode = 0;
    NdsCompositor_RenderScanline(&engine, 0, vram.data(), nullptr, pltt);

    for (int x = 0; x < 256; ++x) {
        ASSERT_EQ(0x00000000u, engine.frameBuffer[0][x]);
    }
}
