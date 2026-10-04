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

struct DRAW_ROOM_INFO *draw_room_init_frame(struct MEM_ARENA *mem, struct GAME_STATE *game)
{
    struct DRAW_ROOM_INFO *dri = mem_add(mem, struct DRAW_ROOM_INFO);

    dri->tile_x_last = (run_state.screen_x + vga_screen.width) / TILE_SIZE;
    if (run_state.screen_x < 0) {
        dri->tile_x_first = 0;
        dri->screen_x = -run_state.screen_x;
    } else {
        dri->tile_x_first = run_state.screen_x / TILE_SIZE;
        if (dri->tile_x_last >= run_state.room_tiles_w) {
            dri->tile_x_last = run_state.room_tiles_w - 1;
        }
        dri->screen_x = -(run_state.screen_x % TILE_SIZE);
    }

    dri->tile_y_last = (run_state.screen_y + vga_screen.height) / TILE_SIZE;
    if (run_state.screen_y < 0) {
        dri->tile_y_first = 0;
        dri->screen_y = -run_state.screen_y;
    } else {
        dri->tile_y_first = run_state.screen_y / TILE_SIZE;
        if (dri->tile_y_last >= run_state.room_tiles_h) {
            dri->tile_y_last = run_state.room_tiles_h-1;
        }
        dri->screen_y = -(run_state.screen_y % TILE_SIZE);
    }

    const struct RAVEN_ROOM *room = &raven_rooms[game->room_id];
    dri->num_maps = room->num_maps;
    for (int i = 0; i < room->num_maps; i++) {
        const struct RAVEN_ROOM_MAP_INFO *map_info = &room->maps[i];
        struct DRAW_ROOM_MAP_INFO *mi = &dri->maps[i];
        mi->tile_x_first = map_info->x;
        mi->tile_y_first = map_info->y;
        mi->tile_x_last = map_info->x + map_info->map->w - 1;
        mi->tile_y_last = map_info->y + map_info->map->h - 1;
        mi->map = map_info->map;
    }

    return dri;
}

