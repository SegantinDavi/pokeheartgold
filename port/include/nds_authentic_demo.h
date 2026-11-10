/**
 * @file nds_authentic_demo.h
 * @brief Authentic Pokémon HeartGold Title Screen and graphical asset preview demo.
 */

#ifndef NDS_AUTHENTIC_DEMO_H
#define NDS_AUTHENTIC_DEMO_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize authentic Pokémon HeartGold assets into VRAM, OAM, and Palettes.
 * Loads Title Screen tilemaps, palettes, and sprites from files/demo/title/titledemo/.
 */
bool NdsAuthenticDemo_Init(void);

/**
 * Render one frame of the authentic Pokémon HeartGold 2D/3D visual demo.
 * Renders 3D geometry pass into BG0 and updates interactive touchscreen sprites.
 *
 * @param frameCount  Current frame counter.
 * @param touchX      Touchscreen X coordinate (0–255).
 * @param touchY      Touchscreen Y coordinate (0–191).
 * @param touching    True if stylus/mouse is held down.
 */
void NdsAuthenticDemo_RenderFrame(int frameCount, int touchX, int touchY, bool touching);

#ifdef __cplusplus
}
#endif

#endif // NDS_AUTHENTIC_DEMO_H
