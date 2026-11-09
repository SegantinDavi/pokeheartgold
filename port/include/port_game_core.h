/**
 * @file port_game_core.h
 * @brief Decompiled game engine lifecycle and threading coordinator.
 */

#ifndef PORT_GAME_CORE_H
#define PORT_GAME_CORE_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum GameCoreStatus {
    GAME_CORE_UNINITIALIZED = 0,
    GAME_CORE_RUNNING       = 1,
    GAME_CORE_WAITING_VBLANK = 2,
    GAME_CORE_STOPPED       = 3
} GameCoreStatus;

typedef void (*GameCoreMainFunc)(void);

/**
 * @brief Sets a custom game entry point function (defaults to NitroMain).
 */
void GameCore_SetMainFunction(GameCoreMainFunc func);

/**
 * @brief Launches the decompiled game loop on a dedicated thread.
 * @return True if thread was successfully started.
 */
bool GameCore_Start(void);

/**
 * @brief Signals the game engine thread to stop and waits for clean exit.
 * @param timeoutMs Maximum milliseconds to wait for join.
 * @return True if thread joined cleanly.
 */
bool GameCore_Stop(uint32_t timeoutMs);

/**
 * @brief Returns the current execution status of the game core.
 */
GameCoreStatus GameCore_GetStatus(void);

/**
 * @brief Returns the count of frames processed by the game engine.
 */
uint64_t GameCore_GetFrameCount(void);

/**
 * @brief Increments the completed game frame counter.
 */
void GameCore_IncrementFrameCount(void);

/**
 * @brief Synchronizes game input state before frame execution.
 */
void GameCore_UpdateInput(uint16_t padButtons, uint8_t touchX, uint8_t touchY, bool touching);

/**
 * @brief Retrieves the latest buffered input state.
 */
void GameCore_GetInput(uint16_t* padButtons, uint8_t* touchX, uint8_t* touchY, bool* touching);

#ifdef __cplusplus
}
#endif

#endif // PORT_GAME_CORE_H
