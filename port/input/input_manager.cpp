/**
 * @file input_manager.cpp
 * @brief Host keyboard, gamepad, and mouse touch translation to Nintendo DS hardware registers.
 */

#include "input_manager.h"
#include "display/display_manager.h"
#include <SDL.h>
#include <atomic>
#include <mutex>
#include <vector>

namespace {

std::atomic<uint16_t> gPadButtons{0};
std::mutex gTouchMutex;
NativeTouchState gTouchState{false, 0, 0, 0, 0};
std::vector<SDL_GameController*> gControllers;

uint16_t KeycodeToPadMask(int keycode) {
    switch (keycode) {
        case SDLK_x:         return NATIVE_PAD_BUTTON_A;
        case SDLK_z:         return NATIVE_PAD_BUTTON_B;
        case SDLK_s:         return NATIVE_PAD_BUTTON_X;
        case SDLK_a:         return NATIVE_PAD_BUTTON_Y;
        case SDLK_UP:        return NATIVE_PAD_KEY_UP;
        case SDLK_DOWN:      return NATIVE_PAD_KEY_DOWN;
        case SDLK_LEFT:      return NATIVE_PAD_KEY_LEFT;
        case SDLK_RIGHT:     return NATIVE_PAD_KEY_RIGHT;
        case SDLK_q:         return NATIVE_PAD_BUTTON_L;
        case SDLK_w:         return NATIVE_PAD_BUTTON_R;
        case SDLK_RETURN:    return NATIVE_PAD_BUTTON_START;
        case SDLK_BACKSPACE: return NATIVE_PAD_BUTTON_SELECT;
        default:             return 0;
    }
}

uint16_t GamepadButtonToPadMask(Uint8 btn) {
    switch (btn) {
        case SDL_CONTROLLER_BUTTON_A:             return NATIVE_PAD_BUTTON_A;
        case SDL_CONTROLLER_BUTTON_B:             return NATIVE_PAD_BUTTON_B;
        case SDL_CONTROLLER_BUTTON_X:             return NATIVE_PAD_BUTTON_X;
        case SDL_CONTROLLER_BUTTON_Y:             return NATIVE_PAD_BUTTON_Y;
        case SDL_CONTROLLER_BUTTON_DPAD_UP:       return NATIVE_PAD_KEY_UP;
        case SDL_CONTROLLER_BUTTON_DPAD_DOWN:     return NATIVE_PAD_KEY_DOWN;
        case SDL_CONTROLLER_BUTTON_DPAD_LEFT:     return NATIVE_PAD_KEY_LEFT;
        case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:    return NATIVE_PAD_KEY_RIGHT;
        case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:  return NATIVE_PAD_BUTTON_L;
        case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: return NATIVE_PAD_BUTTON_R;
        case SDL_CONTROLLER_BUTTON_START:         return NATIVE_PAD_BUTTON_START;
        case SDL_CONTROLLER_BUTTON_BACK:          return NATIVE_PAD_BUTTON_SELECT;
        default:                                  return 0;
    }
}

} // anonymous namespace

extern "C" {

bool NativeInput_Init(void) {
    gPadButtons.store(0);
    {
        std::lock_guard<std::mutex> lock(gTouchMutex);
        gTouchState = {false, 0, 0, 0, 0};
    }

    int numJoysticks = SDL_NumJoysticks();
    for (int i = 0; i < numJoysticks; ++i) {
        if (SDL_IsGameController(i)) {
            SDL_GameController* pad = SDL_GameControllerOpen(i);
            if (pad) {
                gControllers.push_back(pad);
            }
        }
    }
    return true;
}

void NativeInput_SetButtonState(uint16_t buttonMask, bool pressed) {
    if (pressed) {
        gPadButtons.fetch_or(buttonMask);
    } else {
        gPadButtons.fetch_and(~buttonMask);
    }
}

void NativeInput_ProcessKeyEvent(int sdlKeycode, bool isKeyDown) {
    uint16_t mask = KeycodeToPadMask(sdlKeycode);
    if (mask != 0) {
        NativeInput_SetButtonState(mask, isKeyDown);
    }
}

void NativeInput_ProcessMouseEvent(int hostX, int hostY, bool isButtonDown) {
    std::lock_guard<std::mutex> lock(gTouchMutex);
    if (!isButtonDown) {
        gTouchState.touching = false;
        return;
    }

    uint8_t touchX = 0;
    uint8_t touchY = 0;
    bool inTouchScreen = DisplayManager_ScreenToTouchCoords(hostX, hostY, &touchX, &touchY);

    if (inTouchScreen) {
        gTouchState.touching = true;
        gTouchState.screenX = touchX;
        gTouchState.screenY = touchY;
        gTouchState.rawX = static_cast<uint16_t>((static_cast<uint32_t>(touchX) * 4095) / 255);
        gTouchState.rawY = static_cast<uint16_t>((static_cast<uint32_t>(touchY) * 4095) / 191);
    } else {
        gTouchState.touching = false;
    }
}

bool NativeInput_Poll(void) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            return false;
        } else if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
            bool down = (event.type == SDL_KEYDOWN);
            if (down && event.key.keysym.sym == SDLK_F10) {
                DisplayManager_CycleLayoutMode();
            } else {
                NativeInput_ProcessKeyEvent(event.key.keysym.sym, down);
            }
        } else if (event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP) {
            bool down = (event.type == SDL_MOUSEBUTTONDOWN);
            if (event.button.button == SDL_BUTTON_LEFT) {
                NativeInput_ProcessMouseEvent(event.button.x, event.button.y, down);
            }
        } else if (event.type == SDL_MOUSEMOTION) {
            if (event.motion.state & SDL_BUTTON_LMASK) {
                NativeInput_ProcessMouseEvent(event.motion.x, event.motion.y, true);
            }
        } else if (event.type == SDL_CONTROLLERBUTTONDOWN || event.type == SDL_CONTROLLERBUTTONUP) {
            bool down = (event.type == SDL_CONTROLLERBUTTONDOWN);
            uint16_t mask = GamepadButtonToPadMask(event.cbutton.button);
            if (mask != 0) {
                NativeInput_SetButtonState(mask, down);
            }
        }
    }
    return true;
}

uint16_t NativeInput_GetPadButtons(void) {
    return gPadButtons.load();
}

void NativeInput_GetTouchState(NativeTouchState* outState) {
    if (!outState) return;
    std::lock_guard<std::mutex> lock(gTouchMutex);
    *outState = gTouchState;
}

void NativeInput_Shutdown(void) {
    for (SDL_GameController* pad : gControllers) {
        if (pad) {
            SDL_GameControllerClose(pad);
        }
    }
    gControllers.clear();
    gPadButtons.store(0);
}

} // extern "C"
