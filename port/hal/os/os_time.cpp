/**
 * @file os_time.cpp
 * @brief Native OS virtual system clock and timing subsystem.
 */

#include "port_os.h"
#include <chrono>
#include <atomic>

namespace {

using Clock = std::chrono::steady_clock;
Clock::time_point gBaselineTime = Clock::now();
std::atomic<bool> gTickInitialized{false};
std::atomic<OSTick> gTickOffset{0};

} // anonymous namespace

extern "C" {

void OS_InitTick(void) {
    gBaselineTime = Clock::now();
    gTickOffset.store(0);
    gTickInitialized.store(true);
}

BOOL OS_IsTickAvailable(void) {
    return TRUE;
}

OSTick OS_GetTick(void) {
    if (!gTickInitialized.load()) {
        OS_InitTick();
    }
    auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() - gBaselineTime).count();
    if (elapsed < 0) elapsed = 0;
    // 33513982 ticks per 1,000,000,000 ns
    uint64_t ticks = (static_cast<uint64_t>(elapsed) * OS_TIMER_CLOCK) / 1000000000ULL;
    return ticks + gTickOffset.load();
}

u16 OS_GetTickLo(void) {
    return static_cast<u16>(OS_GetTick() & 0xFFFF);
}

void OS_SetTick(OSTick tick) {
    OSTick current = OS_GetTick();
    gTickOffset.store(tick - current + gTickOffset.load());
}

OSTime OS_GetTime(void) {
    return static_cast<OSTime>(OS_GetTick());
}

OSTick OS_MicroSecondsToTicks(u32 us) {
    return (static_cast<OSTick>(us) * OS_TIMER_CLOCK) / 1000000ULL;
}

u32 OS_TicksToMicroSeconds(OSTick ticks) {
    return static_cast<u32>((ticks * 1000000ULL) / OS_TIMER_CLOCK);
}

OSTick OS_MilliSecondsToTicks(u32 ms) {
    return (static_cast<OSTick>(ms) * OS_TIMER_CLOCK) / 1000ULL;
}

u32 OS_TicksToMilliSeconds(OSTick ticks) {
    return static_cast<u32>((ticks * 1000ULL) / OS_TIMER_CLOCK);
}

} // extern "C"
