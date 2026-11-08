/**
 * @file overlay_manager.cpp
 * @brief Overlay monolithic initialization table and symbol dispatch.
 */

#include "port_overlay.h"
#include <vector>
#include <mutex>
#include <cstring>
#include <algorithm>

namespace {

std::vector<OverlayDescriptor> gOverlayRegistry;
std::mutex gOverlayMutex;

} // anonymous namespace

extern "C" {

void NativeOverlay_Reset(void) {
    std::lock_guard<std::mutex> lock(gOverlayMutex);
    gOverlayRegistry.clear();
}

bool NativeOverlay_Register(uint32_t id, const char* name, OverlayInitFunc initFunc, OverlayCleanupFunc cleanupFunc) {
    std::lock_guard<std::mutex> lock(gOverlayMutex);
    for (auto& entry : gOverlayRegistry) {
        if (entry.id == id) {
            entry.name = name;
            entry.initFunc = initFunc;
            entry.cleanupFunc = cleanupFunc;
            return true;
        }
    }
    gOverlayRegistry.push_back({ id, name, initFunc, cleanupFunc, false });
    return true;
}

bool NativeOverlay_Load(uint32_t id) {
    std::lock_guard<std::mutex> lock(gOverlayMutex);
    for (auto& entry : gOverlayRegistry) {
        if (entry.id == id) {
            if (!entry.isLoaded) {
                entry.isLoaded = true;
                if (entry.initFunc) {
                    entry.initFunc();
                }
            }
            return true;
        }
    }
    // In monolithic builds, overlays not explicitly registered are statically linked
    return true;
}

bool NativeOverlay_Unload(uint32_t id) {
    std::lock_guard<std::mutex> lock(gOverlayMutex);
    for (auto& entry : gOverlayRegistry) {
        if (entry.id == id) {
            if (entry.isLoaded) {
                entry.isLoaded = false;
                if (entry.cleanupFunc) {
                    entry.cleanupFunc();
                }
            }
            return true;
        }
    }
    return true;
}

bool NativeOverlay_IsLoaded(uint32_t id) {
    std::lock_guard<std::mutex> lock(gOverlayMutex);
    for (const auto& entry : gOverlayRegistry) {
        if (entry.id == id) {
            return entry.isLoaded;
        }
    }
    return false;
}

const OverlayDescriptor* NativeOverlay_Find(uint32_t id) {
    std::lock_guard<std::mutex> lock(gOverlayMutex);
    for (const auto& entry : gOverlayRegistry) {
        if (entry.id == id) {
            return &entry;
        }
    }
    return nullptr;
}

size_t NativeOverlay_GetCount(void) {
    std::lock_guard<std::mutex> lock(gOverlayMutex);
    return gOverlayRegistry.size();
}

/* --- NitroSDK Overlay Compatibility --- */

BOOL FS_LoadOverlayInfo(FSOverlayInfo* p_ovi, MIProcessor target, FSOverlayID id) {
    if (!p_ovi) return FALSE;
    memset(p_ovi, 0, sizeof(FSOverlayInfo));
    p_ovi->header.id = id;
    p_ovi->target = target;
    return TRUE;
}

BOOL FS_LoadOverlayImage(FSOverlayInfo* p_ovi) {
    if (!p_ovi) return FALSE;
    return NativeOverlay_Load(p_ovi->header.id) ? TRUE : FALSE;
}

BOOL FS_LoadOverlayImageAsync(FSOverlayInfo* p_ovi, void* p_file) {
    (void)p_file;
    return FS_LoadOverlayImage(p_ovi);
}

BOOL FS_UnloadOverlayImage(FSOverlayInfo* p_ovi) {
    if (!p_ovi) return FALSE;
    return NativeOverlay_Unload(p_ovi->header.id) ? TRUE : FALSE;
}

BOOL FS_LoadOverlay(MIProcessor target, FSOverlayID id) {
    (void)target;
    return NativeOverlay_Load(id) ? TRUE : FALSE;
}

BOOL FS_UnloadOverlay(MIProcessor target, FSOverlayID id) {
    (void)target;
    return NativeOverlay_Unload(id) ? TRUE : FALSE;
}

void FS_StartOverlay(FSOverlayInfo* p_ovi) {
    (void)p_ovi;
}

void FS_EndOverlay(FSOverlayInfo* p_ovi) {
    (void)p_ovi;
}

} // extern "C"