static void draw_room_para_layer(struct DRAW_ROOM_INFO *dri, struct DRAW_ROOM_MAP_INFO *mi, unsigned int anim_step)
{
    int tile_x_first = dri->tile_x_first;
    int tile_y_first = dri->tile_y_first;
    int tile_x_last = dri->tile_x_last;
    int tile_y_last = dri->tile_y_last;

    int tiles_stride = run_state.room_tiles_w;
    const struct RAVEN_IMAGE *tileset = mi->map->tileset;
    const uint8_t *tiles = &run_state.para_tiles[tile_y_first * tiles_stride];
    const uint8_t *anim_tiles = &run_state.fx_tiles[tile_y_first * tiles_stride];
    const struct RAVEN_TILE_ANIMATION *anim = mi->map->animation;
    const struct RAVEN_IMAGE *anim_tileset = (anim == NULL) ? NULL : anim->anim_tileset;

    //printf("map para at (%d,%d), tiles (%d,%d)->(%d,%d)\n", dri->screen_x, dri->screen_y, tile_x_first, tile_y_first, tile_x_last, tile_y_last);

    int y_pos = dri->screen_y;
    for (int tile_y = tile_y_first; tile_y <= tile_y_last; tile_y++) {
        int x_pos = dri->screen_x;
        for (int tile_x = tile_x_first; tile_x <= tile_x_last; tile_x++) {
            uint8_t tile_num = tiles[tile_x];
            const struct RAVEN_IMAGE *use_tileset = tileset;
            if (anim != NULL) {
                int anim_tile = anim_tiles[tile_x]>>4;
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

static void draw_map_bg_layer(struct DRAW_ROOM_INFO *dri, struct DRAW_ROOM_MAP_INFO *mi, unsigned int anim_step, bool transparent)
{
    int tile_x_first = dri->tile_x_first;
    int tile_y_first = dri->tile_y_first;
    int tile_x_last = dri->tile_x_last;
    int tile_y_last = dri->tile_y_last;

    int tiles_stride = run_state.room_tiles_w;
    const struct RAVEN_IMAGE *tileset = mi->map->tileset;
    const uint8_t *tiles = &run_state.bg_tiles[tile_y_first * tiles_stride];
    const uint8_t *anim_tiles = &run_state.fx_tiles[tile_y_first * tiles_stride];
    const struct RAVEN_TILE_ANIMATION *anim = mi->map->animation;
    const struct RAVEN_IMAGE *anim_tileset = (anim == NULL) ? NULL : anim->anim_tileset;

    //printf("map bg at (%d,%d), tiles (%d,%d)->(%d,%d)\n", dri->screen_x, dri->screen_y, tile_x_first, tile_y_first, tile_x_last, tile_y_last);

    int y_pos = dri->screen_y;
    for (int tile_y = tile_y_first; tile_y <= tile_y_last; tile_y++) {
        if (tile_y < mi->tile_y_first || tile_y > mi->tile_y_last) {
            tiles += tiles_stride;
            anim_tiles += tiles_stride;
            y_pos += TILE_SIZE;
            continue;
        }
        int x_pos = dri->screen_x;
        for (int tile_x = tile_x_first; tile_x <= tile_x_last; tile_x++) {
            if (tile_x < mi->tile_x_first || tile_x > mi->tile_x_last) {
                x_pos += TILE_SIZE;
                continue;
            }
            uint8_t tile_num = tiles[tile_x];
            const struct RAVEN_IMAGE *use_tileset = tileset;
            if (anim != NULL) {
                int anim_tile = anim_tiles[tile_x]>>4;
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

static void draw_map_fg_layer(struct DRAW_ROOM_INFO *dri, struct DRAW_ROOM_MAP_INFO *mi, unsigned int anim_step)
{
    int tile_x_first = dri->tile_x_first;
    int tile_y_first = dri->tile_y_first;
    int tile_x_last = dri->tile_x_last;
    int tile_y_last = dri->tile_y_last;

    int tiles_stride = run_state.room_tiles_w;
    const struct RAVEN_IMAGE *tileset = mi->map->tileset;
    const uint8_t *tiles = &run_state.fg_tiles[tile_y_first * tiles_stride];
    const uint8_t *anim_tiles = &run_state.fx_tiles[tile_y_first * tiles_stride];
    const struct RAVEN_TILE_ANIMATION *anim = mi->map->animation;
    const struct RAVEN_IMAGE *anim_tileset = (anim == NULL) ? NULL : anim->anim_tileset;

    //printf("map fg at (%d,%d), tiles (%d,%d)->(%d,%d)\n", dri->screen_x, dri->screen_y, tile_x_first, tile_y_first, tile_x_last, tile_y_last);

    int y_pos = dri->screen_y;
    for (int tile_y = tile_y_first; tile_y <= tile_y_last; tile_y++) {
        if (tile_y < mi->tile_y_first || tile_y > mi->tile_y_last) {
            tiles += tiles_stride;
            anim_tiles += tiles_stride;
            y_pos += TILE_SIZE;
            continue;
        }
        int x_pos = dri->screen_x;
        for (int tile_x = tile_x_first; tile_x <= tile_x_last; tile_x++) {
            if (tile_x < mi->tile_x_first || tile_x > mi->tile_x_last) {
                x_pos += TILE_SIZE;
                continue;
            }
            uint8_t tile_num = tiles[tile_x];
            const struct RAVEN_IMAGE *use_tileset = tileset;
            if (anim != NULL) {
                int anim_tile = anim_tiles[tile_x]>>4;
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

void draw_room_bg(struct DRAW_ROOM_INFO *dri, unsigned int anim_step)
{
    if (dri->num_maps > 0) {
        draw_room_para_layer(dri, &dri->maps[0], anim_step);
    }
    bool bg_transparent = run_state.room_tiles_para_w > 0 && run_state.room_tiles_para_h > 0;
    for (int i = 0; i < dri->num_maps; i++) {
        draw_map_bg_layer(dri, &dri->maps[i], anim_step, bg_transparent);
    }
}

void draw_room_fg(struct DRAW_ROOM_INFO *dri, unsigned int anim_step)
{
    for (int i = 0; i < dri->num_maps; i++) {
        draw_map_fg_layer(dri, &dri->maps[i], anim_step);
    }
}
