/**
 * @file test_sprite_renderer.cpp
 * @brief Unit tests for NDS OAM sprite parser and scanline rasterizer (Phase 4 / User Story 2).
 */

#include "test_runner.h"
#include "nds_sprite_renderer.h"
#include "nds_compositor.h"
#include "port_gx.h"
#include <vector>
#include <cstring>

TEST_CASE(SpriteRenderer_BasicTileAndPosition) {
    uint8_t rawOam[1024];
    std::memset(rawOam, 0, sizeof(rawOam));

    // Entry 0:
    // Attr0: Y=20, affineFlag=false, bit 9=0 (enabled), shape=0 (Square)
    // Attr1: X=10, size=0 (8x8), flipH=0, flipV=0
    // Attr2: charName=0, priority=0, paletteIndex=0
    uint16_t* oamWords = reinterpret_cast<uint16_t*>(rawOam);
    oamWords[0] = 20; // Attr0
    oamWords[1] = 10; // Attr1
    oamWords[2] = 0;  // Attr2

    // Disable all remaining 127 sprites by setting bit 9 in Attr0
    for (int i = 1; i < 128; ++i) {
        oamWords[i * 4 + 0] = (1 << 9);
    }

    std::vector<uint8_t> vram(0x4000, 0);
    // Tile 0: 4bpp, 32 bytes. Fill with palette index 1 (0x11)
    std::memset(vram.data(), 0x11, 32);

    uint16_t pltt[256];
    std::memset(pltt, 0, sizeof(pltt));
    pltt[1] = 0x001F; // Red (RGB555)
    uint32_t expectedRed = GX_RGB555ToRGBA(0x001F);

    NdsOamEntry entries[NDS_OAM_ENTRIES];
    NdsSprite_ParseOam(rawOam, entries);

    ASSERT_EQ(10, entries[0].x);
    ASSERT_EQ(20, entries[0].y);
    ASSERT_TRUE(entries[0].enabled);
    ASSERT_FALSE(entries[0].affineFlag);
    ASSERT_FALSE(entries[1].enabled);

    NdsSpritePixel outLine[256];
    NdsSprite_RenderScanline(entries, 20, vram.data(), pltt, outLine, true);

    // Columns 0..9: transparent
    for (int x = 0; x < 10; ++x) {
        ASSERT_EQ(NDS_PIXEL_TRANSPARENT, outLine[x].color);
    }
    // Columns 10..17: Red
    for (int x = 10; x < 18; ++x) {
        ASSERT_EQ(expectedRed, outLine[x].color);
        ASSERT_EQ(0, outLine[x].priority);
    }
    // Columns 18..255: transparent
    for (int x = 18; x < 256; ++x) {
        ASSERT_EQ(NDS_PIXEL_TRANSPARENT, outLine[x].color);
    }
}

TEST_CASE(SpriteRenderer_FlipH) {
    uint8_t rawOam[1024];
    std::memset(rawOam, 0, sizeof(rawOam));

    // Entry 0: Y=20, X=10, 8x8, flipH=true (bit 12 in Attr1)
    uint16_t* oamWords = reinterpret_cast<uint16_t*>(rawOam);
    oamWords[0] = 20;
    oamWords[1] = 10 | (1 << 12); // flipH
    oamWords[2] = 0;

    for (int i = 1; i < 128; ++i) {
        oamWords[i * 4 + 0] = (1 << 9);
    }

    std::vector<uint8_t> vram(0x4000, 0);
    // Asymmetric row 0: pixel 0 is color 1, pixels 1..7 are color 2
    // Nibbles: byte 0 = 0x21, byte 1 = 0x22, byte 2 = 0x22, byte 3 = 0x22
    vram[0] = 0x21;
    vram[1] = 0x22;
    vram[2] = 0x22;
    vram[3] = 0x22;

    uint16_t pltt[256];
    std::memset(pltt, 0, sizeof(pltt));
    pltt[1] = 0x001F; // Red
    pltt[2] = 0x03E0; // Green
    uint32_t expectedRed = GX_RGB555ToRGBA(0x001F);
    uint32_t expectedGreen = GX_RGB555ToRGBA(0x03E0);

    NdsOamEntry entries[NDS_OAM_ENTRIES];
    NdsSprite_ParseOam(rawOam, entries);

    NdsSpritePixel outLine[256];
    NdsSprite_RenderScanline(entries, 20, vram.data(), pltt, outLine, true);

    // Because flipH is true:
    // Pixel index 0 (originally at x=10) is mirrored to x=17!
    // Pixels at x=10..16 are Green (color 2), and x=17 is Red (color 1)!
    for (int x = 10; x < 17; ++x) {
        ASSERT_EQ(expectedGreen, outLine[x].color);
    }
    ASSERT_EQ(expectedRed, outLine[17].color);
}

