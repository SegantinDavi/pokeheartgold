/**
 * @file test_runner.cpp
 * @brief Native Unit Test Runner and Phase 1 Sanity Test Suite.
 */

#include "test_runner.h"
#include "port_common.h"
#include "port_os.h"
#include "port_vfs.h"
#include <cstdint>
#include <nitro/types.h>
#include <atomic>

TEST_CASE(VFS_PathResolution) {
    NativeVFS_Init("files");

    const char* path1 = NativeVFS_ResolvePath("rom:/data/test.bin");
    ASSERT_TRUE(std::string(path1).find("files/data/test.bin") != std::string::npos);

    const char* path2 = NativeVFS_ResolvePath("poketool/personal/pms.narc");
    ASSERT_TRUE(std::string(path2).find("files/poketool/personal/pms.narc") != std::string::npos);
}

TEST_CASE(VFS_FileOperations) {
    NativeVFS_Init("files");
    FSFile file;
    FS_InitFile(&file);

    BOOL opened = FS_OpenFile(&file, "fielddata/script/scr_seq.narc");
    ASSERT_TRUE(opened);

    u32 len = FS_GetLength(&file);
    ASSERT_TRUE(len > 0);

    char header[4];
    int bytesRead = FS_ReadFile(&file, header, 4);
    ASSERT_EQ(4, bytesRead);

    // Seek back to start
    BOOL seeked = FS_SeekFile(&file, 0, FS_SEEK_SET);
    ASSERT_TRUE(seeked);

    char header2[4];
    FS_ReadFile(&file, header2, 4);
    ASSERT_EQ(header[0], header2[0]);
    ASSERT_EQ(header[1], header2[1]);

    BOOL closed = FS_CloseFile(&file);
    ASSERT_TRUE(closed);
}

TEST_CASE(OS_TimeAndTicks) {
    OS_InitTick();
    ASSERT_TRUE(OS_IsTickAvailable());

    OSTick tick1 = OS_GetTick();
    OS_Sleep(5);
    OSTick tick2 = OS_GetTick();
    ASSERT_TRUE(tick2 >= tick1);

    u32 ms = 1000;
    OSTick expectedTicks = OS_TIMER_CLOCK;
    OSTick computedTicks = OS_MilliSecondsToTicks(ms);
    ASSERT_EQ(expectedTicks, computedTicks);
    ASSERT_EQ(ms, OS_TicksToMilliSeconds(computedTicks));
}

static std::atomic<int> sVBlankHandlerCalls{0};
static void TestVBlankHandler(void) {
    sVBlankHandlerCalls++;
}

TEST_CASE(OS_InterruptsAndVBlank) {
    OS_InitIrqTable();
    sVBlankHandlerCalls = 0;

    OS_SetIrqFunction(OS_IE_V_BLANK, TestVBlankHandler);
    ASSERT_TRUE(OS_GetIrqFunction(OS_IE_V_BLANK) == TestVBlankHandler);

    u32 startCount = OS_GetVBlankCount();
    NativeOS_TriggerVBlank();

    ASSERT_EQ(startCount + 1, OS_GetVBlankCount());
    ASSERT_EQ(1, sVBlankHandlerCalls.load());

    // Test waiting for VBlank in a worker thread
    std::atomic<bool> threadUnblocked{false};
    OSThread waitThread;
    OS_CreateThread(&waitThread, [](void* arg) {
        auto* flag = static_cast<std::atomic<bool>*>(arg);
        OS_WaitVBlankIntr();
        flag->store(true);
    }, &threadUnblocked, nullptr, 0, 16);

    OS_WakeupThreadDirect(&waitThread);
    OS_Sleep(5);
    ASSERT_FALSE(threadUnblocked.load());

    NativeOS_TriggerVBlank();
    OS_JoinThread(&waitThread);
    ASSERT_TRUE(threadUnblocked.load());
    OS_DestroyThread(&waitThread);
}

TEST_CASE(OS_MutexBasic) {
    OSMutex mutex;
    OS_InitMutex(&mutex);
    ASSERT_EQ(0, mutex.count);

    OS_LockMutex(&mutex);
    ASSERT_EQ(1, mutex.count);

    BOOL tryResult = OS_TryLockMutex(&mutex);
    ASSERT_TRUE(tryResult);
    ASSERT_EQ(2, mutex.count);

    OS_UnlockMutex(&mutex);
    ASSERT_EQ(1, mutex.count);

    OS_UnlockMutex(&mutex);
    ASSERT_EQ(0, mutex.count);
}

static void ThreadTestEntry(void* arg) {
    auto* counter = static_cast<std::atomic<int>*>(arg);
    for (int i = 0; i < 100; ++i) {
        (*counter)++;
    }
}

TEST_CASE(OS_ThreadCreateAndJoin) {
    std::atomic<int> counter{0};
    OSThread thread;
    OS_CreateThread(&thread, ThreadTestEntry, &counter, nullptr, 0, 16);

    ASSERT_FALSE(OS_IsThreadTerminated(&thread));
    OS_WakeupThreadDirect(&thread);
    OS_JoinThread(&thread);

    ASSERT_EQ(100, counter.load());
    ASSERT_TRUE(OS_IsThreadTerminated(&thread));
    OS_DestroyThread(&thread);
}

TEST_CASE(NitroTypes_NormalizedWidths) {
    ASSERT_EQ(1, sizeof(u8));
    ASSERT_EQ(1, sizeof(s8));
    ASSERT_EQ(2, sizeof(u16));
    ASSERT_EQ(2, sizeof(s16));
    ASSERT_EQ(4, sizeof(u32));
    ASSERT_EQ(4, sizeof(s32));
    ASSERT_EQ(8, sizeof(u64));
    ASSERT_EQ(8, sizeof(s64));
    ASSERT_EQ(4, sizeof(BOOL));
}

TEST_CASE(Sanity_TypeSizes) {
    ASSERT_EQ(1, sizeof(uint8_t));
    ASSERT_EQ(1, sizeof(int8_t));
    ASSERT_EQ(2, sizeof(uint16_t));
    ASSERT_EQ(2, sizeof(int16_t));
    ASSERT_EQ(4, sizeof(uint32_t));
    ASSERT_EQ(4, sizeof(int32_t));
    ASSERT_EQ(8, sizeof(uint64_t));
    ASSERT_EQ(8, sizeof(int64_t));
    ASSERT_GE(sizeof(uintptr_t), 4);
}

TEST_CASE(Sanity_PortConstants) {
    ASSERT_EQ(256, NDS_SCREEN_WIDTH);
    ASSERT_EQ(192, NDS_SCREEN_HEIGHT);
    ASSERT_EQ(256 * 192, NDS_SCREEN_TOTAL_PIXELS);
    ASSERT_EQ(524288, NDS_SAVE_FILE_SIZE);
    ASSERT_EQ(4, DISPLAY_LAYOUT_COUNT);
}

TEST_CASE(Sanity_HelperMacros) {
    ASSERT_EQ(10, PORT_MIN(10, 20));
    ASSERT_EQ(20, PORT_MAX(10, 20));
    ASSERT_EQ(50, PORT_CLAMP(10, 50, 100));
    ASSERT_EQ(100, PORT_CLAMP(150, 50, 100));
    ASSERT_EQ(75, PORT_CLAMP(75, 50, 100));

    int arr[12];
    ASSERT_EQ(12, PORT_ARRAY_SIZE(arr));
}

int main(int argc, char** argv) {
    PORT_UNUSED(argc);
    PORT_UNUSED(argv);
    return PortTest::TestRegistry::Instance().RunAll();
}
