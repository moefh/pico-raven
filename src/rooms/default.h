#ifndef DEFAULT_H_FILE
#define DEFAULT_H_FILE

#include <stdint.h>

struct GAME_STATE;

void default_room_init(uint32_t room_id, struct GAME_STATE *game);
void default_room_update(struct GAME_STATE *game);

#endif /* DEFAULT_H_FILE */
