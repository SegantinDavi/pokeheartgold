/**
 * @file nds_compositor.cpp
 * @brief Per-scanline 2D priority compositor, alpha blending, and brightness adjustment.
 */

#include "nds_compositor.h"
#include "nds_bg_renderer.h"
#include "nds_sprite_renderer.h"
#include "port_gx.h"
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

    // Fast-path: display disabled
    if (engine->dispcnt.dispMode == 0) {
        std::memset(engine->frameBuffer[scanline], 0, sizeof(engine->frameBuffer[scanline]));
        return;
    }

    // Default backdrop color is BG palette index 0
    uint32_t backdrop = pltt ? GX_RGB555ToRGBA(pltt[0]) : 0xFF000000u;

    // Fast-path: all planes disabled
    if (engine->dispcnt.visiblePlanes == 0) {
        for (int x = 0; x < 256; ++x) {
            engine->frameBuffer[scanline][x] = backdrop;
        }
        return;
    }

    // Render enabled BG planes for this scanline
    uint32_t bgLines[4][256];
    bool bgActive[4] = {false, false, false, false};

    for (int i = 0; i < 4; ++i) {
        if ((engine->dispcnt.visiblePlanes & (1 << i)) && engine->bg[i].enabled) {
            bgActive[i] = true;
            if (i == 0 && engine->dispcnt.bg0Is3D && engine->bg0_3d_buffer) {
                // BG0 fed from 3D FBO readback
                const uint32_t* src3D = &engine->bg0_3d_buffer[scanline * 256];
                std::memcpy(bgLines[i], src3D, 256 * sizeof(uint32_t));
            } else if (engine->bg[i].mode == NDS_BGMODE_TEXT) {
                NdsBg_RenderText(&engine->bg[i], scanline, vram, pltt, bgLines[i]);
            } else {
                NdsBg_RenderAffine(&engine->bg[i], scanline, vram, pltt, bgLines[i]);
            }
        } else {
            std::memset(bgLines[i], 0, sizeof(bgLines[i]));
        }
    }

    // Render OAM sprites if enabled (plane 4 in visiblePlanes)
    NdsSpritePixel spriteLine[256];
    bool objActive = false;
    const uint16_t* objPltt = pltt ? (pltt + 256) : nullptr;
    if (oam && (engine->dispcnt.visiblePlanes & (1 << 4))) {
        NdsOamEntry oamEntries[NDS_OAM_ENTRIES];
        NdsSprite_ParseOam(oam, oamEntries);
        NdsSprite_RenderScanline(oamEntries, scanline, vram, objPltt, spriteLine, engine->dispcnt.objMapChar1D);
        objActive = true;
    }

    // Priority sort order: 4 priority levels (0 front, 3 back)
    // Within the same priority level: OBJ > BG0 > BG1 > BG2 > BG3
    struct LayerCandidate {
        uint32_t color;
        int      plane;
        int      priority;
        bool     semiTransparent;
        int      rank;
    };

    for (int x = 0; x < 256; ++x) {
        LayerCandidate candidates[5];
        int numCandidates = 0;

        for (int p = 0; p < 4; ++p) {
            if (bgActive[p] && bgLines[p][x] != NDS_PIXEL_TRANSPARENT) {
                candidates[numCandidates].color = bgLines[p][x];
                candidates[numCandidates].plane = p;
                candidates[numCandidates].priority = engine->bg[p].priority;
                candidates[numCandidates].semiTransparent = false;
                candidates[numCandidates].rank = engine->bg[p].priority * 10 + 1 + p;
                numCandidates++;
            }
        }

        if (objActive && spriteLine[x].color != NDS_PIXEL_TRANSPARENT) {
            candidates[numCandidates].color = spriteLine[x].color;
            candidates[numCandidates].plane = 4;
            candidates[numCandidates].priority = spriteLine[x].priority;
            candidates[numCandidates].semiTransparent = spriteLine[x].semiTransparent;
            candidates[numCandidates].rank = spriteLine[x].priority * 10 + 0;
            numCandidates++;
        }

        // Sort candidates by ascending rank (lower rank = higher priority, front)
        std::sort(candidates, candidates + numCandidates, [](const LayerCandidate& a, const LayerCandidate& b) {
            return a.rank < b.rank;
        });

        if (numCandidates == 0) {
            engine->frameBuffer[scanline][x] = backdrop;
        } else {
            uint32_t finalColor = candidates[0].color;
            int topPlane = candidates[0].plane;

            bool isAlphaMode = (engine->blend.blendMode == 1 && (engine->blend.srcMask & (1 << topPlane)));
            bool isObjSemiTransparent = (topPlane == 4 && candidates[0].semiTransparent);

            // Check alpha blending
            if (isAlphaMode || isObjSemiTransparent) {
                // Search for 2nd target plane in dstMask
                uint32_t dstColor = 0;
                bool hasDst = false;

                for (int c = 1; c < numCandidates; ++c) {
                    if (engine->blend.dstMask & (1 << candidates[c].plane)) {
                        dstColor = candidates[c].color;
                        hasDst = true;
                        break;
                    }
                }

                // If no lower plane matched dstMask, check backdrop (bit 5 in BLDCNT)
                if (!hasDst && (engine->blend.dstMask & (1 << 5))) {
                    dstColor = backdrop;
                    hasDst = true;
                }

                if (hasDst) {
                    finalColor = NdsCompositor_AlphaBlend(finalColor, dstColor, engine->blend.evA, engine->blend.evB);
                }
            }

            engine->frameBuffer[scanline][x] = finalColor;
        }
    }
}

} // extern "C"
