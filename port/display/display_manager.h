/**
 * @file display_manager.h
 * @brief Native Display Manager and Dual-Screen Compositor.
 */

#ifndef PORT_DISPLAY_MANAGER_H
#define PORT_DISPLAY_MANAGER_H

#include "port_common.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct DisplayConfig {
    DisplayLayoutMode mode;
    int32_t windowWidth;
    int32_t windowHeight;
    bool isFullscreen;
    bool vsyncEnabled;
    ViewportRect topViewport;
    ViewportRect touchViewport;
} DisplayConfig;

bool DisplayManager_Init(const DisplayConfig* initialConfig);
void DisplayManager_SetLayoutMode(DisplayLayoutMode mode);
DisplayLayoutMode DisplayManager_GetLayoutMode(void);
void DisplayManager_CycleLayoutMode(void);
void DisplayManager_OnWindowResize(int32_t newWidth, int32_t newHeight);
bool DisplayManager_ScreenToTouchCoords(int32_t hostX, int32_t hostY, uint8_t* outTouchX, uint8_t* outTouchY);
void DisplayManager_CalculateViewports(int32_t windowWidth, int32_t windowHeight, DisplayLayoutMode mode, ViewportRect* outTop, ViewportRect* outTouch);
bool DisplayManager_ToggleFullscreen(void* sdlWindow);
bool DisplayManager_SetFullscreen(void* sdlWindow, bool fullscreen);
bool DisplayManager_IsFullscreen(void);
void DisplayManager_PresentFrame(const void* topScreenPixels, const void* touchScreenPixels);
void DisplayManager_Shutdown(void);

#ifdef __cplusplus
}
#endif

#endif // PORT_DISPLAY_MANAGER_H
