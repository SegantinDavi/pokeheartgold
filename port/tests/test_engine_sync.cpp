/**
 * @file test_engine_sync.cpp
 * @brief Unit tests for input bridge (ReadKeypadAndTouchpad), VBlank pacing, and composite buffer routing.
 */

#include "test_runner.h"
#include "port_game_core.h"
#include "port_os.h"
#include "port_gx.h"
#include "gx_pipeline.h"
#include "nds_authentic_demo.h"
#include "nitro.h"
#include <chrono>
#include <thread>
#include <atomic>
#include <vector>

TEST_CASE(EngineSync_PadReadDefaultAndButtons) {
    // Before any input is supplied, PAD_Read should indicate NO pressed buttons (0)
    GameCore_UpdateInput(0, 0, 0, false);
    u16 initialPad = PAD_Read();
    ASSERT_EQ(0, initialPad);

    // Test individual button masks
    const u16 testButtons[] = {
        0x0001, // A
        0x0002, // B
        0x0004, // SELECT
        0x0008, // START
        0x0010, // RIGHT
        0x0020, // LEFT
        0x0040, // UP
        0x0080, // DOWN
        0x0100, // R
        0x0200, // L
        0x0400, // X
        0x0800  // Y
    };

    for (u16 btn : testButtons) {
        GameCore_UpdateInput(btn, 0, 0, false);
        u16 pad = PAD_Read();
        ASSERT_EQ(btn, pad);
    }

    // Test multi-button combinations (e.g. A + B + START)
    u16 combo = 0x0001 | 0x0002 | 0x0008;
    GameCore_UpdateInput(combo, 0, 0, false);
    ASSERT_EQ(combo, PAD_Read());

    // Clear input
    GameCore_UpdateInput(0, 0, 0, false);
    ASSERT_EQ(0, PAD_Read());
}

TEST_CASE(EngineSync_TouchpadAutoAndSampling) {
    // Touch idle
    GameCore_UpdateInput(0, 0, 0, false);
    TPData tp{};
    TP_GetLatestRawPointInAuto(&tp);
    ASSERT_EQ(TP_TOUCH_OFF, tp.touch);
    ASSERT_EQ(TP_VALIDITY_VALID, tp.validity);

    // Touch active at coordinate (150, 80)
    GameCore_UpdateInput(0, 150, 80, true);
    TP_GetLatestRawPointInAuto(&tp);
    ASSERT_EQ(TP_TOUCH_ON, tp.touch);
    ASSERT_EQ(150, tp.x);
    ASSERT_EQ(80, tp.y);
    ASSERT_EQ(TP_VALIDITY_VALID, tp.validity);

    // Calibrated point should match raw point
    TPData calib{};
    TP_GetCalibratedPoint(&calib, &tp);
    ASSERT_EQ(TP_TOUCH_ON, calib.touch);
    ASSERT_EQ(150, calib.x);
    ASSERT_EQ(80, calib.y);
    ASSERT_EQ(TP_VALIDITY_VALID, calib.validity);

    // TP_RequestRawSampling should also return valid touch coordinates
    TPData sampled{};
    u32 result = TP_RequestRawSampling(&sampled);
    ASSERT_EQ(0, result);
    ASSERT_EQ(TP_TOUCH_ON, sampled.touch);
    ASSERT_EQ(150, sampled.x);
    ASSERT_EQ(80, sampled.y);
    ASSERT_EQ(TP_VALIDITY_VALID, sampled.validity);

    // Release touch
    GameCore_UpdateInput(0, 0, 0, false);
    TP_GetLatestRawPointInAuto(&tp);
    ASSERT_EQ(TP_TOUCH_OFF, tp.touch);
}

TEST_CASE(EngineSync_InputLatencyBenchmark) {
    // SC-004: Input bridge updates must reflect within latency < 16ms
    constexpr int ITERATIONS = 1000;
    auto start = std::chrono::steady_clock::now();

    for (int i = 0; i < ITERATIONS; ++i) {
        uint16_t mask = static_cast<uint16_t>(i & 0x0FFF);
        uint8_t tx = static_cast<uint8_t>(i % 256);
        uint8_t ty = static_cast<uint8_t>(i % 192);
        bool touch = (i % 2) != 0;

        GameCore_UpdateInput(mask, tx, ty, touch);

        u16 pad = PAD_Read();
        ASSERT_EQ(mask, pad);

        TPData data{};
        TP_GetLatestRawPointInAuto(&data);
        ASSERT_EQ(tx, data.x);
        ASSERT_EQ(ty, data.y);
        ASSERT_EQ(touch ? TP_TOUCH_ON : TP_TOUCH_OFF, data.touch);
    }

    auto totalDuration = std::chrono::duration<double, std::milli>(
        std::chrono::steady_clock::now() - start).count();
    double avgLatencyMs = totalDuration / ITERATIONS;

    // Average latency per input update should be well under 1ms (target <16ms per SC-004)
    ASSERT_LE(avgLatencyMs, 1.0);
}

