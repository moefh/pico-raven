#ifndef SCREEN_H_FILE
#define SCREEN_H_FILE

#include "game.h"

struct JOYSTICK;

int screen_init(void);
void screen_render(struct GAME_STATE *game, struct JOYSTICK *joy);

void screen_save_to_scratch(int clear_after);
void screen_fade_from_scratch(int level);

#endif /* SCREEN_H_FILE */
