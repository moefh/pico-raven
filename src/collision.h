#ifndef COLLISION_H_FILE
#define COLLISION_H_FILE

#include "game_data.h"
#include "game.h"

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
#define TILE_FX_FREE        0xf

struct COLLISION_RECT {
    int32_t x;
    int32_t y;
    int16_t w;
    int16_t h;
};

int collision_move(struct COLLISION_RECT *rect, int dx, int dy);

static inline int collision_is_point_in_rect(int x, int y, struct COLLISION_RECT *rect)
{
    return x >= rect->x && y >= rect->y && x < rect->x + rect->w && y < rect->y + rect->h;
}

static inline uint8_t collision_get_room_tile_at(int tx, int ty)
{
    if (tx < 0 || ty < 0 || tx >= game_state.room_tiles_w || ty >= game_state.room_tiles_h) {
        return TILE_FX_BLOCK;
    }
    return game_state.fx_tiles[ty * game_state.room_tiles_w + tx] & 0x0f;
}

#endif /* COLLISION_H_FILE */
