/**
 * @file nds_texture_cache.h
 * @brief Contract: NDS VRAM texture decoder and OpenGL texture cache.
 *
 * Lives in: port/hal/g3/nds_texture_cache.cpp + port/include/nds_texture_cache.h
 *
 * Converts NDS texture formats (A3I5, 4-color, 16-color, 256-color,
 * 4x4-compressed, A5I3, Direct) to RGBA8 and manages a GL texture object cache
 * keyed by VRAM slot + palette offset. Cache is flushed at GX_Init or when
 * VRAM bank remapping is detected.
 */

#ifndef NDS_TEXTURE_CACHE_H
#define NDS_TEXTURE_CACHE_H

#include <stdint.h>
#include <stdbool.h>
#include <SDL_opengl.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Upload a texture to the GL cache (or reuse existing entry).
 *
 * @param vramBase       Pointer to full VRAM region (gHwLcdcVram).
 * @param vramOffset     Byte offset of texture data within VRAM.
 * @param format         NDS texture format (0–7).
 * @param width          Texture width in pixels (8–1024, power of 2).
 * @param height         Texture height in pixels.
 * @param paletteBase    Pointer to palette region.
 * @param paletteOffset  Byte offset of palette within paletteBase.
 * @return               GL texture object name ready for glBindTexture.
 */
GLuint NdsTexCache_Upload(const uint8_t* vramBase, uint32_t vramOffset,
                           uint8_t format, uint16_t width, uint16_t height,
                           const uint8_t* paletteBase, uint32_t paletteOffset);

/**
 * Bind the cached texture for the current GL draw call.
 * Equivalent to glBindTexture(GL_TEXTURE_2D, cached_id).
 */
void NdsTexCache_Bind(GLuint texId);

/**
 * Invalidate all cached GL textures (called on GX_Init or VRAM bank remap).
 * Frees all glDeleteTextures entries.
 */
void NdsTexCache_Flush(void);

#ifdef __cplusplus
}
#endif

#endif /* NDS_TEXTURE_CACHE_H */
