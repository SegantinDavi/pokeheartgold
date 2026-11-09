/**
 * @file port_gx.h
 * @brief 2D Graphics Engine (GX) and VRAM palette emulation interface.
 */

#ifndef PORT_GX_H
#define PORT_GX_H

#include <stdint.h>
#include <stdbool.h>
#include "port_common.h"
#include <nitro/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/* RGB555 to RGBA conversion helper */
static inline uint32_t GX_RGB555ToRGBA(uint16_t rgb555) {
    uint8_t r = (uint8_t)(((rgb555 >> 0) & 0x1F) * 255 / 31);
    uint8_t g = (uint8_t)(((rgb555 >> 5) & 0x1F) * 255 / 31);
    uint8_t b = (uint8_t)(((rgb555 >> 10) & 0x1F) * 255 / 31);
    uint8_t a = 0xFF;
    return ((uint32_t)a << 24) |
           ((uint32_t)b << 16) |
           ((uint32_t)g << 8)  |
           ((uint32_t)r << 0);
}

/* Initialization & Display Control */
#ifndef GX_DISP_SELECT_MAIN_SUB
#define GX_DISP_SELECT_MAIN_SUB 0
#endif
#ifndef GX_DISP_SELECT_SUB_MAIN
#define GX_DISP_SELECT_SUB_MAIN 1
#endif

void GX_Init(void);
void GX_DispOn(void);
void GX_DispOff(void);
void GXS_DispOn(void);
void GXS_DispOff(void);
void GX_SetDispSelect(int val);
int GX_GetDispSelect(void);

/* Palette Management */
void GX_LoadBGPltt(const void* src, u32 offset, u32 sz);
void GX_LoadOBJPltt(const void* src, u32 offset, u32 sz);
void GX_LoadBGExtPltt(const void* src, u32 offset, u32 sz);
void GX_LoadOBJExtPltt(const void* src, u32 offset, u32 sz);

void GXS_LoadBGPltt(const void* src, u32 offset, u32 sz);
void GXS_LoadOBJPltt(const void* src, u32 offset, u32 sz);

/* Tile & Screen Data Loading */
void GX_LoadOBJ(const void* src, u32 offset, u32 sz);
void GX_LoadBG0Char(const void* src, u32 offset, u32 sz);
void GX_LoadBG1Char(const void* src, u32 offset, u32 sz);
void GX_LoadBG2Char(const void* src, u32 offset, u32 sz);
void GX_LoadBG3Char(const void* src, u32 offset, u32 sz);

void GX_LoadBG0Scr(const void* src, u32 offset, u32 sz);
void GX_LoadBG1Scr(const void* src, u32 offset, u32 sz);
void GX_LoadBG2Scr(const void* src, u32 offset, u32 sz);
void GX_LoadBG3Scr(const void* src, u32 offset, u32 sz);

/* Frame Compositing & Pixel Buffer Access */
void GX_RenderFrame(void);
void GX_DrawTestPattern(int frameCounter, uint8_t touchX, uint8_t touchY, bool touching, const char* layoutName);
void GX_SetTestPatternEnabled(bool enabled);
bool GX_IsTestPatternEnabled(void);
void GX_RouteCompositeBuffers(const uint32_t* mainComposite, const uint32_t* subComposite);
void GX_RouteCompositeBuffers16(const uint16_t* mainComposite, const uint16_t* subComposite);
uint32_t* GX_GetTopScreenPixels(void);
uint32_t* GX_GetTouchScreenPixels(void);
void GX_ClearTopScreen(uint32_t color);
void GX_ClearTouchScreen(uint32_t color);

#ifdef __cplusplus
}
#endif

#endif // PORT_GX_H
