/**
 * @file vfs_card.h
 * @brief Virtual Filesystem and Persistent Save Backup Contract
 */

#ifndef PORT_VFS_CARD_H
#define PORT_VFS_CARD_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NDS_FLASH_BACKUP_SIZE (512 * 1024) // 512 KB

/**
 * @brief Initialize Virtual Filesystem mapping host asset directories.
 * @param baseAssetDir Path to unpacked asset files (e.g., "./files").
 */
bool NativeVFS_Init(const char* baseAssetDir);

/**
 * @brief Resolve a Nintendo DS ROM filesystem path to a host file path.
 */
const char* NativeVFS_ResolvePath(const char* ndsPath);

/**
 * @brief Initialize persistent save backup system.
 * @param saveFilePath Path to the .sav file on disk (created if not present).
 */
bool NativeCard_Init(const char* saveFilePath);

/**
 * @brief Read bytes from persistent backup buffer into destination memory.
 */
bool NativeCard_ReadBackup(uint32_t srcOffset, void* dst, size_t length);

/**
 * @brief Write bytes from memory into backup buffer.
 */
bool NativeCard_WriteBackup(uint32_t dstOffset, const void* src, size_t length);

/**
 * @brief Flush backup buffer atomically to disk.
 */
bool NativeCard_FlushBackup(void);

/**
 * @brief Shutdown VFS and flush remaining backup writes.
 */
void NativeVFS_Shutdown(void);

#ifdef __cplusplus
}
#endif

#endif // PORT_VFS_CARD_H
