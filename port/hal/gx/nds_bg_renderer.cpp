/**
 * @file nds_bg_renderer.cpp
 * @brief NDS Background layer tile and affine rasterizer.
 */

#include "nds_bg_renderer.h"
#include "port_gx.h"
#include <cstring>
#include <algorithm>

extern "C" {

uint8_t NdsBg_DecodeTilePixel(const uint8_t* charData,
                              int tx, int ty,
                              int colorMode,
                              bool flipH, bool flipV) {
    if (!charData) return 0;

    int effX = flipH ? (7 - tx) : tx;
    int effY = flipV ? (7 - ty) : ty;

    if (colorMode == 0) {
        // 4bpp mode: 32 bytes per 8x8 tile, 4 bytes per row (2 pixels per byte)
        int byteOffset = effY * 4 + (effX >> 1);
        uint8_t byteVal = charData[byteOffset];
        if ((effX & 1) == 0) {
            return byteVal & 0x0F;
        } else {
            return (byteVal >> 4) & 0x0F;
        }
    } else {
        // 8bpp mode: 64 bytes per 8x8 tile, 8 bytes per row (1 byte per pixel)
        int byteOffset = effY * 8 + effX;
        return charData[byteOffset];
    }
}

void NdsBg_RenderText(const NdsBgLayer* bg,
                      int scanline,
                      const uint8_t* vram,
                      const uint16_t* pltt,
                      uint32_t outLine[256]) {
    if (!outLine) return;
    if (!bg || !bg->enabled || !vram || !pltt) {
        std::memset(outLine, 0, 256 * sizeof(uint32_t));
        return;
    }

    int totalWidthTiles = (bg->screenSize == 1 || bg->screenSize == 3) ? 64 : 32;
    int totalHeightTiles = (bg->screenSize == 2 || bg->screenSize == 3) ? 64 : 32;

    int totalHeightPx = totalHeightTiles * 8;
    int totalWidthPx = totalWidthTiles * 8;

    int effY = (scanline + bg->scrollY) % totalHeightPx;
    if (effY < 0) effY += totalHeightPx;

    int tileY = effY / 8;
    int ty = effY % 8;

    int tileSize = (bg->colorMode == 0) ? 32 : 64;
    uint32_t charBaseOffset = static_cast<uint32_t>(bg->charBase) * 16384;

    for (int x = 0; x < 256; ++x) {
        int effX = (x + bg->scrollX) % totalWidthPx;
        if (effX < 0) effX += totalWidthPx;

        int tileX = effX / 8;
        int tx = effX % 8;

        int blockIdx = 0;
        int curTileX = tileX;
        int curTileY = tileY;

        if (bg->screenSize == 1) { // 512x256
            if (curTileX >= 32) {
                blockIdx = 1;
                curTileX -= 32;
            }
        } else if (bg->screenSize == 2) { // 256x512
            if (curTileY >= 32) {
                blockIdx = 1;
                curTileY -= 32;
            }
        } else if (bg->screenSize == 3) { // 512x512
            int bx = (curTileX >= 32) ? 1 : 0;
            int by = (curTileY >= 32) ? 1 : 0;
            blockIdx = by * 2 + bx;
            curTileX %= 32;
            curTileY %= 32;
        }

        uint32_t screenBaseOffset = (static_cast<uint32_t>(bg->screenBase) + blockIdx) * 2048;
        uint32_t entryOffset = screenBaseOffset + (curTileY * 32 + curTileX) * 2;

        uint16_t screenEntry = *reinterpret_cast<const uint16_t*>(vram + entryOffset);

        uint16_t tileNum = screenEntry & 0x03FF;
        bool hFlip = (screenEntry & (1 << 10)) != 0;
        bool vFlip = (screenEntry & (1 << 11)) != 0;
        uint8_t palNum = static_cast<uint8_t>((screenEntry >> 12) & 0x0F);

        uint32_t tileDataOffset = charBaseOffset + tileNum * tileSize;
        uint8_t palIdx = NdsBg_DecodeTilePixel(vram + tileDataOffset, tx, ty, bg->colorMode, hFlip, vFlip);

        if (palIdx == 0) {
            outLine[x] = NDS_PIXEL_TRANSPARENT;
        } else {
            uint32_t fullPalIdx = (bg->colorMode == 0) ? (palNum * 16 + palIdx) : palIdx;
            uint16_t rgb555 = pltt[fullPalIdx & 0xFF];
            outLine[x] = GX_RGB555ToRGBA(rgb555);
        }
    }
}

void NdsBg_RenderAffine(const NdsBgLayer* bg,
                        int scanline,
                        const uint8_t* vram,
                        const uint16_t* pltt,
                        uint32_t outLine[256]) {
    if (!outLine) return;
    if (!bg || !bg->enabled || !vram || !pltt) {
        std::memset(outLine, 0, 256 * sizeof(uint32_t));
        return;
    }

    // Dimension: 128 << screenSize
    int dim = 128 << (bg->screenSize & 3);
    int tilesPerRow = dim / 8;

    int32_t startX = bg->affineRefX + scanline * bg->affineB;
    int32_t startY = bg->affineRefY + scanline * bg->affineD;

    uint32_t screenBaseOffset = static_cast<uint32_t>(bg->screenBase) * 2048;
    uint32_t charBaseOffset = static_cast<uint32_t>(bg->charBase) * 16384;

    for (int x = 0; x < 256; ++x) {
        int32_t curX = startX + x * bg->affineA;
        int32_t curY = startY + x * bg->affineC;

        int px = curX >> 8;
        int py = curY >> 8;

        if (px < 0 || px >= dim || py < 0 || py >= dim) {
            if (bg->areaOver) {
                px = (px % dim + dim) % dim;
                py = (py % dim + dim) % dim;
            } else {
                outLine[x] = NDS_PIXEL_TRANSPARENT;
                continue;
            }
        }

        int tileX = px / 8;
        int tileY = py / 8;
        int tx = px % 8;
        int ty = py % 8;

        uint8_t tileNum = vram[screenBaseOffset + tileY * tilesPerRow + tileX];
        uint32_t charOffset = charBaseOffset + static_cast<uint32_t>(tileNum) * 64 + ty * 8 + tx;
        uint8_t palIdx = vram[charOffset];

        if (palIdx == 0) {
            outLine[x] = NDS_PIXEL_TRANSPARENT;
        } else {
            uint16_t rgb555 = pltt[palIdx];
            outLine[x] = GX_RGB555ToRGBA(rgb555);
        }
    }
}

} // extern "C"