TEST_CASE(EngineSync_VBlankPacingAndThreadWait) {
    OS_InitIrqTable();
    OS_EnableIrq();
    NativeOS_ResetQuitRequest();

    uint32_t startCount = OS_GetVBlankCount();

    // Trigger VBlank and verify counter increments
    NativeOS_TriggerVBlank();
    ASSERT_EQ(startCount + 1, OS_GetVBlankCount());

    // Test waiting on VBlank from another thread
    std::atomic<bool> waitUnblocked{false};
    std::thread waitThread([&waitUnblocked]() {
        OS_WaitVBlankIntr();
        waitUnblocked.store(true);
    });

    // Wait short interval to ensure thread has blocked
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    ASSERT_FALSE(waitUnblocked.load());

    // Trigger VBlank to wake the thread
    NativeOS_TriggerVBlank();
    waitThread.join();
    ASSERT_TRUE(waitUnblocked.load());

    // Test interrupting VBlank wait on shutdown
    std::atomic<bool> quitUnblocked{false};
    std::thread quitThread([&quitUnblocked]() {
        OS_WaitVBlankIntr();
        quitUnblocked.store(true);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    ASSERT_FALSE(quitUnblocked.load());

    NativeOS_InterruptVBlankWait();
    quitThread.join();
    ASSERT_TRUE(quitUnblocked.load());
    ASSERT_TRUE(NativeOS_IsQuitRequested());

    NativeOS_ResetQuitRequest();
}

TEST_CASE(EngineSync_CompositeBufferRouting) {
    GX_Init();
    GX_SetDispSelect(GX_DISP_SELECT_MAIN_SUB);
    ASSERT_EQ(GX_DISP_SELECT_MAIN_SUB, GX_GetDispSelect());

    constexpr size_t PIXEL_COUNT = NDS_SCREEN_WIDTH * NDS_SCREEN_HEIGHT;
    std::vector<uint32_t> mainBuf(PIXEL_COUNT, 0xFF112233);
    std::vector<uint32_t> subBuf(PIXEL_COUNT, 0xFF445566);

    // Route buffers with standard Main->Top, Sub->Touch layout
    GX_RouteCompositeBuffers(mainBuf.data(), subBuf.data());

    uint32_t* top = GX_GetTopScreenPixels();
    uint32_t* touch = GX_GetTouchScreenPixels();

    ASSERT_EQ(0xFF112233, top[0]);
    ASSERT_EQ(0xFF112233, top[PIXEL_COUNT - 1]);
    ASSERT_EQ(0xFF445566, touch[0]);
    ASSERT_EQ(0xFF445566, touch[PIXEL_COUNT - 1]);

    // Swap display select: Main->Touch, Sub->Top
    GX_SetDispSelect(GX_DISP_SELECT_SUB_MAIN);
    ASSERT_EQ(GX_DISP_SELECT_SUB_MAIN, GX_GetDispSelect());

    GX_RouteCompositeBuffers(mainBuf.data(), subBuf.data());
    ASSERT_EQ(0xFF445566, top[0]);
    ASSERT_EQ(0xFF112233, touch[0]);

    // Test 16-bit RGB555 routing
    std::vector<uint16_t> mainBuf16(PIXEL_COUNT, 0x7C00); // Pure Red (RGB555)
    std::vector<uint16_t> subBuf16(PIXEL_COUNT, 0x03E0);  // Pure Green (RGB555)

    GX_SetDispSelect(GX_DISP_SELECT_MAIN_SUB);
    GX_RouteCompositeBuffers16(mainBuf16.data(), subBuf16.data());

    uint32_t redRgba = GX_RGB555ToRGBA(0x7C00);
    uint32_t greenRgba = GX_RGB555ToRGBA(0x03E0);

    ASSERT_EQ(redRgba, top[0]);
    ASSERT_EQ(greenRgba, touch[0]);
}

TEST_CASE(EngineSync_NdsGfxPresentFrameDispSelectSwap) {
    NdsGfx_Init();
    for (int y = 0; y < 192; ++y) {
        for (int x = 0; x < 256; ++x) {
            gNdsGfx.main.frameBuffer[y][x] = 0xFFAABBCC;
            gNdsGfx.sub.frameBuffer[y][x] = 0xFFDDEEFF;
        }
    }

    GX_SetDispSelect(GX_DISP_SELECT_MAIN_SUB);
    NdsGfx_PresentFrame();

    ASSERT_EQ(0xFFAABBCC, GX_GetTopScreenPixels()[0]);
    ASSERT_EQ(0xFFDDEEFF, GX_GetTouchScreenPixels()[0]);

    // Swap mid-frame
    GX_SetDispSelect(GX_DISP_SELECT_SUB_MAIN);
    NdsGfx_PresentFrame();

    ASSERT_EQ(0xFFDDEEFF, GX_GetTopScreenPixels()[0]);
    ASSERT_EQ(0xFFAABBCC, GX_GetTouchScreenPixels()[0]);
}

TEST_CASE(EngineSync_AuthenticDemoPipelinePixels) {
    NdsGfx_Init();
    NdsAuthenticDemo_RenderFrame(0, 100, 100, false);
    NdsGfx_RenderMain();
    NdsGfx_RenderSub();
    NdsGfx_PresentFrame();

    int nonBlackTop = 0;
    int nonBlackSub = 0;
    uint32_t* top = GX_GetTopScreenPixels();
    uint32_t* sub = GX_GetTouchScreenPixels();

    for (int i = 0; i < 256 * 192; ++i) {
        if ((top[i] & 0x00FFFFFF) != 0) nonBlackTop++;
        if ((sub[i] & 0x00FFFFFF) != 0) nonBlackSub++;
    }

    ASSERT_TRUE(nonBlackTop > 0);
    ASSERT_TRUE(nonBlackSub > 0);
}



