/**
 * @file test_rom_icon.cpp
 * @brief Unit tests for 32x32 Pokémon HeartGold ROM window icon data and decoding.
 */

#include "test_runner.h"
#include "display/rom_icon_data.h"
#include "display/display_manager.h"

TEST_CASE(RomIcon_Dimensions) {
    ASSERT_EQ(32, ROM_ICON_WIDTH);
    ASSERT_EQ(32, ROM_ICON_HEIGHT);
    ASSERT_EQ(1024, ROM_ICON_PIXELS);
}

TEST_CASE(RomIcon_EmbeddedPixelBuffer) {
    const uint32_t* pixelData = RomIcon_GetEmbeddedPixelData();
    ASSERT_NE(0ULL, reinterpret_cast<uintptr_t>(pixelData));

    // Verify all pixels have valid opaque alpha (0xFF in high byte 0xAABBGGRR)
    size_t opaqueCount = 0;
    for (size_t i = 0; i < ROM_ICON_PIXELS; ++i) {
        uint32_t pixel = pixelData[i];
        uint8_t alpha = static_cast<uint8_t>((pixel >> 24) & 0xFF);
        if (alpha == 0xFF) {
            opaqueCount++;
        }
    }

    ASSERT_EQ(1024, opaqueCount);
}

TEST_CASE(RomIcon_SetNullWindow) {
    // Setting icon on a null window pointer must return false and not crash
    bool result = DisplayManager_SetWindowIcon(nullptr);
    ASSERT_FALSE(result);
}
