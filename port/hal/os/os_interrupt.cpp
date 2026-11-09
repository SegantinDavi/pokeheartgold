/**
 * @file os_interrupt.cpp
 * @brief Native OS interrupt simulation, IRQ table, and VBlank pacing.
 */

#include "port_os.h"
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <unordered_map>

namespace {

std::atomic<bool> gMasterInterruptEnable{true};
std::atomic<OSIrqMask> gInterruptMask{0};
std::atomic<u32> gVBlankCounter{0};
std::atomic<bool> gQuitRequested{false};

std::mutex gVBlankMutex;
std::condition_variable gVBlankCV;
uint32_t gVBlankSignalSequence = 0;

std::mutex gIrqMutex;
std::unordered_map<OSIrqMask, OSIrqFunction> gIrqTable;

} // anonymous namespace

extern "C" {

void OS_InitIrqTable(void) {
    std::lock_guard<std::mutex> lock(gIrqMutex);
    gIrqTable.clear();
    gInterruptMask.store(0);
    gMasterInterruptEnable.store(true);
}

void OS_SetIrqFunction(OSIrqMask intrBit, OSIrqFunction function) {
    std::lock_guard<std::mutex> lock(gIrqMutex);
    if (function) {
        gIrqTable[intrBit] = function;
    } else {
        gIrqTable.erase(intrBit);
    }
}

OSIrqFunction OS_GetIrqFunction(OSIrqMask intrBit) {
    std::lock_guard<std::mutex> lock(gIrqMutex);
    auto it = gIrqTable.find(intrBit);
    return (it != gIrqTable.end()) ? it->second : nullptr;
}

BOOL OS_EnableIrq(void) {
    return gMasterInterruptEnable.exchange(true) ? TRUE : FALSE;
}

BOOL OS_DisableIrq(void) {
    return gMasterInterruptEnable.exchange(false) ? TRUE : FALSE;
}

BOOL OS_RestoreIrq(BOOL enable) {
    return gMasterInterruptEnable.exchange(enable != FALSE) ? TRUE : FALSE;
}

OSIrqMask OS_GetIrqMask(void) {
    return gInterruptMask.load();
}

OSIrqMask OS_SetIrqMask(OSIrqMask mask) {
    return gInterruptMask.exchange(mask);
}

OSIrqMask OS_EnableIrqMask(OSIrqMask mask) {
    return gInterruptMask.fetch_or(mask);
}

OSIrqMask OS_DisableIrqMask(OSIrqMask mask) {
    return gInterruptMask.fetch_and(~mask);
}

void OS_WaitIrq(BOOL clear, OSIrqMask irqFlags) {
    (void)clear;
    if (irqFlags & OS_IE_V_BLANK) {
        OS_WaitVBlankIntr();
    }
}

void OS_WaitVBlankIntr(void) {
    std::unique_lock<std::mutex> lock(gVBlankMutex);
    uint32_t targetSeq = gVBlankSignalSequence + 1;
    gVBlankCV.wait(lock, [targetSeq]() {
        return gQuitRequested.load() || static_cast<int32_t>(gVBlankSignalSequence - targetSeq) >= 0;
    });
}

u32 OS_GetVBlankCount(void) {
    return gVBlankCounter.load();
}

void NativeOS_TriggerVBlank(void) {
    gVBlankCounter++;

    // Fire registered VBlank handler if enabled
    if (gMasterInterruptEnable.load()) {
        OSIrqFunction vblankFunc = nullptr;
        {
            std::lock_guard<std::mutex> lock(gIrqMutex);
            auto it = gIrqTable.find(OS_IE_V_BLANK);
            if (it != gIrqTable.end()) {
                vblankFunc = it->second;
            }
        }
        if (vblankFunc) {
            vblankFunc();
        }
    }

    // Wake threads waiting on VBlank
    {
        std::lock_guard<std::mutex> lock(gVBlankMutex);
        gVBlankSignalSequence++;
    }
    gVBlankCV.notify_all();
}

void NativeOS_InterruptVBlankWait(void) {
    gQuitRequested.store(true);
    {
        std::lock_guard<std::mutex> lock(gVBlankMutex);
        gVBlankSignalSequence++;
    }
    gVBlankCV.notify_all();
}

bool NativeOS_IsQuitRequested(void) {
    return gQuitRequested.load();
}

} // extern "C"
