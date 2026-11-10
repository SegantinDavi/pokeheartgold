# AI Agent Guide: Pokémon HeartGold / SoulSilver Native PC Port

This repository contains the complete C/C++ decompilation of **Pokémon HeartGold and SoulSilver**.
The goal of this branch (`003-graphics-pipeline`) is to implement a **native recompilation for PC (Linux, Windows, macOS)**, producing a standalone executable that runs the game directly on the user's host operating system with modern peripheral support (keyboard, controller, mouse for touchscreen), without requiring an emulator.

---

## 1. Architectural Overview

The original game was designed specifically for the **Nintendo DS** hardware architecture:
- **Dual Processors**: ARM946E-S (game logic and graphics pipeline) and ARM7TDMI (audio synthesis, touchscreen digitizer, wireless/Wi-Fi, RTC).
- **Dual Displays**: Main Screen (Top) and Sub Screen (Bottom/Touchscreen), each with a native resolution of 256x192 (4:3 aspect ratio).
- **NitroSDK & NitroSystem**: Nintendo's low-level hardware abstraction APIs managing interrupts and threading (`OS`), cartridge access and filesystem (`FS`/`CARD`), 2D/3D graphics pipelines (`GX`/`G2`/`G3`), and audio streaming (`SND`).
- **Overlay System**: Code and resource modules loaded dynamically into RAM on demand depending on the active game context (e.g., field, battle, Pokégear, minigames).

### Repository Structure

- `src/`: Decompiled game logic (overworld, battle engine, menus, events, scripts, game subsystems).
- `include/`: Game headers and global type declarations.
- `lib/NitroSDK/`: Decompiled Nintendo DS system SDK implementation.
- `lib/NitroSystem/` & `lib/NitroDWC/`: Middleware libraries (animation, 3D models, sound, and network).
- `files/`: Game filesystem assets (graphics, maps, scripts, texts, NARCs).
- `port/`: Native hardware abstraction layer (HAL), window management, audio backend, graphics renderer, and PC entry point.
- `specs/`: Project specifications, checklists, and implementation plans.

---

## 2. Implementation Roadmap

### Phase 1: Native Build Pipeline and Base Types
- [x] **CMake Configuration**: Create a modern `CMakeLists.txt` to compile C/C++ sources using modern desktop toolchains (GCC, Clang, MSVC) targeting x86_64 and ARM64.
- [x] **Type Normalization & 64-bit Portability**:
  - Ensure fixed-width integer types (`u32`, `s32`, `u16`, `s16`, `u8`, `s8`, `BOOL`) retain exact sizes across platforms.
  - Audit and fix unsafe casts between pointers (64-bit on desktop) and integers (32-bit on DS).
- [x] **Platform Isolation Macros**: Introduce `#ifdef PLATFORM_NATIVE` guards to isolate DS-specific register/hardware calls without altering original decompiled logic.

### Phase 2: Operating System & Virtual Filesystem (Base HAL)
- [x] **OS Abstraction (`OS_`)**:
  - Implement native threading, mutexes, and message queues (using standard C++ / SDL threading primitives).
  - Emulate system clock (`OS_GetTick`, `OS_GetTime`) and virtual vertical/horizontal blank interrupts (VBlank/HBlank at 60 FPS).
- [x] **Virtual Filesystem (`FS_` and `CARD_`)**:
  - Redirect cartridge read calls and NARC archive lookups to the local `files/` directory or bundled asset packs.
  - Implement local save game persistence on the host filesystem in place of DS Flash/SRAM.

### Phase 3: Windowing, Input & Display Management (SDL2 / SDL3)
- [x] **Configurable Dual-Screen Window Management**:
  - Render both 256x192 viewports within a single native window.
  - **Default Layout**: Horizontal Side-by-Side (Top Screen Left, Touchscreen Right) optimized for widescreen desktop monitors.
  - **Configurable Layouts**: Support runtime switching between Horizontal, Swapped Horizontal, Classic DS Vertical (Top above Bottom), and Swapped Vertical.
  - Preserve 4:3 screen aspect ratios with automatic pillarboxing/letterboxing on window resize.
- [x] **Input & Digitizer Mapping (`PAD_`)**:
  - Map keyboard keys and modern gamepads to Nintendo DS buttons (A, B, X, Y, D-Pad, L, R, Start, Select).
  - Emulate the Touchscreen digitizer by dynamically transforming mouse cursor clicks/drags to virtual coordinates [0..255, 0..191] based on active viewport position and scaling.
- [x] **Audio Backend (`SND_`)**:
  - Route NitroSDK audio synthesizer output to host audio streams via SDL Audio or miniaudio.

### Phase 4: Graphics Pipeline (2D and 3D)
- [x] **2D Graphics Engine (`GX_`, `G2_`)**:
  - Emulate background engines (BG0, BG1, BG2, BG3) in text, affine, and extended affine modes with seamless wrapping and 4bpp/8bpp tile decoding.
  - Render OAM sprites (1D/2D mapping, H/V flips, size decoding, scanline clipping, priority ranking 0..3).
  - Scanline compositor with 5-layer candidate priority sandwiching, alpha blend control (EVA/EVB 0..16), and master brightness fade (+16 white / -16 black).
- [x] **3D Graphics Engine (`G3_`, `G3X_`)**:
  - Emulate fixed-function 3D vertex/matrix pipeline (Projection, Position, Vector stacks with hardware depth clamping) translating to OpenGL Core/Compatibility.
  - Offscreen 256×192 FBO with depth renderbuffer readback into BG0 layer for 2D/3D compositing.
  - Texture cache supporting all 7 NDS texture formats with palette binding and flush.
  - Interactive visual demo previewing authentic Pokémon HeartGold 2D & 3D Title Screen assets and responsive stylus/mouse cursor Pokéball.

### Phase 5: Overlay Architecture & ARM7 Subsystems
- [ ] **Overlay Loading Strategy**:
  - Implement static monolithic linking or dynamic module loading for game overlays.
- [ ] **ARM7 Subsystems**:
  - Host ARM7 service tasks (audio processing, RTC, hardware state) via background threads or high-level emulation (HLE).

---

## 3. Guidelines for AI Agents

All AI agents contributing to this codebase MUST strictly follow these rules:

1. **English Language Standard:**
   - **All code, variable/function identifiers, types, comments, documentation, and commit messages MUST be written exclusively in English.**
2. **Modularity and Non-Destructive Changes:**
   - All native platform compatibility code must reside strictly within `port/` (e.g., `port/hal/`, `port/graphics/`, `port/audio/`, `port/input/`, `port/display/`).
   - Do not modify original source files in `src/` unless strictly necessary for 64-bit portability or platform abstraction hooks (`#ifdef PLATFORM_NATIVE`).
3. **Memory Safety and 64-Bit Compatibility:**
   - Never assume `sizeof(void*) == sizeof(uint32_t)`. Use `uintptr_t` whenever pointer-integer conversions are unavoidable.
   - Respect alignment and little-endian byte ordering.
4. **Documentation Preservation:**
   - Preserve all existing decompilation comments and document all newly created native HAL interfaces clearly.
5. **Small, Testable Increments:**
   - Work incrementally, verifying each subsystem independently (Filesystem -> Window/Display -> 2D Video -> Input -> Audio -> 3D Video).
