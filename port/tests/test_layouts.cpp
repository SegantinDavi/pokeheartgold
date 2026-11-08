/**
 * @file test_layouts.cpp
 * @brief Unit tests for all 4 dual-screen layout viewport calculations, cycling, and mouse mapping.
 */

#include "test_runner.h"
#include "port_common.h"
#include "display/display_manager.h"

TEST_CASE(Layout_HorizontalDefault_Viewports) {
    ViewportRect top, touch;
    DisplayManager_CalculateViewports(1280, 720, DISPLAY_LAYOUT_HORIZONTAL_DEFAULT, &top, &touch);

    ASSERT_EQ(0, top.x);
    ASSERT_EQ(120, top.y);
    ASSERT_EQ(640, top.width);
    ASSERT_EQ(480, top.height);

    ASSERT_EQ(640, touch.x);
    ASSERT_EQ(120, touch.y);
    ASSERT_EQ(640, touch.width);
    ASSERT_EQ(480, touch.height);
}

TEST_CASE(Layout_HorizontalSwapped_Viewports) {
    ViewportRect top, touch;
    DisplayManager_CalculateViewports(1280, 720, DISPLAY_LAYOUT_HORIZONTAL_SWAPPED, &top, &touch);

    // In swapped mode, Touchscreen is on left, Top is on right
    ASSERT_EQ(0, touch.x);
    ASSERT_EQ(120, touch.y);
    ASSERT_EQ(640, touch.width);
    ASSERT_EQ(480, touch.height);

    ASSERT_EQ(640, top.x);
    ASSERT_EQ(120, top.y);
    ASSERT_EQ(640, top.width);
    ASSERT_EQ(480, top.height);
}

TEST_CASE(Layout_VerticalClassic_Viewports) {
    ViewportRect top, touch;
    // 720x960 window: aspect ratio is 3:4 (0.75), target is 2:3 (~0.6667)
    // Pillarbox: width = 960 * 2 / 3 = 640. X offset = (720 - 640) / 2 = 40.
    // Each screen: 640x480.
    DisplayManager_CalculateViewports(720, 960, DISPLAY_LAYOUT_VERTICAL_CLASSIC, &top, &touch);

    // Top screen above
    ASSERT_EQ(40, top.x);
    ASSERT_EQ(0, top.y);
    ASSERT_EQ(640, top.width);
    ASSERT_EQ(480, top.height);

    // Touchscreen below
    ASSERT_EQ(40, touch.x);
    ASSERT_EQ(480, touch.y);
    ASSERT_EQ(640, touch.width);
    ASSERT_EQ(480, touch.height);
}

TEST_CASE(Layout_VerticalSwapped_Viewports) {
    ViewportRect top, touch;
    DisplayManager_CalculateViewports(720, 960, DISPLAY_LAYOUT_VERTICAL_SWAPPED, &top, &touch);

    // Touchscreen above
    ASSERT_EQ(40, touch.x);
    ASSERT_EQ(0, touch.y);
    ASSERT_EQ(640, touch.width);
    ASSERT_EQ(480, touch.height);

    // Top screen below
    ASSERT_EQ(40, top.x);
    ASSERT_EQ(480, top.y);
    ASSERT_EQ(640, top.width);
    ASSERT_EQ(480, top.height);
}