TEST_CASE(SpriteRenderer_ClippedAtRightEdge) {
    uint8_t rawOam[1024];
    std::memset(rawOam, 0, sizeof(rawOam));

    // Entry 0: shape=1 (Wide: 16x8 for size=0), X=250, Y=20
    uint16_t* oamWords = reinterpret_cast<uint16_t*>(rawOam);
    oamWords[0] = 20 | (1 << 14); // Wide shape (1)
    oamWords[1] = 250;            // X = 250, size = 0 -> 16x8
    oamWords[2] = 0;

    for (int i = 1; i < 128; ++i) {
        oamWords[i * 4 + 0] = (1 << 9);
    }

    std::vector<uint8_t> vram(0x4000, 0x11); // Fill tiles with color 1
    uint16_t pltt[256];
    std::memset(pltt, 0, sizeof(pltt));
    pltt[1] = 0x7C00; // Blue
    uint32_t expectedBlue = GX_RGB555ToRGBA(0x7C00);

    NdsOamEntry entries[NDS_OAM_ENTRIES];
    NdsSprite_ParseOam(rawOam, entries);

    NdsSpritePixel outLine[256];
    NdsSprite_RenderScanline(entries, 20, vram.data(), pltt, outLine, true);

    // Columns 0..249: transparent
    for (int x = 0; x < 250; ++x) {
        ASSERT_EQ(NDS_PIXEL_TRANSPARENT, outLine[x].color);
    }
    // Columns 250..255 (6 pixels visible): Blue
    for (int x = 250; x < 256; ++x) {
        ASSERT_EQ(expectedBlue, outLine[x].color);
    }
}

TEST_CASE(SpriteRenderer_ColorZeroTransparent) {
    uint8_t rawOam[1024];
    std::memset(rawOam, 0, sizeof(rawOam));

    uint16_t* oamWords = reinterpret_cast<uint16_t*>(rawOam);
    oamWords[0] = 20;
    oamWords[1] = 10;
    oamWords[2] = 0;

    for (int i = 1; i < 128; ++i) {
        oamWords[i * 4 + 0] = (1 << 9);
    }

    std::vector<uint8_t> vram(0x4000, 0);
    // Row 0 has nibble 0 at pixel 0, nibble 1 at pixel 1
    vram[0] = 0x10; // pixel 0 = 0 (transparent), pixel 1 = 1

    uint16_t pltt[256];
    std::memset(pltt, 0, sizeof(pltt));
    pltt[0] = 0x7FFF; // Even if palette[0] has non-zero RGB, index 0 is transparent!
    pltt[1] = 0x001F;

    NdsOamEntry entries[NDS_OAM_ENTRIES];
    NdsSprite_ParseOam(rawOam, entries);

    NdsSpritePixel outLine[256];
    NdsSprite_RenderScanline(entries, 20, vram.data(), pltt, outLine, true);

    // Pixel at column 10 is color index 0 -> MUST BE 0x00000000 (transparent)
    ASSERT_EQ(NDS_PIXEL_TRANSPARENT, outLine[10].color);
    // Pixel at column 11 is color index 1 -> Red
    ASSERT_EQ(GX_RGB555ToRGBA(0x001F), outLine[11].color);
}

