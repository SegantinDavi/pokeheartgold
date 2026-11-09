/**
 * @file nitro.h
 * @brief Master NitroSDK forwarder shim for native desktop compilation.
 */

#ifndef PORT_NITRO_H
#define PORT_NITRO_H

#include <nitro/types.h>
#include <string.h>
#include <stdlib.h>
#include "port_common.h"
#include "port_os.h"
#include "port_vfs.h"
#include "port_card.h"
#include "port_snd.h"
#include "port_gx.h"
#include "port_g3.h"
#include "port_overlay.h"
#include "port_hw_registers.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Arena identifiers */
typedef enum {
    OS_ARENA_MAIN = 0,
    OS_ARENA_MAIN_SUBPRIV = 1,
    OS_ARENA_MAINEX = 2,
    OS_ARENA_ITCM = 3,
    OS_ARENA_DTCM = 4,
    OS_ARENA_SHARED = 5,
    OS_ARENA_WRAM_MAIN = 6,
    OS_ARENA_WRAM_SUB = 7,
    OS_ARENA_WRAM_SUBPRIV = 8,
    OS_ARENA_MAX = 9
} OSArenaId;

/* PAD (Keypad) Definitions */
#define PAD_PLUS_KEY_MASK       0x00f0
#define PAD_BUTTON_MASK         0x2f0f
#define PAD_DEBUG_BUTTON_MASK   0x2000
#define PAD_ALL_MASK            0x2fff
#define PAD_RCNTPORT_MASK       0x2c00
#define PAD_KEYPORT_MASK        0x03ff
#define PAD_DETECT_FOLD_MASK    0x8000

#define PAD_BUTTON_A            0x0001
#define PAD_BUTTON_B            0x0002
#define PAD_BUTTON_SELECT       0x0004
#define PAD_BUTTON_START        0x0008
#define PAD_KEY_RIGHT           0x0010
#define PAD_KEY_LEFT            0x0020
#define PAD_KEY_UP              0x0040
#define PAD_KEY_DOWN            0x0080
#define PAD_BUTTON_R            0x0100
#define PAD_BUTTON_L            0x0200
#define PAD_BUTTON_X            0x0400
#define PAD_BUTTON_Y            0x0800
#define PAD_BUTTON_DEBUG        0x2000

/* TP (Touchpad) Definitions */
#define TP_TOUCH_OFF            0
#define TP_TOUCH_ON             1
#define TP_VALIDITY_VALID       0
#define TP_VALIDITY_INVALID_X   1
#define TP_VALIDITY_INVALID_Y   2
#define TP_VALIDITY_INVALID_XY  (TP_VALIDITY_INVALID_X | TP_VALIDITY_INVALID_Y)

typedef struct TPData {
    u16 x;
    u16 y;
    u16 touch;
    u16 validity;
} TPData;

typedef struct TPCalibrateParam {
    s16 x0;
    s16 y0;
    s16 xDotSize;
    s16 yDotSize;
} TPCalibrateParam;

/* Power & Display Shims */
#define GX_POWER_ALL            0x00000001
#define GX_PLANEMASK_BG0        0x01
#define GX_PLANEMASK_BG1        0x02
#define GX_PLANEMASK_BG2        0x04
#define GX_PLANEMASK_BG3        0x08
#define GX_PLANEMASK_OBJ        0x10

#define GX_DISP_SELECT_MAIN_SUB 0
#define GX_VRAM_LCDC_ALL        0x000000FF
#define OS_IE_SPFIFO_RECV       (1UL << 29)

typedef u32 GXOamMode;
typedef u32 GXTexFmt;
typedef u32 GXTexSizeS;
typedef u32 GXTexSizeT;
typedef u32 GXOBJVRamModeChar;
typedef u32 GXBlendPlaneMask;
typedef u32 GXCull;
typedef u32 GXPolygonMode;
typedef struct GXOamAttr GXOamAttr;

typedef u32 GXVRamBG;
typedef u32 GXVRamBGExtPltt;
typedef u32 GXVRamSubBG;
typedef u32 GXVRamSubBGExtPltt;
typedef u32 GXVRamOBJ;
typedef u32 GXVRamOBJExtPltt;
typedef u32 GXVRamSubOBJ;
typedef u32 GXVRamSubOBJExtPltt;
typedef u32 GXVRamTex;
typedef u32 GXVRamTexPltt;
typedef u32 GXDispMode;
typedef u32 GXBGMode;
typedef u32 GXBG0As;
typedef u32 GXSortMode;
typedef u32 GXBufferMode;
typedef struct MICAutoParam MICAutoParam;

static inline u32 OS_GetResetParameter(void) { return 0; }

