/**
 * @file test_runner.cpp
 * @brief Native Unit Test Runner and Phase 1 Sanity Test Suite.
 */

#include "test_runner.h"
#include "port_common.h"
#include "port_os.h"
#include "port_vfs.h"
#include "port_gx.h"
#include "port_g3.h"
#include "port_card.h"
#include "port_snd.h"
#include "audio/audio_backend.h"
#include "input/input_manager.h"
#include "display/display_manager.h"
#include "display/display_config.h"
#include <SDL.h>
#include <cstdint>
#include <cstdio>
#include <nitro/types.h>
#include <atomic>

TEST_CASE(SND_VolumeAndAudioRouting) {
    SND_Init();
    ASSERT_EQ(SND_MASTER_VOLUME_MAX, SND_GetMasterVolume());

    SND_SetMasterVolume(64);
    ASSERT_EQ(64, SND_GetMasterVolume());

    SND_SetMasterVolume(-5);
    ASSERT_EQ(0, SND_GetMasterVolume());

    SND_SetMasterVolume(250);
    ASSERT_EQ(SND_MASTER_VOLUME_MAX, SND_GetMasterVolume());

    // Push audio samples and check queued frames
    std::vector<int16_t> samples(128, 500); // 64 stereo frames
    SND_ProcessAudio(samples.data(), 64);
    ASSERT_TRUE(NativeAudio_GetQueuedFrames() >= 64);

    SND_Shutdown();
}

TEST_CASE(DisplayConfig_DefaultsAndPersistence) {
    DisplayConfig cfg{};
    DisplayConfig_SetDefaults(&cfg);

    ASSERT_EQ(DISPLAY_LAYOUT_HORIZONTAL_DEFAULT, cfg.mode);
    ASSERT_EQ(1280, cfg.windowWidth);
    ASSERT_EQ(720, cfg.windowHeight);
    ASSERT_FALSE(cfg.isFullscreen);
    ASSERT_TRUE(cfg.vsyncEnabled);

    // Modify and save
    cfg.mode = DISPLAY_LAYOUT_VERTICAL_CLASSIC;
    cfg.windowWidth = 1600;
    cfg.windowHeight = 900;
    cfg.isFullscreen = true;
    cfg.vsyncEnabled = false;

    const char* testPath = "/tmp/test_display_config.ini";
    bool saved = DisplayConfig_Save(&cfg, testPath);
    ASSERT_TRUE(saved);

    // Load back
    DisplayConfig loadedCfg{};
    bool loaded = DisplayConfig_Load(&loadedCfg, testPath);
    ASSERT_TRUE(loaded);

    ASSERT_EQ(DISPLAY_LAYOUT_VERTICAL_CLASSIC, loadedCfg.mode);
    ASSERT_EQ(1600, loadedCfg.windowWidth);
    ASSERT_EQ(900, loadedCfg.windowHeight);
    ASSERT_TRUE(loadedCfg.isFullscreen);
    ASSERT_FALSE(loadedCfg.vsyncEnabled);

    remove(testPath);
}

TEST_CASE(G3_FixedPointAndMatrixMode) {
    ASSERT_NEAR(1.0f, FX32_TO_FLOAT(4096), 0.0001f);
    ASSERT_NEAR(0.5f, FX32_TO_FLOAT(2048), 0.0001f);
    ASSERT_NEAR(-1.0f, FX32_TO_FLOAT(-4096), 0.0001f);

    G3_Init();
    ASSERT_EQ(GX_MTXMODE_POSITION_VECTOR, G3_GetMtxMode());

    G3_MtxMode(GX_MTXMODE_PROJECTION);
    ASSERT_EQ(GX_MTXMODE_PROJECTION, G3_GetMtxMode());

    G3_MtxMode(GX_MTXMODE_POSITION_VECTOR);
    ASSERT_EQ(GX_MTXMODE_POSITION_VECTOR, G3_GetMtxMode());

    // Safe execution of geometry pipeline calls in headless mode
    G3_Identity();
    G3_Translate(4096, 2048, 0);
    G3_Scale(4096, 4096, 4096);
    G3_Color(0x7FFF);
    G3_Begin(GX_BEGIN_TRIANGLES);
    G3_Vtx(0, 0, 0);
    G3_Vtx(4096, 0, 0);
    G3_Vtx(0, 4096, 0);
    G3_End();
}

