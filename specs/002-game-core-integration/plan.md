# Implementation Plan: Game Core Integration & Window Branding

**Branch**: `002-game-core-integration` | **Date**: 2026-11-08 | **Spec**: [spec.md](spec.md)  
**Input**: Feature specification from `/specs/002-game-core-integration/spec.md`

---

## Summary

Integrate the decompiled Nintendo DS game engine (`src/`) with the native Hardware Abstraction Layer (`port/`) to execute the authentic game loop (`NitroMain`) natively on PC. This feature implements:
1. Native SDK compatibility shims (`nitro.h`, `nnsys.h`, `MSL.h`) in `port/include/`.
2. Dedicated dual-thread execution model (Host UI Event Loop + Game Core Thread).
3. 59.8261 Hz virtual VBlank interrupt synchronization (`OS_WaitIrq`).
4. Official 32x32 Pokémon HeartGold ROM icon extraction and desktop window application via `SDL_SetWindowIcon`.
5. Bidirectional input and graphics framebuffer bridging (`GX_` to dual viewports).

---

## Technical Context

**Language/Version**: C11 (game logic & shims) and C++17 (host HAL, threading, synchronization)  
**Primary Dependencies**: SDL2 (Windowing, Events, Audio, Input), OpenGL 3.3 Compatibility Profile (Compositing, 2D/3D Rendering)  
**Storage**: Virtual Filesystem (`files/` directory, NARC archives), 512KB Flash save game (`pokemon_heartgold.sav`)  
**Testing**: `ctest` with native C++ test framework (`port/tests/`)  
**Target Platform**: Linux (x86_64 / ARM64, WSL2), Windows (x64), macOS (Apple Silicon / Intel)  
**Project Type**: Single desktop executable (`pokeheartgold`) + automated test suite (`port_tests`)  
**Performance Goals**: 59.8261 Hz locked game loop (16.6ms per frame), zero frame tearing, <16ms input latency  
**Constraints**: Zero regression in `src/` (decompilation fidelity preserved), English language standard, strict 64-bit portability  
**Scale/Scope**: Decompiled Pokémon HeartGold game engine (~400 source files in `src/`, ~100 overlays, dual 256x192 screens)

---

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

| Principle / Rule | Requirement | Status | Notes |
|---|---|:---:|---|
| **English Language Standard** | All identifiers, code, comments, documentation, and commits in English | **PASS** | Fully followed across all specs, contracts, and code. |
| **Modularity & Non-Destructive Changes** | Native platform code in `port/`, original `src/` untouched unless gated | **PASS** | Shims placed in `port/include/` (`nitro.h`, `MSL.h`, `nnsys.h`); original `src/` files preserved with only minimal `#ifdef PLATFORM_NATIVE` guards strictly for 64-bit portability where necessary (`system.c`, `main.c`). |
| **Memory Safety & 64-Bit Compatibility** | Normalized types, no 32-bit pointer truncation, safe pointer math | **PASS** | Exact fixed-width types used (`u32`, `s32`, `uintptr_t`) under `PLATFORM_NATIVE`. |
| **Documentation Preservation** | Preserve decompilation comments and document new HAL interfaces | **PASS** | All new contracts and shims fully documented with Doxygen comments. |
| **Small, Testable Increments** | Work in phased, independently testable slices | **PASS** | Phased progression: Window Icon -> SDK Shims -> Game Thread -> VBlank Sync -> Graphics Bridge. |

---

## Project Structure

### Documentation (this feature)

```text
specs/002-game-core-integration/
├── plan.md              # Implementation Plan
├── research.md          # Technical research & architectural decisions
├── data-model.md        # Entities, lifecycle states, and hardware registers
├── quickstart.md        # Build and run instructions
├── contracts/           # Native interfaces & contracts
│   ├── rom_icon.h       # 32x32 ROM icon decoding and window application
│   └── game_core.h      # Game thread lifecycle, NitroMain startup, and input bridge
└── checklists/
    └── requirements.md  # Spec quality validation checklist
```

### Source Code Layout

```text
port/
├── display/
│   ├── display_manager.cpp    # Updated to apply ROM icon on window creation
│   ├── display_manager.h
│   └── rom_icon_data.h        # Embedded 32x32 RGBA pixel array decoded from heartgold.us/
├── include/
│   ├── MSL.h                  # Metrowerks standard C library forwarder
│   ├── nitro.h                # NitroSDK master umbrella header
│   ├── nnsys.h                # NitroSystem middleware stubs
│   ├── nitro/
│   │   ├── types.h            # 64-bit normalized types
│   │   ├── code16.h           # Thumb-16 mode stub (no-op on desktop)
│   │   └── code32.h           # ARM-32 mode stub
│   ├── port_game_core.h       # Game thread lifecycle & NitroMain coordinator
│   └── port_hw_registers.h    # Emulated DS I/O registers (reg_GX_*, reg_G2_*)
├── hal/
│   ├── os/
│   │   ├── game_core.cpp      # GameCore_Start(), NitroMain thread spawn, and shutdown
│   │   └── os_interrupt.cpp   # VBlank condition variable barrier unblocking OS_WaitIrq
│   └── gx/
│       ├── gx_native.cpp      # 2D/3D framebuffer composite routing to dual viewports
│       └── hw_registers.cpp   # Hardware register storage & VRAM buffer mappings
├── main.cpp                   # Updated to spawn game core thread and update inputs
└── tests/
    ├── test_rom_icon.cpp      # Unit test for 32x32 icon decoding and pixel dimensions
    ├── test_game_core.cpp     # Unit test for game thread startup, VBlank wait, and stop
    └── test_engine_sync.cpp   # Unit test for input bridge (ReadKeypadAndTouchpad) and VBlank pacing
```

---

## Implementation Phases

### Phase 1: Window Icon & ROM Branding (User Story 2 - P2)
- Decode 32x32 icon from `heartgold.us/icon.nbfc` + `icon.nbfp` / `icon.png`.
- Generate `port/display/rom_icon_data.h` and implement `DisplayManager_SetWindowIcon`.
- Verify host window displays the authentic icon in titlebar and taskbar.

### Phase 2: Nintendo DS SDK Compatibility Shims (User Story 1 - P1)
- Create `port/include/MSL.h`, `port/include/nitro.h`, `port/include/nnsys.h`, and `port/include/nitro/code16.h`.
- Verify that a sample `src/` file can compile cleanly with host GCC/Clang.

### Phase 3: Game Core Thread & Lifecycle (User Story 1 - P1)
- Implement `port/hal/os/game_core.cpp` and `port_game_core.h`.
- Connect `NativeOS_TriggerVBlank()` to `OS_WaitIrq(TRUE, OS_IE_VBLANK)` via condition variable.
- Integrate game thread launch into `port/main.cpp` with graceful shutdown on `SDL_QUIT`.

### Phase 4: Input & Graphics Framebuffer Bridging (User Story 3 - P3)
- Feed `NativeInput_GetTouchState` and gamepad bits into `ReadKeypadAndTouchpad()`.
- Route game render outputs to `gTopScreenPixels` and `gTouchScreenPixels`.
- Execute full automated test suite via `ctest`.
