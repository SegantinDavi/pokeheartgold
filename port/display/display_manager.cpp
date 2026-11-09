/**
 * @file display_manager.cpp
 * @brief Native display manager, dual-screen compositor, and coordinate transformation.
 */

#include "display_manager.h"
#include "framebuffer.h"
#include "rom_icon_data.h"
#include <SDL.h>
#include <SDL_opengl.h>
#include <cstring>
#include <algorithm>

namespace {

DisplayConfig gActiveConfig{};

void RenderTexturedQuad(uint32_t textureId, const ViewportRect& vp, int32_t windowHeight) {
    if (!textureId) return;

    // OpenGL viewport Y is measured from bottom-left
    int32_t glY = windowHeight - (vp.y + vp.height);
    glViewport(vp.x, glY, vp.width, vp.height);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, 1.0, 1.0, 0.0, -1.0, 1.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, textureId);

    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f); glVertex2f(0.0f, 0.0f);
        glTexCoord2f(1.0f, 0.0f); glVertex2f(1.0f, 0.0f);
        glTexCoord2f(1.0f, 1.0f); glVertex2f(1.0f, 1.0f);
        glTexCoord2f(0.0f, 1.0f); glVertex2f(0.0f, 1.0f);
    glEnd();

    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);
}

} // anonymous namespace

