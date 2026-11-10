/**
 * @file nds_display_engine.h
 * @brief Core 2D display engine structures, BG layers, OAM entries, and blend control.
 */

#ifndef NDS_DISPLAY_ENGINE_H
#define NDS_DISPLAY_ENGINE_H

#include <stdint.h>
#include <stdbool.h>
#include "port_common.h"

#ifdef __cplusplus
extern "C" {
#endif

#define NDS_BGMODE_TEXT        0
#define NDS_BGMODE_AFFINE      1
#define NDS_BGMODE_AFFINE_EXT  2

/** Background Plane State (BG0-BG3) */
typedef struct {
    bool     enabled;           /**< Visible bit in DISPCNT */
    uint8_t  priority;          /**< 0 (front) – 3 (back) */
    uint8_t  mode;              /**< NDS_BGMODE_TEXT / AFFINE / AFFINE_EXT */
    uint8_t  colorMode;         /**< 0 = 4bpp (16-color), 1 = 8bpp (256-color) */
    uint8_t  screenBase;        /**< 2KB block index -> offset = screenBase * 2048 */
    uint8_t  charBase;          /**< 16KB block index -> offset = charBase * 16384 */
    uint8_t  screenSize;        /**< 0–3: 256x256 / 512x256 / 256x512 / 512x512 */
    uint8_t  mosaicEnable;
    bool     extPltt;           /**< Use extended palette bank */
    int16_t  scrollX;           /**< Pixel scroll offsets (wrapping) */
    int16_t  scrollY;
    // Affine-only fields (BG2/BG3)
    int32_t  affineRefX;        /**< fx20.8 reference point */
    int32_t  affineRefY;
    int16_t  affineA, affineB;  /**< 2x2 rotation-scale matrix (fx8.8) */
    int16_t  affineC, affineD;
    bool     areaOver;          /**< Wrap or clamp out-of-bounds */
} NdsBgLayer;

/** Object Attribute Memory (OAM) Sprite Entry */
typedef struct {
    bool     enabled;        /**< false if disabled via bit 9 (when !affineFlag) */
    // Attribute 0
    uint8_t  y;              /**< Y coordinate (wrapping at 192) */
    bool     affineFlag;     /**< Rotation/scale enabled */
    bool     doubleSized;    /**< Only if affineFlag=true */
    uint8_t  objMode;        /**< 0=normal, 1=alpha blend, 2=OBJ window, 3=bitmap */
    bool     mosaicEnable;
    uint8_t  colorMode;      /**< 0=4bpp, 1=8bpp */
    uint8_t  shape;          /**< 0=square, 1=wide, 2=tall */
    // Attribute 1
    int16_t  x;              /**< X coordinate (signed 9-bit, -256..511) */
    uint8_t  affineIndex;    /**< Index into OAM affine parameter set (if affineFlag) */
    bool     flipH;          /**< Horizontal flip (if !affineFlag) */
    bool     flipV;          /**< Vertical flip (if !affineFlag) */
    uint8_t  size;           /**< Combined with shape -> pixel dimensions */
    // Attribute 2
    uint16_t charName;       /**< Character base index */
    uint8_t  priority;       /**< 0–3 */
    uint8_t  paletteIndex;   /**< 4bpp: 0–15 palette slot; 8bpp: ignored */
} NdsOamEntry;

/** Color Lookup Tables */
typedef uint16_t NdsPaletteBank[256];
typedef uint16_t NdsExtPaletteBank[16][256];

/** DISPCNT Register Mirror */
typedef struct {
    uint8_t  bgMode;          /**< 0–6 */
    bool     bg0Is3D;         /**< BG0 feeds from 3D FBO */
    bool     objMapChar1D;    /**< OBJ character map layout */
    uint8_t  visiblePlanes;   /**< Bitmask: bits 8–12 of DISPCNT (BG0-BG3, OBJ) */
    uint8_t  visibleWindows;  /**< Bitmask: bits 13–15 (WIN0, WIN1, OBJ WIN) */
    uint8_t  dispMode;        /**< 0=off, 1=graphics, 2=VRAM-A, 3=memory display */
    bool     bgExtPltt;
    bool     objExtPltt;
    bool     hBlankObjProc;
} NdsDisplayControl;

/** Alpha Blend & Brightness State */
typedef struct {
    uint8_t  blendMode;        /**< 0=none, 1=alpha, 2=brighten, 3=darken */
    uint8_t  srcMask;          /**< BLDCNT first target plane bitmask */
    uint8_t  dstMask;          /**< BLDCNT second target plane bitmask */
    uint8_t  evA;              /**< 0–16, source weight */
    uint8_t  evB;              /**< 0–16, destination weight */
    int8_t   masterBrightness; /**< -16 (black) … +16 (white) */
} NdsBlendControl;

/** Per-Engine Aggregate State */
typedef struct {
    NdsBgLayer        bg[4];
    NdsBlendControl   blend;
    NdsDisplayControl dispcnt;
    uint32_t          frameBuffer[NDS_SCREEN_HEIGHT][NDS_SCREEN_WIDTH]; /**< RGBA8 */
    uint32_t*         bg0_3d_buffer; /**< 256x192 RGBA8, filled by G3_SwapBuffers (Main only) */
    uint32_t          objVramOffset; /**< Byte offset into LCDC VRAM for OBJ character tiles */
} NdsDisplayEngine;

#ifdef __cplusplus
}
#endif

#endif // NDS_DISPLAY_ENGINE_H
