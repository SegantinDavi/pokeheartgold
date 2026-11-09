# Technical Research: Game Core Integration & Window Branding

**Feature**: `002-game-core-integration`  
**Date**: 2026-11-08  
**Status**: Completed

---

## 1. Official ROM Window Icon Architecture

### Decision: Embedded 32x32 RGBA Decoder & `SDL_SetWindowIcon`
- **Context**: The game repository contains official ROM banner assets in `heartgold.us/`:
  - `icon.nbfc` (512 bytes): 4x4 grid of 8x8 tiles in 4bpp format (32x32 pixels total).
  - `icon.nbfp` (32 bytes): 16-color palette in RGB555 format.
  - `icon.png` (391 bytes): 32x32 pre-rendered PNG.
- **Chosen Solution**:
  1. Generate a self-contained embedded C source `port/display/rom_icon_data.h` containing the exact 32x32 RGBA pixel array decoded from the ROM metadata.
  2. Implement `DisplayManager_SetWindowIcon(SDL_Window* window)` in `port/display/display_manager.cpp`.
  3. Create an `SDL_Surface*` from the pixel buffer using `SDL_CreateRGBSurfaceWithFormatFrom` (32x32, 32-bit RGBA) and apply it via `SDL_SetWindowIcon`.
- **Rationale**:
  - Zero external runtime library dependencies (does not require `SDL_image`, `libpng`, or external asset lookups).
  - Works consistently on Linux, Windows, and macOS, including when the binary is run outside the repository folder.
- **Alternatives Considered**:
  - *Dynamic runtime PNG loading via `stb_image`*: Adds 7,000 lines of third-party parsing code just for a 32x32 icon.
  - *Windows `.ico` resource compiling*: Platform-specific; would not work on Linux or macOS.

---

## 2. Nintendo DS SDK Compatibility Shims

### Decision: Header-Based Redirection (`port/include/nitro.h`, `nnsys.h`, `MSL.h`)
- **Context**: Decompiled code in `src/` was written against Nintendo's NitroSDK, NitroSystem, and CodeWarrior MSL headers:
  - `<MSL.h>`: Metrowerks standard C library.
  - `<nitro.h>`: Core NitroSDK interfaces (`OS_`, `FS_`, `GX_`, `MI_`, `CARD_`, `SND_`).
  - `<nnsys.h>`: NitroSystem helper libraries (`NNS_G2D`, `NNS_G3D`, `NNS_SND`).
  - `<nitro/code16.h>`: Pragmas switching the ARM compiler to 16-bit Thumb mode.
- **Chosen Solution**:
  - In `port/include/`:
    - `MSL.h`: Standard library forwarder (`<stdlib.h>`, `<string.h>`, `<stdio.h>`, `<stdbool.h>`, `<stdint.h>`, `<assert.h>`).
    - `nitro.h`: Includes normalized types (`<nitro/types.h>`), HAL OS (`port_os.h`), VFS (`port_vfs.h`), Card (`port_card.h`), Sound (`port_snd.h`), GX (`port_gx.h`), G3 (`port_g3.h`), and Overlay (`port_overlay.h`).
    - `nitro/code16.h`: Empty stub (Thumb mode switching is irrelevant on desktop x86_64/ARM64).
    - `nnsys.h`: Stub/adapter declarations for NitroSystem utilities.
- **Rationale**:
  - Strictly follows `AGENTS.md` Rule 2: preserves all decompiled sources in `src/` without intrusive edits.
  - Transparently maps legacy SDK calls to our tested native HAL implementations.
- **Alternatives Considered**:
  - *Refactoring `src/` to remove Nintendo headers*: Modifies hundreds of decompiled source files, destroying diff-matching capabilities with the original ROM.

---

## 3. Game Core Threading & Execution Model

### Decision: Dual-Thread Architecture (Host UI Thread + Game Core Thread)
- **Context**: `src/main.c` contains `NitroMain(void)`, which runs an infinite loop executing game ticks, task queues, and waiting for VBlank via `OS_WaitIrq(TRUE, OS_IE_VBLANK)`.
- **Chosen Solution**:
  - **Host / UI Thread (Main Thread)**:
    - Runs the SDL2 event loop (window events, keyboard, gamepad, mouse touchscreen, `F10` layout cycling, `F11` fullscreen).
    - Manages the OpenGL compositor and swaps buffers (`SDL_GL_SwapWindow`).
    - Paces the virtual 59.8261 Hz frame timer and triggers `NativeOS_TriggerVBlank()`.
  - **Game Core Thread (`GameThread`)**:
    - Spawns at application startup and executes `NitroMain()`.
    - Handles game state machines, overworld logic, scripts, and overlays.
    - Synchronizes each frame by blocking on `OS_WaitIrq(TRUE, OS_IE_VBLANK)`.
  - **VBlank Barrier**:
    - `OS_WaitIrq` blocks on a condition variable.
    - `NativeOS_TriggerVBlank()` signals the condition variable and waits for the game frame to complete (or operates in a pipelined double-buffered frame state).
  - **Thread-Safe Shutdown**:
    - An atomic `gQuitRequested` flag interrupts `OS_WaitIrq`, allowing `NitroMain` to exit its loop cleanly, save data to flush, and the game thread to join before the process terminates.
- **Rationale**:
  - Keeps the OS desktop window completely responsive at all times (preventing OS "application hanging" dialogs during heavy game loading).
  - Matches the dual-processor design of Nintendo DS hardware (ARM9 game logic operating asynchronously from display/touchscreen refresh).
- **Alternatives Considered**:
  - *Single-threaded coroutines / fibers*: Coroutines cannot easily yield from deep recursive calls inside decompiled C code without compiler-specific extensions (`setjmp`/`longjmp`), leading to stack corruption.

---

## 4. Hardware Memory Mapping & Register Stubs

### Decision: Virtual Memory Arena & Structured I/O Registers
- **Context**: Original ARM9 code uses memory-mapped I/O addresses:
  - Main RAM: `0x02000000 - 0x023FFFFF` (4 MB)
  - Hardware I/O Registers: `0x04000000 - 0x04FFFFFF`
  - VRAM: `0x06000000 - 0x06FFFFFF`
- **Chosen Solution**:
  - Allocate a contiguous virtual memory buffer for Main RAM emulation (`4MB`).
  - Define structured register overlays for `reg_GX_*`, `reg_G2_*`, and `reg_G3_*` inside `port/hal/gx/hw_registers.cpp`.
  - Provide safe no-op or logging handlers for coprocessor registers (CP15, cache control, wait control) which are irrelevant on modern CPUs with unified virtual memory caches.
- **Rationale**:
  - Prevents segmentation faults from pointer arithmetic while preserving original DS pointer logic.

---

## 5. Input & Graphics Bridge

### Decision: Direct Memory Linkage
- **Input Bridge**:
  - `port/input/input_manager.cpp` writes directly to `gSystem.heldKeysRaw` and updates touchpad structures during `ReadKeypadAndTouchpad()`.
- **Graphics Bridge**:
  - When the game engine calls display swap or VBlank ends, the native 2D graphics engine composite writes directly into `gTopScreenPixels` and `gTouchScreenPixels`.
  - The OpenGL texture sub-image updates render the game's actual visuals to the dual viewports.
