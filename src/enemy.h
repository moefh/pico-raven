#ifndef ENEMY_H_FILE
#define ENEMY_H_FILE

#include "game.h"

struct RAVEN_ROOM_TRIGGER_INFO;

void enemy_init(struct GAME_STATE *game, int enemy_index, const struct RAVEN_ROOM_TRIGGER_INFO *spawn);
void enemy_update(struct GAME_STATE *game, int enemy_index);

#endif /* PLAYER_H_FILE */
