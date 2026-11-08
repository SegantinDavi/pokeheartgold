/**
 * @file test_overlay_manager.cpp
 * @brief Unit tests for overlay monolithic initialization table and symbol dispatch.
 */

#include "test_runner.h"
#include "port_overlay.h"
#include <atomic>

static std::atomic<int> sInitCount{0};
static std::atomic<int> sCleanupCount{0};

static void DummyOverlayInit(void) {
    sInitCount++;
}

static void DummyOverlayCleanup(void) {
    sCleanupCount++;
}

TEST_CASE(Overlay_RegistryAndDispatch) {
    NativeOverlay_Reset();
    ASSERT_EQ(0, NativeOverlay_GetCount());

    sInitCount = 0;
    sCleanupCount = 0;

    // Register overlay 42
    bool regOk = NativeOverlay_Register(42, "overlay_42", DummyOverlayInit, DummyOverlayCleanup);
    ASSERT_TRUE(regOk);
    ASSERT_EQ(1, NativeOverlay_GetCount());
    ASSERT_FALSE(NativeOverlay_IsLoaded(42));

    // Find registered overlay
    const OverlayDescriptor* desc = NativeOverlay_Find(42);
    ASSERT_TRUE(desc != nullptr);
    ASSERT_EQ(42, desc->id);

    // Load overlay 42
    bool loadOk = NativeOverlay_Load(42);
    ASSERT_TRUE(loadOk);
    ASSERT_TRUE(NativeOverlay_IsLoaded(42));
    ASSERT_EQ(1, sInitCount.load());

    // Unload overlay 42
    bool unloadOk = NativeOverlay_Unload(42);
    ASSERT_TRUE(unloadOk);
    ASSERT_FALSE(NativeOverlay_IsLoaded(42));
    ASSERT_EQ(1, sCleanupCount.load());
}

TEST_CASE(Overlay_NitroSDKEmulation) {
    NativeOverlay_Reset();
    sInitCount = 0;
    sCleanupCount = 0;

    NativeOverlay_Register(1, "poke_field", DummyOverlayInit, DummyOverlayCleanup);

    // Test FS_LoadOverlay & FS_UnloadOverlay
    BOOL fsLoad = FS_LoadOverlay(0, 1);
    ASSERT_TRUE(fsLoad == TRUE);
    ASSERT_TRUE(NativeOverlay_IsLoaded(1));
    ASSERT_EQ(1, sInitCount.load());

    BOOL fsUnload = FS_UnloadOverlay(0, 1);
    ASSERT_TRUE(fsUnload == TRUE);
    ASSERT_FALSE(NativeOverlay_IsLoaded(1));
    ASSERT_EQ(1, sCleanupCount.load());

    // Unknown overlay loads gracefully in monolithic binary
    BOOL unkLoad = FS_LoadOverlay(0, 999);
    ASSERT_TRUE(unkLoad == TRUE);
}
