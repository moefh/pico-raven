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
int room_tiles_block_rect(struct COLLISION_RECT *rect);

static inline int collision_is_point_in_rect(int x, int y, struct COLLISION_RECT *rect)
{
    return x >= rect->x && y >= rect->y && x < rect->x + rect->w && y < rect->y + rect->h;
}

static inline int collision_rects_intersect(struct COLLISION_RECT *ra, struct COLLISION_RECT *rb)
{
    int a1_x = ra->x;
    int a2_x = ra->x + ra->w;
    int a1_y = ra->y;
    int a2_y = ra->y + ra->h;
    int b1_x = rb->x;
    int b2_x = rb->x + rb->w;
    int b1_y = rb->y;
    int b2_y = rb->y + rb->h;
    return a1_x < b2_x && b1_x < a2_x && a1_y < b2_y && b1_y < a2_y;
}

static inline uint8_t collision_get_room_tile_at(int tx, int ty)
{
    if (tx < 0 || ty < 0 || tx >= game_state.room_tiles_w || ty >= game_state.room_tiles_h) {
        return TILE_FX_BLOCK;
    }
    return game_state.fx_tiles[ty * game_state.room_tiles_w + tx] & 0x0f;
}

#endif /* COLLISION_H_FILE */
