/**
 * @file card_backup.cpp
 * @brief Persistent 512KB save game manager with atomic file flushing.
 */

#include "port_card.h"
#include <cstdio>
#include <cstring>
#include <string>
#include <mutex>
#include <algorithm>

namespace {

uint8_t gSaveBuffer[NDS_FLASH_BACKUP_SIZE];
std::string gSavePath = DEFAULT_SAVE_FILE_PATH;
bool gIsDirty = false;
std::mutex gCardMutex;

} // anonymous namespace

extern "C" {

bool NativeCard_Init(const char* saveFilePath) {
    std::lock_guard<std::mutex> lock(gCardMutex);
    if (saveFilePath && saveFilePath[0] != '\0') {
        gSavePath = saveFilePath;
    } else {
        gSavePath = DEFAULT_SAVE_FILE_PATH;
    }

    // Initialize Flash buffer to 0xFF (unwritten state)
    memset(gSaveBuffer, 0xFF, sizeof(gSaveBuffer));
    gIsDirty = false;

    FILE* f = fopen(gSavePath.c_str(), "rb");
    if (f) {
        size_t readBytes = fread(gSaveBuffer, 1, sizeof(gSaveBuffer), f);
        (void)readBytes;
        fclose(f);
    }
    return true;
}

bool NativeCard_ReadBackup(uint32_t srcOffset, void* dst, size_t length) {
    if (!dst || length == 0 || srcOffset >= NDS_FLASH_BACKUP_SIZE) {
        return false;
    }

    std::lock_guard<std::mutex> lock(gCardMutex);
    size_t copyLen = std::min(length, static_cast<size_t>(NDS_FLASH_BACKUP_SIZE - srcOffset));
    memcpy(dst, gSaveBuffer + srcOffset, copyLen);
    return true;
}

bool NativeCard_WriteBackup(uint32_t dstOffset, const void* src, size_t length) {
    if (!src || length == 0 || dstOffset >= NDS_FLASH_BACKUP_SIZE) {
        return false;
    }

    std::lock_guard<std::mutex> lock(gCardMutex);
    size_t copyLen = std::min(length, static_cast<size_t>(NDS_FLASH_BACKUP_SIZE - dstOffset));
    memcpy(gSaveBuffer + dstOffset, src, copyLen);
    gIsDirty = true;
    return true;
}

bool NativeCard_FlushBackup(void) {
    std::lock_guard<std::mutex> lock(gCardMutex);
    if (!gIsDirty) {
        return true;
    }

    std::string tempPath = gSavePath + ".tmp";
    FILE* f = fopen(tempPath.c_str(), "wb");
    if (!f) {
        return false;
    }

    size_t written = fwrite(gSaveBuffer, 1, sizeof(gSaveBuffer), f);
    fclose(f);

    if (written != sizeof(gSaveBuffer)) {
        remove(tempPath.c_str());
        return false;
    }

    if (rename(tempPath.c_str(), gSavePath.c_str()) == 0) {
        gIsDirty = false;
        return true;
    }
    return false;
}

uint8_t* NativeCard_GetBuffer(void) {
    return gSaveBuffer;
}

void NativeCard_Shutdown(void) {
    NativeCard_FlushBackup();
}

void CARD_Init(void) {
    NativeCard_Init(DEFAULT_SAVE_FILE_PATH);
}

void CARD_LockBackup(s32 id) {
    (void)id;
}

void CARD_UnlockBackup(s32 id) {
    (void)id;
}

BOOL CARD_ReadBackup(u32 offset, void* dst, u32 sz) {
    return NativeCard_ReadBackup(offset, dst, sz) ? TRUE : FALSE;
}

void CARD_ReadBackupAsync(u32 offset, void* dst, u32 sz, void (*callback)(void*), void* arg) {
    CARD_ReadBackup(offset, dst, sz);
    if (callback) {
        callback(arg);
    }
}

void CARD_WriteAndVerifyBackupAsync(u32 offset, void* src, u32 sz, void (*callback)(void*), void* arg) {
    NativeCard_WriteBackup(offset, src, sz);
    NativeCard_FlushBackup();
    if (callback) {
        callback(arg);
    }
}

BOOL CARD_WaitBackupAsync(void) {
    return TRUE;
}

s32 CARD_GetResultCode(void) {
    return CARD_RESULT_SUCCESS;
}

} // extern "C"
