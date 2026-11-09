/**
 * @file nnsys.h
 * @brief NitroSystem forwarder and types for native desktop compilation.
 */

#ifndef PORT_NNSYS_H
#define PORT_NNSYS_H

#include "nitro.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Foundations (FND) Heaps & Allocators */
#include <nnsys/fnd.h>

/* G3D Types */
typedef u32 NNSG3dTexKey;
typedef u32 NNSG3dPlttKey;
typedef struct NNSG3dResTex NNSG3dResTex;
typedef struct NNSG3dResFileHeader NNSG3dResFileHeader;

/* G2D Basic Enums & Data Types */
typedef enum {
    NNS_G2D_VRAM_TYPE_2DMAIN = 0,
    NNS_G2D_VRAM_TYPE_2DSUB  = 1,
    NNS_G2D_VRAM_TYPE_2DBOTH = 2,
    NNS_G2D_VRAM_TYPE_3DMAIN = 3,
    NNS_G2D_VRAM_TYPE_MAX    = 4
} NNS_G2D_VRAM_TYPE;

typedef struct NNSG2dCharacterData NNSG2dCharacterData;
typedef struct NNSG2dPaletteData NNSG2dPaletteData;
typedef struct NNSG2dCellDataBank NNSG2dCellDataBank;
typedef struct NNSG2dCellData NNSG2dCellData;
typedef struct NNSG2dCellAnimBankData NNSG2dCellAnimBankData;
typedef struct NNSG2dMultiCellDataBank NNSG2dMultiCellDataBank;
typedef struct NNSG2dMultiCellData NNSG2dMultiCellData;
typedef struct NNSG2dMultiCellAnimBankData NNSG2dMultiCellAnimBankData;
typedef struct NNSG2dAnimBankData NNSG2dAnimBankData;
typedef struct NNSG2dScreenData NNSG2dScreenData;
typedef struct NNSG2dPaletteSwapTable NNSG2dPaletteSwapTable;
typedef struct NNSG2dNode NNSG2dNode;
typedef struct NNSG2dAnimSequence NNSG2dAnimSequence;

/* Concrete G2D Structs */
typedef struct NNSG2dVRamLocation {
    u32 baseAddrOfVram[4];
} NNSG2dVRamLocation;

typedef struct NNSG2dImageAttr {
    u32 sizeS;
    u32 sizeT;
    u32 fmt;
    BOOL bExtendedPlt;
    u32 plttUse;
    u32 mappingType;
} NNSG2dImageAttr;

typedef struct NNSG2dImageProxy {
    NNSG2dVRamLocation vramLocation;
    NNSG2dImageAttr attr;
} NNSG2dImageProxy;

typedef struct NNSG2dImagePaletteProxy {
    u32 fmt;
    BOOL bExtendedPlt;
    NNSG2dVRamLocation vramLocation;
} NNSG2dImagePaletteProxy;

typedef struct NNSG2dSRTControl {
    u32 srtFlag;
    fx32 scaleX;
    fx32 scaleY;
    u16 rotZ;
} NNSG2dSRTControl;

typedef struct NNSG2dAnimController {
    void* pCurrent;
    void* pAnimSequence;
    u16 currentFrameIdx;
    u16 currentAnimSequenceIdx;
    fx32 currentTime;
    fx32 speed;
    u32 flags;
} NNSG2dAnimController;

typedef struct NNSG2dCellAnimation {
    NNSG2dAnimController animCtrl;
    const NNSG2dCellData *pCurrentCell;
    const NNSG2dCellDataBank *pCellDataBank;
    u32 cellTransferStateHandle;
    NNSG2dSRTControl srtCtrl;
} NNSG2dCellAnimation;

typedef struct NNSG2dMultiCellAnimation {
    NNSG2dAnimController animCtrl;
    u16 totalVideoFrame;
    u16 pad16;
    struct {
        const NNSG2dMultiCellData *pCurrentMultiCell;
        const NNSG2dCellAnimBankData *pAnimDataBank;
        u32 mcType;
        void *pCellAnimInstasnces;
    } multiCellInstance;
    const NNSG2dMultiCellDataBank *pMultiCellDataBank;
    NNSG2dSRTControl srtCtrl;
} NNSG2dMultiCellAnimation;

typedef struct NNSG2dViewRect {
    int x0, y0, x1, y1;
} NNSG2dViewRect;

typedef struct NNSG2dRenderSurface {
    uint8_t opaque[128];
} NNSG2dRenderSurface;

typedef struct NNSG2dRendererInstance {
    uint8_t opaque[256];
} NNSG2dRendererInstance;

/* SND Types */
typedef struct NNSSndHandle NNSSndHandle;
typedef struct NNSSndHeapHandle NNSSndHeapHandle;
typedef struct SNDChannelInfo SNDChannelInfo;
typedef struct SNDTrackInfo SNDTrackInfo;
typedef struct NNSSndWaveOut* NNSSndWaveOutHandle;
typedef enum NNSSndWaveFormat {
    NNS_SND_WAVE_FORMAT_PCM8,
    NNS_SND_WAVE_FORMAT_PCM16
} NNSSndWaveFormat;

#ifdef __cplusplus
}
#endif

#endif // PORT_NNSYS_H
