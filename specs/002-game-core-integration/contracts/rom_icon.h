/**
 * @file rom_icon.h
 * @brief ROM metadata window icon decoding and application contract.
 */

#ifndef CONTRACT_ROM_ICON_H
#define CONTRACT_ROM_ICON_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ROM_ICON_WIDTH  32
#define ROM_ICON_HEIGHT 32
#define ROM_ICON_PIXELS (ROM_ICON_WIDTH * ROM_ICON_HEIGHT)

typedef struct RomIcon {
    int32_t width;
    int32_t height;
    const uint32_t* pixelData; // 32x32 RGBA pixels (0xAABBGGRR)
    bool isEmbedded;
} RomIcon;

/**
 * @brief Loads the official ROM icon from assets or embedded fallback.
 * @param assetPath Optional path to external PNG/NCGR asset (e.g. "heartgold.us/icon.png")
 * @return Decoded RomIcon descriptor.
 */
RomIcon RomIcon_Load(const char* assetPath);

/**
 * @brief Applies the ROM icon to the active SDL window.
 * @param window Pointer to SDL_Window.
 * @return True if icon was successfully set.
 */
bool RomIcon_ApplyToWindow(void* window);

#ifdef __cplusplus
}
#endif

#endif // CONTRACT_ROM_ICON_H
