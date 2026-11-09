/**
 * @file nitro.h
 * @brief Master NitroSDK forwarder shim for native desktop compilation.
 */

#ifndef PORT_NITRO_H
#define PORT_NITRO_H

#include <nitro/types.h>
#include "port_common.h"
#include "port_os.h"
#include "port_vfs.h"
#include "port_card.h"
#include "port_snd.h"
#include "port_gx.h"
#include "port_g3.h"
#include "port_overlay.h"

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
