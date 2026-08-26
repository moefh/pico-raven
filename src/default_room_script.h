#ifndef ROOMS_H_FILE
#define ROOMS_H_FILE

struct GAME_STATE;

void default_room_init(uint32_t room_id, struct GAME_STATE *game);
void default_room_update(struct GAME_STATE *game);

#endif /* ROOMS_H_FILE */