/* Power Management (PM) Definitions */
typedef enum {
    PM_BACKLIGHT_OFF = 0,
    PM_BACKLIGHT_ON = 1
} PMBackLightSwitch;

typedef enum {
    PM_LCD_TOP = 0,
    PM_LCD_BOTTOM = 1,
    PM_LCD_ALL = 2
} PMLCDTarget;

#define PM_TRIGGER_COVER_OPEN   0x01
#define PM_TRIGGER_CARD         0x02
#define PM_TRIGGER_CARTRIDGE    0x04
#define PM_PAD_LOGIC_OR         0

typedef u32 PMWakeUpTrigger;
typedef u32 PMLogic;

static inline void PM_Init(void) {}
static inline void PM_GetBackLight(PMBackLightSwitch *top, PMBackLightSwitch *bottom) {
    if (top) *top = PM_BACKLIGHT_ON;
    if (bottom) *bottom = PM_BACKLIGHT_ON;
}
static inline u32 PM_SetBackLight(PMLCDTarget target, PMBackLightSwitch status) {
    (void)target; (void)status; return 0;
}
static inline void PM_GoSleepMode(PMWakeUpTrigger trigger, PMLogic logic, u16 keyPattern) {
    (void)trigger; (void)logic; (void)keyPattern;
}
static inline u32 PM_ForceToPowerOff(void) { return 0; }
static inline BOOL CTRDG_IsPulledOut(void) { return FALSE; }
static inline BOOL CARD_IsPulledOut(void) { return FALSE; }
static inline BOOL CARD_TryWaitBackupAsync(void) { return TRUE; }
static inline void OS_ResetSystem(u32 param) { (void)param; }

static inline void GX_SetDispSelect(int val) { (void)val; }
static inline void GX_VBlankIntr(BOOL enable) { (void)enable; }
static inline void GX_SetBankForLCDC(u32 bank) { (void)bank; }
static inline void GX_DisableBankForLCDC(void) {}

static inline u32 FS_TryLoadTable(void *p_mem, u32 size) {
    (void)p_mem;
    (void)size;
    return 0;
}

static inline void CARD_SetCacheFlushThreshold(u32 a, u32 b) {
    (void)a;
    (void)b;
}

u16 PAD_Read(void);
static inline BOOL PAD_DetectFold(void) { return FALSE; }

static inline void TP_Init(void) {}
static inline BOOL TP_GetUserInfo(TPCalibrateParam *param) { (void)param; return TRUE; }
static inline void TP_SetCalibrateParam(const TPCalibrateParam *param) { (void)param; }
static inline u32 TP_RequestRawSampling(TPData *result) { (void)result; return 0; }
void TP_GetLatestRawPointInAuto(TPData *result);
void TP_GetCalibratedPoint(TPData *calib, const TPData *raw);

/* Entropy & Math */
#define OS_LOW_ENTROPY_DATA_SIZE 32
#define MATH_MD5_DIGEST_SIZE     16

static inline void OS_GetLowEntropyData(u32 buffer[OS_LOW_ENTROPY_DATA_SIZE / sizeof(u32)]) {
    for (size_t i = 0; i < OS_LOW_ENTROPY_DATA_SIZE / sizeof(u32); ++i) {
        buffer[i] = (u32)(0x12345678U ^ (i * 0x9E3779B9U));
    }
}

static inline void MATH_CalcMD5(void *digest, const void *data, u32 dataLength) {
    (void)data;
    (void)dataLength;
    if (digest) {
        memset(digest, 0x5A, MATH_MD5_DIGEST_SIZE);
    }
}

static inline void OS_Init(void) {}
static inline void GX_SetPower(u32 flags) { (void)flags; }
static inline void GX_HBlankIntr(BOOL enable) { (void)enable; }
static inline void GXx_SetMasterBrightness_(vu16 *reg, s16 brightness) {
    if (reg) *reg = (u16)brightness;
}

void* OS_AllocFromArenaLo(OSArenaId id, u32 size, u32 align);
void* OS_AllocFromArenaHi(OSArenaId id, u32 size, u32 align);

/* Essential NitroSDK aliases and macros used throughout decompiled code */
#ifndef OS_IE_VBLANK
#define OS_IE_VBLANK OS_IE_V_BLANK
#endif

#ifndef OS_IE_HBLANK
#define OS_IE_HBLANK OS_IE_H_BLANK
#endif

#ifndef GX_DEFAULT_DMAID
#define GX_DEFAULT_DMAID 3
#endif

static inline void MI_WaitDma(u32 dmaNo) {
    (void)dmaNo;
}

static inline void OS_SetIrqCheckFlag(OSIrqMask intr) {
    (void)intr;
}

#ifdef __cplusplus
}
#endif

#endif // PORT_NITRO_H
