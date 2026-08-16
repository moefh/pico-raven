#ifndef COLLISION_H_FILE
#define COLLISION_H_FILE

#include "game_data.h"

#define COLLISION_FLAGS_RAMP  (1<<0)
#define COLLISION_FLAGS_DOWN  (1<<1)
#define COLLISION_FLAGS_UP    (1<<2)
#define COLLISION_FLAGS_LEFT  (1<<3)
#define COLLISION_FLAGS_RIGHT (1<<4)

struct COLLISION_RECT {
    int32_t x;
    int32_t y;
    int16_t w;
    int16_t h;
};

struct GAME_STATE;

int collision_move(struct GAME_STATE *game, struct COLLISION_RECT *rect, int dx, int dy);
uint8_t collision_get_room_tile_at(struct GAME_STATE *game, int tx, int ty);

#endif /* COLLISION_H_FILE */
