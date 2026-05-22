#include "draw_room.h"

#include "game.h"
#include "lib/vga_8bit.h"
#include "lib/mem.h"

#define TILE_SIZE 16

struct DRAW_ROOM_INFO *draw_room_init(struct MEM_ARENA *mem, struct GAME_STATE *game)
{
    struct DRAW_ROOM_INFO *draw_room_info = mem_add(mem, struct DRAW_ROOM_INFO);
    draw_room_info->n_maps = 0;
    for (int i = 0; i < game->room->num_maps; i++) {
        const struct RAVEN_ROOM_MAP_INFO *map_info = &game->room->maps[i];

        int map_x = game->screen_x - map_info->x * TILE_SIZE;
        int map_y = game->screen_y - map_info->y * TILE_SIZE;

        if (map_x + vga_screen.width <= 0  || map_x >= map_info->map->w * TILE_SIZE ||
            map_y + vga_screen.height <= 0 || map_y >= map_info->map->h * TILE_SIZE)
            continue; // map outside screen

        int map_tiles_w = map_info->map->w;
        int map_tiles_h = map_info->map->h;

        // allocate draw info
        struct DRAW_MAP_INFO *mi = mem_add(mem, struct DRAW_MAP_INFO);
        draw_room_info->maps[draw_room_info->n_maps++] = mi;

        // fill draw info
        mi->map = map_info->map;

        mi->tile_x_last = (map_x + vga_screen.width) / TILE_SIZE;
        if (map_x < 0) {
            mi->tile_x_first = 0;
            mi->screen_x = -map_x;
        } else {
            mi->tile_x_first = map_x / TILE_SIZE;
            if (mi->tile_x_last >= map_tiles_w) {
                mi->tile_x_last = map_tiles_w - 1;
            }
            mi->screen_x = -(map_x % TILE_SIZE);
        }

        mi->tile_y_last = (map_y + vga_screen.height) / TILE_SIZE;
        if (map_y < 0) {
            mi->tile_y_first = 0;
            mi->screen_y = -map_y;
        } else {
            mi->tile_y_first = map_y / TILE_SIZE;
            if (mi->tile_y_last >= map_tiles_h) {
                mi->tile_y_last = map_tiles_h-1;
            }
            mi->screen_y = -(map_y % TILE_SIZE);
        }
    }

    return draw_room_info;
}

static void draw_map_bg(struct DRAW_MAP_INFO *mi)
{
    const struct RAVEN_IMAGE *tileset = mi->map->tileset;
    int tile_x_first = mi->tile_x_first;
    int tile_y_first = mi->tile_y_first;
    int tile_x_last = mi->tile_x_last;
    int tile_y_last = mi->tile_y_last;

    int tiles_stride = mi->map->w;
    const uint8_t *tiles = &mi->map->tiles[mi->map->w*mi->map->h + tile_y_first * tiles_stride];

    //printf("map bg at (%d,%d), tiles (%d,%d)->(%d,%d)\n", mi->screen_x, mi->screen_y, tile_x_first, tile_y_first, tile_x_last, tile_y_last);

    int y_pos = mi->screen_y;
    for (int tile_y = tile_y_first; tile_y <= tile_y_last; tile_y++) {
        int x_pos = mi->screen_x;
        for (int tile_x = tile_x_first; tile_x <= tile_x_last; tile_x++) {
            uint8_t tile_num = tiles[tile_x];
            if (tile_num != 0xff) {
                vga_image_draw_frame(tileset, tile_num, x_pos, y_pos, true);
            }
            x_pos += TILE_SIZE;
        }
        tiles += tiles_stride;
        y_pos += TILE_SIZE;
    }
}

static void draw_map_fg(struct DRAW_MAP_INFO *mi)
{
    const struct RAVEN_IMAGE *tileset = mi->map->tileset;
    int tile_x_first = mi->tile_x_first;
    int tile_y_first = mi->tile_y_first;
    int tile_x_last = mi->tile_x_last;
    int tile_y_last = mi->tile_y_last;

    int tiles_stride = mi->map->w;
    const uint8_t *tiles = &mi->map->tiles[tile_y_first * tiles_stride];

    int y_pos = mi->screen_y;
    for (int tile_y = tile_y_first; tile_y <= tile_y_last; tile_y++) {
        int x_pos = mi->screen_x;
        for (int tile_x = tile_x_first; tile_x <= tile_x_last; tile_x++) {
            uint8_t tile_num = tiles[tile_x];
            if (tile_num != 0xff) {
                vga_image_draw_frame(tileset, tile_num, x_pos, y_pos, true);
            }
            x_pos += TILE_SIZE;
        }
        tiles += tiles_stride;
        y_pos += TILE_SIZE;
    }
}

#if 1
void draw_room_bg(struct DRAW_ROOM_INFO *info)
{
    for (int i = 0; i < info->n_maps; i++) {
        draw_map_bg(info->maps[i]);
    }
}

void draw_room_fg(struct DRAW_ROOM_INFO *info)
{
    for (int i = 0; i < info->n_maps; i++) {
        draw_map_fg(info->maps[i]);
    }
}
#else
void draw_room_bg(struct DRAW_ROOM_INFO *info)
{
    draw_map_bg(info->maps[0]);
}

void draw_room_fg(struct DRAW_ROOM_INFO *info)
{
    draw_map_fg(info->maps[0]);
}
#endif
