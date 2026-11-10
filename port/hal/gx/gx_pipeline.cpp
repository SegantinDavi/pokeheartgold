/**
 * @file gx_pipeline.cpp
 * @brief Top-level NDS Graphics Pipeline lifecycle, dispatch, and presentation.
 */

#include "gx_pipeline.h"
#include "port_gx.h"
#include "nds_compositor.h"
#include "display/display_manager.h"
#include <cstring>

NdsGraphicsPipeline gNdsGfx;

extern "C" {

void NdsGfx_Init(void) {
    std::memset(&gNdsGfx, 0, sizeof(gNdsGfx));
    gNdsGfx.dispSelect = GX_DISP_SELECT_MAIN_SUB;
    PortHwRegisters_Init();
    G3Pipeline_Init(&gNdsGfx.g3);
}

void NdsGfx_RenderMain(void) {
    for (int y = 0; y < 192; ++y) {
        NdsCompositor_RenderScanline(&gNdsGfx.main, y, gHwLcdcVram, gHwOam, reinterpret_cast<const uint16_t*>(gHwPltt));
    }
    NdsCompositor_ApplyBrightness(gNdsGfx.main.frameBuffer, gNdsGfx.main.blend.masterBrightness);
}

void NdsGfx_RenderSub(void) {
    for (int y = 0; y < 192; ++y) {
        NdsCompositor_RenderScanline(&gNdsGfx.sub, y, gHwLcdcVram, gHwDbOam, reinterpret_cast<const uint16_t*>(gHwDbPltt));
    }
    NdsCompositor_ApplyBrightness(gNdsGfx.sub.frameBuffer, gNdsGfx.sub.blend.masterBrightness);
}

void NdsGfx_PresentFrame(void) {
    // Route composite buffers respecting display select
    GX_RouteCompositeBuffers(
        reinterpret_cast<const uint32_t*>(gNdsGfx.main.frameBuffer),
        reinterpret_cast<const uint32_t*>(gNdsGfx.sub.frameBuffer)
    );
    DisplayManager_PresentFrame(GX_GetTopScreenPixels(), GX_GetTouchScreenPixels());
}

} // extern "C"
