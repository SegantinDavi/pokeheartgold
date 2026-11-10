/**
 * @file port_g3.h
 * @brief 3D Geometry Fixed-Function Pipeline (G3) interface.
 */

#ifndef PORT_G3_H
#define PORT_G3_H

#include <stdint.h>
#include <stdbool.h>
#include <nitro/types.h>
#include <nitro/fx.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef FX32_TO_FLOAT
#define FX32_TO_FLOAT(x) ((float)(x) / (float)(1 << FX32_SHIFT))
#endif

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

typedef u32 GXSortMode;
typedef u32 GXBufferMode;
typedef u32 GXTexFmt;
typedef u32 GXTexGen;
typedef u32 GXTexSizeS;
typedef u32 GXTexSizeT;

#define GX_TEXREPEAT_NONE 0
#define GX_TEXREPEAT_S    1
#define GX_TEXREPEAT_T    2
#define GX_TEXREPEAT_ST   3

typedef u32 GXTexRepeat;
typedef u32 GXTexFlip;
typedef u32 GXTexPlttColor0;

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
