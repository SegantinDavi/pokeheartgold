# Implementation Plan: Native PC Port with Configurable Dual-Screen Display

**Branch**: `001-native-pc-port` | **Date**: 2026-11-08 | **Spec**: [spec.md](./spec.md)  
**Input**: Feature specification from `specs/001-native-pc-port/spec.md`  

---

## Summary

This plan outlines the architecture and phased implementation to recompile **Pokémon HeartGold** natively for desktop platforms (Linux, Windows, macOS) as a 64-bit standalone executable.

The architecture introduces a clean Hardware Abstraction Layer (HAL) within `port/` to emulate Nintendo DS subsystems (`OS_`, `FS_`, `PAD_`, `TP_`, `GX_`, `G2_`, `G3_`, `SND_`) without altering the original decompiled logic in `src/`. The display subsystem provides a configurable dual-screen compositor rendering both 256x192 viewports in SDL2/OpenGL, defaulting to an ergonomic **Horizontal Side-by-Side** layout for modern widescreen monitors, with runtime cycling between horizontal swapped, classic vertical DS, and vertical swapped modes, paired with dynamic mouse-to-touchscreen coordinate translation.

---

## Technical Context

**Language/Version**: C11 and C++17 (for modern platform abstractions, threading, and standard containers)
**Primary Dependencies**: SDL2 (Windowing, Events, Gamepad, Audio) and OpenGL 3.3 Core (Compositing, Viewports, 3D Rendering)
**Storage**: Local host filesystem (512 KB persistent save file + files/ asset directory)
**Testing**: Automated unit/contract tests for HAL modules (type widths, coordinate math, ring buffers) + interactive gameplay smoke testing
**Target Platform**: Desktop operating systems targeting 64-bit architectures (Linux x86_64/ARM64, Windows x86_64, macOS x86_64/Apple Silicon)
**Project Type**: single
**Performance Goals**: 60.0 FPS locked game loop (16.6 ms per frame), <50 ms audio latency, <100 ms layout switch time
**Constraints**: Modularity (all native code in port/), 64-bit safety (uintptr_t, exact-width types), English language standard
**Scale/Scope**: Full Pokémon HeartGold game engine recompilation with over 100 overlays, audio synthesizer, and dual 256x192 screen rendering

---

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-checked after Phase 1 design.*

| Principle / Rule | Requirement | Status | Verification Notes |
|---|---|---|---|
| **English Language Standard** | All code, identifiers, comments, commits, and docs in English | **PASS** | Specification, plan, contracts, and code written exclusively in English. |
| **Modularity & Non-Destructive Changes** | All native platform code strictly in `port/` | **PASS** | Target source layout isolates HAL, display, audio, and input inside `port/`. Original `src/` kept clean. |
| **Memory Safety & 64-Bit Compatibility** | Normalized fixed-width types, no unsafe pointer truncations | **PASS** | Type normalization mapped to `<stdint.h>` under `PLATFORM_NATIVE`. Unsafe pointer casts replaced with `uintptr_t`. |
| **Documentation Preservation** | Preserve decompilation comments and document new HAL interfaces | **PASS** | Original source headers retained; new contracts documented with Doxygen comments. |
| **Small, Testable Increments** | Work in phased, testable increments | **PASS** | Phased progression: Toolchain -> HAL/VFS -> Display & Input -> Audio -> Graphics -> Overlays. |

---

## Project Structure

### Documentation (this feature)

```text
specs/001-native-pc-port/
├── spec.md              # Feature specification
├── plan.md              # This implementation plan
├── research.md          # Phase 0: Technical research and architectural decisions
├── data-model.md        # Phase 1: Data models and entity definitions
├── quickstart.md        # Phase 1: Setup, build, and run instructions
├── contracts/           # Phase 1: C/C++ HAL interface contracts
│   ├── display.h        # Display manager and dual-screen compositor contract
│   ├── input.h          # Input polling and digitizer translation contract
│   ├── audio.h          # Native audio backend contract
│   └── vfs_card.h       # VFS asset loading and save backup contract
├── checklists/
│   └── requirements.md  # Specification quality checklist
└── tasks.md             # Phase 2: Actionable tasks (created via /speckit.tasks)
```

### Source Code (repository root)

```text
CMakeLists.txt           # Modern top-level CMake build configuration
port/                    # Native PC Hardware Abstraction Layer & Application Entry
├── main.cpp             # Host desktop entry point (SDL2 lifecycle and main loop)
├── display/             # Dual-screen display manager & OpenGL compositor
│   ├── display_manager.h
│   ├── display_manager.cpp
│   ├── display_config.cpp
│   ├── framebuffer.cpp
│   └── shaders.h
├── input/               # Keyboard, Gamepad, and Mouse touch translation
│   ├── input_manager.h
│   └── input_manager.cpp
├── audio/               # SDL Audio playback & ring buffer
│   ├── audio_backend.h
│   └── audio_backend.cpp
├── hal/                 # Native NitroSDK reimplementations
│   ├── os/              # Threading, mutexes, virtual clock & VBlank IRQ
│   │   ├── os_thread.cpp
│   │   ├── os_time.cpp
│   │   ├── os_interrupt.cpp
│   │   └── overlay_manager.cpp
│   ├── fs/              # Virtual filesystem mapping host files/
│   │   └── fs_vfs.cpp
│   ├── card/            # Persistent 512KB save game manager
│   │   └── card_backup.cpp
│   ├── gx/              # 2D graphics engine emulation & framebuffers
│   │   └── gx_native.cpp
│   ├── g3/              # 3D fixed-function geometry translation
│   │   └── g3_native.cpp
│   └── snd/             # NitroSDK sound subsystem integration
│       └── snd_native.cpp
├── include/             # Port-specific internal headers
│   └── port_common.h
└── tests/               # Unit and regression tests for HAL components
    ├── test_runner.cpp
    ├── test_coordinates.cpp
    ├── test_layouts.cpp
    ├── test_ring_buffer.cpp
    └── test_save_backup.cpp
```

**Structure Decision**: A dedicated `port/` tree cleanly decouples desktop host concerns (SDL2, OpenGL, OS filesystem, host audio) from the existing `src/` and `lib/NitroSDK/` sources, guaranteeing zero pollution of the decompiled game logic.

---

## Complexity Tracking

*No constitution violations. Architecture adheres strictly to all project constraints.*

| Violation | Why Needed | Simpler Alternative Rejected Because |
|---|---|---|
| *None* | N/A | N/A |
