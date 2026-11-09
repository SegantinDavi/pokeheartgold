/**
 * @file gx_pipeline.h
 * @brief Contract: Top-level NDS Graphics Pipeline lifecycle and frame entry points.
 *
 * Lives in: port/hal/gx/gx_pipeline.cpp + port/include/gx_pipeline.h
 *
 * Singleton NdsGraphicsPipeline owns both display engines and the 3D pipeline.
 * Called from the VBlank handler (port/hal/os/vblank.cpp).
 */

#ifndef GX_PIPELINE_H
#define GX_PIPELINE_H

#include <stdint.h>
#include <stdbool.h>
#include "port_hw_registers.h"
#include "nds_display_engine.h"  // NdsDisplayEngine
#include "g3_pipeline.h"         // Nds3DPipelineState

#ifdef __cplusplus
extern "C" {
#endif

/** Top-level pipeline singleton. */
typedef struct NdsGraphicsPipeline {
    NdsDisplayEngine main;  /**< Main engine — top screen by default. */
    NdsDisplayEngine sub;   /**< Sub engine  — touchscreen by default. */
    Nds3DPipelineState g3;  /**< 3D fixed-function pipeline. */
    int dispSelect;         /**< GX_DISP_SELECT_MAIN_SUB (0) or SUB_MAIN (1). */
} NdsGraphicsPipeline;

extern NdsGraphicsPipeline gNdsGfx;

/**
 * Initialize all graphics pipeline state.
 * Must be called before any GX_*, G2_*, G3_* functions.
 */
void NdsGfx_Init(void);

/**
 * Render the Main display engine into main.frameBuffer.
 * Called during VBlank after all VRAM/OAM writes are complete.
 */
void NdsGfx_RenderMain(void);

/**
 * Render the Sub display engine into sub.frameBuffer.
 */
void NdsGfx_RenderSub(void);

/**
 * Present both framebuffers: routes them through GX_RouteCompositeBuffers
 * respecting dispSelect, then calls DisplayManager_PresentFrame.
 */
void NdsGfx_PresentFrame(void);

#ifdef __cplusplus
}
#endif

#endif /* GX_PIPELINE_H */
