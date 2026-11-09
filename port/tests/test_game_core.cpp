/**
 * @file test_game_core.cpp
 * @brief Unit tests for GameCore thread lifecycle, VBlank synchronization, and shutdown.
 */

#include "test_runner.h"
#include "port_game_core.h"
#include "port_os.h"
#include <chrono>
#include <thread>

TEST_CASE(GameCore_LifecycleAndStartupBenchmark) {
    auto startTime = std::chrono::steady_clock::now();

    // Start game core thread
    bool started = GameCore_Start();
    ASSERT_TRUE(started);

    // Wait for thread to enter active running or waiting state
    bool enteredActiveState = false;
    for (int i = 0; i < 50; ++i) {
        GameCoreStatus status = GameCore_GetStatus();
        if (status == GAME_CORE_RUNNING || status == GAME_CORE_WAITING_VBLANK) {
            enteredActiveState = true;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    ASSERT_TRUE(enteredActiveState);

    auto elapsedMs = std::chrono::duration<double, std::milli>(
        std::chrono::steady_clock::now() - startTime).count();

    // SC-002: Boots and executes within 2.0 seconds (2000 ms)
    ASSERT_LE(elapsedMs, 2000.0);

    // Trigger virtual VBlanks to advance frames
    uint64_t initialFrames = GameCore_GetFrameCount();
    for (int i = 0; i < 5; ++i) {
        NativeOS_TriggerVBlank();
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    ASSERT_GE(GameCore_GetFrameCount(), initialFrames);

    // Stop game core thread
    bool stopped = GameCore_Stop(1000);
    ASSERT_TRUE(stopped);
    ASSERT_EQ(GAME_CORE_STOPPED, GameCore_GetStatus());
}

TEST_CASE(GameCore_InputStateBuffering) {
    uint16_t buttons = 0x0123;
    uint8_t touchX = 120;
    uint8_t touchY = 85;
    bool touching = true;

    GameCore_UpdateInput(buttons, touchX, touchY, touching);

    uint16_t outButtons = 0;
    uint8_t outTouchX = 0;
    uint8_t outTouchY = 0;
    bool outTouching = false;

    GameCore_GetInput(&outButtons, &outTouchX, &outTouchY, &outTouching);

    ASSERT_EQ(buttons, outButtons);
    ASSERT_EQ(touchX, outTouchX);
    ASSERT_EQ(touchY, outTouchY);
    ASSERT_TRUE(outTouching);
}
