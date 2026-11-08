/**
 * @file port_card.h
 * @brief Persistent 512KB EEPROM/Flash backup manager and NitroSDK CARD emulation.
 */

#ifndef PORT_CARD_H
#define PORT_CARD_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <nitro/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NDS_FLASH_BACKUP_SIZE (512 * 1024) // 512 KB
#define DEFAULT_SAVE_FILE_PATH "pokemon_heartgold.sav"

typedef enum {
    CARD_RESULT_SUCCESS = 0,
    CARD_RESULT_FAILURE = 1,
    CARD_RESULT_TIMEOUT = 2
} CardResult;

/* --- Native Host Save APIs --- */
bool NativeCard_Init(const char* saveFilePath);
bool NativeCard_ReadBackup(uint32_t srcOffset, void* dst, size_t length);
bool NativeCard_WriteBackup(uint32_t dstOffset, const void* src, size_t length);
bool NativeCard_FlushBackup(void);
uint8_t* NativeCard_GetBuffer(void);
void NativeCard_Shutdown(void);

/* --- NitroSDK CARD Emulation APIs --- */
void CARD_Init(void);
void CARD_LockBackup(s32 id);
void CARD_UnlockBackup(s32 id);
BOOL CARD_ReadBackup(u32 offset, void* dst, u32 sz);
void CARD_ReadBackupAsync(u32 offset, void* dst, u32 sz, void (*callback)(void*), void* arg);
void CARD_WriteAndVerifyBackupAsync(u32 offset, void* src, u32 sz, void (*callback)(void*), void* arg);
BOOL CARD_WaitBackupAsync(void);
s32 CARD_GetResultCode(void);

#ifdef __cplusplus
}
#endif

#endif // PORT_CARD_H
