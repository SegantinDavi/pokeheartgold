/**
 * @file display.h
 * @brief Native Display Manager and Dual-Screen Compositor Contract
 */

#ifndef PORT_DISPLAY_H
#define PORT_DISPLAY_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NDS_SCREEN_WIDTH  256
#define NDS_SCREEN_HEIGHT 192

typedef enum {
    DISPLAY_LAYOUT_HORIZONTAL_DEFAULT = 0, // Top Screen Left, Touchscreen Right
    DISPLAY_LAYOUT_HORIZONTAL_SWAPPED = 1, // Touchscreen Left, Top Screen Right
    DISPLAY_LAYOUT_VERTICAL_CLASSIC   = 2, // Top Screen Above, Touchscreen Below
    DISPLAY_LAYOUT_VERTICAL_SWAPPED   = 3, // Touchscreen Above, Top Screen Below
    DISPLAY_LAYOUT_COUNT
} DisplayLayoutMode;

typedef struct {
    int32_t x;
    int32_t y;
    int32_t width;
    int32_t height;
} ViewportRect;

typedef struct {
    DisplayLayoutMode mode;
    int32_t windowWidth;
    int32_t windowHeight;
    bool isFullscreen;
    bool vsyncEnabled;
    ViewportRect topViewport;
    ViewportRect touchViewport;
} DisplayConfig;

/**
 * @brief Initialize the native display manager with target configuration.
 */
bool DisplayManager_Init(const DisplayConfig* initialConfig);

/**
 * @brief Update layout mode at runtime (e.g., from hotkey or options menu).
 */
void DisplayManager_SetLayoutMode(DisplayLayoutMode mode);

/**
 * @brief Get the currently active layout mode.
 */
DisplayLayoutMode DisplayManager_GetLayoutMode(void);

/**
 * @brief Cycle to the next available display layout mode.
 */
void DisplayManager_CycleLayoutMode(void);

/**
 * @brief Called when the host window is resized or maximized.
 */
void DisplayManager_OnWindowResize(int32_t newWidth, int32_t newHeight);

/**
 * @brief Map host window mouse coordinates to Nintendo DS touchscreen coordinates.
 * @param hostX Host window X in pixels.
 * @param hostY Host window Y in pixels.
 * @param outTouchX Output DS touch X [0..255].
 * @param outTouchY Output DS touch Y [0..191].
 * @return true if (hostX, hostY) falls within the active touchscreen viewport, false otherwise.
 */
bool DisplayManager_ScreenToTouchCoords(int32_t hostX, int32_t hostY, uint8_t* outTouchX, uint8_t* outTouchY);

/**
 * @brief Present a pair of rendered DS framebuffers (256x192 RGBA/RGB555) to the host window.
 */
void DisplayManager_PresentFrame(const void* topScreenPixels, const void* touchScreenPixels);

/**
 * @brief Shutdown and release display resources.
 */
void DisplayManager_Shutdown(void);

#ifdef __cplusplus
}
#endif

#endif // PORT_DISPLAY_H
