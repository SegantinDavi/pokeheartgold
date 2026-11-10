/**
 * @file g3_pipeline.cpp
 * @brief Fixed-function 3D geometry pipeline, matrix stack, and FBO readback.
 */

#define GL_GLEXT_PROTOTYPES 1
#include "g3_pipeline.h"
#include "nds_texture_cache.h"
#include "port_hw_registers.h"
#include "gx_pipeline.h"
#include <SDL.h>
#include <SDL_opengl.h>
#include <cstring>
#include <vector>
#include <algorithm>

namespace {

bool HasActiveGLContext() {
    return SDL_GL_GetCurrentContext() != nullptr;
}

} // anonymous namespace

extern "C" {

void G3Pipeline_Init(Nds3DPipelineState* state) {
    if (!state) return;
    std::memset(state, 0, sizeof(*state));
    state->mtxMode = GX_MTXMODE_POSITION_VECTOR;
    state->polyAlpha = 31;
    state->depthTestEnable = true;

    // Link state->bg0Readback into gNdsGfx.main.bg0_3d_buffer
    gNdsGfx.main.bg0_3d_buffer = state->bg0Readback;

    if (!HasActiveGLContext()) return;

    if (state->fbo == 0) {
        glGenFramebuffers(1, &state->fbo);
        glGenTextures(1, &state->fboColorTex);
        glBindTexture(GL_TEXTURE_2D, state->fboColorTex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 256, 192, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glGenRenderbuffers(1, &state->fboDepthRb);
        glBindRenderbuffer(GL_RENDERBUFFER, state->fboDepthRb);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 256, 192);

        glBindFramebuffer(GL_FRAMEBUFFER, state->fbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, state->fboColorTex, 0);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, state->fboDepthRb);

        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClearDepth(1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glBindRenderbuffer(GL_RENDERBUFFER, 0);
    }
}

void G3Pipeline_Destroy(Nds3DPipelineState* state) {
    if (!state) return;
    if (HasActiveGLContext()) {
        if (state->fbo) {
            glDeleteFramebuffers(1, &state->fbo);
            state->fbo = 0;
        }
        if (state->fboColorTex) {
            glDeleteTextures(1, &state->fboColorTex);
            state->fboColorTex = 0;
        }
        if (state->fboDepthRb) {
            glDeleteRenderbuffers(1, &state->fboDepthRb);
            state->fboDepthRb = 0;
        }
    }
    NdsTexCache_Flush();
    std::memset(state, 0, sizeof(*state));
}

void G3_PushMtx(void) {
    if (HasActiveGLContext()) {
        glPushMatrix();
    }
    GXMtxMode mode = G3_GetMtxMode();
    if (mode == GX_MTXMODE_PROJECTION) {
        if (gNdsGfx.g3.projStackDepth < 3) {
            gNdsGfx.g3.projStackDepth++;
        } else {
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "G3_PushMtx: Projection stack overflow (max 3)");
        }
    } else if (mode == GX_MTXMODE_POSITION || mode == GX_MTXMODE_POSITION_VECTOR) {
        if (gNdsGfx.g3.posStackDepth < 30) {
            gNdsGfx.g3.posStackDepth++;
        } else {
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "G3_PushMtx: Position stack overflow (max 30)");
        }
    } else if (mode == GX_MTXMODE_TEXTURE) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "G3_PushMtx: Texture stack overflow (max 0)");
    }
}

void G3_PopMtx(int count) {
    if (count <= 0) return;
    if (HasActiveGLContext()) {
        for (int i = 0; i < count; ++i) {
            glPopMatrix();
        }
    }
    GXMtxMode mode = G3_GetMtxMode();
    if (mode == GX_MTXMODE_PROJECTION) {
        gNdsGfx.g3.projStackDepth -= count;
        if (gNdsGfx.g3.projStackDepth < 0) {
            gNdsGfx.g3.projStackDepth = 0;
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "G3_PopMtx: Projection stack underflow");
        }
    } else if (mode == GX_MTXMODE_POSITION || mode == GX_MTXMODE_POSITION_VECTOR) {
        gNdsGfx.g3.posStackDepth -= count;
        if (gNdsGfx.g3.posStackDepth < 0) {
            gNdsGfx.g3.posStackDepth = 0;
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "G3_PopMtx: Position stack underflow");
        }
    } else if (mode == GX_MTXMODE_TEXTURE) {
        gNdsGfx.g3.texStackDepth = 0;
    }
}

void G3_LoadMtx44(const int32_t* mtx44) {
    if (!mtx44) return;
    GLfloat glMtx[16];
    for (int i = 0; i < 16; ++i) {
        glMtx[i] = FX32_TO_FLOAT(mtx44[i]);
    }
    if (HasActiveGLContext()) {
        glLoadMatrixf(glMtx);
    }
}

void G3_MultMtx44(const int32_t* mtx44) {
    if (!mtx44) return;
    GLfloat glMtx[16];
    for (int i = 0; i < 16; ++i) {
        glMtx[i] = FX32_TO_FLOAT(mtx44[i]);
    }
    if (HasActiveGLContext()) {
        glMultMatrixf(glMtx);
    }
}

void G3_Perspective(int32_t fovySin, int32_t fovyCos, int32_t aspect,
                    int32_t nearClip, int32_t farClip) {
    float nearF = FX32_TO_FLOAT(nearClip);
    float farF = FX32_TO_FLOAT(farClip);
    float sinF = FX32_TO_FLOAT(fovySin);
    float cosF = FX32_TO_FLOAT(fovyCos);
    float aspectF = FX32_TO_FLOAT(aspect);

    float top = (cosF != 0.0f) ? (nearF * sinF / cosF) : nearF;
    float bottom = -top;
    float right = top * aspectF;
    float left = -right;

    if (HasActiveGLContext()) {
        glFrustum(left, right, bottom, top, nearF, farF);
    }
}

