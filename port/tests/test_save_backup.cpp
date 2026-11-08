/**
 * @file test_save_backup.cpp
 * @brief Unit tests for persistent 512KB save game manager and atomic flushing.
 */

#include "test_runner.h"
#include "port_card.h"
#include <cstdio>
#include <cstring>
#include <atomic>

TEST_CASE(SaveBackup_InitAndEraseState) {
    const char* path = "/tmp/test_save_fresh.sav";
    remove(path);

    bool inited = NativeCard_Init(path);
    ASSERT_TRUE(inited);

    uint8_t probe[16];
    bool readOk = NativeCard_ReadBackup(0, probe, sizeof(probe));
    ASSERT_TRUE(readOk);

    // Initialized flash state must be 0xFF
    for (size_t i = 0; i < sizeof(probe); ++i) {
        ASSERT_EQ(0xFF, probe[i]);
    }

    remove(path);
}

TEST_CASE(SaveBackup_WriteReadAndFlush) {
    const char* path = "/tmp/test_save_data.sav";
    remove(path);

    NativeCard_Init(path);

    const char* magic = "POKEMON_HG_SAVE";
    size_t len = strlen(magic);
    bool written = NativeCard_WriteBackup(0x200, magic, len);
    ASSERT_TRUE(written);

    bool flushed = NativeCard_FlushBackup();
    ASSERT_TRUE(flushed);

    // Reload from disk
    NativeCard_Init(path);

    char readBuf[32]{};
    bool readOk = NativeCard_ReadBackup(0x200, readBuf, len);
    ASSERT_TRUE(readOk);
    ASSERT_EQ(0, strcmp(magic, readBuf));

    remove(path);
}

static std::atomic<bool> sCallbackInvoked{false};
static void TestSaveCallback(void* arg) {
    (void)arg;
    sCallbackInvoked.store(true);
}

TEST_CASE(SaveBackup_AsyncCallbacks) {
    sCallbackInvoked.store(false);

    uint32_t val = 0x12345678;
    CARD_WriteAndVerifyBackupAsync(0, &val, sizeof(val), TestSaveCallback, nullptr);

    ASSERT_TRUE(sCallbackInvoked.load());
    ASSERT_EQ(CARD_RESULT_SUCCESS, CARD_GetResultCode());
    ASSERT_TRUE(CARD_WaitBackupAsync());
}
