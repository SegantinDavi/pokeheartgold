/**
 * @file main.cpp
 * @brief Native PC port standalone application entry point and integrated SDL2 event loop.
 */

#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <sys/stat.h>
#include <SDL.h>
#include <SDL_opengl.h>

#include "port_common.h"
#include "port_os.h"
#include "port_game_core.h"
#include "port_vfs.h"
#include "port_gx.h"
#include "port_g3.h"
#include "port_card.h"
#include "port_snd.h"
#include "display/display_manager.h"
#include "display/display_config.h"
#include "input/input_manager.h"

namespace {

constexpr int DEFAULT_WINDOW_WIDTH = 1280;
constexpr int DEFAULT_WINDOW_HEIGHT = 720;
constexpr double TARGET_FRAME_TIME_MS = 1000.0 / NDS_FRAME_RATE; // ~16.715 ms

} // anonymous namespace

int main(int argc, char** argv) {
    bool testRun = false;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--test-run" || arg == "--headless") {
            testRun = true;
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "Pokémon HeartGold Native PC Port\n"
                      << "Usage: " << argv[0] << " [options]\n\n"
                      << "Options:\n"
                      << "  --test-run, --headless  Execute startup and integrated loop validation then exit\n"
                      << "  --help, -h              Show this help message\n";
            return 0;
        }
    }

    std::cout << "Starting Pokémon HeartGold Native PC Port (User Story 1: Side-by-Side Dual Screens)...\n";

    // Initialize SDL2
    Uint32 sdlFlags = SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER | SDL_INIT_TIMER;
    if (SDL_Init(sdlFlags) != 0) {
        std::cerr << "Failed to initialize SDL2: " << SDL_GetError() << "\n";
        return 1;
    }

    // Initialize Native HAL Subsystems with working-directory independent path resolution
    std::string assetDir = "files";
    struct stat st;
    if (stat("files", &st) != 0) {
        if (stat("../files", &st) == 0) {
            assetDir = "../files";
        } else {
            char* basePath = SDL_GetBasePath();
            if (basePath) {
                std::string base(basePath);
                SDL_free(basePath);
                if (stat((base + "files").c_str(), &st) == 0) {
                    assetDir = base + "files";
                } else if (stat((base + "../files").c_str(), &st) == 0) {
                    assetDir = base + "../files";
                }
            }
        }
    }
    NativeVFS_Init(assetDir.c_str());
    CARD_Init();
    SND_Init();
    OS_InitTick();
    OS_InitThread();
    OS_InitIrqTable();

    // Set OpenGL profile attributes (Compatibility profile ensures textured quads render across all drivers)
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    // Load or initialize display configuration preferences
    DisplayConfig dispConfig{};
    if (!DisplayConfig_Load(&dispConfig, DEFAULT_DISPLAY_CONFIG_PATH)) {
        DisplayConfig_SetDefaults(&dispConfig);
    }

    int windowWidth = dispConfig.windowWidth;
    int windowHeight = dispConfig.windowHeight;

    Uint32 windowFlags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI;
    if (dispConfig.isFullscreen) {
        windowFlags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
    }
    if (testRun) {
        windowFlags |= SDL_WINDOW_HIDDEN;
    }

    SDL_Window* window = nullptr;
    SDL_GLContext glContext = nullptr;

    if (testRun) {
        window = SDL_CreateWindow(
            "Pokémon HeartGold (Test Run)",
            SDL_WINDOWPOS_CENTERED,
            SDL_WINDOWPOS_CENTERED,
            windowWidth,
            windowHeight,
            windowFlags
        );
        if (window) {
            glContext = SDL_GL_CreateContext(window);
        } else {
            window = SDL_CreateWindow(
                "Pokémon HeartGold (Test Run)",
                SDL_WINDOWPOS_CENTERED,
                SDL_WINDOWPOS_CENTERED,
                windowWidth,
                windowHeight,
                SDL_WINDOW_HIDDEN
            );
        }
    } else {
        window = SDL_CreateWindow(
            "Pokémon HeartGold",
            SDL_WINDOWPOS_CENTERED,
            SDL_WINDOWPOS_CENTERED,
            windowWidth,
            windowHeight,
            windowFlags
        );
        if (window) {
            glContext = SDL_GL_CreateContext(window);
            if (!glContext) {
                SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, 0);
                glContext = SDL_GL_CreateContext(window);
            }
        }
    }

    if (!window) {
        std::cerr << "Failed to create SDL window: " << SDL_GetError() << "\n";
        SDL_Quit();
        return 1;
    }

    // Apply official 32x32 Pokémon HeartGold ROM icon to desktop window and taskbar
    if (DisplayManager_SetWindowIcon(window)) {
        std::cout << "Applied official 32x32 Pokémon HeartGold ROM window icon.\n";
    }

    if (glContext) {
        SDL_GL_SetSwapInterval(dispConfig.vsyncEnabled ? 1 : 0);
    }

    // Initialize Dual-Screen Display Manager with loaded preferences
    DisplayManager_Init(&dispConfig);

    // Initialize Input, 2D and 3D graphics engines
    NativeInput_Init();
    GX_Init();
    G3_Init();

    std::cout << "Dual-screen display manager initialized (Horizontal Side-by-Side: "
              << DEFAULT_WINDOW_WIDTH << "x" << DEFAULT_WINDOW_HEIGHT << ").\n";

    // Launch game core thread
    GameCore_Start();

    bool running = true;
    int frameCount = 0;
    auto nextFrameTime = std::chrono::steady_clock::now() +
        std::chrono::duration<double, std::milli>(TARGET_FRAME_TIME_MS);

    while (running) {
        // Poll input events (Keyboard, Gamepad, Mouse Touchscreen)
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            } else if (event.type == SDL_WINDOWEVENT) {
                if (event.window.event == SDL_WINDOWEVENT_RESIZED ||
                    event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                    DisplayManager_OnWindowResize(event.window.data1, event.window.data2);
                }
            } else if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
                bool down = (event.type == SDL_KEYDOWN);
                if (down && event.key.keysym.sym == SDLK_ESCAPE) {
                    running = false;
                } else if (down && event.key.keysym.sym == SDLK_F10) {
                    DisplayManager_CycleLayoutMode();
                } else if (down && event.key.keysym.sym == SDLK_F11) {
                    bool fs = DisplayManager_ToggleFullscreen(window);
                    std::cout << "[Display] Fullscreen toggled (F11): " << (fs ? "ON" : "OFF") << "\n";
                } else {
                    NativeInput_ProcessKeyEvent(event.key.keysym.sym, down);
                }
            } else if (event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP) {
                bool down = (event.type == SDL_MOUSEBUTTONDOWN);
                if (event.button.button == SDL_BUTTON_LEFT) {
                    NativeInput_ProcessMouseEvent(event.button.x, event.button.y, down);
                }
            } else if (event.type == SDL_MOUSEMOTION) {
                if (event.motion.state & SDL_BUTTON_LMASK) {
                    NativeInput_ProcessMouseEvent(event.motion.x, event.motion.y, true);
                }
            }
        }

        // Query active touchscreen input state
        NativeTouchState touch{};
        NativeInput_GetTouchState(&touch);
        uint16_t padButtons = NativeInput_GetPadButtons();
        GameCore_UpdateInput(padButtons, touch.screenX, touch.screenY, touch.touching);

        const char* layoutName = "SIDE-BY-SIDE";
        switch (DisplayManager_GetLayoutMode()) {
            case DISPLAY_LAYOUT_HORIZONTAL_DEFAULT: layoutName = "HORIZONTAL (SIDE-BY-SIDE)"; break;
            case DISPLAY_LAYOUT_HORIZONTAL_SWAPPED: layoutName = "SWAPPED HORIZONTAL"; break;
            case DISPLAY_LAYOUT_VERTICAL_CLASSIC:   layoutName = "VERTICAL (CLASSIC DS)"; break;
            case DISPLAY_LAYOUT_VERTICAL_SWAPPED:   layoutName = "SWAPPED VERTICAL"; break;
            default: break;
        }

        // Render interactive dual-screen presentation card & touch feedback
        if (GX_IsTestPatternEnabled()) {
            GX_DrawTestPattern(frameCount, touch.screenX, touch.screenY, touch.touching, layoutName);
        }

        // Render current 2D/3D graphics frame into dual viewports
        GX_RenderFrame();

        // Swap host window buffers if OpenGL context is available
        if (glContext) {
            SDL_GL_SwapWindow(window);
        }

        // Trigger virtual VBlank at 59.8261 Hz pacing
        NativeOS_TriggerVBlank();
        frameCount++;

        if (testRun && frameCount >= 10) {
            std::cout << "Integrated test run completed (" << frameCount << " frames processed).\n";
            running = false;
        }

        // Frame rate limiter locked to 59.8261 Hz without time drift (SC-003)
        std::this_thread::sleep_until(nextFrameTime);
        nextFrameTime += std::chrono::duration<double, std::milli>(TARGET_FRAME_TIME_MS);
        auto now = std::chrono::steady_clock::now();
        if (nextFrameTime < now) {
            nextFrameTime = now + std::chrono::duration<double, std::milli>(TARGET_FRAME_TIME_MS);
        }
    }

    std::cout << "Shutting down application...\n";

    // Stop game core thread and flush persistent save data
    GameCore_Stop(1000);
    NativeCard_FlushBackup();

    // Save display preferences
    dispConfig.mode = DisplayManager_GetLayoutMode();
    if (!testRun) {
        DisplayConfig_Save(&dispConfig, DEFAULT_DISPLAY_CONFIG_PATH);
    }

    // Cleanup resources
    NativeInput_Shutdown();
    DisplayManager_Shutdown();
    SND_Shutdown();
    NativeCard_Shutdown();
    if (glContext) {
        SDL_GL_DeleteContext(glContext);
    }
    if (window) {
        SDL_DestroyWindow(window);
    }
    NativeVFS_Shutdown();
    SDL_Quit();

    std::cout << "Application exited cleanly.\n";
    return 0;
}
