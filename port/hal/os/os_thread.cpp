/**
 * @file os_thread.cpp
 * @brief Native OS HAL implementation for threads and mutexes using C++17 threading.
 */

#include "port_os.h"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <unordered_map>
#include <memory>
#include <chrono>

namespace {

struct NativeThreadControl {
    std::thread worker;
    std::mutex cvMutex;
    std::condition_variable cv;
    bool shouldRun = false;
    bool completed = false;
    void (*entry)(void*) = nullptr;
    void* arg = nullptr;
    OSThread* osThread = nullptr;
};

std::mutex gRegistryLock;
std::unordered_map<const OSThread*, std::unique_ptr<NativeThreadControl>> gThreadRegistry;
std::unordered_map<const OSMutex*, std::unique_ptr<std::recursive_mutex>> gMutexRegistry;

thread_local OSThread* tCurrentOSThread = nullptr;

std::recursive_mutex& GetNativeMutex(const OSMutex* mutex) {
    std::lock_guard<std::mutex> lock(gRegistryLock);
    auto& ptr = gMutexRegistry[mutex];
    if (!ptr) {
        ptr = std::make_unique<std::recursive_mutex>();
    }
    return *ptr;
}

} // anonymous namespace

extern "C" {

void OS_InitThread(void) {
    // Thread subsystem ready
}

BOOL OS_IsThreadAvailable(void) {
    return TRUE;
}

void OS_CreateThread(OSThread* thread, void (*func)(void*), void* arg, void* stack, u32 stackSize, u32 prio) {
    (void)stack;
    (void)stackSize;
    if (!thread) return;

    thread->state = OS_THREAD_STATE_WAITING;
    thread->priority = prio;

    auto ctrl = std::make_unique<NativeThreadControl>();
    ctrl->entry = func;
    ctrl->arg = arg;
    ctrl->osThread = thread;
    ctrl->shouldRun = false;
    ctrl->completed = false;

    NativeThreadControl* rawCtrl = ctrl.get();

    rawCtrl->worker = std::thread([rawCtrl]() {
        tCurrentOSThread = rawCtrl->osThread;

        // Wait until started via OS_WakeupThreadDirect
        {
            std::unique_lock<std::mutex> lock(rawCtrl->cvMutex);
            rawCtrl->cv.wait(lock, [rawCtrl]() { return rawCtrl->shouldRun; });
        }

        if (rawCtrl->entry) {
            rawCtrl->entry(rawCtrl->arg);
        }

        {
            std::lock_guard<std::mutex> lock(rawCtrl->cvMutex);
            rawCtrl->completed = true;
            if (rawCtrl->osThread) {
                rawCtrl->osThread->state = OS_THREAD_STATE_TERMINATED;
            }
        }
        rawCtrl->cv.notify_all();
    });

    std::lock_guard<std::mutex> lock(gRegistryLock);
    gThreadRegistry[thread] = std::move(ctrl);
}

void OS_ExitThread(void) {
    // If called from within a thread, it will terminate upon returning
}

void OS_DestroyThread(OSThread* thread) {
    if (!thread) return;
    std::unique_ptr<NativeThreadControl> ctrl;
    {
        std::lock_guard<std::mutex> lock(gRegistryLock);
        auto it = gThreadRegistry.find(thread);
        if (it != gThreadRegistry.end()) {
            ctrl = std::move(it->second);
            gThreadRegistry.erase(it);
        }
    }
    if (ctrl) {
        if (ctrl->worker.joinable()) {
            ctrl->worker.detach();
        }
    }
}

void OS_JoinThread(OSThread* thread) {
    if (!thread) return;
    NativeThreadControl* rawCtrl = nullptr;
    {
        std::lock_guard<std::mutex> lock(gRegistryLock);
        auto it = gThreadRegistry.find(thread);
        if (it != gThreadRegistry.end()) {
            rawCtrl = it->second.get();
        }
    }
    if (rawCtrl && rawCtrl->worker.joinable()) {
        rawCtrl->worker.join();
    }
}

BOOL OS_IsThreadTerminated(const OSThread* thread) {
    if (!thread) return TRUE;
    return (thread->state == OS_THREAD_STATE_TERMINATED) ? TRUE : FALSE;
}

void OS_SleepThread(OSThreadQueue* queue) {
    (void)queue;
    std::this_thread::yield();
}

void OS_WakeupThread(OSThreadQueue* queue) {
    (void)queue;
}

void OS_WakeupThreadDirect(OSThread* thread) {
    if (!thread) return;
    thread->state = OS_THREAD_STATE_READY;
    NativeThreadControl* rawCtrl = nullptr;
    {
        std::lock_guard<std::mutex> lock(gRegistryLock);
        auto it = gThreadRegistry.find(thread);
        if (it != gThreadRegistry.end()) {
            rawCtrl = it->second.get();
        }
    }
    if (rawCtrl) {
        {
            std::lock_guard<std::mutex> lock(rawCtrl->cvMutex);
            rawCtrl->shouldRun = true;
        }
        rawCtrl->cv.notify_all();
    }
}

void OS_YieldThread(void) {
    std::this_thread::yield();
}

void OS_Sleep(u32 msec) {
    std::this_thread::sleep_for(std::chrono::milliseconds(msec));
}

BOOL OS_SetThreadPriority(OSThread* thread, u32 prio) {
    if (!thread) return FALSE;
    thread->priority = prio;
    return TRUE;
}

u32 OS_GetThreadPriority(const OSThread* thread) {
    return thread ? thread->priority : 0;
}

OSThread* OS_GetCurrentThread(void) {
    return tCurrentOSThread;
}

void OS_InitMutex(OSMutex* mutex) {
    if (!mutex) return;
    mutex->count = 0;
    mutex->thread = nullptr;
    GetNativeMutex(mutex);
}

void OS_LockMutex(OSMutex* mutex) {
    if (!mutex) return;
    GetNativeMutex(mutex).lock();
    mutex->count++;
}

void OS_UnlockMutex(OSMutex* mutex) {
    if (!mutex) return;
    mutex->count--;
    GetNativeMutex(mutex).unlock();
}

BOOL OS_TryLockMutex(OSMutex* mutex) {
    if (!mutex) return FALSE;
    if (GetNativeMutex(mutex).try_lock()) {
        mutex->count++;
        return TRUE;
    }
    return FALSE;
}

} // extern "C"
