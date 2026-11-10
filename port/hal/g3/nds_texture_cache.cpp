/**
 * @file nds_texture_cache.cpp
 * @brief NDS VRAM texture decoder and OpenGL texture cache.
 */

#define GL_GLEXT_PROTOTYPES 1
#include "nds_texture_cache.h"
#include "nds_compositor.h"
#include "port_gx.h"
#include <SDL.h>
#include <SDL_opengl.h>
#include <unordered_map>
#include <vector>
#include <cstring>

namespace {

std::unordered_map<uint64_t, GLuint> s_texCache;

bool HasActiveGLContext() {
    return SDL_GL_GetCurrentContext() != nullptr;
}

uint16_t ReadRGB555(const uint8_t* pltt, uint32_t offset, int idx) {
    if (!pltt) return 0;
    const uint16_t* p = reinterpret_cast<const uint16_t*>(pltt + offset);
    return p[idx];
}

void DecodeFormat_A3I5(const uint8_t* src, uint16_t width, uint16_t height,
                       const uint8_t* pltt, uint32_t plttOffset,
                       uint32_t* dst) {
    int totalPixels = width * height;
    for (int i = 0; i < totalPixels; ++i) {
        uint8_t byte = src[i];
        uint8_t idx = byte & 0x1F;
        uint8_t a = static_cast<uint8_t>(((byte >> 5) & 0x07) * 255 / 7);
        uint16_t rgb555 = ReadRGB555(pltt, plttOffset, idx);
        uint8_t r = static_cast<uint8_t>(((rgb555 >> 0) & 0x1F) * 255 / 31);
        uint8_t g = static_cast<uint8_t>(((rgb555 >> 5) & 0x1F) * 255 / 31);
        uint8_t b = static_cast<uint8_t>(((rgb555 >> 10) & 0x1F) * 255 / 31);
        dst[i] = GX_PackRGBA(r, g, b, a);
    }
}

void DecodeFormat_4Color(const uint8_t* src, uint16_t width, uint16_t height,
                         const uint8_t* pltt, uint32_t plttOffset,
                         uint32_t* dst) {
    int totalPixels = width * height;
    int srcIdx = 0;
    for (int i = 0; i < totalPixels; i += 4) {
        uint8_t byte = src[srcIdx++];
        for (int p = 0; p < 4 && (i + p) < totalPixels; ++p) {
            uint8_t idx = (byte >> (p * 2)) & 0x03;
            if (idx == 0) {
                dst[i + p] = NDS_PIXEL_TRANSPARENT;
            } else {
                uint16_t rgb555 = ReadRGB555(pltt, plttOffset, idx);
                dst[i + p] = GX_RGB555ToRGBA(rgb555);
            }
        }
    }
}

void DecodeFormat_16Color(const uint8_t* src, uint16_t width, uint16_t height,
                          const uint8_t* pltt, uint32_t plttOffset,
                          uint32_t* dst) {
    int totalPixels = width * height;
    int srcIdx = 0;
    for (int i = 0; i < totalPixels; i += 2) {
        uint8_t byte = src[srcIdx++];
        uint8_t idx0 = byte & 0x0F;
        uint8_t idx1 = (byte >> 4) & 0x0F;
        dst[i + 0] = (idx0 == 0) ? NDS_PIXEL_TRANSPARENT : GX_RGB555ToRGBA(ReadRGB555(pltt, plttOffset, idx0));
        if (i + 1 < totalPixels) {
            dst[i + 1] = (idx1 == 0) ? NDS_PIXEL_TRANSPARENT : GX_RGB555ToRGBA(ReadRGB555(pltt, plttOffset, idx1));
        }
    }
}

void DecodeFormat_256Color(const uint8_t* src, uint16_t width, uint16_t height,
                           const uint8_t* pltt, uint32_t plttOffset,
                           uint32_t* dst) {
    int totalPixels = width * height;
    for (int i = 0; i < totalPixels; ++i) {
        uint8_t idx = src[i];
        dst[i] = (idx == 0) ? NDS_PIXEL_TRANSPARENT : GX_RGB555ToRGBA(ReadRGB555(pltt, plttOffset, idx));
    }
}

void DecodeFormat_4x4Compressed(const uint8_t* src, uint16_t width, uint16_t height,
                                const uint8_t* pltt, uint32_t plttOffset,
                                uint32_t* dst) {
    // 4x4 texel compressed:
    // width and height are multiples of 4
    int blocksX = width / 4;
    int blocksY = height / 4;
    int blockIdx = 0;

    for (int by = 0; by < blocksY; ++by) {
        for (int bx = 0; bx < blocksX; ++bx) {
            uint32_t texels = *reinterpret_cast<const uint32_t*>(src + blockIdx * 4);
            // Palette index data offset
            uint16_t plttData = *reinterpret_cast<const uint16_t*>(pltt + plttOffset + blockIdx * 2);
            uint16_t pOffset = (plttData & 0x3FFF) * 4;
            int mode = (plttData >> 14) & 0x3;

            uint16_t c0 = ReadRGB555(pltt, pOffset, 0);
            uint16_t c1 = ReadRGB555(pltt, pOffset, 1);
            uint32_t colorTable[4];
            colorTable[0] = GX_RGB555ToRGBA(c0);
            colorTable[1] = GX_RGB555ToRGBA(c1);

            if (mode == 0) {
                // c0, c1, c2=(c0+c1)/2, c3=transparent
                colorTable[2] = NdsCompositor_AlphaBlend(colorTable[0], colorTable[1], 8, 8);
                colorTable[3] = NDS_PIXEL_TRANSPARENT;
            } else if (mode == 1) {
                // c0, c1, c2=(2*c0+c1)/3, c3=(c0+2*c1)/3
                colorTable[2] = NdsCompositor_AlphaBlend(colorTable[0], colorTable[1], 11, 5);
                colorTable[3] = NdsCompositor_AlphaBlend(colorTable[0], colorTable[1], 5, 11);
            } else if (mode == 2) {
                // c0, c1, c2, c3=transparent
                uint16_t c2 = ReadRGB555(pltt, pOffset, 2);
                colorTable[2] = GX_RGB555ToRGBA(c2);
                colorTable[3] = NDS_PIXEL_TRANSPARENT;
            } else {
                // c0, c1, c2, c3
                uint16_t c2 = ReadRGB555(pltt, pOffset, 2);
                uint16_t c3 = ReadRGB555(pltt, pOffset, 3);
                colorTable[2] = GX_RGB555ToRGBA(c2);
                colorTable[3] = GX_RGB555ToRGBA(c3);
            }

            for (int py = 0; py < 4; ++py) {
                for (int px = 0; px < 4; ++px) {
                    int shift = (py * 4 + px) * 2;
                    uint8_t cIdx = (texels >> shift) & 0x3;
                    int dstX = bx * 4 + px;
                    int dstY = by * 4 + py;
                    if (dstX < width && dstY < height) {
                        dst[dstY * width + dstX] = colorTable[cIdx];
                    }
                }
            }
            blockIdx++;
        }
    }
}

void DecodeFormat_A5I3(const uint8_t* src, uint16_t width, uint16_t height,
                       const uint8_t* pltt, uint32_t plttOffset,
                       uint32_t* dst) {
    int totalPixels = width * height;
    for (int i = 0; i < totalPixels; ++i) {
        uint8_t byte = src[i];
        uint8_t idx = byte & 0x07;
        uint8_t a = static_cast<uint8_t>(((byte >> 3) & 0x1F) * 255 / 31);
        uint16_t rgb555 = ReadRGB555(pltt, plttOffset, idx);
        uint8_t r = static_cast<uint8_t>(((rgb555 >> 0) & 0x1F) * 255 / 31);
        uint8_t g = static_cast<uint8_t>(((rgb555 >> 5) & 0x1F) * 255 / 31);
        uint8_t b = static_cast<uint8_t>(((rgb555 >> 10) & 0x1F) * 255 / 31);
        dst[i] = GX_PackRGBA(r, g, b, a);
    }
}

void DecodeFormat_Direct(const uint8_t* src, uint16_t width, uint16_t height,
                         uint32_t* dst) {
    int totalPixels = width * height;
    const uint16_t* src16 = reinterpret_cast<const uint16_t*>(src);
    for (int i = 0; i < totalPixels; ++i) {
        uint16_t val = src16[i];
        uint8_t r = static_cast<uint8_t>(((val >> 0) & 0x1F) * 255 / 31);
        uint8_t g = static_cast<uint8_t>(((val >> 5) & 0x1F) * 255 / 31);
        uint8_t b = static_cast<uint8_t>(((val >> 10) & 0x1F) * 255 / 31);
        uint8_t a = (val & 0x8000) ? 255 : 0;
        dst[i] = GX_PackRGBA(r, g, b, a);
    }
}

} // anonymous namespace

