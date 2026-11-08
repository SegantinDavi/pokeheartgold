/**
 * @file gx_native.cpp
 * @brief 2D Graphics Engine (GX/G2) emulation and palette management for native PC port.
 */

#include "port_gx.h"
#include "display/display_manager.h"
#include <cstring>
#include <algorithm>

namespace {

constexpr size_t SCREEN_PIXELS = NDS_SCREEN_WIDTH * NDS_SCREEN_HEIGHT;

uint32_t gTopScreenPixels[SCREEN_PIXELS];
uint32_t gTouchScreenPixels[SCREEN_PIXELS];

uint16_t gMainBGPltt[256];
uint16_t gMainOBJPltt[256];
uint16_t gSubBGPltt[256];
uint16_t gSubOBJPltt[256];

bool gMainDispOn = true;
bool gSubDispOn = true;

// VRAM banks (512 KB virtual VRAM allocation)
uint8_t gVram[512 * 1024];

} // anonymous namespace

extern "C" {

void GX_Init(void) {
    memset(gTopScreenPixels, 0, sizeof(gTopScreenPixels));
    memset(gTouchScreenPixels, 0, sizeof(gTouchScreenPixels));
    memset(gMainBGPltt, 0, sizeof(gMainBGPltt));
    memset(gMainOBJPltt, 0, sizeof(gMainOBJPltt));
    memset(gSubBGPltt, 0, sizeof(gSubBGPltt));
    memset(gSubOBJPltt, 0, sizeof(gSubOBJPltt));
    memset(gVram, 0, sizeof(gVram));
    gMainDispOn = true;
    gSubDispOn = true;
}

void GX_DispOn(void) {
    gMainDispOn = true;
}

void GX_DispOff(void) {
    gMainDispOn = false;
}

void GXS_DispOn(void) {
    gSubDispOn = true;
}

void GXS_DispOff(void) {
    gSubDispOn = false;
}

void GX_LoadBGPltt(const void* src, u32 offset, u32 sz) {
    if (!src || offset >= sizeof(gMainBGPltt)) return;
    size_t copySize = std::min(static_cast<size_t>(sz), sizeof(gMainBGPltt) - offset);
    memcpy(reinterpret_cast<uint8_t*>(gMainBGPltt) + offset, src, copySize);
}

void GX_LoadOBJPltt(const void* src, u32 offset, u32 sz) {
    if (!src || offset >= sizeof(gMainOBJPltt)) return;
    size_t copySize = std::min(static_cast<size_t>(sz), sizeof(gMainOBJPltt) - offset);
    memcpy(reinterpret_cast<uint8_t*>(gMainOBJPltt) + offset, src, copySize);
}

void GX_LoadBGExtPltt(const void* src, u32 offset, u32 sz) {
    GX_LoadBGPltt(src, offset, sz);
}

void GX_LoadOBJExtPltt(const void* src, u32 offset, u32 sz) {
    GX_LoadOBJPltt(src, offset, sz);
}

void GXS_LoadBGPltt(const void* src, u32 offset, u32 sz) {
    if (!src || offset >= sizeof(gSubBGPltt)) return;
    size_t copySize = std::min(static_cast<size_t>(sz), sizeof(gSubBGPltt) - offset);
    memcpy(reinterpret_cast<uint8_t*>(gSubBGPltt) + offset, src, copySize);
}

void GXS_LoadOBJPltt(const void* src, u32 offset, u32 sz) {
    if (!src || offset >= sizeof(gSubOBJPltt)) return;
    size_t copySize = std::min(static_cast<size_t>(sz), sizeof(gSubOBJPltt) - offset);
    memcpy(reinterpret_cast<uint8_t*>(gSubOBJPltt) + offset, src, copySize);
}

void GX_LoadOBJ(const void* src, u32 offset, u32 sz) {
    if (!src || offset >= sizeof(gVram)) return;
    size_t copySize = std::min(static_cast<size_t>(sz), sizeof(gVram) - offset);
    memcpy(gVram + offset, src, copySize);
}

void GX_LoadBG0Char(const void* src, u32 offset, u32 sz) {
    GX_LoadOBJ(src, offset, sz);
}

void GX_LoadBG1Char(const void* src, u32 offset, u32 sz) {
    GX_LoadOBJ(src, offset, sz);
}

void GX_LoadBG2Char(const void* src, u32 offset, u32 sz) {
    GX_LoadOBJ(src, offset, sz);
}

void GX_LoadBG3Char(const void* src, u32 offset, u32 sz) {
    GX_LoadOBJ(src, offset, sz);
}

void GX_LoadBG0Scr(const void* src, u32 offset, u32 sz) {
    GX_LoadOBJ(src, offset, sz);
}

void GX_LoadBG1Scr(const void* src, u32 offset, u32 sz) {
    GX_LoadOBJ(src, offset, sz);
}

void GX_LoadBG2Scr(const void* src, u32 offset, u32 sz) {
    GX_LoadOBJ(src, offset, sz);
}

void GX_LoadBG3Scr(const void* src, u32 offset, u32 sz) {
    GX_LoadOBJ(src, offset, sz);
}

uint32_t* GX_GetTopScreenPixels(void) {
    return gTopScreenPixels;
}

uint32_t* GX_GetTouchScreenPixels(void) {
    return gTouchScreenPixels;
}

void GX_ClearTopScreen(uint32_t color) {
    std::fill(gTopScreenPixels, gTopScreenPixels + SCREEN_PIXELS, color);
}

void GX_ClearTouchScreen(uint32_t color) {
    std::fill(gTouchScreenPixels, gTouchScreenPixels + SCREEN_PIXELS, color);
}

void GX_RenderFrame(void) {
    const void* top = gMainDispOn ? gTopScreenPixels : nullptr;
    const void* touch = gSubDispOn ? gTouchScreenPixels : nullptr;
    DisplayManager_PresentFrame(top, touch);
}

} // extern "C"
