/**
 * @file port_os.h
 * @brief Native OS HAL declarations for threads, mutexes, timing, and interrupts.
 */

#ifndef PORT_OS_H
#define PORT_OS_H

#include <stdint.h>
#include <stdbool.h>
#include <nitro/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/* --- Thread Definitions --- */
typedef enum {
    OS_THREAD_STATE_WAITING = 0,
    OS_THREAD_STATE_READY = 1,
    OS_THREAD_STATE_TERMINATED = 2
} OSThreadState;

typedef struct OSThreadQueue {
    void* head;
    void* tail;
} OSThreadQueue;

typedef struct OSThreadLink {
    void* prev;
    void* next;
} OSThreadLink;

typedef struct OSMutexQueue {
    void* head;
    void* tail;
} OSMutexQueue;

typedef struct OSMutexLink {
    void* next;
    void* prev;
} OSMutexLink;

typedef struct OSMutex {
    OSThreadQueue queue;
    void* thread;
    s32 count;
    OSMutexLink link;
    void* native_handle;
} OSMutex;

typedef struct OSThread {
    OSThreadState state;
    void* next;
    u32 id;
    u32 priority;
    void* profiler;
    OSThreadQueue* queue;
    OSThreadLink link;
    OSMutex* mutex;
    OSMutexQueue mutexQueue;
    u32 stackTop;
    u32 stackBottom;
    u32 stackWarningOffset;
    OSThreadQueue joinQueue;
    void* native_handle;
} OSThread;

/* --- Thread APIs --- */
void OS_InitThread(void);
BOOL OS_IsThreadAvailable(void);
void OS_CreateThread(OSThread* thread, void (*func)(void*), void* arg, void* stack, u32 stackSize, u32 prio);
void OS_ExitThread(void);
void OS_DestroyThread(OSThread* thread);
void OS_JoinThread(OSThread* thread);
BOOL OS_IsThreadTerminated(const OSThread* thread);
void OS_SleepThread(OSThreadQueue* queue);
void OS_WakeupThread(OSThreadQueue* queue);
void OS_WakeupThreadDirect(OSThread* thread);
void OS_YieldThread(void);
void OS_Sleep(u32 msec);
BOOL OS_SetThreadPriority(OSThread* thread, u32 prio);
u32 OS_GetThreadPriority(const OSThread* thread);
OSThread* OS_GetCurrentThread(void);

/* --- Mutex APIs --- */
void OS_InitMutex(OSMutex* mutex);
void OS_LockMutex(OSMutex* mutex);
void OS_UnlockMutex(OSMutex* mutex);
BOOL OS_TryLockMutex(OSMutex* mutex);

/* --- Tick and Time Definitions & APIs --- */
typedef u64 OSTick;
typedef u64 OSTime;

#define OS_SYSTEM_CLOCK_ARM9 67027964ULL
#define OS_TIMER_CLOCK       (OS_SYSTEM_CLOCK_ARM9 / 2) // 33.513982 MHz

void OS_InitTick(void);
BOOL OS_IsTickAvailable(void);
OSTick OS_GetTick(void);
u16 OS_GetTickLo(void);
void OS_SetTick(OSTick tick);
OSTime OS_GetTime(void);

OSTick OS_MicroSecondsToTicks(u32 us);
u32 OS_TicksToMicroSeconds(OSTick ticks);
OSTick OS_MilliSecondsToTicks(u32 ms);
u32 OS_TicksToMilliSeconds(OSTick ticks);

/* --- Interrupt and VBlank Definitions & APIs --- */
typedef u32 OSIrqMask;
typedef void (*OSIrqFunction)(void);

#define OS_IE_V_BLANK   (1UL << 0)
#define OS_IE_H_BLANK   (1UL << 1)
#define OS_IE_V_COUNTER (1UL << 2)
#define OS_IE_TIMER0    (1UL << 3)
#define OS_IE_TIMER1    (1UL << 4)
#define OS_IE_TIMER2    (1UL << 5)
#define OS_IE_TIMER3    (1UL << 6)
#define OS_IE_DMA0      (1UL << 8)
#define OS_IE_DMA1      (1UL << 9)
#define OS_IE_DMA2      (1UL << 10)
#define OS_IE_DMA3      (1UL << 11)
#define OS_IE_KEY       (1UL << 12)
#define OS_IE_CARTRIDGE (1UL << 13)

void OS_InitIrqTable(void);
void OS_SetIrqFunction(OSIrqMask intrBit, OSIrqFunction function);
OSIrqFunction OS_GetIrqFunction(OSIrqMask intrBit);
BOOL OS_EnableIrq(void);
BOOL OS_DisableIrq(void);
BOOL OS_RestoreIrq(BOOL enable);
OSIrqMask OS_GetIrqMask(void);
OSIrqMask OS_SetIrqMask(OSIrqMask mask);
OSIrqMask OS_EnableIrqMask(OSIrqMask mask);
OSIrqMask OS_DisableIrqMask(OSIrqMask mask);
void OS_WaitIrq(BOOL clear, OSIrqMask irqFlags);

void OS_WaitVBlankIntr(void);
u32 OS_GetVBlankCount(void);

/* Host trigger to simulate virtual 59.8261 Hz VBlank */
void NativeOS_TriggerVBlank(void);
void NativeOS_InterruptVBlankWait(void);
bool NativeOS_IsQuitRequested(void);

#ifdef __cplusplus
}
#endif

#endif /* PORT_OS_H */
