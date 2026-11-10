/**
 * @file test_g3_pipeline.cpp
 * @brief Unit tests for fixed-function 3D geometry pipeline, matrix stacks, texture cache, and FBO readback (Phase 5 / User Story 3).
 */

#define GL_GLEXT_PROTOTYPES 1
#include "test_runner.h"
#include "g3_pipeline.h"
#include "nds_texture_cache.h"
#include "gx_pipeline.h"
#include <SDL.h>
#include <SDL_opengl.h>
#include <cstring>
#include <vector>

TEST_CASE(G3_MatrixStackClamping) {
    NdsGfx_Init();

    // Mode: Projection (NDS stack limit: 1 current + 3 stack = depth max 3)
    G3_MtxMode(GX_MTXMODE_PROJECTION);
    ASSERT_EQ(0, gNdsGfx.g3.projStackDepth);

    // Push 4 times (exceeds limit 3)
    G3_PushMtx();
    ASSERT_EQ(1, gNdsGfx.g3.projStackDepth);
    G3_PushMtx();
    ASSERT_EQ(2, gNdsGfx.g3.projStackDepth);
    G3_PushMtx();
    ASSERT_EQ(3, gNdsGfx.g3.projStackDepth);
    G3_PushMtx(); // Should clamp at 3 with no crash
    ASSERT_EQ(3, gNdsGfx.g3.projStackDepth);

    // Pop 1
    G3_PopMtx(1);
    ASSERT_EQ(2, gNdsGfx.g3.projStackDepth);

    // Pop 4 (underflows past 0)
    G3_PopMtx(4);
    ASSERT_EQ(0, gNdsGfx.g3.projStackDepth);

    // Mode: Position/Vector (NDS stack limit: depth max 30)
    G3_MtxMode(GX_MTXMODE_POSITION_VECTOR);
    ASSERT_EQ(0, gNdsGfx.g3.posStackDepth);

    for (int i = 0; i < 35; ++i) {
        G3_PushMtx();
    }
    // Must be clamped at 30
    ASSERT_EQ(30, gNdsGfx.g3.posStackDepth);

    // Pop past 0
    G3_PopMtx(35);
    ASSERT_EQ(0, gNdsGfx.g3.posStackDepth);
}

TEST_CASE(G3_TextureCacheUploadAndFlush) {
    std::vector<uint8_t> vram(0x8000, 0);
    // Fill 16x16 4-color texture (format 2: 2bpp -> 16x16/4 = 64 bytes)
    std::memset(vram.data(), 0x55, 64);

    uint16_t pltt[4] = {0x0000, 0x001F, 0x03E0, 0x7C00};

    GLuint texId1 = NdsTexCache_Upload(
        vram.data(), 0, 2, 16, 16,
        reinterpret_cast<const uint8_t*>(pltt), 0
    );
    ASSERT_NE(0u, texId1);

    // Re-uploading same parameters must hit cache and return identical ID
    GLuint texId2 = NdsTexCache_Upload(
        vram.data(), 0, 2, 16, 16,
        reinterpret_cast<const uint8_t*>(pltt), 0
    );
    ASSERT_EQ(texId1, texId2);

    // Flush cache
    NdsTexCache_Flush();

    // Re-uploading after flush creates a new entry
    GLuint texId3 = NdsTexCache_Upload(
        vram.data(), 0, 2, 16, 16,
        reinterpret_cast<const uint8_t*>(pltt), 0
    );
    ASSERT_NE(0u, texId3);

    NdsTexCache_Flush();
}

TEST_CASE(G3_GeometryPipelineAndFboReadback) {
    NdsGfx_Init();

    // Try creating a hidden OpenGL context for FBO test
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("G3Test", 0, 0, 256, 192, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);

    if (!window) {
        // In headless environment without GL display, verify pipeline state structures
        ASSERT_TRUE(gNdsGfx.main.bg0_3d_buffer != nullptr);
        return;
    }

    SDL_GLContext ctx = SDL_GL_CreateContext(window);
    if (!ctx) {
        SDL_DestroyWindow(window);
        ASSERT_TRUE(gNdsGfx.main.bg0_3d_buffer != nullptr);
        return;
    }

    SDL_GL_MakeCurrent(window, ctx);

    // Initialize 3D pipeline FBO
    G3Pipeline_Init(&gNdsGfx.g3);
    ASSERT_NE(0u, gNdsGfx.g3.fbo);

    // Bind FBO and draw a solid full-viewport triangle in red
    glBindFramebuffer(GL_FRAMEBUFFER, gNdsGfx.g3.fbo);
    glViewport(0, 0, 256, 192);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Draw solid triangle
    glBegin(GL_TRIANGLES);
    glColor4f(1.0f, 0.0f, 0.0f, 1.0f);
    glVertex3f(-0.8f, -0.8f, 0.0f);
    glVertex3f(0.8f, -0.8f, 0.0f);
    glVertex3f(0.0f, 0.8f, 0.0f);
    glEnd();

    // Call SwapBuffers
    G3Pipeline_SwapBuffers(&gNdsGfx.g3);

    // Verify center pixel is red
    int centerX = 128;
    int centerY = 96;
    uint32_t centerPixel = gNdsGfx.g3.bg0Readback[centerY * 256 + centerX];

    uint8_t r = centerPixel & 0xFF;
    uint8_t a = (centerPixel >> 24) & 0xFF;
    ASSERT_TRUE(r > 200);
    ASSERT_TRUE(a > 200);

    G3Pipeline_Destroy(&gNdsGfx.g3);
    SDL_GL_DeleteContext(ctx);
    SDL_DestroyWindow(window);
}
