/**
 * @file port_overlay.h
 * @brief Overlay monolithic initialization table and symbol dispatch.
 */

#ifndef PORT_OVERLAY_H
#define PORT_OVERLAY_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <nitro/types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*OverlayInitFunc)(void);
typedef void (*OverlayCleanupFunc)(void);

typedef struct OverlayDescriptor {
    uint32_t id;
    const char* name;
    OverlayInitFunc initFunc;
    OverlayCleanupFunc cleanupFunc;
    bool isLoaded;
} OverlayDescriptor;

/* Native overlay registry management */
void NativeOverlay_Reset(void);
bool NativeOverlay_Register(uint32_t id, const char* name, OverlayInitFunc initFunc, OverlayCleanupFunc cleanupFunc);
bool NativeOverlay_Load(uint32_t id);
bool NativeOverlay_Unload(uint32_t id);
bool NativeOverlay_IsLoaded(uint32_t id);
const OverlayDescriptor* NativeOverlay_Find(uint32_t id);
size_t NativeOverlay_GetCount(void);

/* NitroSDK Forward Declarations & Types */
typedef u32 FSOverlayID;
typedef u32 MIProcessor;

#define FS_EXTERN_OVERLAY(name) extern u32 SDK_OVERLAY_##name##_ID[1]
#define FS_OVERLAY_ID(name)     ((FSOverlayID)(uintptr_t)SDK_OVERLAY_##name##_ID)

#define MI_PROCESSOR_ARM9 0
#define MI_PROCESSOR_ARM7 1

typedef struct FSOverlayInfoHeader {
    u32 id;
    u8* ram_address;
    u32 ram_size;
    u32 bss_size;
    void** sinit_init;
    void** sinit_init_end;
    u32 file_id;
    u32 compressed: 24;
    u32 flag: 8;
} FSOverlayInfoHeader;

typedef struct FSOverlayInfo {
    FSOverlayInfoHeader header;
    MIProcessor target;
    uint8_t file_pos[16];
} FSOverlayInfo;

/* NitroSDK Overlay API Emulation */
BOOL FS_LoadOverlayInfo(FSOverlayInfo* p_ovi, MIProcessor target, FSOverlayID id);
BOOL FS_LoadOverlayImageAsync(FSOverlayInfo* p_ovi, void* p_file);
BOOL FS_LoadOverlayImage(FSOverlayInfo* p_ovi);
BOOL FS_UnloadOverlayImage(FSOverlayInfo* p_ovi);
BOOL FS_LoadOverlay(MIProcessor target, FSOverlayID id);
BOOL FS_UnloadOverlay(MIProcessor target, FSOverlayID id);
void FS_StartOverlay(FSOverlayInfo* p_ovi);
void FS_EndOverlay(FSOverlayInfo* p_ovi);

#ifdef __cplusplus
}
#endif

#endif // PORT_OVERLAY_H