void G3_Ortho(int32_t left, int32_t right, int32_t bottom, int32_t top,
              int32_t nearClip, int32_t farClip) {
    if (HasActiveGLContext()) {
        glOrtho(FX32_TO_FLOAT(left), FX32_TO_FLOAT(right),
                FX32_TO_FLOAT(bottom), FX32_TO_FLOAT(top),
                FX32_TO_FLOAT(nearClip), FX32_TO_FLOAT(farClip));
    }
}

void G3Pipeline_PolygonAttr(Nds3DPipelineState* state,
                            uint8_t polyMode, uint8_t alpha,
                            bool cullBack, bool cullFront, bool depthTestEnable) {
    if (!state) return;
    state->polyMode = polyMode;
    state->polyAlpha = alpha;
    state->cullBack = cullBack;
    state->cullFront = cullFront;
    state->depthTestEnable = depthTestEnable;

    if (HasActiveGLContext()) {
        if (cullBack && cullFront) {
            glEnable(GL_CULL_FACE);
            glCullFace(GL_FRONT_AND_BACK);
        } else if (cullBack) {
            glEnable(GL_CULL_FACE);
            glCullFace(GL_BACK);
        } else if (cullFront) {
            glEnable(GL_CULL_FACE);
            glCullFace(GL_FRONT);
        } else {
            glDisable(GL_CULL_FACE);
        }

        if (depthTestEnable) {
            glEnable(GL_DEPTH_TEST);
            glDepthFunc(GL_LESS);
        } else {
            glDisable(GL_DEPTH_TEST);
        }
    }
}

void G3_PolygonAttr(int light, int polyMode, int cullMode, int polygonID, int alpha, int misc) {
    (void)light;
    (void)polygonID;
    bool cullFront = (cullMode == 1 || cullMode == 3);
    bool cullBack = (cullMode == 2 || cullMode == 3);
    bool depthTest = (misc & (1 << 14)) == 0;
    G3Pipeline_PolygonAttr(&gNdsGfx.g3, static_cast<uint8_t>(polyMode & 3), static_cast<uint8_t>(alpha & 31), cullBack, cullFront, depthTest);
}

void G3Pipeline_TexImageParam(Nds3DPipelineState* state,
                              uint32_t vramOffset, uint8_t format,
                              uint8_t sSize, uint8_t tSize,
                              bool repeatS, bool repeatT) {
    if (!state) return;
    state->texVramOffset = vramOffset;
    state->texFormat = format;
    state->texWidth = 8 << (sSize & 7);
    state->texHeight = 8 << (tSize & 7);
    state->texRepeatS = repeatS;
    state->texRepeatT = repeatT;
    state->texEnabled = (format != 0);

    if (state->texEnabled && HasActiveGLContext()) {
        GLuint texId = NdsTexCache_Upload(
            gHwLcdcVram, state->texVramOffset, state->texFormat,
            state->texWidth, state->texHeight,
            gHwLcdcVram, state->texPaletteOffset
        );
        NdsTexCache_Bind(texId);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, repeatS ? GL_REPEAT : GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, repeatT ? GL_REPEAT : GL_CLAMP_TO_EDGE);
    }
}

void G3Pipeline_TexPlttBase(Nds3DPipelineState* state, uint32_t paletteOffset) {
    if (!state) return;
    state->texPaletteOffset = paletteOffset;
}

void G3_TexImageParam(GXTexFmt texFmt, GXTexGen texGen, GXTexSizeS s, GXTexSizeT t, GXTexRepeat repeat, GXTexFlip flip, GXTexPlttColor0 pltt0, u32 addr) {
    (void)texGen;
    (void)flip;
    (void)pltt0;
    bool repS = (repeat == GX_TEXREPEAT_S || repeat == GX_TEXREPEAT_ST);
    bool repT = (repeat == GX_TEXREPEAT_T || repeat == GX_TEXREPEAT_ST);
    G3Pipeline_TexImageParam(&gNdsGfx.g3, addr, static_cast<uint8_t>(texFmt), static_cast<uint8_t>(s), static_cast<uint8_t>(t), repS, repT);
}

void G3_TexPlttBase(u32 addr, GXTexFmt texfmt) {
    (void)texfmt;
    G3Pipeline_TexPlttBase(&gNdsGfx.g3, addr);
}

void G3Pipeline_SwapBuffers(Nds3DPipelineState* state) {
    if (!state) return;
    if (state->fbo == 0 && HasActiveGLContext()) {
        G3Pipeline_Init(state);
    }

    if (HasActiveGLContext() && state->fbo != 0) {
        glBindFramebuffer(GL_FRAMEBUFFER, state->fbo);
        glViewport(0, 0, 256, 192);

        std::vector<uint32_t> temp(256 * 192, 0);
        glReadPixels(0, 0, 256, 192, GL_RGBA, GL_UNSIGNED_BYTE, temp.data());

        for (int y = 0; y < 192; ++y) {
            std::memcpy(&state->bg0Readback[y * 256], &temp[(191 - y) * 256], 256 * sizeof(uint32_t));
        }

        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClearDepth(1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    gNdsGfx.main.bg0_3d_buffer = state->bg0Readback;
}

void G3_SwapBuffers(GXSortMode am, GXBufferMode zw) {
    (void)am;
    (void)zw;
    G3Pipeline_SwapBuffers(&gNdsGfx.g3);
}

} // extern "C"
