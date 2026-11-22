#ifndef GAMEPAD_H_FILE
#define GAMEPAD_H_FILE

#include "game.h"
#include "lib/joystick.h"

#define GAMEPAD_UP      JOY_BTN_UP
#define GAMEPAD_DOWN    JOY_BTN_DOWN
#define GAMEPAD_LEFT    JOY_BTN_LEFT
#define GAMEPAD_RIGHT   JOY_BTN_RIGHT
#define GAMEPAD_B       JOY_BTN_B
#define GAMEPAD_Y       JOY_BTN_Y
#define GAMEPAD_A       JOY_BTN_A
#define GAMEPAD_X       JOY_BTN_X
#define GAMEPAD_L1      JOY_BTN_L1
#define GAMEPAD_R1      JOY_BTN_R1
#define GAMEPAD_L2      JOY_BTN_L2
#define GAMEPAD_R2      JOY_BTN_R2
#define GAMEPAD_SELECT  JOY_BTN_SELECT
#define GAMEPAD_START   JOY_BTN_START

#define PAD_PRESSED(btn)  ((game_state.joy_cur_state & (btn)) && ! (game_state.joy_last_state & (btn)))
#define PAD_RELEASED(btn) ((game_state.joy_last_state & (btn)) && ! (game_state.joy_cur_state & (btn)))
#define PAD_HELD(btn)     (game_state.joy_cur_state & (btn))

#endif /* GAMEPAD_H_FILE */
