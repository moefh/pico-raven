#ifndef PLAYER_H_FILE
#define PLAYER_H_FILE

#include "game.h"

void player_set_sprite_anim(int animation_id);
void player_init(void);
void player_update_sprite_info(void);
void player_update(void);

void player_control_init(void);
void player_control_update(void);

#endif /* PLAYER_H_FILE */
