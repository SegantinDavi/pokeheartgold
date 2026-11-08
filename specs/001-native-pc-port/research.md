# Technical Research & Architecture Decisions: Native PC Port

**Feature**: Native PC Port with Configurable Dual-Screen Display  
**Branch**: `001-native-pc-port`  
**Date**: 2026-11-08  
**Status**: Completed  

---

## 1. Windowing, Input & Display Management

### Decision: SDL2 (Simple DirectMedia Layer 2)
- **Chosen**: SDL2 (with OpenGL 3.3 Core / OpenGL ES 3.0 context)
- **Rationale**:
  - Unsurpassed cross-platform portability across Linux (X11 & Wayland), Windows (MSVC & MinGW), and macOS (Cocoa/Metal).
  - Out-of-the-box support for window management, high-DPI scaling, frame timing, multiple viewports, and aspect ratio preservation.
  - Native unified gamepad abstraction (`SDL_GameController` with community controller database) supporting Xbox, PlayStation, Switch Pro, and generic USB controllers.
  - Low-latency audio streaming callback API (`SDL_OpenAudioDevice`).
  - Seamless mouse event handling with floating-point coordinate transforms.
- **Alternatives Considered**:
  - *GLFW + ImGui*: Lacks built-in audio handling and has less mature controller database mapping out of the box compared to SDL2.
  - *SDL3*: Modern and promising, but still relatively new with fewer pre-packaged binary distributions across long-term support Linux distros. SDL2 is universally available.
  - *Native Platform APIs (Win32 / X11 / Cocoa)*: Violates DRY and drastically increases maintenance burden across 3 operating systems.

---

## 2. Configurable Dual-Screen Display Pipeline

### Decision: Dual Offscreen Render Targets (FBOs) with Quad Compositor
- **Chosen**:
  - Offscreen rendering: Two independent 256x192 textures/framebuffers (Top Screen FBO and Touchscreen FBO).
  - Compositor: A display manager in `port/display/` that maps both screen textures to destination viewports inside the host window.
  - 4 Viewport Layout Modes:
    1. **Mode 1 (Default)**: Horizontal Side-by-Side (Top Screen Left, Touchscreen Right) — best for 16:9 / 21:9 monitors.
    2. **Mode 2**: Swapped Horizontal (Touchscreen Left, Top Screen Right).
    3. **Mode 3**: Classic DS Vertical (Top Screen Above, Touchscreen Below) — best for portrait / vertical monitors.
    4. **Mode 4**: Swapped Vertical (Touchscreen Above, Top Screen Below).
  - Aspect Ratio & Scaling: Fixed 4:3 per screen maintained via automatic pillarboxing or letterboxing. Nearest-neighbor or integer scaling with optional linear/CRT filtering.
  - Touchscreen Coordinate Transformation:
    - Host mouse coordinates $(X_{win}, Y_{win})$ are tested against the Touchscreen viewport rectangle $(V_x, V_y, V_w, V_h)$.
    - If inside bounds:
      $$X_{touch} = \text{clamp}\left(\left\lfloor \frac{X_{win} - V_x}{V_w} \times 256.0 \right\rfloor, 0, 255\right)$$
      $$Y_{touch} = \text{clamp}\left(\left\lfloor \frac{Y_{win} - V_y}{V_h} \times 192.0 \right\rfloor, 0, 191\right)$$
    - When mouse leaves the viewport during an active drag, touch coordinates are clamped to the boundary or smoothly released based on hardware digitizer emulation rules.
- **Rationale**:
  - Decouples game logic rendering (which strictly produces 256x192 buffers) from host window dimensions, resolutions, and screen rotations.
  - Switching layout modes is an instant $O(1)$ viewport rect recalculation with zero GPU memory reallocation and zero game stall.
- **Alternatives Considered**:
  - *Rendering directly to the window backbuffer*: Forces game rendering to know host window coordinates, breaking layout switching and aspect ratio guarantees.

---

## 3. 64-Bit Compatibility and Type Normalization

### Decision: Exact-Width Standard Types via `<stdint.h>` in `PLATFORM_NATIVE`
- **Problem**:
  - In Nintendo DS code (`lib/include/nitro/types.h`), `u32` is defined as `unsigned long`, and `s32` is `signed long`.
  - On 64-bit Linux and macOS (LP64 data model), `unsigned long` is 64 bits wide! Struct layouts, unions, and bitfields would break immediately.
