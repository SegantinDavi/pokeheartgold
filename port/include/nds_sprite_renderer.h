/**
 * @file nds_sprite_renderer.h
 * @brief NDS OAM sprite parser and per-scanline sprite rasterizer.
 */

#ifndef NDS_SPRITE_RENDERER_H
#define NDS_SPRITE_RENDERER_H

#include <stdint.h>
#include <stdbool.h>
#include "nds_display_engine.h"

#ifdef __cplusplus
extern "C" {
#endif

#define NDS_OAM_ENTRIES 128

/** Sprite line buffer slot: RGBA pixel + priority. */
typedef struct {
    uint32_t color;     /**< RGBA8 pixel; 0x00000000 = transparent. */
    uint8_t  priority;  /**< Hardware priority 0–3. */
} NdsSpritePixel;

/**
 * Parse raw OAM memory into an array of NdsOamEntry structs.
 *
 * @param rawOam      Pointer to 1024-byte OAM region.
 * @param outEntries  Output array of NDS_OAM_ENTRIES entries.
 */
void NdsSprite_ParseOam(const uint8_t* rawOam,
                        NdsOamEntry outEntries[NDS_OAM_ENTRIES]);

/**
 * Render all active OAM sprites for one scanline.
 *
 * @param entries   Pre-parsed OAM entry array.
 * @param scanline  Current scanline (0–191).
 * @param vram      VRAM base pointer (OBJ character data).
 * @param pltt      OBJ palette base pointer (512 bytes).
 * @param outLine   Output: 256 sprite pixels with priority annotations.
 * @param objMap1D  true = 1D character map, false = 2D character map.
 */
void NdsSprite_RenderScanline(const NdsOamEntry entries[NDS_OAM_ENTRIES],
                              int scanline,
                              const uint8_t* vram,
                              const uint16_t* pltt,
                              NdsSpritePixel outLine[256],
                              bool objMap1D);

#ifdef __cplusplus
}
#endif

#endif /* NDS_SPRITE_RENDERER_H */