TEST_CASE(SpriteRenderer_PrioritySandwichingInCompositor) {
    // Test sandwiching:
    // BG0: priority 0, blue color
    // BG1: priority 2, red color
    // Sprite: priority 1, green color
    // At columns 0..9: BG0 is transparent, so Sprite (pri 1) renders over BG1 (pri 2)!
    // At columns 10..19: BG0 is visible (pri 0), so BG0 renders over Sprite (pri 1)!
    std::vector<uint8_t> vram(0x20000, 0);
    uint16_t pltt[512]; // 256 BG + 256 OBJ
    std::memset(pltt, 0, sizeof(pltt));

    // BG Palette
    pltt[1] = 0x001F; // Red (BG1)
    pltt[2] = 0x7C00; // Blue (BG0)
    // OBJ Palette (starts at +256)
    pltt[256 + 1] = 0x03E0; // Green (Sprite)

    uint32_t expectedRed = GX_RGB555ToRGBA(0x001F);
    uint32_t expectedBlue = GX_RGB555ToRGBA(0x7C00);
    uint32_t expectedGreen = GX_RGB555ToRGBA(0x03E0);

    // BG0 tile: tile 1 in charBase 0 has Blue
    std::memset(vram.data() + 32, 0x22, 32);
    // BG1 tile: tile 2 in charBase 0 has Red
    std::memset(vram.data() + 64, 0x11, 32);
    // OBJ tile: tile 3 in charBase 0 has Green
    std::memset(vram.data() + 96, 0x11, 32);

    // BG0 screen map at 2048:
    // Columns 0..7 (tile 0) = empty (transparent)
    // Columns 8..15 (tile 1) = tile 1 (Blue)
    uint16_t* bg0Map = reinterpret_cast<uint16_t*>(vram.data() + 2048);
    bg0Map[0] = 0; // tile 0 (transparent)
    bg0Map[1] = 1; // tile 1 (Blue)

    // BG1 screen map at 4096:
    // All tiles = tile 2 (Red)
    uint16_t* bg1Map = reinterpret_cast<uint16_t*>(vram.data() + 4096);
    bg1Map[0] = 2; // tile 2 (Red)
    bg1Map[1] = 2; // tile 2 (Red)

    // OAM: Sprite spanning columns 4..19 (16x16, size 1)
    uint8_t rawOam[1024];
    std::memset(rawOam, 0, sizeof(rawOam));
    uint16_t* oamWords = reinterpret_cast<uint16_t*>(rawOam);
    oamWords[0] = 0;                      // Y=0, shape=0
    oamWords[1] = 4 | (1 << 14);          // X=4, size=1 (16x16)
    oamWords[2] = 3 | (1 << 10);          // charName=3, priority=1

    for (int i = 1; i < 128; ++i) {
        oamWords[i * 4 + 0] = (1 << 9);
    }

    NdsDisplayEngine engine{};
    engine.dispcnt.dispMode = 1;
    engine.dispcnt.visiblePlanes = (1 << 0) | (1 << 1) | (1 << 4); // BG0, BG1, OBJ
    engine.dispcnt.objMapChar1D = true;

    // BG0: Priority 0
    engine.bg[0].enabled = true;
    engine.bg[0].mode = NDS_BGMODE_TEXT;
    engine.bg[0].colorMode = 0;
    engine.bg[0].priority = 0;
    engine.bg[0].screenBase = 1; // 2048
    engine.bg[0].charBase = 0;

    // BG1: Priority 2
    engine.bg[1].enabled = true;
    engine.bg[1].mode = NDS_BGMODE_TEXT;
    engine.bg[1].colorMode = 0;
    engine.bg[1].priority = 2;
    engine.bg[1].screenBase = 2; // 4096
    engine.bg[1].charBase = 0;

    NdsCompositor_RenderScanline(&engine, 0, vram.data(), rawOam, pltt);

    // Columns 0..3: Only BG1 is visible -> Red (BG1 priority 2)
    for (int x = 0; x < 4; ++x) {
        ASSERT_EQ(expectedRed, engine.frameBuffer[0][x]);
    }
    // Columns 4..7: Sprite (pri 1) is visible, BG0 is transparent -> Green (Sprite pri 1 beats BG1 pri 2)
    for (int x = 4; x < 8; ++x) {
        ASSERT_EQ(expectedGreen, engine.frameBuffer[0][x]);
    }
    // Columns 8..15: BG0 (pri 0) is visible -> Blue (BG0 pri 0 beats Sprite pri 1)
    for (int x = 8; x < 16; ++x) {
        ASSERT_EQ(expectedBlue, engine.frameBuffer[0][x]);
    }
}
