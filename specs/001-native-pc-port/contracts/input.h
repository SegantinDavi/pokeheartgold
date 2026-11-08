/**
 * @file input.h
 * @brief Native Input and Digitizer Translation Contract
 */

#ifndef PORT_INPUT_H
#define PORT_INPUT_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    bool touching;
    uint8_t screenX;     // [0..255]
    uint8_t screenY;     // [0..191]
    uint16_t rawX;       // [0..4095]
    uint16_t rawY;       // [0..4095]
} NativeTouchState;

/**
 * @brief Initialize input polling subsystem (SDL Keyboard, Mouse, and GameController).
 */
bool NativeInput_Init(void);

/**
 * @brief Poll host events from SDL event pump.
 * @return false if quit requested (window close / Alt+F4), true to continue running.
 */
bool NativeInput_Poll(void);

/**
 * @brief Get current 16-bit bitmask of Nintendo DS buttons (matching PAD_BUTTON_*).
 */
uint16_t NativeInput_GetPadButtons(void);

/**
 * @brief Get current touchscreen state for digitizer emulation.
 */
void NativeInput_GetTouchState(NativeTouchState* outState);

/**
 * @brief Shutdown input subsystem and close open game controllers.
 */
void NativeInput_Shutdown(void);

#ifdef __cplusplus
}
#endif

#endif // PORT_INPUT_H
