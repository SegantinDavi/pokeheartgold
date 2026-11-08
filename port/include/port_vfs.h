/**
 * @file port_vfs.h
 * @brief Native Virtual Filesystem (VFS) and NitroFS abstraction.
 */

#ifndef PORT_VFS_H
#define PORT_VFS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <nitro/types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum FSSeekFileMode {
    FS_SEEK_SET = 0,
    FS_SEEK_CUR = 1,
    FS_SEEK_END = 2
} FSSeekFileMode;

typedef struct FSFile {
    uint8_t opaque[128];
    struct {
        struct {
            u32 own_id;
            u32 top;
            u32 bottom;
            u32 pos;
        } file;
    } prop;
} FSFile;

/* --- Native VFS Host APIs --- */
bool NativeVFS_Init(const char* baseAssetDir);
const char* NativeVFS_ResolvePath(const char* ndsPath);
void NativeVFS_Shutdown(void);

/* --- NitroSDK FS Emulation APIs --- */
void FS_Init(u32 default_dma_no);
BOOL FS_IsAvailable(void);
void FS_InitFile(FSFile* p_file);
BOOL FS_OpenFile(FSFile* p_file, const char* path);
int FS_ReadFile(FSFile* p_file, void* dst, s32 len);
BOOL FS_SeekFile(FSFile* p_file, s32 offset, FSSeekFileMode origin);
BOOL FS_CloseFile(FSFile* p_file);
u32 FS_GetLength(const FSFile* p_file);

#ifdef __cplusplus
}
#endif

#endif /* PORT_VFS_H */
