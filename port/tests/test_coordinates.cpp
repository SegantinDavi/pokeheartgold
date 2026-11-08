/**
 * @file test_coordinates.cpp
 * @brief Unit tests for dual-screen aspect ratio math and coordinate translation.
 */

#include "test_runner.h"
#include "port_common.h"
#include "display/display_manager.h"

TEST_CASE(Display_HorizontalAspectRatioMath_16x9) {
    // 1280x720 window: 16:9 aspect ratio (1.777:1)
    // Horizontal dual screen total aspect ratio is 8:3 (2.666:1)
    // 1280 / (8/3) = 480 height. Letterboxed: (720 - 480) / 2 = 120 Y offset.
    // Each screen width is 1280 / 2 = 640. Each screen height is 480.
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

TEST_CASE(Display_HorizontalAspectRatioMath_Ultrawide) {
    // 2560x1080 window: 21.33:9 aspect ratio (~2.37:1) -> still narrower than 8:3 (2.666:1)
    // 2560 / (8/3) = 960 height. Letterbox offset: (1080 - 960) / 2 = 60.
    ViewportRect top, touch;
    DisplayManager_CalculateViewports(2560, 1080, DISPLAY_LAYOUT_HORIZONTAL_DEFAULT, &top, &touch);

    ASSERT_EQ(0, top.x);
    ASSERT_EQ(60, top.y);
    ASSERT_EQ(1280, top.width);
    ASSERT_EQ(960, top.height);

    ASSERT_EQ(1280, touch.x);
    ASSERT_EQ(60, touch.y);
    ASSERT_EQ(1280, touch.width);
    ASSERT_EQ(960, touch.height);
}

TEST_CASE(Display_ScreenToTouchTranslation_Horizontal) {
    DisplayConfig config{};
    config.mode = DISPLAY_LAYOUT_HORIZONTAL_DEFAULT;
    config.windowWidth = 1280;
    config.windowHeight = 720;
    DisplayManager_Init(&config);

    uint8_t touchX = 0;
    uint8_t touchY = 0;

    // Top-left of touchscreen: (640, 120) -> (0, 0)
    bool touching = DisplayManager_ScreenToTouchCoords(640, 120, &touchX, &touchY);
    ASSERT_TRUE(touching);
    ASSERT_EQ(0, touchX);
    ASSERT_EQ(0, touchY);

    // Center of touchscreen: (640 + 320, 120 + 240) = (960, 360) -> (128, 96)
    touching = DisplayManager_ScreenToTouchCoords(960, 360, &touchX, &touchY);
    ASSERT_TRUE(touching);
    ASSERT_EQ(128, touchX);
    ASSERT_EQ(96, touchY);

    // Bottom-right corner of touchscreen: (1279, 599) -> (255, 191)
    touching = DisplayManager_ScreenToTouchCoords(1279, 599, &touchX, &touchY);
    ASSERT_TRUE(touching);
    ASSERT_EQ(255, touchX);
    ASSERT_EQ(191, touchY);

    // Click inside Top Screen (left viewport) -> false
    touching = DisplayManager_ScreenToTouchCoords(320, 360, &touchX, &touchY);
    ASSERT_FALSE(touching);

    // Click inside top letterbox margin (e.g. y = 50) -> false
    touching = DisplayManager_ScreenToTouchCoords(960, 50, &touchX, &touchY);
    ASSERT_FALSE(touching);

    // Click inside bottom letterbox margin (e.g. y = 650) -> false
    touching = DisplayManager_ScreenToTouchCoords(960, 650, &touchX, &touchY);
    ASSERT_FALSE(touching);
}
