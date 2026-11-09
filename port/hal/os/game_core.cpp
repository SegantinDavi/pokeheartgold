/**
 * @file game_core.cpp
 * @brief Game core thread manager and lifecycle controller.
 */

#include "port_game_core.h"
#include "port_os.h"
#include <thread>
#include <atomic>
#include <mutex>
#include <chrono>
#include <iostream>

/* Optional weak symbol for NitroMain */
#if defined(__GNUC__) || defined(__clang__)
extern "C" void NitroMain(void) __attribute__((weak));
#else
extern "C" void NitroMain(void);
#endif

namespace {

std::thread s_gameThread;
std::atomic<GameCoreStatus> s_status{GAME_CORE_UNINITIALIZED};
std::atomic<bool> s_stopRequested{false};
std::atomic<uint64_t> s_frameCount{0};

GameCoreMainFunc s_customMainFunc = nullptr;

struct InputState {
    uint16_t buttons = 0xFFFF;
    uint8_t touchX = 0;
    uint8_t touchY = 0;
    bool touching = false;
};

std::mutex s_inputMutex;
InputState s_inputState;

void DefaultGameCoreWorker() {
    s_status.store(GAME_CORE_RUNNING);

    if (s_customMainFunc) {
        s_customMainFunc();
    } else if (NitroMain) {
        NitroMain();
    } else {
        while (!s_stopRequested.load() && !NativeOS_IsQuitRequested()) {
            s_status.store(GAME_CORE_WAITING_VBLANK);
            OS_WaitIrq(TRUE, OS_IE_V_BLANK);
            if (s_stopRequested.load() || NativeOS_IsQuitRequested()) {
                break;
            }
            s_status.store(GAME_CORE_RUNNING);
            s_frameCount.fetch_add(1);
        }
    }

    s_status.store(GAME_CORE_STOPPED);
}

} // anonymous namespace

extern "C" {

void GameCore_SetMainFunction(GameCoreMainFunc func) {
    s_customMainFunc = func;
}

bool GameCore_Start(void) {
    if (s_status.load() == GAME_CORE_RUNNING || s_status.load() == GAME_CORE_WAITING_VBLANK) {
        return false;
    }

    s_stopRequested.store(false);
    s_status.store(GAME_CORE_UNINITIALIZED);

    s_gameThread = std::thread(DefaultGameCoreWorker);
    return true;
}

bool GameCore_Stop(uint32_t timeoutMs) {
    s_stopRequested.store(true);
    NativeOS_InterruptVBlankWait();

    if (!s_gameThread.joinable()) {
        s_status.store(GAME_CORE_STOPPED);
        return true;
    }

    auto start = std::chrono::steady_clock::now();
    bool joined = false;

    std::thread joinWatcher([&]() {
        if (s_gameThread.joinable()) {
            s_gameThread.join();
            joined = true;
        }
    });

    auto timeoutDuration = std::chrono::milliseconds(timeoutMs);
    auto deadline = start + timeoutDuration;

    while (std::chrono::steady_clock::now() < deadline) {
        if (joined) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    if (joinWatcher.joinable()) {
        joinWatcher.join();
    }

    s_status.store(GAME_CORE_STOPPED);
    return joined;
}

GameCoreStatus GameCore_GetStatus(void) {
    return s_status.load();
}

uint64_t GameCore_GetFrameCount(void) {
    return s_frameCount.load();
}

void GameCore_IncrementFrameCount(void) {
    s_frameCount.fetch_add(1);
}

void GameCore_UpdateInput(uint16_t padButtons, uint8_t touchX, uint8_t touchY, bool touching) {
    std::lock_guard<std::mutex> lock(s_inputMutex);
    s_inputState.buttons = padButtons;
    s_inputState.touchX = touchX;
    s_inputState.touchY = touchY;
    s_inputState.touching = touching;
}

void GameCore_GetInput(uint16_t* padButtons, uint8_t* touchX, uint8_t* touchY, bool* touching) {
    std::lock_guard<std::mutex> lock(s_inputMutex);
    if (padButtons) *padButtons = s_inputState.buttons;
    if (touchX) *touchX = s_inputState.touchX;
    if (touchY) *touchY = s_inputState.touchY;
    if (touching) *touching = s_inputState.touching;
}

} // extern "C"
