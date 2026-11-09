/**
 * @file nds_compositor.h
 * @brief Per-scanline 2D priority compositor and post-process effects.
 */

#ifndef NDS_COMPOSITOR_H
#define NDS_COMPOSITOR_H

#include <stdint.h>
#include "nds_display_engine.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Render a single scanline into engine->frameBuffer[scanline][0..255].
 *
 * @param engine    Pointer to Main or Sub display engine state.
 * @param scanline  Current scanline index (0–191).
 * @param vram      Pointer to the VRAM region (gHwLcdcVram).
 * @param oam       Pointer to the OAM region (gHwOam or gHwDbOam).
 * @param pltt      Pointer to the palette region (gHwPltt or gHwDbPltt).
 */
void NdsCompositor_RenderScanline(NdsDisplayEngine* engine,
                                  int scanline,
                                  const uint8_t* vram,
                                  const uint8_t* oam,
                                  const uint16_t* pltt);

/**
 * Apply master brightness to the completed frameBuffer.
 * Must be called after all 192 scanlines are rendered.
 *
 * @param frameBuffer  Pointer to 256x192 RGBA8 pixel array.
 * @param level        -16 (black fade) … +16 (white fade). 0 = no change.
 */
void NdsCompositor_ApplyBrightness(uint32_t frameBuffer[192][256], int8_t level);

/**
 * Apply two-plane alpha blend to a single pixel.
 * Returns the blended RGBA8 color.
 *
 * @param colorA  Source (foreground) RGBA8 pixel.
 * @param colorB  Destination (background) RGBA8 pixel.
 * @param evA     Source weight 0–16.
 * @param evB     Destination weight 0–16.
 */
uint32_t NdsCompositor_AlphaBlend(uint32_t colorA, uint32_t colorB,
                                  uint8_t evA, uint8_t evB);

#ifdef __cplusplus
}
#endif

#endif /* NDS_COMPOSITOR_H */
