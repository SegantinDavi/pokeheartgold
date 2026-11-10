/**
 * @file gx_pipeline.cpp
 * @brief Top-level NDS Graphics Pipeline lifecycle, dispatch, and presentation.
 */

#include "gx_pipeline.h"
#include "port_gx.h"
#include "nds_compositor.h"
#include "nds_authentic_demo.h"
#include "display/display_manager.h"
#include <SDL.h>
#include <cstring>
#include <algorithm>

NdsGraphicsPipeline gNdsGfx;
static uint64_t s_renderStartPerf = 0;

extern "C" {

void NdsGfx_Init(void) {
    std::memset(&gNdsGfx, 0, sizeof(gNdsGfx));
    gNdsGfx.dispSelect = GX_DISP_SELECT_MAIN_SUB;
    PortHwRegisters_Init();
    G3Pipeline_Init(&gNdsGfx.g3);
    NdsAuthenticDemo_Init();
}

void NdsGfx_SyncRegisters(NdsDisplayEngine* engine, bool isSub) {
    if (!engine) return;
    uint32_t baseOffset = isSub ? 0x1000 : 0x0000;

    // Check if I/O registers have been initialized / written
    uint32_t dispcnt = *reinterpret_cast<const uint32_t*>(gHardwareIoRegMemory + baseOffset + 0x00);
    uint16_t bg0 = *reinterpret_cast<const uint16_t*>(gHardwareIoRegMemory + baseOffset + 0x08);
    uint16_t bg1 = *reinterpret_cast<const uint16_t*>(gHardwareIoRegMemory + baseOffset + 0x0A);
    uint16_t bg2 = *reinterpret_cast<const uint16_t*>(gHardwareIoRegMemory + baseOffset + 0x0C);
    uint16_t bg3 = *reinterpret_cast<const uint16_t*>(gHardwareIoRegMemory + baseOffset + 0x0E);

    if (dispcnt != 0 || bg0 != 0 || bg1 != 0 || bg2 != 0 || bg3 != 0) {
        engine->dispcnt.bgMode = dispcnt & 0x7;
        engine->dispcnt.bg0Is3D = (dispcnt & (1 << 3)) != 0;
        engine->dispcnt.objMapChar1D = (dispcnt & (1 << 4)) != 0;
        engine->dispcnt.visiblePlanes = (dispcnt >> 8) & 0x1F;
        engine->dispcnt.visibleWindows = (dispcnt >> 13) & 0x7;
        engine->dispcnt.dispMode = (dispcnt >> 16) & 0x3;
        engine->dispcnt.bgExtPltt = (dispcnt & (1 << 30)) != 0;
        engine->dispcnt.objExtPltt = (dispcnt & (1 << 31)) != 0;

        for (int i = 0; i < 4; ++i) {
            uint16_t bgcnt = *reinterpret_cast<const uint16_t*>(gHardwareIoRegMemory + baseOffset + 0x08 + i * 2);
            engine->bg[i].priority = bgcnt & 0x3;
            engine->bg[i].charBase = (bgcnt >> 2) & 0xF;
            engine->bg[i].mosaicEnable = (bgcnt >> 6) & 0x1;
            engine->bg[i].colorMode = (bgcnt >> 7) & 0x1;
            engine->bg[i].screenBase = (bgcnt >> 8) & 0x1F;
            engine->bg[i].extPltt = (bgcnt >> 13) & 0x1;
            engine->bg[i].screenSize = (bgcnt >> 14) & 0x3;
            engine->bg[i].enabled = (engine->dispcnt.visiblePlanes & (1 << i)) != 0;

            if (i < 2) {
                engine->bg[i].mode = NDS_BGMODE_TEXT;
            } else if (i == 2) {
                engine->bg[i].mode = (engine->dispcnt.bgMode >= 2 && engine->dispcnt.bgMode <= 5)
                                   ? ((engine->dispcnt.bgMode >= 3) ? NDS_BGMODE_AFFINE_EXT : NDS_BGMODE_AFFINE)
                                   : NDS_BGMODE_TEXT;
            } else /* i == 3 */ {
                engine->bg[i].mode = (engine->dispcnt.bgMode >= 1 && engine->dispcnt.bgMode <= 5)
                                   ? ((engine->dispcnt.bgMode >= 3) ? NDS_BGMODE_AFFINE_EXT : NDS_BGMODE_AFFINE)
                                   : NDS_BGMODE_TEXT;
            }

            engine->bg[i].scrollX = *reinterpret_cast<const uint16_t*>(gHardwareIoRegMemory + baseOffset + 0x10 + i * 4) & 0x1FF;
            engine->bg[i].scrollY = *reinterpret_cast<const uint16_t*>(gHardwareIoRegMemory + baseOffset + 0x12 + i * 4) & 0x1FF;
        }

        engine->bg[2].affineA = *reinterpret_cast<const int16_t*>(gHardwareIoRegMemory + baseOffset + 0x20);
        engine->bg[2].affineB = *reinterpret_cast<const int16_t*>(gHardwareIoRegMemory + baseOffset + 0x22);
        engine->bg[2].affineC = *reinterpret_cast<const int16_t*>(gHardwareIoRegMemory + baseOffset + 0x24);
        engine->bg[2].affineD = *reinterpret_cast<const int16_t*>(gHardwareIoRegMemory + baseOffset + 0x26);
        engine->bg[2].affineRefX = *reinterpret_cast<const int32_t*>(gHardwareIoRegMemory + baseOffset + 0x28);
        engine->bg[2].affineRefY = *reinterpret_cast<const int32_t*>(gHardwareIoRegMemory + baseOffset + 0x2C);
        engine->bg[2].areaOver = (*reinterpret_cast<const uint16_t*>(gHardwareIoRegMemory + baseOffset + 0x0C) & (1 << 13)) != 0;

        engine->bg[3].affineA = *reinterpret_cast<const int16_t*>(gHardwareIoRegMemory + baseOffset + 0x30);
        engine->bg[3].affineB = *reinterpret_cast<const int16_t*>(gHardwareIoRegMemory + baseOffset + 0x32);
        engine->bg[3].affineC = *reinterpret_cast<const int16_t*>(gHardwareIoRegMemory + baseOffset + 0x34);
        engine->bg[3].affineD = *reinterpret_cast<const int16_t*>(gHardwareIoRegMemory + baseOffset + 0x36);
        engine->bg[3].affineRefX = *reinterpret_cast<const int32_t*>(gHardwareIoRegMemory + baseOffset + 0x38);
        engine->bg[3].affineRefY = *reinterpret_cast<const int32_t*>(gHardwareIoRegMemory + baseOffset + 0x3C);
        engine->bg[3].areaOver = (*reinterpret_cast<const uint16_t*>(gHardwareIoRegMemory + baseOffset + 0x0E) & (1 << 13)) != 0;

        uint16_t bldcnt = *reinterpret_cast<const uint16_t*>(gHardwareIoRegMemory + baseOffset + 0x50);
        engine->blend.srcMask = bldcnt & 0x3F;
        engine->blend.blendMode = (bldcnt >> 6) & 0x3;
        engine->blend.dstMask = (bldcnt >> 8) & 0x3F;

        uint16_t bldalpha = *reinterpret_cast<const uint16_t*>(gHardwareIoRegMemory + baseOffset + 0x52);
        engine->blend.evA = std::min<uint8_t>(bldalpha & 0x1F, 16);
        engine->blend.evB = std::min<uint8_t>((bldalpha >> 8) & 0x1F, 16);

        uint16_t mb = *reinterpret_cast<const uint16_t*>(gHardwareIoRegMemory + baseOffset + 0x6C);
        uint8_t mbMode = (mb >> 14) & 0x3;
        int8_t mbLevel = static_cast<int8_t>(mb & 0x1F);
        if (mbLevel > 16) mbLevel = 16;
        if (mbMode == 1) {
            engine->blend.masterBrightness = mbLevel;
        } else if (mbMode == 2) {
            engine->blend.masterBrightness = -mbLevel;
        } else {
            engine->blend.masterBrightness = 0;
        }
    }
}

void NdsGfx_RenderMain(void) {
    s_renderStartPerf = SDL_GetPerformanceCounter();
    NdsGfx_SyncRegisters(&gNdsGfx.main, false);
    for (int y = 0; y < 192; ++y) {
        NdsCompositor_RenderScanline(&gNdsGfx.main, y, gHwLcdcVram, gHwOam, reinterpret_cast<const uint16_t*>(gHwPltt));
    }
    NdsCompositor_ApplyBrightness(gNdsGfx.main.frameBuffer, gNdsGfx.main.blend.masterBrightness);
}

void NdsGfx_RenderSub(void) {
    NdsGfx_SyncRegisters(&gNdsGfx.sub, true);
    for (int y = 0; y < 192; ++y) {
        NdsCompositor_RenderScanline(&gNdsGfx.sub, y, gHwLcdcVram, gHwDbOam, reinterpret_cast<const uint16_t*>(gHwDbPltt));
    }
    NdsCompositor_ApplyBrightness(gNdsGfx.sub.frameBuffer, gNdsGfx.sub.blend.masterBrightness);

    // Frame compositing budget measurement (T057, SC-005)
    uint64_t endPerf = SDL_GetPerformanceCounter();
    double elapsedMs = (static_cast<double>(endPerf - s_renderStartPerf) * 1000.0) / static_cast<double>(SDL_GetPerformanceFrequency());
    static uint32_t s_budgetWarningCounter = 0;
    if (elapsedMs > 4.0) {
        if ((s_budgetWarningCounter++ % 60) == 0) {
            SDL_Log("Warning: 2D/3D compositing exceeded 4 ms budget: %.2f ms", elapsedMs);
        }
    }
}

void NdsGfx_PresentFrame(void) {
    // Route composite buffers respecting display select (T051)
    gNdsGfx.dispSelect = GX_GetDispSelect();
    GX_RouteCompositeBuffers(
        reinterpret_cast<const uint32_t*>(gNdsGfx.main.frameBuffer),
        reinterpret_cast<const uint32_t*>(gNdsGfx.sub.frameBuffer)
    );
    DisplayManager_PresentFrame(GX_GetTopScreenPixels(), GX_GetTouchScreenPixels());
}

} // extern "C"
