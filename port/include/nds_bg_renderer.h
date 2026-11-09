/**
 * @file nds_bg_renderer.h
 * @brief NDS Background layer tile and affine rasterizer.
 */

#ifndef NDS_BG_RENDERER_H
#define NDS_BG_RENDERER_H

#include <stdint.h>
#include <stdbool.h>
#include "nds_display_engine.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Sentinel value for a transparent pixel (palette index 0). */
#define NDS_PIXEL_TRANSPARENT 0x00000000u  /* RGBA8 transparent */

/**
 * Render one scanline of a text-mode background layer.
 *
 * @param bg        Background layer configuration.
 * @param scanline  Current scanline (0–191).
 * @param vram      VRAM base pointer.
 * @param pltt      Palette base pointer (512 bytes = 256 RGB555 entries for BG).
 * @param outLine   Output: 256 RGBA8 pixels. Transparent pixels = 0x00000000.
 */
void NdsBg_RenderText(const NdsBgLayer* bg,
                      int scanline,
                      const uint8_t* vram,
                      const uint16_t* pltt,
                      uint32_t outLine[256]);

/**
 * Render one scanline of an affine (rotation/scale) background layer.
 * Applies 2x2 rotation-scale matrix and reference point.
 */
void NdsBg_RenderAffine(const NdsBgLayer* bg,
                        int scanline,
                        const uint8_t* vram,
                        const uint16_t* pltt,
                        uint32_t outLine[256]);

/**
 * Decode a single 8x8 character tile pixel.
 *
 * @param charData   Pointer to the start of the tile's character data in VRAM.
 * @param tx         X within tile (0–7).
 * @param ty         Y within tile (0–7).
 * @param colorMode  0 = 4bpp, 1 = 8bpp.
 * @param flipH      Horizontal flip.
 * @param flipV      Vertical flip.
 * @return Palette index (0 = transparent).
 */
uint8_t NdsBg_DecodeTilePixel(const uint8_t* charData,
                              int tx, int ty,
                              int colorMode,
                              bool flipH, bool flipV);

#ifdef __cplusplus
}
#endif

#endif /* NDS_BG_RENDERER_H */
