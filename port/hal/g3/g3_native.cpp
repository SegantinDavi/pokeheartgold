/**
 * @file g3_native.cpp
 * @brief Fixed-function 3D geometry pipeline translation to OpenGL Core/Compatibility.
 */

#include "port_g3.h"
#include <SDL.h>
#include <SDL_opengl.h>

namespace {

GXMtxMode gCurrentMtxMode = GX_MTXMODE_POSITION_VECTOR;

bool HasActiveGLContext() {
    return SDL_GL_GetCurrentContext() != nullptr;
}

GLenum MapMatrixMode(GXMtxMode mode) {
    switch (mode) {
        case GX_MTXMODE_PROJECTION:      return GL_PROJECTION;
        case GX_MTXMODE_POSITION:
        case GX_MTXMODE_POSITION_VECTOR: return GL_MODELVIEW;
        case GX_MTXMODE_TEXTURE:         return GL_TEXTURE;
        default:                         return GL_MODELVIEW;
    }
}

GLenum MapBeginMode(GXBeginMode mode) {
    switch (mode) {
        case GX_BEGIN_TRIANGLES:      return GL_TRIANGLES;
        case GX_BEGIN_QUADS:          return GL_QUADS;
        case GX_BEGIN_TRIANGLE_STRIP: return GL_TRIANGLE_STRIP;
        case GX_BEGIN_QUAD_STRIP:     return GL_QUAD_STRIP;
        default:                      return GL_TRIANGLES;
    }
}

} // anonymous namespace

extern "C" {

void G3_Init(void) {
    gCurrentMtxMode = GX_MTXMODE_POSITION_VECTOR;
}

void G3_MtxMode(GXMtxMode mode) {
    gCurrentMtxMode = mode;
    if (HasActiveGLContext()) {
        glMatrixMode(MapMatrixMode(mode));
    }
}

GXMtxMode G3_GetMtxMode(void) {
    return gCurrentMtxMode;
}

void G3_Identity(void) {
    if (HasActiveGLContext()) {
        glLoadIdentity();
    }
}

void G3_Translate(fx32 x, fx32 y, fx32 z) {
    if (HasActiveGLContext()) {
        glTranslatef(FX32_TO_FLOAT(x), FX32_TO_FLOAT(y), FX32_TO_FLOAT(z));
    }
}

void G3_Scale(fx32 x, fx32 y, fx32 z) {
    if (HasActiveGLContext()) {
        glScalef(FX32_TO_FLOAT(x), FX32_TO_FLOAT(y), FX32_TO_FLOAT(z));
    }
}

void G3_Color(uint16_t rgb555) {
    if (HasActiveGLContext()) {
        float r = ((rgb555 >> 0) & 0x1F) / 31.0f;
        float g = ((rgb555 >> 5) & 0x1F) / 31.0f;
        float b = ((rgb555 >> 10) & 0x1F) / 31.0f;
        glColor4f(r, g, b, 1.0f);
    }
}

void G3_Normal(fx16 x, fx16 y, fx16 z) {
    if (HasActiveGLContext()) {
        glNormal3f(FX32_TO_FLOAT(x), FX32_TO_FLOAT(y), FX32_TO_FLOAT(z));
    }
}

void G3_TexCoord(fx32 s, fx32 t) {
    if (HasActiveGLContext()) {
        glTexCoord2f(FX32_TO_FLOAT(s), FX32_TO_FLOAT(t));
    }
}

void G3_Vtx(fx16 x, fx16 y, fx16 z) {
    if (HasActiveGLContext()) {
        glVertex3f(FX32_TO_FLOAT(x), FX32_TO_FLOAT(y), FX32_TO_FLOAT(z));
    }
}

void G3_Begin(GXBeginMode mode) {
    if (HasActiveGLContext()) {
        glBegin(MapBeginMode(mode));
    }
}

void G3_End(void) {
    if (HasActiveGLContext()) {
        glEnd();
    }
}

} // extern "C"
