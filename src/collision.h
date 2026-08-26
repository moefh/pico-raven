#ifndef COLLISION_H_FILE
#define COLLISION_H_FILE

#include "game_data.h"

#define COLLISION_FLAGS_RAMP  (1<<0)
#define COLLISION_FLAGS_DOWN  (1<<1)
#define COLLISION_FLAGS_UP    (1<<2)
#define COLLISION_FLAGS_LEFT  (1<<3)
#define COLLISION_FLAGS_RIGHT (1<<4)

#define TILE_FX_BLOCK       0
#define TILE_FX_L_RAMP_FULL 1
#define TILE_FX_R_RAMP_FULL 2
#define TILE_FX_L_RAMP_BOT  3
#define TILE_FX_L_RAMP_TOP  4
#define TILE_FX_R_RAMP_BOT  5
#define TILE_FX_R_RAMP_TOP  6
#define TILE_FX_FREE        0xff

struct COLLISION_RECT {
    int32_t x;
    int32_t y;
    int16_t w;
    int16_t h;
};

struct GAME_STATE;

int collision_move(struct GAME_STATE *game, struct COLLISION_RECT *rect, int dx, int dy);
uint8_t collision_get_room_tile_at(struct GAME_STATE *game, int tx, int ty);

static inline int collision_is_point_in_rect(int x, int y, struct COLLISION_RECT *rect)
{
    return x >= rect->x && y >= rect->y && x < rect->x + rect->w && y < rect->y + rect->h;
}

#endif /* COLLISION_H_FILE */
