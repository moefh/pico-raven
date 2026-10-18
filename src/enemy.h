#ifndef ENEMY_H_FILE
#define ENEMY_H_FILE

struct GAME_STATE;
struct RAVEN_ROOM_TRIGGER_INFO;

void enemy_init(int enemy_index, const struct RAVEN_ROOM_TRIGGER_INFO *spawn);
void enemy_update(struct GAME_STATE *game, int enemy_index);

#endif /* PLAYER_H_FILE */
