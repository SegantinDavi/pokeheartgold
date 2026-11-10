/**
 * @file gx_native.cpp
 * @brief 2D Graphics Engine (GX/G2) emulation and palette management for native PC port.
 */

#include "port_gx.h"
#include "port_hw_registers.h"
#include "gx_pipeline.h"
#include "display/display_manager.h"
#include "display/test_font.h"
#include <cstring>
#include <cstdio>
#include <algorithm>

namespace {

constexpr size_t SCREEN_PIXELS = NDS_SCREEN_WIDTH * NDS_SCREEN_HEIGHT;

uint32_t gTopScreenPixels[SCREEN_PIXELS];
uint32_t gTouchScreenPixels[SCREEN_PIXELS];

bool gMainDispOn = true;
bool gSubDispOn = true;
int s_dispSelect = GX_DISP_SELECT_MAIN_SUB;
bool s_testPatternEnabled = false;

} // anonymous namespace

extern "C" {

void GX_Init(void) {
    memset(gTopScreenPixels, 0, sizeof(gTopScreenPixels));
    memset(gTouchScreenPixels, 0, sizeof(gTouchScreenPixels));
    gMainDispOn = true;
    gSubDispOn = true;
    s_dispSelect = GX_DISP_SELECT_MAIN_SUB;
    s_testPatternEnabled = false;
    NdsGfx_Init();
}

void GX_SetDispSelect(int val) {
    s_dispSelect = val;
}

int GX_GetDispSelect(void) {
    return s_dispSelect;
}

void GX_SetTestPatternEnabled(bool enabled) {
    s_testPatternEnabled = enabled;
}

bool GX_IsTestPatternEnabled(void) {
    return s_testPatternEnabled;
}

void GX_RouteCompositeBuffers(const uint32_t* mainComposite, const uint32_t* subComposite) {
    if (s_dispSelect == GX_DISP_SELECT_MAIN_SUB) {
        if (mainComposite) {
            std::memcpy(gTopScreenPixels, mainComposite, sizeof(gTopScreenPixels));
        }
        if (subComposite) {
            std::memcpy(gTouchScreenPixels, subComposite, sizeof(gTouchScreenPixels));
        }
    } else {
        if (subComposite) {
            std::memcpy(gTopScreenPixels, subComposite, sizeof(gTopScreenPixels));
        }
        if (mainComposite) {
            std::memcpy(gTouchScreenPixels, mainComposite, sizeof(gTouchScreenPixels));
        }
    }
}

void GX_RouteCompositeBuffers16(const uint16_t* mainComposite, const uint16_t* subComposite) {
    if (s_dispSelect == GX_DISP_SELECT_MAIN_SUB) {
        if (mainComposite) {
            for (size_t i = 0; i < SCREEN_PIXELS; ++i) {
                gTopScreenPixels[i] = GX_RGB555ToRGBA(mainComposite[i]);
            }
        }
        if (subComposite) {
            for (size_t i = 0; i < SCREEN_PIXELS; ++i) {
                gTouchScreenPixels[i] = GX_RGB555ToRGBA(subComposite[i]);
            }
        }
    } else {
        if (subComposite) {
            for (size_t i = 0; i < SCREEN_PIXELS; ++i) {
                gTopScreenPixels[i] = GX_RGB555ToRGBA(subComposite[i]);
            }
        }
        if (mainComposite) {
            for (size_t i = 0; i < SCREEN_PIXELS; ++i) {
                gTouchScreenPixels[i] = GX_RGB555ToRGBA(mainComposite[i]);
            }
        }
    }
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
    if (!src || offset >= 512) return;
    size_t copySize = std::min(static_cast<size_t>(sz), static_cast<size_t>(512 - offset));
    memcpy(gHwPltt + offset, src, copySize);
}

void GX_LoadOBJPltt(const void* src, u32 offset, u32 sz) {
    if (!src || offset >= 512) return;
    size_t copySize = std::min(static_cast<size_t>(sz), static_cast<size_t>(512 - offset));
    memcpy(gHwPltt + 512 + offset, src, copySize);
}

void GX_LoadBGExtPltt(const void* src, u32 offset, u32 sz) {
    GX_LoadBGPltt(src, offset, sz);
}

void GX_LoadOBJExtPltt(const void* src, u32 offset, u32 sz) {
    GX_LoadOBJPltt(src, offset, sz);
}

void GXS_LoadBGPltt(const void* src, u32 offset, u32 sz) {
    if (!src || offset >= 512) return;
    size_t copySize = std::min(static_cast<size_t>(sz), static_cast<size_t>(512 - offset));
    memcpy(gHwDbPltt + offset, src, copySize);
}

void GXS_LoadOBJPltt(const void* src, u32 offset, u32 sz) {
    if (!src || offset >= 512) return;
    size_t copySize = std::min(static_cast<size_t>(sz), static_cast<size_t>(512 - offset));
    memcpy(gHwDbPltt + 512 + offset, src, copySize);
}

void GX_LoadOBJ(const void* src, u32 offset, u32 sz) {
    if (!src || offset >= HW_LCDC_VRAM_SIZE) return;
    size_t copySize = std::min(static_cast<size_t>(sz), static_cast<size_t>(HW_LCDC_VRAM_SIZE - offset));
    memcpy(gHwLcdcVram + offset, src, copySize);
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

void GX_DrawTestPattern(int frameCounter, uint8_t touchX, uint8_t touchY, bool touching, const char* layoutName) {
    using namespace PortDisplay;
    const int W = NDS_SCREEN_WIDTH;
    const int H = NDS_SCREEN_HEIGHT;

    // --- TOP SCREEN: Main Display Presentation ---
    for (int y = 0; y < H; ++y) {
        uint8_t r = static_cast<uint8_t>(20 + (y * 25) / H);
        uint8_t g = static_cast<uint8_t>(28 + (y * 35) / H);
        uint8_t b = static_cast<uint8_t>(50 + (y * 55) / H);
        uint32_t bgCol = (0xFF << 24) | (b << 16) | (g << 8) | r;
        for (int x = 0; x < W; ++x) {
            gTopScreenPixels[y * W + x] = bgCol;
        }
    }

    uint32_t gold = 0xFF37AFD4;     // R=212, G=175, B=55 (HeartGold Gold)
    uint32_t darkNavy = 0xFF281810; // R=16, G=24, B=40
    uint32_t white = 0xFFFFFFFF;
    uint32_t amber = 0xFF1EC8FF;    // R=255, G=200, B=30
    uint32_t lightCyan = 0xFFE0F7FA;
    uint32_t cyan = 0xFFF7C34F;     // R=79, G=195, B=247
    uint32_t faintCyan = 0xFF403025;

    // Top screen border
    DrawRect(gTopScreenPixels, W, H, 2, 2, W - 4, H - 4, gold);

    // Header banner
    FillRect(gTopScreenPixels, W, H, 6, 6, W - 12, 24, gold);
    DrawString(gTopScreenPixels, W, H, 14, 11, "POKEMON HEARTGOLD", darkNavy, 2);

    // Subtitle
    DrawString(gTopScreenPixels, W, H, 10, 36, "NATIVE PC RECOMPILATION (64-BIT)", amber, 1);

    // Info card
    DrawRect(gTopScreenPixels, W, H, 10, 52, W - 20, 106, 0xFF604535);
    FillRect(gTopScreenPixels, W, H, 11, 53, W - 22, 104, 0xFF302015);

    DrawString(gTopScreenPixels, W, H, 18, 60, "ENGINE: STANDALONE PC PORT", white, 1);
    DrawString(gTopScreenPixels, W, H, 18, 76, "SCREEN: TOP DISPLAY (256X192)", lightCyan, 1);

    char layoutBuf[64];
    snprintf(layoutBuf, sizeof(layoutBuf), "LAYOUT: %s", layoutName ? layoutName : "SIDE-BY-SIDE");
    DrawString(gTopScreenPixels, W, H, 18, 92, layoutBuf, amber, 1);

    DrawString(gTopScreenPixels, W, H, 18, 108, "PACING: 59.8261 HZ VBLANK LOCKED", white, 1);
    DrawString(gTopScreenPixels, W, H, 18, 124, "KEYBINDS: F10 LAYOUT | F11 FULLSCREEN", gold, 1);
    DrawString(gTopScreenPixels, W, H, 18, 140, "INPUT: KEYBOARD / GAMEPAD / MOUSE", lightCyan, 1);

    // 60 FPS heartbeat animation bar
    int pulseX = 14 + ((frameCounter * 3) % (W - 28 - 20));
    FillRect(gTopScreenPixels, W, H, 10, 168, W - 20, 14, 0xFF201510);
    DrawRect(gTopScreenPixels, W, H, 10, 168, W - 20, 14, 0xFF503525);
    FillRect(gTopScreenPixels, W, H, pulseX, 170, 20, 10, gold);
    DrawString(gTopScreenPixels, W, H, 16, 171, "RUNNING 60 FPS", white, 1);

    // --- TOUCHSCREEN: Sub Display Digitizer ---
    FillRect(gTouchScreenPixels, W, H, 0, 0, W, H, 0xFF221C18);

    // Grid lines every 32 px
    for (int gx = 32; gx < W; gx += 32) {
        for (int y = 4; y < H - 4; ++y) {
            if ((y % 4) == 0) DrawPixel(gTouchScreenPixels, W, H, gx, y, faintCyan);
        }
    }
    for (int gy = 32; gy < H; gy += 32) {
        for (int x = 4; x < W - 4; ++x) {
            if ((x % 4) == 0) DrawPixel(gTouchScreenPixels, W, H, x, gy, faintCyan);
        }
    }

    // Touch screen border
    DrawRect(gTouchScreenPixels, W, H, 2, 2, W - 4, H - 4, cyan);

    // Header
    FillRect(gTouchScreenPixels, W, H, 6, 6, W - 12, 20, 0xFF352A20);
    DrawRect(gTouchScreenPixels, W, H, 6, 6, W - 12, 20, cyan);
    DrawString(gTouchScreenPixels, W, H, 14, 11, "TOUCHSCREEN DIGITIZER", cyan, 1);

    DrawString(gTouchScreenPixels, W, H, 24, 75, "CLICK AND DRAG MOUSE TO TEST TOUCH", white, 1);
    DrawString(gTouchScreenPixels, W, H, 34, 95, "COORDINATES TRANSFORM DYNAMICALLY", 0xFF908070, 1);
    DrawString(gTouchScreenPixels, W, H, 50, 115, "ACROSS ALL 4 LAYOUT MODES", 0xFF908070, 1);

    // Touch feedback
    char touchBuf[64];
    if (touching) {
        snprintf(touchBuf, sizeof(touchBuf), "TOUCH: ACTIVE (X=%03d, Y=%03d)", touchX, touchY);
        DrawString(gTouchScreenPixels, W, H, 14, 168, touchBuf, amber, 1);

        // Draw crosshair cursor at touch point
        int tx = touchX;
        int ty = touchY;
        for (int d = -8; d <= 8; ++d) {
            DrawPixel(gTouchScreenPixels, W, H, tx + d, ty, amber);
            DrawPixel(gTouchScreenPixels, W, H, tx, ty + d, amber);
        }
        DrawRect(gTouchScreenPixels, W, H, tx - 4, ty - 4, 9, 9, white);
    } else {
        snprintf(touchBuf, sizeof(touchBuf), "TOUCH: IDLE [CLICK DIGITIZER AREA]");
        DrawString(gTouchScreenPixels, W, H, 14, 168, touchBuf, 0xFF706050, 1);
    }
}

void GX_RenderFrame(void) {
    if (!s_testPatternEnabled) {
        NdsGfx_RenderMain();
        NdsGfx_RenderSub();
        NdsGfx_PresentFrame();
    } else {
        const void* top = gMainDispOn ? gTopScreenPixels : nullptr;
        const void* touch = gSubDispOn ? gTouchScreenPixels : nullptr;
        DisplayManager_PresentFrame(top, touch);
    }
}

} // extern "C"