extern "C" {

GLuint NdsTexCache_Upload(const uint8_t* vramBase, uint32_t vramOffset,
                          uint8_t format, uint16_t width, uint16_t height,
                          const uint8_t* paletteBase, uint32_t paletteOffset) {
    if (format == 0 || width == 0 || height == 0 || !vramBase) {
        return 0;
    }

    uint64_t key = (static_cast<uint64_t>(vramOffset) << 32) | paletteOffset;
    auto it = s_texCache.find(key);
    if (it != s_texCache.end()) {
        return it->second;
    }

    std::vector<uint32_t> rgbaPixels(width * height, 0);
    const uint8_t* src = vramBase + vramOffset;

    switch (format) {
        case 1: // A3I5
            DecodeFormat_A3I5(src, width, height, paletteBase, paletteOffset, rgbaPixels.data());
            break;
        case 2: // 4-color
            DecodeFormat_4Color(src, width, height, paletteBase, paletteOffset, rgbaPixels.data());
            break;
        case 3: // 16-color
            DecodeFormat_16Color(src, width, height, paletteBase, paletteOffset, rgbaPixels.data());
            break;
        case 4: // 256-color
            DecodeFormat_256Color(src, width, height, paletteBase, paletteOffset, rgbaPixels.data());
            break;
        case 5: // 4x4 texel compressed
            DecodeFormat_4x4Compressed(src, width, height, paletteBase, paletteOffset, rgbaPixels.data());
            break;
        case 6: // A5I3
            DecodeFormat_A5I3(src, width, height, paletteBase, paletteOffset, rgbaPixels.data());
            break;
        case 7: // Direct color
            DecodeFormat_Direct(src, width, height, rgbaPixels.data());
            break;
        default:
            return 0;
    }

    GLuint texId = 0;
    if (HasActiveGLContext()) {
        glGenTextures(1, &texId);
        glBindTexture(GL_TEXTURE_2D, texId);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgbaPixels.data());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glBindTexture(GL_TEXTURE_2D, 0);
    } else {
        // Fallback non-zero handle for contextless tests
        static GLuint s_dummyTexId = 1000;
        texId = s_dummyTexId++;
    }

    s_texCache[key] = texId;
    return texId;
}

void NdsTexCache_Bind(GLuint texId) {
    if (HasActiveGLContext() && texId != 0) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, texId);
    }
}

void NdsTexCache_Flush(void) {
    if (HasActiveGLContext()) {
        for (auto& pair : s_texCache) {
            if (pair.second != 0) {
                glDeleteTextures(1, &pair.second);
            }
        }
    }
    s_texCache.clear();
}

} // extern "C"