TEST_CASE(GX_RGB555ColorConversion) {
    // Pure Black (0x0000) -> 0xFF000000
    uint32_t black = GX_RGB555ToRGBA(0x0000);
    ASSERT_EQ(0xFF000000, black);

    // Pure White (0x7FFF) -> 0xFFFFFFFF
    uint32_t white = GX_RGB555ToRGBA(0x7FFF);
    ASSERT_EQ(0xFFFFFFFF, white);

    // Pure Red (0x001F) -> 0xFF0000FF
    uint32_t red = GX_RGB555ToRGBA(0x001F);
    ASSERT_EQ(0xFF0000FF, red);

    // Pure Green (0x03E0) -> 0xFF00FF00
    uint32_t green = GX_RGB555ToRGBA(0x03E0);
    ASSERT_EQ(0xFF00FF00, green);

    // Pure Blue (0x7C00) -> 0xFFFF0000
    uint32_t blue = GX_RGB555ToRGBA(0x7C00);
    ASSERT_EQ(0xFFFF0000, blue);
}

TEST_CASE(GX_ScreenBuffersAndClear) {
    GX_Init();

    uint32_t* top = GX_GetTopScreenPixels();
    ASSERT_TRUE(top != nullptr);

    GX_ClearTopScreen(0xFF0000FF);
    ASSERT_EQ(0xFF0000FF, top[0]);
    ASSERT_EQ(0xFF0000FF, top[NDS_SCREEN_WIDTH * NDS_SCREEN_HEIGHT - 1]);

    uint32_t* touch = GX_GetTouchScreenPixels();
    ASSERT_TRUE(touch != nullptr);

    GX_ClearTouchScreen(0xFF00FF00);
    ASSERT_EQ(0xFF00FF00, touch[0]);
    ASSERT_EQ(0xFF00FF00, touch[NDS_SCREEN_WIDTH * NDS_SCREEN_HEIGHT - 1]);
}

TEST_CASE(Input_KeyboardMapping) {
    NativeInput_Init();
    ASSERT_EQ(0, NativeInput_GetPadButtons());

    NativeInput_ProcessKeyEvent(SDLK_x, true);
    ASSERT_TRUE(NativeInput_GetPadButtons() & NATIVE_PAD_BUTTON_A);

    NativeInput_ProcessKeyEvent(SDLK_UP, true);
    ASSERT_TRUE(NativeInput_GetPadButtons() & NATIVE_PAD_KEY_UP);
    ASSERT_TRUE(NativeInput_GetPadButtons() & NATIVE_PAD_BUTTON_A);

    NativeInput_ProcessKeyEvent(SDLK_x, false);
    ASSERT_FALSE(NativeInput_GetPadButtons() & NATIVE_PAD_BUTTON_A);
    ASSERT_TRUE(NativeInput_GetPadButtons() & NATIVE_PAD_KEY_UP);

    NativeInput_ProcessKeyEvent(SDLK_UP, false);
    ASSERT_EQ(0, NativeInput_GetPadButtons());
    NativeInput_Shutdown();
}

TEST_CASE(Input_MouseTouchMapping) {
    DisplayConfig config{};
    config.mode = DISPLAY_LAYOUT_HORIZONTAL_DEFAULT;
    config.windowWidth = 1280;
    config.windowHeight = 720;
    DisplayManager_Init(&config);
    NativeInput_Init();

    // Click at center of touchscreen (960, 360)
    NativeInput_ProcessMouseEvent(960, 360, true);
    NativeTouchState state{};
    NativeInput_GetTouchState(&state);

    ASSERT_TRUE(state.touching);
    ASSERT_EQ(128, state.screenX);
    ASSERT_EQ(96, state.screenY);
    ASSERT_TRUE(state.rawX > 0);
    ASSERT_TRUE(state.rawY > 0);

    // Release mouse
    NativeInput_ProcessMouseEvent(960, 360, false);
    NativeInput_GetTouchState(&state);
    ASSERT_FALSE(state.touching);

    NativeInput_Shutdown();
}

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
