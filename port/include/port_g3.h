/**
 * @file port_g3.h
 * @brief 3D Geometry Fixed-Function Pipeline (G3) interface.
 */

#ifndef PORT_G3_H
#define PORT_G3_H

#include <stdint.h>
#include <stdbool.h>
#include <nitro/types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef s32 fx32;
typedef s16 fx16;

#define FX32_SHIFT 12
#define FX32_TO_FLOAT(x) (static_cast<float>(x) / static_cast<float>(1 << FX32_SHIFT))

typedef enum {
    GX_MTXMODE_PROJECTION      = 0,
    GX_MTXMODE_POSITION        = 1,
    GX_MTXMODE_POSITION_VECTOR = 2,
    GX_MTXMODE_TEXTURE         = 3
} GXMtxMode;

typedef enum {
    GX_BEGIN_TRIANGLES      = 0,
    GX_BEGIN_QUADS          = 1,
    GX_BEGIN_TRIANGLE_STRIP = 2,
    GX_BEGIN_QUAD_STRIP     = 3
} GXBeginMode;

void G3_Init(void);
void G3_MtxMode(GXMtxMode mode);
GXMtxMode G3_GetMtxMode(void);
void G3_Identity(void);
void G3_Translate(fx32 x, fx32 y, fx32 z);
void G3_Scale(fx32 x, fx32 y, fx32 z);
void G3_Color(uint16_t rgb555);
void G3_Normal(fx16 x, fx16 y, fx16 z);
void G3_TexCoord(fx32 s, fx32 t);
void G3_Vtx(fx16 x, fx16 y, fx16 z);
void G3_Begin(GXBeginMode mode);
void G3_End(void);

#ifdef __cplusplus
}
#endif

#endif // PORT_G3_H
