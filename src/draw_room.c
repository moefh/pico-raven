#include <string.h>

#include "draw_room.h"

#include "game.h"
#include "game_data.h"
#include "run_state.h"
#include "lib/vga_8bit.h"
#include "lib/mem.h"

void draw_room_init_room(struct GAME_STATE *game)
{
    (void)game;
}

static void init_map_frame(struct DRAW_MAP_INFO *mi, const struct RAVEN_ROOM_MAP_INFO *map_info, int map_x, int map_y)
{
    int map_tiles_w = map_info->map->w;
    int map_tiles_h = map_info->map->h;

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

struct DRAW_ROOM_INFO *draw_room_init_frame(struct MEM_ARENA *mem, struct GAME_STATE *game)
{
    const struct RAVEN_ROOM *room = &raven_rooms[game->room_id];

    // fg/bg layers (all maps)
    struct DRAW_ROOM_INFO *draw_room_info = mem_add(mem, struct DRAW_ROOM_INFO);
    draw_room_info->n_maps = 0;
    for (int i = 0; i < room->num_maps; i++) {
        const struct RAVEN_ROOM_MAP_INFO *map_info = &room->maps[i];

        int map_x = run_state.screen_x - map_info->x * TILE_SIZE;
        int map_y = run_state.screen_y - map_info->y * TILE_SIZE;

        if (map_x + vga_screen.width <= 0  || map_x >= map_info->map->w * TILE_SIZE ||
            map_y + vga_screen.height <= 0 || map_y >= map_info->map->h * TILE_SIZE)
            continue; // map outside screen

        // allocate draw info
        struct DRAW_MAP_INFO *mi = mem_add(mem, struct DRAW_MAP_INFO);
        draw_room_info->maps[draw_room_info->n_maps++] = mi;

        init_map_frame(mi, map_info, map_x, map_y);
    }

    // parallax layer (only first map)
    if (room->num_maps != 0 && room->maps[0].map->para_w != 0 && room->maps[0].map->para_h != 0) {
        const struct RAVEN_ROOM_MAP_INFO *map_info = &room->maps[0];
        int para_w = map_info->map->w * TILE_SIZE;
        int para_h = map_info->map->h * TILE_SIZE;

        int map_x = run_state.screen_x * (para_w - vga_screen.width) / (run_state.room_w - vga_screen.width);
        int map_y = run_state.screen_y * (para_h - vga_screen.height) / (run_state.room_h - vga_screen.height);

        init_map_frame(&draw_room_info->parallax, map_info, map_x, map_y);
    } else {
        draw_room_info->parallax.map = NULL;
    }

    return draw_room_info;
}

static void draw_map_para(struct DRAW_MAP_INFO *mi, unsigned int anim_step)
{
    int tile_x_first = mi->tile_x_first;
    int tile_y_first = mi->tile_y_first;
    int tile_x_last = mi->tile_x_last;
    int tile_y_last = mi->tile_y_last;

    int tiles_stride = mi->map->para_w;
    const struct RAVEN_IMAGE *tileset = mi->map->tileset;
    const uint8_t *tiles = &mi->map->tiles[3*mi->map->w*mi->map->h + tile_y_first * tiles_stride];
    const uint8_t *anim_tiles = &mi->map->tiles[2*mi->map->w*mi->map->h + tile_y_first * tiles_stride];
    const struct RAVEN_TILE_ANIMATION *anim = mi->map->animation;
    const struct RAVEN_IMAGE *anim_tileset = (anim == NULL) ? NULL : anim->anim_tileset;

    //printf("map para at (%d,%d), tiles (%d,%d)->(%d,%d)\n", mi->screen_x, mi->screen_y, tile_x_first, tile_y_first, tile_x_last, tile_y_last);

    int y_pos = mi->screen_y;
    for (int tile_y = tile_y_first; tile_y <= tile_y_last; tile_y++) {
        int x_pos = mi->screen_x;
        for (int tile_x = tile_x_first; tile_x <= tile_x_last; tile_x++) {
            uint8_t tile_num = tiles[tile_x];
            const struct RAVEN_IMAGE *use_tileset = tileset;
            if (anim != NULL) {
                int anim_tile = anim_tiles[tile_x]>>4;
                if (! run_state.room_deactivated && anim_tile > 8 && anim_tile < 15) { anim_tile -= 8; }
                switch (anim_tile) {
                case 1:
                    {
                        int loop_len = anim->loops[tile_num].len;
                        if (loop_len > 0) {
                            tile_num = anim->loops[tile_num].start + (anim_step>>3) % loop_len;
                            use_tileset = anim_tileset;
                        }
                    }
                    break;
                case 2:
                    {
                        int loop_len = anim->loops[tile_num].len;
                        if (loop_len > 0) {
                            tile_num = anim->loops[tile_num].start + (loop_len - 1 - (anim_step>>3)%loop_len) % loop_len;
                            use_tileset = anim_tileset;
                        }
                    }
                    break;
                }
            }
            if (tile_num != 0xff) {
                vga_image_draw_frame(use_tileset, tile_num, x_pos, y_pos, false);
            }
            x_pos += TILE_SIZE;
        }
        tiles += tiles_stride;
        anim_tiles += tiles_stride;
        y_pos += TILE_SIZE;
    }
}

static void draw_map_bg(struct DRAW_MAP_INFO *mi, unsigned int anim_step, bool transparent)
{
    int tile_x_first = mi->tile_x_first;
    int tile_y_first = mi->tile_y_first;
    int tile_x_last = mi->tile_x_last;
    int tile_y_last = mi->tile_y_last;

    int tiles_stride = mi->map->w;
    const struct RAVEN_IMAGE *tileset = mi->map->tileset;
    const uint8_t *tiles = &mi->map->tiles[mi->map->w*mi->map->h + tile_y_first * tiles_stride];
    const uint8_t *anim_tiles = &mi->map->tiles[2*mi->map->w*mi->map->h + tile_y_first * tiles_stride];
    const struct RAVEN_TILE_ANIMATION *anim = mi->map->animation;
    const struct RAVEN_IMAGE *anim_tileset = (anim == NULL) ? NULL : anim->anim_tileset;

    //printf("map bg at (%d,%d), tiles (%d,%d)->(%d,%d)\n", mi->screen_x, mi->screen_y, tile_x_first, tile_y_first, tile_x_last, tile_y_last);

    int y_pos = mi->screen_y;
    for (int tile_y = tile_y_first; tile_y <= tile_y_last; tile_y++) {
        int x_pos = mi->screen_x;
        for (int tile_x = tile_x_first; tile_x <= tile_x_last; tile_x++) {
            uint8_t tile_num = tiles[tile_x];
            const struct RAVEN_IMAGE *use_tileset = tileset;
            if (anim != NULL) {
                int anim_tile = anim_tiles[tile_x]>>4;
                if (! run_state.room_deactivated && anim_tile > 8 && anim_tile < 15) { anim_tile -= 8; }
                switch (anim_tile) {
                case 3:
                    {
                        int loop_len = anim->loops[tile_num].len;
                        if (loop_len > 0) {
                            tile_num = anim->loops[tile_num].start + (anim_step>>3) % loop_len;
                            use_tileset = anim_tileset;
                        }
                    }
                    break;
                case 4:
                    {
                        int loop_len = anim->loops[tile_num].len;
                        if (loop_len > 0) {
                            tile_num = anim->loops[tile_num].start + (loop_len - 1 - (anim_step>>3)%loop_len) % loop_len;
                            use_tileset = anim_tileset;
                        }
                    }
                    break;
                }
            }
            if (tile_num != 0xff) {
                vga_image_draw_frame(use_tileset, tile_num, x_pos, y_pos, transparent);
            }
            x_pos += TILE_SIZE;
        }
        tiles += tiles_stride;
        anim_tiles += tiles_stride;
        y_pos += TILE_SIZE;
    }
}

static void draw_map_fg(struct DRAW_MAP_INFO *mi, unsigned int anim_step)
{
    int tile_x_first = mi->tile_x_first;
    int tile_y_first = mi->tile_y_first;
    int tile_x_last = mi->tile_x_last;
    int tile_y_last = mi->tile_y_last;

    int tiles_stride = mi->map->w;
    const struct RAVEN_IMAGE *tileset = mi->map->tileset;
    const uint8_t *tiles = &mi->map->tiles[tile_y_first * tiles_stride];
    const uint8_t *anim_tiles = &mi->map->tiles[2*mi->map->w*mi->map->h + tile_y_first * tiles_stride];
    const struct RAVEN_TILE_ANIMATION *anim = mi->map->animation;
    const struct RAVEN_IMAGE *anim_tileset = (anim == NULL) ? NULL : anim->anim_tileset;

    //printf("map fg at (%d,%d), tiles (%d,%d)->(%d,%d)\n", mi->screen_x, mi->screen_y, tile_x_first, tile_y_first, tile_x_last, tile_y_last);

    int y_pos = mi->screen_y;
    for (int tile_y = tile_y_first; tile_y <= tile_y_last; tile_y++) {
        int x_pos = mi->screen_x;
        for (int tile_x = tile_x_first; tile_x <= tile_x_last; tile_x++) {
            uint8_t tile_num = tiles[tile_x];
            const struct RAVEN_IMAGE *use_tileset = tileset;
            if (anim != NULL) {
                int anim_tile = anim_tiles[tile_x]>>4;
                if (! run_state.room_deactivated && anim_tile > 8 && anim_tile < 15) { anim_tile -= 8; }
                switch (anim_tile) {
                case 5:
                    {
                        int loop_len = anim->loops[tile_num].len;
                        if (loop_len > 0) {
                            tile_num = anim->loops[tile_num].start + (anim_step>>3) % loop_len;
                            use_tileset = anim_tileset;
                        }
                    }
                    break;
                case 6:
                    {
                        int loop_len = anim->loops[tile_num].len;
                        if (loop_len > 0) {
                            tile_num = anim->loops[tile_num].start + (loop_len - 1 - (anim_step>>3)%loop_len) % loop_len;
                            use_tileset = anim_tileset;
                        }
                    }
                    break;
                }
            }
            if (tile_num != 0xff) {
                vga_image_draw_frame(use_tileset, tile_num, x_pos, y_pos, true);
            }
            x_pos += TILE_SIZE;
        }
        tiles += tiles_stride;
        anim_tiles += tiles_stride;
        y_pos += TILE_SIZE;
    }
}

void draw_room_bg(struct DRAW_ROOM_INFO *info, unsigned int anim_step)
{
    if (info->parallax.map != NULL) {
        draw_map_para(&info->parallax, anim_step);
    }
    for (int i = 0; i < info->n_maps; i++) {
        draw_map_bg(info->maps[i], anim_step, info->parallax.map != NULL);
    }
}

void draw_room_fg(struct DRAW_ROOM_INFO *info, unsigned int anim_step)
{
    for (int i = 0; i < info->n_maps; i++) {
        draw_map_fg(info->maps[i], anim_step);
    }
}