- **Chosen**:
  - Under `#ifdef PLATFORM_NATIVE`, map types to exact fixed-width C99 standard types:
    - `u8` $\rightarrow$ `uint8_t`, `s8` $\rightarrow$ `int8_t`
    - `u16` $\rightarrow$ `uint16_t`, `s16` $\rightarrow$ `int16_t`
    - `u32` $\rightarrow$ `uint32_t`, `s32` $\rightarrow$ `int32_t`
    - `u64` $\rightarrow$ `uint64_t`, `s64` $\rightarrow$ `int64_t`
    - `BOOL` $\rightarrow$ `int32_t` (`int`)
  - Replace any DS-specific pointer-integer casts with `uintptr_t`.
  - Enforce explicit `#pragma pack` or structure alignment attributes matching ARM ABI conventions for serialized game structures.
- **Alternatives Considered**:
  - *Compiling as 32-bit binary (-m32)*: Fails on modern macOS (which dropped 32-bit support entirely since macOS Catalina 10.15) and requires multiarch libraries on 64-bit Linux. A true 64-bit native port is required.

---

## 4. Operating System & Virtual Hardware Emulation (`OS_`, `FS_`, `PAD_`, `SND_`)

### Decision: Clean Hardware Abstraction Layer (HAL) in `port/hal/`

#### A. Operating System (`port/hal/os/`)
- **Threading & Mutexes**: Standard C++17 threading primitives (`std::thread`, `std::mutex`, `std::condition_variable`, `std::atomic`).
- **Timing & Interrupts**:
  - High-precision virtual system clock (`OS_GetTick`, `OS_GetTime`) mapped to `std::chrono::steady_clock` or `SDL_GetPerformanceCounter`.
  - Dedicated virtual VBlank timer running at 59.8261 Hz (16.715 ms per frame), triggering VBlank IRQ handlers and unblocking `OS_WaitVBlankIntr`.

#### B. Virtual Filesystem & Save Persistence (`port/hal/fs/`, `port/hal/card/`)
- **VFS Asset Loading**:
  - Maps NitroFS calls (`FS_OpenFile`, `FS_ReadFile`, `FS_SeekFile`, `FS_CloseFile`) to host paths inside `files/`.
  - Supports loading from uncompressed directory trees or bundled archive files.
- **Save Game Management**:
  - Standard 512KB EEPROM/Flash backup emulation.
  - Automatically writes save data atomically (write to temp file then rename) to `<user_save_dir>/pokemon_heartgold.sav` on in-game save events.

#### C. Input Emulation (`port/hal/pad/`, `port/hal/tp/`)
- Maps host keyboard keys and gamepad buttons to NitroSDK `PAD_BUTTON_*` masks.
- Translates mouse cursor coordinates to virtual touch coordinates [0..255, 0..191] and touch contact flag.

#### D. Audio Backend (`port/hal/snd/`)
- SDL Audio device opened at 32768 Hz / 44100 Hz 16-bit stereo.
- Audio synthesis buffer serviced by SDL audio callback via thread-safe circular ring buffer.

---

## 5. Overlay Loading Strategy

### Decision: Monolithic Linking with Overlay Dispatch Table
- **Problem**:
  - Original Nintendo DS games load overlays dynamically into RAM overlays regions (`FS_LoadOverlay`).
  - Different overlays might share address spaces or have duplicate symbol names.
- **Chosen**:
  - **Monolithic Linking**: Compile all overlays into the single executable.
  - Symbol Isolation: Any duplicate symbol names across overlays are namespace-isolated or renamed with overlay prefixes (e.g. `Overlay01_...`).
  - Overlay Manager: `FS_LoadOverlay` is redirected to initialize the corresponding overlay's static BSS and invoke its initialization table in place of copying raw binary blobs to RAM.
- **Rationale**:
  - Completely avoids dynamic library loading (`dlopen`/`LoadLibrary`) differences across OSes.
  - Allows full compiler link-time optimization (LTO) and deterministic execution.
- **Alternatives Considered**:
  - *Dynamic `.so` / `.dll` / `.dylib` loading*: Complex build setup, fragile symbol exports across MSVC and GCC, and security/path resolution overhead.

---

## 6. Build System & Toolchain

### Decision: Modern CMake (>= 3.22) with C11 / C++17
- **Compiler Support**: Clang 14+, GCC 11+, MSVC 2022.
- **Configuration Flags**:
  - Definition: `-DPLATFORM_NATIVE=1`
  - Warning levels: `-Wall -Wextra -Wno-unused-parameter`
  - Sanity sanitizers available in Debug builds (`-fsanitize=address,undefined`).
- **Dependencies Management**:
  - CMake `find_package(SDL2 REQUIRED)` with fallback to `FetchContent` if SDL2 is not installed on the host system.
  - OpenGL / OpenGL ES / glad loader for portable graphics rendering.