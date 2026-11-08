# Feature Specification: Native PC Port with Configurable Dual-Screen Display

**Feature Branch**: `001-native-pc-port`  
**Created**: 2026-11-08  
**Status**: Draft  
**Input**: User description: "Native PC port with configurable dual screen layout. By default, screens are positioned side-by-side to fit modern widescreen monitor resolutions, with support for swapping them or using the original Nintendo DS vertical layout."

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Native PC Gameplay with Side-by-Side Dual Screens (Priority: P1)

A player launches Pokémon HeartGold natively on their desktop PC without an emulator. The game window opens in a widescreen-friendly side-by-side presentation, placing the Top Screen (overworld/battle graphics) on the left and the Touchscreen (menus, Pokégear, battle commands) on the right. The player controls their character with their keyboard or gamepad and interacts with touch menus using their mouse.

**Why this priority**: This is the core MVP experience. Standard modern PC monitors are 16:9 or 21:9 widescreen; presenting the dual 4:3 screens side-by-side (8:3 aspect ratio) provides the most ergonomic default experience on desktop while enabling immediate gameplay.

**Independent Test**: Can be fully tested by launching the compiled executable, confirming both screens render in real time side-by-side at 60 FPS, moving the player character with the keyboard/gamepad, and clicking on touchscreen UI elements with the mouse.

**Acceptance Scenarios**:

1. **Given** the native application executable, **When** the player launches it, **Then** a window opens with two viewports side-by-side (Top Screen left, Touchscreen right), displaying the game intro and title screen at 60 FPS.
2. **Given** the side-by-side display layout, **When** the player clicks or drags the mouse cursor over the right-hand touchscreen area, **Then** the game registers touch inputs mapped to coordinates [0..255, 0..191].
3. **Given** the game is running, **When** the player presses mapped keyboard keys or gamepad buttons, **Then** character movement, dialogue progression, and menu navigation respond with zero perceived latency.

---

### User Story 2 - Configurable Screen Layouts and Orientation (Priority: P2)

A player wants to customize the arrangement of the two screens based on their display setup (e.g., vertical monitor, personal ergonomic preference, or touchscreen laptop). The player can switch among horizontal side-by-side (default), swapped horizontal, classic vertical DS (stacked top-over-bottom), or swapped vertical layout. The mouse touch coordinates dynamically adapt to the new screen position.

**Why this priority**: DS games alternate focus between the top and bottom screens depending on the activity (Pokéathlon, Pokédex, Battles). Allowing dynamic layout adjustments provides vital accessibility and comfort.

**Independent Test**: Can be tested by triggering a layout toggle hotkey or configuration setting; the viewports immediately rearrange into the selected layout, and mouse clicks on the touchscreen register accurately in its new position.

**Acceptance Scenarios**:

1. **Given** the application in horizontal side-by-side mode, **When** the user presses the layout toggle shortcut or updates configuration, **Then** the viewports rearrange to vertical mode (Top Screen above Touchscreen) without restarting the game.
2. **Given** any of the four layout modes (Horizontal, Swapped Horizontal, Vertical, Swapped Vertical), **When** clicking or dragging inside the touchscreen viewport, **Then** the touch coordinates are accurately translated to [0..255, 0..191] regardless of window scale or viewport position.
3. **Given** a player selected their preferred layout, **When** the application restarts, **Then** the game launches directly in the previously chosen layout.

---

### User Story 3 - Audio Synthesis and Save Game Persistence (Priority: P3)

A player embarks on an adventure, saves their progress using the in-game save feature, and resumes later. The game outputs rich, synchronized stereo audio for music and sound effects, and stores save files locally on the host filesystem.

**Why this priority**: Required for full game progression and immersion beyond standalone visual demos.

**Independent Test**: Can be tested by playing to the first save prompt, saving progress, closing the game, reopening, and verifying that the player's saved game is restored from the local save file.

**Acceptance Scenarios**:

1. **Given** active gameplay, **When** the player saves their game via the in-game touch save or start menu, **Then** a valid save file is written to the user's persistent local directory.
2. **Given** an existing local save file, **When** starting the application, **Then** the continue option appears on the title screen with accurate player details and loads without corruption.
3. **Given** the game is playing background music and sound effects, **When** audio events occur, **Then** sound is rendered through the host audio output in stereo without popping, stuttering, or desynchronization.

