/**
 * @file game_core.cpp
 * @brief Game core thread manager and lifecycle controller.
 */

#include "port_game_core.h"
#include "port_os.h"
#include "nitro.h"
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
    uint16_t buttons = 0;
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
    NativeOS_ResetQuitRequest();
    s_status.store(GAME_CORE_UNINITIALIZED);

    s_gameThread = std::thread(DefaultGameCoreWorker);
    return true;
}

bool GameCore_Stop(uint32_t timeoutMs) {
    (void)timeoutMs;
    s_stopRequested.store(true);
    NativeOS_InterruptVBlankWait();

    if (s_gameThread.joinable()) {
        s_gameThread.join();
    }

    NativeOS_ResetQuitRequest();
    s_status.store(GAME_CORE_STOPPED);
    return true;
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

void* OS_AllocFromArenaLo(OSArenaId id, u32 size, u32 align) {
    (void)id;
    (void)align;
    return std::malloc(size);
}

void* OS_AllocFromArenaHi(OSArenaId id, u32 size, u32 align) {
    (void)id;
    (void)align;
    return std::malloc(size);
}

u16 PAD_Read(void) {
    uint16_t buttons = 0;
    GameCore_GetInput(&buttons, nullptr, nullptr, nullptr);
    return buttons;
}

u32 TP_RequestRawSampling(TPData *result) {
    if (result) {
        TP_GetLatestRawPointInAuto(result);
    }
    return 0;
}

void TP_GetLatestRawPointInAuto(TPData *result) {
    if (!result) return;
    uint8_t tx = 0, ty = 0;
    bool touching = false;
    GameCore_GetInput(nullptr, &tx, &ty, &touching);
    result->x = tx;
    result->y = ty;
    result->touch = touching ? TP_TOUCH_ON : TP_TOUCH_OFF;
    result->validity = TP_VALIDITY_VALID;
}

void TP_GetCalibratedPoint(TPData *calib, const TPData *raw) {
    if (!calib) return;
    if (raw) {
        *calib = *raw;
    } else {
        TP_GetLatestRawPointInAuto(calib);
    }
}

} // extern "C"