extern "C" {

void DisplayManager_CalculateViewports(
    int32_t windowWidth,
    int32_t windowHeight,
    DisplayLayoutMode mode,
    ViewportRect* outTop,
    ViewportRect* outTouch
) {
    if (!outTop || !outTouch) return;
    if (windowWidth <= 0) windowWidth = 1280;
    if (windowHeight <= 0) windowHeight = 720;

    if (mode == DISPLAY_LAYOUT_HORIZONTAL_DEFAULT || mode == DISPLAY_LAYOUT_HORIZONTAL_SWAPPED) {
        // Combined aspect ratio is 8:3 (approx 2.6667)
        int32_t renderWidth, renderHeight;
        int32_t offsetX, offsetY;

        if (windowWidth * 3 >= windowHeight * 8) {
            renderHeight = windowHeight;
            renderWidth = (windowHeight * 8) / 3;
            offsetX = (windowWidth - renderWidth) / 2;
            offsetY = 0;
        } else {
            renderWidth = windowWidth;
            renderHeight = (windowWidth * 3) / 8;
            offsetX = 0;
            offsetY = (windowHeight - renderHeight) / 2;
        }

        int32_t screenWidth = renderWidth / 2;
        int32_t screenHeight = renderHeight;

        if (mode == DISPLAY_LAYOUT_HORIZONTAL_DEFAULT) {
            *outTop = { offsetX, offsetY, screenWidth, screenHeight };
            *outTouch = { offsetX + screenWidth, offsetY, screenWidth, screenHeight };
        } else {
            *outTouch = { offsetX, offsetY, screenWidth, screenHeight };
            *outTop = { offsetX + screenWidth, offsetY, screenWidth, screenHeight };
        }
    } else {
        // Combined aspect ratio is 4:6 (2:3, approx 0.6667)
        int32_t renderWidth, renderHeight;
        int32_t offsetX, offsetY;

        if (windowWidth * 3 >= windowHeight * 2) {
            renderHeight = windowHeight;
            renderWidth = (windowHeight * 2) / 3;
            offsetX = (windowWidth - renderWidth) / 2;
            offsetY = 0;
        } else {
            renderWidth = windowWidth;
            renderHeight = (windowWidth * 3) / 2;
            offsetX = 0;
            offsetY = (windowHeight - renderHeight) / 2;
        }

        int32_t screenWidth = renderWidth;
        int32_t screenHeight = renderHeight / 2;

        if (mode == DISPLAY_LAYOUT_VERTICAL_CLASSIC) {
            *outTop = { offsetX, offsetY, screenWidth, screenHeight };
            *outTouch = { offsetX, offsetY + screenHeight, screenWidth, screenHeight };
        } else {
            *outTouch = { offsetX, offsetY, screenWidth, screenHeight };
            *outTop = { offsetX, offsetY + screenHeight, screenWidth, screenHeight };
        }
    }
}

bool DisplayManager_Init(const DisplayConfig* initialConfig) {
    if (initialConfig) {
        gActiveConfig = *initialConfig;
    } else {
        memset(&gActiveConfig, 0, sizeof(DisplayConfig));
        gActiveConfig.mode = DISPLAY_LAYOUT_HORIZONTAL_DEFAULT;
        gActiveConfig.windowWidth = 1280;
        gActiveConfig.windowHeight = 720;
        gActiveConfig.vsyncEnabled = true;
    }

    DisplayManager_CalculateViewports(
        gActiveConfig.windowWidth,
        gActiveConfig.windowHeight,
        gActiveConfig.mode,
        &gActiveConfig.topViewport,
        &gActiveConfig.touchViewport
    );

    Framebuffer_Init();
    return true;
}

void DisplayManager_SetLayoutMode(DisplayLayoutMode mode) {
    if (mode >= DISPLAY_LAYOUT_COUNT) {
        mode = DISPLAY_LAYOUT_HORIZONTAL_DEFAULT;
    }
    gActiveConfig.mode = mode;
    DisplayManager_CalculateViewports(
        gActiveConfig.windowWidth,
        gActiveConfig.windowHeight,
        gActiveConfig.mode,
        &gActiveConfig.topViewport,
        &gActiveConfig.touchViewport
    );
}

DisplayLayoutMode DisplayManager_GetLayoutMode(void) {
    return gActiveConfig.mode;
}

void DisplayManager_CycleLayoutMode(void) {
    int next = (static_cast<int>(gActiveConfig.mode) + 1) % DISPLAY_LAYOUT_COUNT;
    DisplayManager_SetLayoutMode(static_cast<DisplayLayoutMode>(next));
}

void DisplayManager_OnWindowResize(int32_t newWidth, int32_t newHeight) {
    gActiveConfig.windowWidth = newWidth;
    gActiveConfig.windowHeight = newHeight;
    DisplayManager_CalculateViewports(
        newWidth,
        newHeight,
        gActiveConfig.mode,
        &gActiveConfig.topViewport,
        &gActiveConfig.touchViewport
    );
}

bool DisplayManager_ToggleFullscreen(void* sdlWindow) {
    return DisplayManager_SetFullscreen(sdlWindow, !gActiveConfig.isFullscreen);
}

bool DisplayManager_SetFullscreen(void* sdlWindow, bool fullscreen) {
    gActiveConfig.isFullscreen = fullscreen;
    if (sdlWindow) {
        SDL_Window* win = static_cast<SDL_Window*>(sdlWindow);
        uint32_t flags = fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0;
        SDL_SetWindowFullscreen(win, flags);
    }
    return gActiveConfig.isFullscreen;
}

bool DisplayManager_IsFullscreen(void) {
    return gActiveConfig.isFullscreen;
}

bool DisplayManager_ScreenToTouchCoords(int32_t hostX, int32_t hostY, uint8_t* outTouchX, uint8_t* outTouchY) {
    const ViewportRect& vp = gActiveConfig.touchViewport;
    if (hostX < vp.x || hostX >= (vp.x + vp.width) ||
        hostY < vp.y || hostY >= (vp.y + vp.height)) {
        if (outTouchX) *outTouchX = 0;
        if (outTouchY) *outTouchY = 0;
        return false;
    }

    double relX = static_cast<double>(hostX - vp.x) / static_cast<double>(vp.width);
    double relY = static_cast<double>(hostY - vp.y) / static_cast<double>(vp.height);

    int32_t dsX = static_cast<int32_t>(relX * NDS_SCREEN_WIDTH);
    int32_t dsY = static_cast<int32_t>(relY * NDS_SCREEN_HEIGHT);

    dsX = std::clamp(dsX, 0, 255);
    dsY = std::clamp(dsY, 0, 191);

    if (outTouchX) *outTouchX = static_cast<uint8_t>(dsX);
    if (outTouchY) *outTouchY = static_cast<uint8_t>(dsY);
    return true;
}

void DisplayManager_PresentFrame(const void* topScreenPixels, const void* touchScreenPixels) {
    if (topScreenPixels) {
        Framebuffer_UpdateTopScreen(topScreenPixels);
    }
    if (touchScreenPixels) {
        Framebuffer_UpdateTouchScreen(touchScreenPixels);
    }

    if (SDL_GL_GetCurrentContext() == nullptr) {
        return;
    }

    // Clear pillarbox/letterbox margin to black
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    RenderTexturedQuad(Framebuffer_GetTopTexture(), gActiveConfig.topViewport, gActiveConfig.windowHeight);
    RenderTexturedQuad(Framebuffer_GetTouchTexture(), gActiveConfig.touchViewport, gActiveConfig.windowHeight);
}

void DisplayManager_Shutdown(void) {
    Framebuffer_Shutdown();
}

bool DisplayManager_SetWindowIcon(void* window) {
    if (!window) {
        return false;
    }

    SDL_Window* sdlWindow = static_cast<SDL_Window*>(window);
    const uint32_t* pixelData = RomIcon_GetEmbeddedPixelData();
    if (!pixelData) {
        return false;
    }

    // RGBA 32-bit format mask for 0xAABBGGRR in little endian
    Uint32 rmask = 0x000000FF;
    Uint32 gmask = 0x0000FF00;
    Uint32 bmask = 0x00FF0000;
    Uint32 amask = 0xFF000000;

    SDL_Surface* surface = SDL_CreateRGBSurfaceFrom(
        const_cast<void*>(static_cast<const void*>(pixelData)),
        ROM_ICON_WIDTH,
        ROM_ICON_HEIGHT,
        32,
        ROM_ICON_WIDTH * sizeof(uint32_t),
        rmask, gmask, bmask, amask
    );

    if (!surface) {
        return false;
    }

    SDL_SetWindowIcon(sdlWindow, surface);
    SDL_FreeSurface(surface);
    return true;
}

bool RomIcon_ApplyToWindow(void* window) {
    return DisplayManager_SetWindowIcon(window);
}

} // extern "C"