---

### Edge Cases

- **Window Resizing and Scaling**: When the user resizes the window, both screen viewports must maintain their fixed 4:3 aspect ratio with appropriate pillarboxing/letterboxing, and touch coordinate normalization must adapt proportionally.
- **Mouse Drag Leaving Viewport**: If the player begins a touch drag on the touchscreen and the cursor leaves the touchscreen bounds, the touch must either clamp to the nearest screen edge or release touch contact consistently with hardware digitizer behavior.
- **Layout Switch Mid-Animation / Mid-3D Render**: Switching display layouts while a 3D cutscene or battle transition is active must not crash the renderer or corrupt graphics buffers.
- **Missing or Corrupted Save File**: If a save file cannot be read or is invalid, the game must report the issue gracefully and offer to start a new game without crashing.
- **Missing Asset Files**: If extracted game assets or data archives are missing from the asset directory, the executable must log clear error diagnostics detailing the missing assets.

---

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: System MUST compile into a native 64-bit standalone executable for modern desktop operating systems (Linux, Windows, macOS).
- **FR-002**: System MUST render both Nintendo DS video streams (Top Screen 256x192 and Sub/Touchscreen 256x192) at a smooth 60 frames per second.
- **FR-003**: System MUST provide a configurable display layout manager supporting four view arrangements:
  - **Mode 1 (Default)**: Horizontal Side-by-Side (Top Screen Left, Touchscreen Right).
  - **Mode 2**: Swapped Horizontal (Touchscreen Left, Top Screen Right).
  - **Mode 3**: Vertical Classic (Top Screen Above, Touchscreen Below).
  - **Mode 4**: Swapped Vertical (Touchscreen Above, Top Screen Below).
- **FR-004**: System MUST dynamically translate mouse pointer events into Nintendo DS touchscreen coordinates [0..255, 0..191] corresponding to the active position, scale, and aspect ratio of the touchscreen viewport in any layout mode.
- **FR-005**: System MUST allow runtime switching of screen layouts through user-configurable shortcuts or settings without interrupting game execution.
- **FR-006**: System MUST persist display preferences (chosen layout mode, window dimensions, fullscreen state) in a local configuration file.
- **FR-007**: System MUST provide customizable input mapping for standard Nintendo DS buttons (A, B, X, Y, D-Pad, L, R, Start, Select) supporting keyboard and modern game controllers.
- **FR-008**: System MUST stream Nintendo DS audio channels and sound sequences to the host audio output device in stereo.
- **FR-009**: System MUST read game assets (graphics, sound archives, text, maps) directly from the local filesystem or packaged asset directories.
- **FR-010**: System MUST persist and restore game progress to and from persistent storage on the host filesystem using standard 512KB save file format.

### Key Entities

- **Screen Layout Configuration**: Encapsulates the visual arrangement, position offsets, and scaling metrics of both Nintendo DS screen viewports within the host application window.
- **Input Context**: Maps physical host input events (keyboard keys, gamepad buttons/axes, mouse cursor position and clicks) to virtual Nintendo DS button states and normalized touch coordinates.
- **Host Audio Stream**: Represents the continuous PCM audio output buffer fed to the host audio backend (SDL Audio/miniaudio), generated from the game's sound subsystem.
- **Save File Profile**: Represents persistent player progress, settings, and party data stored on the host filesystem.

---

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: Application launches and renders the main title screen in under 3 seconds on standard desktop hardware.
- **SC-002**: Game maintains a consistent 60 frames per second with frame rendering latency under 16.6 milliseconds.
- **SC-003**: 100% of mouse click and drag interactions within the touchscreen viewport translate to the exact corresponding 256x192 Nintendo DS touch coordinates across all 4 layout configurations.
- **SC-004**: Display layout switches execute in under 100 milliseconds with zero visual distortion, framebuffer artifacts, or game logic stalls.
- **SC-005**: Game progress can be saved and reloaded across application lifecycles with 100% data fidelity.
- **SC-006**: Audio latency remains below 50 milliseconds with zero buffer underruns, clicks, or pops during regular gameplay and battles.
