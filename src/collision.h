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

void collision_init_frame(uint16_t room_id);
int collision_move(struct COLLISION_RECT *rect, int dx, int dy);

#endif /* COLLISION_H_FILE */