TEST_CASE(Layout_MouseMapping_AllModes) {
    DisplayConfig config{};
    config.windowWidth = 1280;
    config.windowHeight = 720;
    DisplayManager_Init(&config);

    uint8_t touchX = 0;
    uint8_t touchY = 0;

    // --- Mode 0: Horizontal Default (Touch on right) ---
    DisplayManager_SetLayoutMode(DISPLAY_LAYOUT_HORIZONTAL_DEFAULT);
    // Click on right screen center (960, 360)
    ASSERT_TRUE(DisplayManager_ScreenToTouchCoords(960, 360, &touchX, &touchY));
    ASSERT_EQ(128, touchX);
    ASSERT_EQ(96, touchY);
    // Click on left screen center (320, 360) -> false
    ASSERT_FALSE(DisplayManager_ScreenToTouchCoords(320, 360, &touchX, &touchY));

    // --- Mode 1: Horizontal Swapped (Touch on left) ---
    DisplayManager_SetLayoutMode(DISPLAY_LAYOUT_HORIZONTAL_SWAPPED);
    // Click on left screen center (320, 360) -> true
    ASSERT_TRUE(DisplayManager_ScreenToTouchCoords(320, 360, &touchX, &touchY));
    ASSERT_EQ(128, touchX);
    ASSERT_EQ(96, touchY);
    // Click on right screen center (960, 360) -> false
    ASSERT_FALSE(DisplayManager_ScreenToTouchCoords(960, 360, &touchX, &touchY));

    // --- Mode 2: Vertical Classic (Touch below) ---
    DisplayManager_OnWindowResize(720, 960);
    DisplayManager_SetLayoutMode(DISPLAY_LAYOUT_VERTICAL_CLASSIC);
    // Click on bottom screen center (360, 480 + 240 = 720) -> true
    ASSERT_TRUE(DisplayManager_ScreenToTouchCoords(360, 720, &touchX, &touchY));
    ASSERT_EQ(128, touchX);
    ASSERT_EQ(96, touchY);
    // Click on top screen center (360, 240) -> false
    ASSERT_FALSE(DisplayManager_ScreenToTouchCoords(360, 240, &touchX, &touchY));

    // --- Mode 3: Vertical Swapped (Touch above) ---
    DisplayManager_SetLayoutMode(DISPLAY_LAYOUT_VERTICAL_SWAPPED);
    // Click on top screen center (360, 240) -> true
    ASSERT_TRUE(DisplayManager_ScreenToTouchCoords(360, 240, &touchX, &touchY));
    ASSERT_EQ(128, touchX);
    ASSERT_EQ(96, touchY);
    // Click on bottom screen center (360, 720) -> false
    ASSERT_FALSE(DisplayManager_ScreenToTouchCoords(360, 720, &touchX, &touchY));
}

TEST_CASE(Layout_CycleModesSequence) {
    DisplayConfig config{};
    config.windowWidth = 1280;
    config.windowHeight = 720;
    DisplayManager_Init(&config);

    DisplayManager_SetLayoutMode(DISPLAY_LAYOUT_HORIZONTAL_DEFAULT);
    ASSERT_EQ(DISPLAY_LAYOUT_HORIZONTAL_DEFAULT, DisplayManager_GetLayoutMode());

    DisplayManager_CycleLayoutMode();
    ASSERT_EQ(DISPLAY_LAYOUT_HORIZONTAL_SWAPPED, DisplayManager_GetLayoutMode());

    DisplayManager_CycleLayoutMode();
    ASSERT_EQ(DISPLAY_LAYOUT_VERTICAL_CLASSIC, DisplayManager_GetLayoutMode());

    DisplayManager_CycleLayoutMode();
    ASSERT_EQ(DISPLAY_LAYOUT_VERTICAL_SWAPPED, DisplayManager_GetLayoutMode());

    DisplayManager_CycleLayoutMode();
    ASSERT_EQ(DISPLAY_LAYOUT_HORIZONTAL_DEFAULT, DisplayManager_GetLayoutMode());
}

TEST_CASE(Layout_FullscreenToggle) {
    DisplayConfig config{};
    config.isFullscreen = false;
    DisplayManager_Init(&config);

    ASSERT_FALSE(DisplayManager_IsFullscreen());

    // Toggle to true without SDL_Window (headless/mock)
    bool fs = DisplayManager_ToggleFullscreen(nullptr);
    ASSERT_TRUE(fs);
    ASSERT_TRUE(DisplayManager_IsFullscreen());

    // Toggle back to false
    fs = DisplayManager_ToggleFullscreen(nullptr);
    ASSERT_FALSE(fs);
    ASSERT_FALSE(DisplayManager_IsFullscreen());

    // Explicit set
    DisplayManager_SetFullscreen(nullptr, true);
    ASSERT_TRUE(DisplayManager_IsFullscreen());
    DisplayManager_SetFullscreen(nullptr, false);
    ASSERT_FALSE(DisplayManager_IsFullscreen());
}
