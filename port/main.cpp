/**
 * @file main.cpp
 * @brief Native PC port standalone application entry point and SDL2 event loop.
 */

#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <SDL.h>
#include <SDL_opengl.h>

#include "port_common.h"
#include "port_os.h"
#include "port_vfs.h"

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
                      << "  --test-run, --headless  Execute startup and event loop validation then exit\n"
                      << "  --help, -h              Show this help message\n";
            return 0;
        }
    }

    std::cout << "Starting Pokémon HeartGold Native PC Port...\n";

    // Initialize SDL2
    Uint32 sdlFlags = SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER | SDL_INIT_TIMER;
    if (SDL_Init(sdlFlags) != 0) {
        std::cerr << "Failed to initialize SDL2: " << SDL_GetError() << "\n";
        return 1;
    }

    // Initialize Native HAL Subsystems
    NativeVFS_Init("files");
    OS_InitTick();
    OS_InitThread();
    OS_InitIrqTable();

    // Set OpenGL 3.3 Core profile attributes
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    Uint32 windowFlags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI;
    if (testRun) {
        windowFlags |= SDL_WINDOW_HIDDEN;
    }

    SDL_Window* window = nullptr;
    SDL_GLContext glContext = nullptr;

    if (testRun) {
        // In test-run mode, try OpenGL window; if dummy driver lacks GL, fallback to headless window
        window = SDL_CreateWindow(
            "Pokémon HeartGold (Test Run)",
            SDL_WINDOWPOS_CENTERED,
            SDL_WINDOWPOS_CENTERED,
            DEFAULT_WINDOW_WIDTH,
            DEFAULT_WINDOW_HEIGHT,
            windowFlags
        );
        if (window) {
            glContext = SDL_GL_CreateContext(window);
        } else {
            window = SDL_CreateWindow(
                "Pokémon HeartGold (Test Run)",
                SDL_WINDOWPOS_CENTERED,
                SDL_WINDOWPOS_CENTERED,
                DEFAULT_WINDOW_WIDTH,
                DEFAULT_WINDOW_HEIGHT,
                SDL_WINDOW_HIDDEN
            );
        }
    } else {
        window = SDL_CreateWindow(
            "Pokémon HeartGold (Native PC Port)",
            SDL_WINDOWPOS_CENTERED,
            SDL_WINDOWPOS_CENTERED,
            DEFAULT_WINDOW_WIDTH,
            DEFAULT_WINDOW_HEIGHT,
            windowFlags
        );
        if (window) {
            glContext = SDL_GL_CreateContext(window);
        }
    }

    if (!window) {
        std::cerr << "Failed to create SDL window: " << SDL_GetError() << "\n";
        SDL_Quit();
        return 1;
    }

    if (glContext) {
        SDL_GL_SetSwapInterval(1); // Enable VSync
    }

    std::cout << "Display initialized (" << DEFAULT_WINDOW_WIDTH << "x" << DEFAULT_WINDOW_HEIGHT << ").\n";

    bool running = true;
    int frameCount = 0;
    auto lastFrameTime = std::chrono::steady_clock::now();

    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            } else if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    running = false;
                } else if (event.key.keysym.sym == SDLK_F10) {
                    std::cout << "[Display] Layout cycle requested (F10).\n";
                } else if (event.key.keysym.sym == SDLK_F11) {
                    Uint32 flags = SDL_GetWindowFlags(window);
                    bool isFullscreen = (flags & SDL_WINDOW_FULLSCREEN_DESKTOP) != 0;
                    SDL_SetWindowFullscreen(window, isFullscreen ? 0 : SDL_WINDOW_FULLSCREEN_DESKTOP);
                    std::cout << "[Display] Fullscreen toggled (F11): " << (!isFullscreen ? "ON" : "OFF") << "\n";
                }
            }
        }

        // Trigger virtual VBlank at DS frame pacing
        NativeOS_TriggerVBlank();

        // Swap OpenGL window buffers if context is available
        if (glContext) {
            SDL_GL_SwapWindow(window);
        }
        frameCount++;

        if (testRun && frameCount >= 5) {
            std::cout << "Test run completed (" << frameCount << " frames processed).\n";
            running = false;
        }

        // Frame rate limiter (~59.8261 Hz)
        auto now = std::chrono::steady_clock::now();
        double elapsedMs = std::chrono::duration<double, std::milli>(now - lastFrameTime).count();
        if (elapsedMs < TARGET_FRAME_TIME_MS) {
            std::this_thread::sleep_for(std::chrono::duration<double, std::milli>(TARGET_FRAME_TIME_MS - elapsedMs));
        }
        lastFrameTime = std::chrono::steady_clock::now();
    }

    std::cout << "Shutting down application...\n";

    // Cleanup resources
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
