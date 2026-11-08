/**
 * @file framebuffer.cpp
 * @brief Offscreen framebuffer management for Nintendo DS Top and Touchscreen viewports.
 */

#include "framebuffer.h"
#include <SDL.h>
#include <SDL_opengl.h>
#include <cstring>

namespace {

DualFramebuffer gDualFbo{};

bool HasActiveGLContext() {
    return SDL_GL_GetCurrentContext() != nullptr;
}

void CreateScreenTexture(uint32_t* textureId) {
    if (!HasActiveGLContext()) return;

    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, NDS_SCREEN_WIDTH, NDS_SCREEN_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glBindTexture(GL_TEXTURE_2D, 0);

    *textureId = static_cast<uint32_t>(tex);
}

} // anonymous namespace

extern "C" {

bool Framebuffer_Init(void) {
    memset(&gDualFbo, 0, sizeof(DualFramebuffer));

    if (HasActiveGLContext()) {
        CreateScreenTexture(&gDualFbo.topTexture);
        CreateScreenTexture(&gDualFbo.touchTexture);
        gDualFbo.glInitialized = true;
    }

    return true;
}

void Framebuffer_UpdateTopScreen(const void* pixels) {
    if (!pixels) return;
    memcpy(gDualFbo.topPixels, pixels, sizeof(gDualFbo.topPixels));

    if (HasActiveGLContext()) {
        if (!gDualFbo.topTexture) {
            CreateScreenTexture(&gDualFbo.topTexture);
        }
        glBindTexture(GL_TEXTURE_2D, gDualFbo.topTexture);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, NDS_SCREEN_WIDTH, NDS_SCREEN_HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}

void Framebuffer_UpdateTouchScreen(const void* pixels) {
    if (!pixels) return;
    memcpy(gDualFbo.touchPixels, pixels, sizeof(gDualFbo.touchPixels));

    if (HasActiveGLContext()) {
        if (!gDualFbo.touchTexture) {
            CreateScreenTexture(&gDualFbo.touchTexture);
        }
        glBindTexture(GL_TEXTURE_2D, gDualFbo.touchTexture);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, NDS_SCREEN_WIDTH, NDS_SCREEN_HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}

uint32_t Framebuffer_GetTopTexture(void) {
    return gDualFbo.topTexture;
}

uint32_t Framebuffer_GetTouchTexture(void) {
    return gDualFbo.touchTexture;
}

uint32_t* Framebuffer_GetTopPixelBuffer(void) {
    return gDualFbo.topPixels;
}

uint32_t* Framebuffer_GetTouchPixelBuffer(void) {
    return gDualFbo.touchPixels;
}

void Framebuffer_BindTopFbo(void) {
    // Used when 3D geometry engine renders directly into Top FBO
}

void Framebuffer_BindTouchFbo(void) {
    // Used when rendering into Touch FBO
}

void Framebuffer_UnbindFbo(void) {
    // Reset back to main window backbuffer
}

void Framebuffer_Shutdown(void) {
    if (HasActiveGLContext()) {
        if (gDualFbo.topTexture) {
            GLuint tex = gDualFbo.topTexture;
            glDeleteTextures(1, &tex);
        }
        if (gDualFbo.touchTexture) {
            GLuint tex = gDualFbo.touchTexture;
            glDeleteTextures(1, &tex);
        }
    }
    memset(&gDualFbo, 0, sizeof(DualFramebuffer));
}

} // extern "C"
