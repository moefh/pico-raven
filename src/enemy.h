#ifndef ENEMY_H_FILE
#define ENEMY_H_FILE

#include <stdint.h>

struct RAVEN_ROOM_TRIGGER_INFO;

void enemy_init(int enemy_index, const struct RAVEN_ROOM_TRIGGER_INFO *spawn);
void enemy_remove_id(uint16_t enemy_id);
void enemy_update_all(void);

#endif /* ENEMY_H_FILE */
