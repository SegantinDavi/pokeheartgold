/**
 * @file g3_pipeline.h
 * @brief NDS Fixed-Function 3D Pipeline extensions and FBO compositing.
 */

#ifndef G3_PIPELINE_H
#define G3_PIPELINE_H

#include <stdint.h>
#include <stdbool.h>
#include <SDL_opengl.h>
#include "port_g3.h"

#ifdef __cplusplus
extern "C" {
#endif

/** 3D pipeline internal state (embedded in NdsGraphicsPipeline). */
typedef struct Nds3DPipelineState {
    GXMtxMode  mtxMode;

    /* Matrix stack depth counters (NDS limits). */
    int        projStackDepth;   /**< Max 3 (4-entry stack, top at 0). */
    int        posStackDepth;    /**< Max 30 (31-entry stack). */
    int        texStackDepth;    /**< Must stay 0 (1-entry stack). */

    /* Current texture slot (VRAM offset + format). */
    uint32_t   texVramOffset;
    uint32_t   texPaletteOffset;
    uint8_t    texFormat;        /**< 0–7 NDS texture format. */
    uint16_t   texWidth;
    uint16_t   texHeight;
    bool       texEnabled;
    bool       texRepeatS;
    bool       texRepeatT;

    /* Polygon render state. */
    uint8_t    polyMode;         /**< 0=modulate, 1=decal, 2=toon, 3=shadow. */
    uint8_t    polyAlpha;        /**< 0=wireframe, 31=opaque. */
    bool       depthTestEnable;
    bool       cullBack;
    bool       cullFront;

    /* FBO for 3D → BG0 readback. */
    GLuint     fbo;
    GLuint     fboColorTex;      /**< GL_RGBA8 256×192 color attachment. */
    GLuint     fboDepthRb;       /**< GL_DEPTH_COMPONENT24 renderbuffer. */
    uint32_t   bg0Readback[256 * 192]; /**< CPU-side RGBA8 BG0 output. */
} Nds3DPipelineState;

/* ---- Lifecycle ---- */

/** Initialize FBO and GL state. Must be called with an active GL context. */
void G3Pipeline_Init(Nds3DPipelineState* state);

/** Destroy FBO resources. */
void G3Pipeline_Destroy(Nds3DPipelineState* state);

/* ---- Matrix Stack ---- */

/** Push current matrix onto the active stack. Clamps on overflow (NDS behavior). */
void G3_PushMtx(void);

/** Pop matrix from the active stack. Clamps on underflow. */
void G3_PopMtx(int count);

/** Load a 4×4 column-major fixed-point matrix into the current mode. */
void G3_LoadMtx44(const int32_t* mtx44);

/** Multiply current matrix by a 4×4 fixed-point matrix. */
void G3_MultMtx44(const int32_t* mtx44);

/* ---- Projection ---- */

/** Set up a perspective projection (NDS FOVY in degrees × 4096, aspect fx32). */
void G3_Perspective(int32_t fovySin, int32_t fovyCos, int32_t aspect,
                    int32_t nearClip, int32_t farClip);

/** Set up an orthographic projection. */
void G3_Ortho(int32_t left, int32_t right, int32_t bottom, int32_t top,
              int32_t nearClip, int32_t farClip);

/* ---- Texture (Internal Pipeline) ---- */

/**
 * Configure the texture image parameters on pipeline state.
 *
 * @param state       3D pipeline state.
 * @param vramOffset  Byte offset within gHwLcdcVram for texture data.
 * @param format      NDS texture format (0–7).
 * @param sSize       Texture width  as power-of-2 index (0=8, 7=1024).
 * @param tSize       Texture height as power-of-2 index.
 * @param repeatS     Wrap horizontally.
 * @param repeatT     Wrap vertically.
 */
void G3Pipeline_TexImageParam(Nds3DPipelineState* state,
                              uint32_t vramOffset, uint8_t format,
                              uint8_t sSize, uint8_t tSize,
                              bool repeatS, bool repeatT);

/**
 * Configure the texture palette on pipeline state.
 *
 * @param state          3D pipeline state.
 * @param paletteOffset  Byte offset within gHwLcdcVram for palette data.
 */
void G3Pipeline_TexPlttBase(Nds3DPipelineState* state, uint32_t paletteOffset);

/* ---- Polygon Attributes (Internal Pipeline) ---- */

/** Set polygon render attributes on pipeline state (mode, alpha, culling, depth). */
void G3Pipeline_PolygonAttr(Nds3DPipelineState* state,
                            uint8_t polyMode, uint8_t alpha,
                            bool cullBack, bool cullFront, bool depthTestEnable);

/* ---- Swap / Readback (Internal Pipeline) ---- */

/**
 * End the 3D geometry pass and render into the FBO.
 * Reads back the result into state->bg0Readback for use as BG0 in the compositor.
 * Must be called once per VBlank before NdsGfx_RenderMain().
 *
 * @param state  3D pipeline state (from gNdsGfx.g3).
 */
void G3Pipeline_SwapBuffers(Nds3DPipelineState* state);

/* ---- NitroSDK Public Forwarders ---- */
void G3_SwapBuffers(GXSortMode am, GXBufferMode zw);
void G3_PolygonAttr(int light, int polyMode, int cullMode, int polygonID, int alpha, int misc);
void G3_TexImageParam(GXTexFmt texFmt, GXTexGen texGen, GXTexSizeS s, GXTexSizeT t, GXTexRepeat repeat, GXTexFlip flip, GXTexPlttColor0 pltt0, u32 addr);
void G3_TexPlttBase(u32 addr, GXTexFmt texfmt);

#ifdef __cplusplus
}
#endif

#endif /* G3_PIPELINE_H */
