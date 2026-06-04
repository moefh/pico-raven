#ifndef DRAW_ROOM_H_FILE
#define DRAW_ROOM_H_FILE

#include "game_data.h"

#define DRAW_ROOM_INFO_MAX_MAPS 16

struct DRAW_MAP_INFO {
    // first/last tile indices
    int tile_x_first;
    int tile_x_last;
    int tile_y_first;
    int tile_y_last;

    // position of first tile in screen coords (may be negative)
    int screen_x;
    int screen_y;

    const struct RAVEN_MAP *map;
};

struct DRAW_ROOM_INFO {
    int n_maps;
    struct DRAW_MAP_INFO *maps[DRAW_ROOM_INFO_MAX_MAPS];
};

struct MEM_ARENA;
struct GAME_STATE;

void draw_room_init_room(struct GAME_STATE *game);
struct DRAW_ROOM_INFO *draw_room_init_frame(struct MEM_ARENA *mem, struct GAME_STATE *game);
void draw_room_bg(struct DRAW_ROOM_INFO *info);
void draw_room_fg(struct DRAW_ROOM_INFO *info);

#endif /* DRAW_ROOM_H_FILE */
