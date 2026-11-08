/**
 * @file framebuffer.h
 * @brief Dual offscreen framebuffer management (Top Screen and Touchscreen FBOs).
 */

#ifndef PORT_FRAMEBUFFER_H
#define PORT_FRAMEBUFFER_H

#include <stdint.h>
#include <stdbool.h>
#include "port_common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct DualFramebuffer {
    uint32_t topTexture;
    uint32_t touchTexture;
    uint32_t topFbo;
    uint32_t touchFbo;
    uint32_t topRbo;
    uint32_t touchRbo;
    uint32_t topPixels[NDS_SCREEN_WIDTH * NDS_SCREEN_HEIGHT];
    uint32_t touchPixels[NDS_SCREEN_WIDTH * NDS_SCREEN_HEIGHT];
    bool glInitialized;
} DualFramebuffer;

bool Framebuffer_Init(void);
void Framebuffer_UpdateTopScreen(const void* pixels);
void Framebuffer_UpdateTouchScreen(const void* pixels);
uint32_t Framebuffer_GetTopTexture(void);
uint32_t Framebuffer_GetTouchTexture(void);
uint32_t* Framebuffer_GetTopPixelBuffer(void);
uint32_t* Framebuffer_GetTouchPixelBuffer(void);
void Framebuffer_BindTopFbo(void);
void Framebuffer_BindTouchFbo(void);
void Framebuffer_UnbindFbo(void);
void Framebuffer_Shutdown(void);

#ifdef __cplusplus
}
#endif

#endif // PORT_FRAMEBUFFER_H
