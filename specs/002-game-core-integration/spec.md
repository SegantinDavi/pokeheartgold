# Feature Specification: Game Core Integration & Window Branding

**Feature Branch**: `002-game-core-integration`  
**Created**: 2026-11-08  
**Status**: Draft  
**Input**: "Game core decomp engine integration with native HAL, NitroMain startup, and ROM window icon"

---

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Decompiled Game Core Startup & Lifecycle (Priority: P1)

As a player launching the native desktop application, I want the executable to initialize the decompiled game engine (`NitroMain`) and execute the game's startup sequence, so that the game transitions from platform initialization directly into Pokémon HeartGold gameplay without manual intervention.

**Why this priority**: Connecting the decompiled game logic in `src/` to the native Hardware Abstraction Layer (`port/`) is the central milestone that transforms the project from a platform test harness into the actual playable game.

**Independent Test**: Launch the executable `./build-native/pokeheartgold`, verify that platform subsystems initialize and the game logic thread starts, executes `NitroMain()`, and begins advancing the engine state.

**Acceptance Scenarios**:

1. **Given** the standalone executable is launched, **When** platform HAL initialization completes, **Then** the decompiled game core entry point (`NitroMain`) is invoked on a coordinated game thread.
2. **Given** the game core is running, **When** the game executes its primary while-loop, **Then** frame execution synchronizes with the 59.8261 Hz virtual VBlank interrupt without thread deadlocks.
3. **Given** the user closes the application window, **When** the quit event is triggered, **Then** the game core shuts down gracefully and releases all allocated memory resources.

---

### User Story 2 - Official ROM Window Icon & Title Branding (Priority: P2)

As a player on desktop (Linux/Windows/macOS), I want the application window and taskbar to display the authentic Pokémon HeartGold 32x32 game cartridge icon and title, so that the port feels like an authentic, polished native desktop experience.

**Why this priority**: High visual polish and immediate visual identity. The user specifically requested extracting the original icon from the game's ROM metadata (`heartgold.us/icon.png`) and setting it as the window icon.

**Independent Test**: Launch `./build-native/pokeheartgold` on a graphical desktop, inspect the host window titlebar, taskbar, and alt-tab switcher, verifying that the 32x32 HeartGold icon is rendered sharply with proper transparency.

**Acceptance Scenarios**:

1. **Given** the application starts, **When** the host SDL2 window is created, **Then** the 32x32 icon from ROM assets (`heartgold.us/icon.png` or decoded RGBA icon buffer) is loaded and applied via `SDL_SetWindowIcon`.
2. **Given** the window is displayed on the host operating system, **When** viewed in the taskbar or application switcher, **Then** the icon appears with clean transparency and correct aspect ratio.
3. **Given** the asset file is missing or corrupted, **When** the window initializes, **Then** the application falls back gracefully to a built-in embedded icon without crashing.

---

### User Story 3 - Input & Graphics Framebuffer Synchronization (Priority: P3)

As a player interacting with the game, I want my keyboard, gamepad, and mouse-touch actions to be read directly by the decompiled game's input routines (`ReadKeypadAndTouchpad`), and the game's rendered 2D/3D output to be presented onto the dual viewports, so that I can see and control the game seamlessly.

**Why this priority**: Completes the bidirectional feedback loop between the host hardware and the game engine, enabling interactive control of the title screen and menus.

**Independent Test**: Press mapped input keys (D-Pad, A, B, Start) and click the touchscreen while the game is running, verify that `gSystem.heldKeysRaw` and touch structures reflect the inputs in the game logic, and rendered frames update the Top and Touchscreen viewports.

**Acceptance Scenarios**:

1. **Given** the player presses a mapped key or moves the mouse, **When** the game executes `ReadKeypadAndTouchpad()`, **Then** the decompiled game state receives the active button bitmask and virtual touch coordinates [0..255, 0..191].
2. **Given** the game engine updates its display memory (VRAM / BG / OAM), **When** a frame finishes rendering, **Then** the pixel output is copied into `gTopScreenPixels` and `gTouchScreenPixels` and presented via the OpenGL compositor.

---

### Edge Cases

- **Asset Path Resolution**: What happens if the game is executed from a working directory other than the repository root? The VFS and asset loader must resolve `files/` and `heartgold.us/` relative to the executable path or current directory.
- **VBlank Starvation / Drift**: If the host OS experiences a lag spike or window drag, how does the game loop recover? The virtual interrupt scheduler must prevent timer debt accumulation by capping catch-up frames.
- **Unimplemented Hardware Registers**: When `src/` reads or writes memory-mapped I/O registers not yet fully emulated in software, the accesses must log a warning and return safe default values rather than triggering a host memory segmentation fault.

---

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: System MUST provide SDK compatibility shims for `<nitro.h>`, `<nnsys.h>`, and `<MSL.h>` that map Nintendo DS system calls to the native Hardware Abstraction Layer.
- **FR-002**: System MUST compile core decompiled engine sources from `src/` (system initialization, memory management, task queues, and main state machine) into the native desktop binary using the desktop C11/C++17 toolchain.
- **FR-003**: System MUST launch the decompiled game entry point (`NitroMain`) within a coordinated game execution thread while preserving host event handling in the main thread.
- **FR-004**: System MUST extract and decode the official 32x32 Pokémon HeartGold icon from ROM metadata assets (`heartgold.us/icon.png` or embedded RGBA pixel array) and apply it to the application window via `SDL_SetWindowIcon`.
- **FR-005**: System MUST bridge host input events (keyboard, gamepad, and mouse-touch) into the game's global system structures (`gSystem.heldKeysRaw` and touch digitizer records).
- **FR-006**: System MUST route 2D and 3D graphics buffers generated by the game engine directly into the dual-screen compositor (`gTopScreenPixels` and `gTouchScreenPixels`).
- **FR-007**: System MUST synchronize game logic execution with the virtual 59.8261 Hz VBlank interrupt (`OS_WaitIrq(TRUE, OS_IE_VBLANK)`), locking frame timing to 16.6ms per frame.
- **FR-008**: System MUST support clean shutdown on window close (`SDL_QUIT`), safely terminating the game thread and cleanly flushing save data to `pokemon_heartgold.sav`.

---

### Key Entities

- **`RomIconData`**: 32x32 RGBA pixel buffer decoded from `heartgold.us/icon.png` or embedded fallback data, used for host window icon decoration.
- **`GameCoreThread`**: Dedicated background thread managing the execution cycle of `NitroMain()` and the primary engine state loop.
- **`HardwareRegisterMap`**: Memory-mapped or structured representation of Nintendo DS I/O registers (`reg_GX_*`, `reg_G2_*`, VRAM banks) backed by native host buffers.
- **`FrameSyncBarrier`**: Synchronization primitive synchronizing the game core thread's VBlank wait with the host window's 59.8261 Hz compositor refresh.

---

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: The application window renders the official 32x32 Pokémon HeartGold icon in the window titlebar and desktop taskbar immediately upon creation.
- **SC-002**: The native application boots and executes `NitroMain()` within 2.0 seconds of process startup.
- **SC-003**: Frame timing remains locked at 59.8261 Hz (16.6ms ± 1ms) throughout the game loop execution.
- **SC-004**: Keyboard, gamepad, and mouse touch inputs are registered and reflected in the game's input state structure with latency under 16ms.
- **SC-005**: The application shuts down cleanly with exit code 0 when closed via window decoration or `ESC`, with zero memory leaks in the HAL subsystems.
