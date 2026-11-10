/**
 * @file nds_sprite_renderer.cpp
 * @brief NDS OAM sprite parser and per-scanline sprite rasterizer.
 */

#include "nds_sprite_renderer.h"
#include "port_gx.h"
#include "port_hw_registers.h"
#include <cstring>
#include <algorithm>

namespace {

// Shape & Size to Width x Height table (in pixels):
// Shape 0: Square (8x8, 16x16, 32x32, 64x64)
// Shape 1: Wide   (16x8, 32x8, 32x16, 64x32)
// Shape 2: Tall   (8x16, 8x32, 16x32, 32x64)
const int s_spriteDimensions[3][4][2] = {
    // Square
    {{8, 8}, {16, 16}, {32, 32}, {64, 64}},
    // Wide
    {{16, 8}, {32, 8}, {32, 16}, {64, 32}},
    // Tall
    {{8, 16}, {8, 32}, {16, 32}, {32, 64}}
};

} // anonymous namespace

extern "C" {

void NdsSprite_ParseOam(const uint8_t* rawOam,
                        NdsOamEntry outEntries[NDS_OAM_ENTRIES]) {
    if (!rawOam || !outEntries) return;

    for (int i = 0; i < NDS_OAM_ENTRIES; ++i) {
        const uint16_t* src = reinterpret_cast<const uint16_t*>(rawOam + i * 8);
        uint16_t attr0 = src[0];
        uint16_t attr1 = src[1];
        uint16_t attr2 = src[2];

        NdsOamEntry& entry = outEntries[i];
        entry.y = static_cast<uint8_t>(attr0 & 0xFF);
        entry.affineFlag = (attr0 & (1 << 8)) != 0;
        if (entry.affineFlag) {
            entry.enabled = true;
            entry.doubleSized = (attr0 & (1 << 9)) != 0;
        } else {
            // When !affineFlag, bit 9 is OBJ Disable (1 = disabled, 0 = enabled)
            entry.enabled = (attr0 & (1 << 9)) == 0;
            entry.doubleSized = false;
        }
        entry.objMode = static_cast<uint8_t>((attr0 >> 10) & 0x3);
        entry.mosaicEnable = (attr0 & (1 << 12)) != 0;
        entry.colorMode = static_cast<uint8_t>((attr0 >> 13) & 0x1);
        entry.shape = static_cast<uint8_t>((attr0 >> 14) & 0x3);

        uint16_t rawX = attr1 & 0x1FF;
        entry.x = (rawX >= 256) ? static_cast<int16_t>(rawX - 512) : static_cast<int16_t>(rawX);

        if (entry.affineFlag) {
            entry.affineIndex = static_cast<uint8_t>((attr1 >> 9) & 0x1F);
            entry.flipH = false;
            entry.flipV = false;
        } else {
            entry.affineIndex = 0;
            entry.flipH = (attr1 & (1 << 12)) != 0;
            entry.flipV = (attr1 & (1 << 13)) != 0;
        }
        entry.size = static_cast<uint8_t>((attr1 >> 14) & 0x3);

        entry.charName = attr2 & 0x3FF;
        entry.priority = static_cast<uint8_t>((attr2 >> 10) & 0x3);
        entry.paletteIndex = static_cast<uint8_t>((attr2 >> 12) & 0x0F);
    }
}

void NdsSprite_RenderScanline(const NdsOamEntry entries[NDS_OAM_ENTRIES],
                              int scanline,
                              const uint8_t* vram,
                              const uint16_t* pltt,
                              NdsSpritePixel outLine[256],
                              bool objMap1D) {
    if (!outLine) return;
    for (int x = 0; x < 256; ++x) {
        outLine[x].color = NDS_PIXEL_TRANSPARENT;
        outLine[x].priority = 4;
        outLine[x].semiTransparent = false;
    }
    if (!entries || !vram || !pltt || scanline < 0 || scanline >= 192) return;

    // Track winning priority per pixel
    uint8_t pixelPriority[256];
    std::memset(pixelPriority, 255, sizeof(pixelPriority));

    for (int i = 0; i < NDS_OAM_ENTRIES; ++i) {
        const NdsOamEntry& e = entries[i];
        if (!e.enabled || e.affineFlag || e.objMode == 2) {
            continue; // Ignore disabled, affine (for basic 2D), or OBJ window
        }
        if (e.shape >= 3) continue;

        int width = s_spriteDimensions[e.shape][e.size][0];
        int height = s_spriteDimensions[e.shape][e.size][1];

        // Y coordinate wrap: 8-bit Y >= 192 is placed at top of screen (y - 256)
        int effY = (e.y >= 192) ? (e.y - 256) : e.y;
        if (scanline < effY || scanline >= effY + height) {
            continue; // Does not overlap this scanline
        }

        int sprY = scanline - effY;
        int fetchY = e.flipV ? (height - 1 - sprY) : sprY;
        int tileRow = fetchY / 8;
        int tilePy = fetchY % 8;
        int tilesPerRow = width / 8;

        for (int c = 0; c < width; ++c) {
            int screenX = e.x + c;
            if (screenX < 0 || screenX >= 256) {
                continue; // Clip to screen boundaries
            }

            // Only overwrite if this sprite has strictly higher priority (lower priority number)
            // Or if pixel has not been written yet (pixelPriority == 255)
            // Lower OAM index wins if priorities are equal
            if (pixelPriority[screenX] <= e.priority) {
                continue;
            }

            int fetchX = e.flipH ? (width - 1 - c) : c;
            int tileCol = fetchX / 8;
            int tilePx = fetchX % 8;

            uint8_t palIdx = 0;

            if (e.colorMode == 0) {
                // 4bpp mode: 32 bytes per tile
                uint32_t tileIndex = 0;
                if (objMap1D) {
                    tileIndex = e.charName + tileRow * tilesPerRow + tileCol;
                } else {
                    int baseTileX = e.charName & 0x1F;
                    int baseTileY = (e.charName >> 5) & 0x1F;
                    int curTileX = (baseTileX + tileCol) & 0x1F;
                    int curTileY = (baseTileY + tileRow) & 0x1F;
                    tileIndex = curTileY * 32 + curTileX;
                }

                uint32_t byteOffset = tileIndex * 32 + tilePy * 4 + (tilePx >> 1);
                if (byteOffset < HW_LCDC_VRAM_SIZE) {
                    uint8_t byteVal = vram[byteOffset];
                    palIdx = (tilePx & 1) ? ((byteVal >> 4) & 0x0F) : (byteVal & 0x0F);
                }
            } else {
                // 8bpp mode: 64 bytes per tile
                uint32_t tileOffset = 0;
                if (objMap1D) {
                    tileOffset = (e.charName * 32) + (tileRow * tilesPerRow + tileCol) * 64;
                } else {
                    int baseTileX = e.charName & 0x1F;
                    int baseTileY = (e.charName >> 5) & 0x1F;
                    int curTileX = (baseTileX + tileCol) & 0x1F;
                    int curTileY = (baseTileY + tileRow) & 0x1F;
                    tileOffset = (curTileY * 32 + curTileX) * 64;
                }

                uint32_t byteOffset = tileOffset + tilePy * 8 + tilePx;
                if (byteOffset < HW_LCDC_VRAM_SIZE) {
                    palIdx = vram[byteOffset];
                }
            }

            // Palette index 0 is transparent
            if (palIdx == 0) {
                continue;
            }

            uint32_t fullPalIdx = (e.colorMode == 0) ? (e.paletteIndex * 16 + palIdx) : palIdx;
            uint16_t rgb555 = pltt[fullPalIdx & 0xFF];

            outLine[screenX].color = GX_RGB555ToRGBA(rgb555);
            outLine[screenX].priority = e.priority;
            outLine[screenX].semiTransparent = (e.objMode == 1);
            pixelPriority[screenX] = e.priority;
        }
    }
}

} // extern "C"
