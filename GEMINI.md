# pokeheartgold Development Guidelines

Auto-generated from all feature plans. Last updated: 2026-11-08

## Active Technologies
- C11 (game logic & shims) and C++17 (host HAL, threading, synchronization) + SDL2 (Windowing, Events, Audio, Input), OpenGL 3.3 Compatibility Profile (Compositing, 2D/3D Rendering) (002-game-core-integration)
- Virtual Filesystem (`files/` directory, NARC archives), 512KB Flash save game (`pokemon_heartgold.sav`) (002-game-core-integration)
- C11 (game-side shims, NitroSDK headers) + C++17 (HAL, compositor engine, texture cache) + SDL2 (window, GL context), OpenGL 3.3 Compatibility Profile (fixed-function 3D pipeline + FBO), existing `port/hal/gx/` and `port/hal/g3/` stubs (003-graphics-pipeline)
- VRAM emulated in `gHwLcdcVram[656KB]`, OAM in `gHwOam[1KB]`/`gHwDbOam[1KB]`, palettes in `gHwPltt[1KB]`/`gHwDbPltt[1KB]` (all defined in `port_hw_registers.h`) (003-graphics-pipeline)

- C11 and C++17 (for modern platform abstractions, threading, and standard containers) + SDL2 (Windowing, Events, Gamepad, Audio) and OpenGL 3.3 Core (Compositing, Viewports, 3D Rendering) (001-native-pc-port)

## Project Structure

```text
src/
tests/
```

## Commands

# Add commands for C11 and C++17 (for modern platform abstractions, threading, and standard containers)

## Code Style

C11 and C++17 (for modern platform abstractions, threading, and standard containers): Follow standard conventions

## Recent Changes
- 003-graphics-pipeline: Added C11 (game-side shims, NitroSDK headers) + C++17 (HAL, compositor engine, texture cache) + SDL2 (window, GL context), OpenGL 3.3 Compatibility Profile (fixed-function 3D pipeline + FBO), existing `port/hal/gx/` and `port/hal/g3/` stubs
- 002-game-core-integration: Added C11 (game logic & shims) and C++17 (host HAL, threading, synchronization) + SDL2 (Windowing, Events, Audio, Input), OpenGL 3.3 Compatibility Profile (Compositing, 2D/3D Rendering)

- 001-native-pc-port: Added C11 and C++17 (for modern platform abstractions, threading, and standard containers) + SDL2 (Windowing, Events, Gamepad, Audio) and OpenGL 3.3 Core (Compositing, Viewports, 3D Rendering)

<!-- MANUAL ADDITIONS START -->
<!-- MANUAL ADDITIONS END -->
