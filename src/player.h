#ifndef PLAYER_H_FILE
#define PLAYER_H_FILE

#include "game.h"

struct JOYSTICK;

void player_set_sprite_anim(struct GAME_STATE *game, int animation_id);
void player_init(struct GAME_STATE *game);
void player_update_sprite_info(struct GAME_STATE *game);
void player_update(struct GAME_STATE *game, struct JOYSTICK *joy);

void player_control_init(struct GAME_STATE *game);
void player_control_update(struct GAME_STATE *game, struct JOYSTICK *joy);

#endif /* PLAYER_H_FILE */
