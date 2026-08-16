#ifndef SCREEN_H_FILE
#define SCREEN_H_FILE

#include "game.h"

struct JOYSTICK;

int screen_init(void);
void screen_render(struct GAME_STATE *game, struct JOYSTICK *joy);

#endif /* SCREEN_H_FILE */
