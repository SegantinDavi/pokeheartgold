/**
 * @file input_manager.h
 * @brief Native input polling, button mapping, and touchscreen digitizer emulation.
 */

#ifndef PORT_INPUT_MANAGER_H
#define PORT_INPUT_MANAGER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NATIVE_PAD_BUTTON_A      0x0001
#define NATIVE_PAD_BUTTON_B      0x0002
#define NATIVE_PAD_BUTTON_SELECT 0x0004
#define NATIVE_PAD_BUTTON_START  0x0008
#define NATIVE_PAD_KEY_RIGHT     0x0010
#define NATIVE_PAD_KEY_LEFT      0x0020
#define NATIVE_PAD_KEY_UP        0x0040
#define NATIVE_PAD_KEY_DOWN      0x0080
#define NATIVE_PAD_BUTTON_R      0x0100
#define NATIVE_PAD_BUTTON_L      0x0200
#define NATIVE_PAD_BUTTON_X      0x0400
#define NATIVE_PAD_BUTTON_Y      0x0800

typedef struct {
    bool touching;
    uint8_t screenX;     // [0..255]
    uint8_t screenY;     // [0..191]
    uint16_t rawX;       // [0..4095]
    uint16_t rawY;       // [0..4095]
} NativeTouchState;

bool NativeInput_Init(void);
bool NativeInput_Poll(void);
uint16_t NativeInput_GetPadButtons(void);
void NativeInput_GetTouchState(NativeTouchState* outState);

/* Simulation and event handling */
void NativeInput_SetButtonState(uint16_t buttonMask, bool pressed);
void NativeInput_ProcessKeyEvent(int sdlKeycode, bool isKeyDown);
void NativeInput_ProcessMouseEvent(int hostX, int hostY, bool isButtonDown);
void NativeInput_Shutdown(void);

#ifdef __cplusplus
}
#endif

#endif // PORT_INPUT_MANAGER_H
