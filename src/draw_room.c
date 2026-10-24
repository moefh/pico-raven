#include <string.h>

#include "draw_room.h"

#include "game.h"
#include "game_data.h"
#include "lib/vga_8bit.h"
#include "lib/mem.h"

struct DRAW_ROOM_INFO *draw_room_init_frame(struct MEM_ARENA *mem)
{
    struct DRAW_ROOM_INFO *dri = mem_add(mem, struct DRAW_ROOM_INFO);
    int32_t screen_width = vga_image_screen.x_max;
    int32_t screen_height = vga_image_screen.y_max - vga_image_screen.y_min;

    dri->tile_x_last = (game_state.screen_x + screen_width) / TILE_SIZE;
    if (game_state.screen_x < 0) {
        dri->tile_x_first = 0;
        dri->screen_x = -game_state.screen_x;
    } else {
        dri->tile_x_first = game_state.screen_x / TILE_SIZE;
        if (dri->tile_x_last >= game_state.room_tiles_w) {
            dri->tile_x_last = game_state.room_tiles_w - 1;
        }
        dri->screen_x = -(game_state.screen_x % TILE_SIZE);
    }

    dri->tile_y_last = (game_state.screen_y + screen_height) / TILE_SIZE;
    if (game_state.screen_y < 0) {
        dri->tile_y_first = 0;
        dri->screen_y = vga_image_screen.y_min - game_state.screen_y;
    } else {
        dri->tile_y_first = game_state.screen_y / TILE_SIZE;
        if (dri->tile_y_last >= game_state.room_tiles_h) {
            dri->tile_y_last = game_state.room_tiles_h-1;
        }
        dri->screen_y = vga_image_screen.y_min - (game_state.screen_y % TILE_SIZE);
    }

    const struct RAVEN_ROOM *room = &raven_rooms[game_state.room_id];
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

    int para_x = game_state.screen_x * (room->parallax.width*TILE_SIZE - screen_width) / (game_state.room_w - screen_width);
    int para_y = game_state.screen_y * (room->parallax.height*TILE_SIZE - screen_height) / (game_state.room_h - screen_height);
    dri->parallax.room_parallax = &room->parallax;

    dri->parallax.tile_x_last = (para_x + screen_width) / TILE_SIZE;
    if (para_x < 0) {
        dri->parallax.tile_x_first = 0;
        dri->parallax.screen_x = -para_x;
    } else {
        dri->parallax.tile_x_first = para_x / TILE_SIZE;
        if (dri->parallax.tile_x_last >= room->parallax.width) {
            dri->parallax.tile_x_last = room->parallax.width - 1;
        }
        dri->parallax.screen_x = -(para_x % TILE_SIZE);
    }

    dri->parallax.tile_y_last = (para_y + screen_height) / TILE_SIZE;
    if (para_y < 0) {
        dri->parallax.tile_y_first = 0;
        dri->parallax.screen_y = vga_image_screen.y_min - para_y;
    } else {
        dri->parallax.tile_y_first = para_y / TILE_SIZE;
        if (dri->parallax.tile_y_last >= room->parallax.height) {
            dri->parallax.tile_y_last = room->parallax.height - 1;
        }
        dri->parallax.screen_y = vga_image_screen.y_min - (para_y % TILE_SIZE);
    }

    return dri;
}

static void draw_map_layer(struct DRAW_ROOM_INFO *dri, struct DRAW_ROOM_MAP_INFO *mi,
                           const uint8_t *layer_tiles, int fx_tile_base, bool transparent)
{
    int tile_x_first = dri->tile_x_first;
    int tile_y_first = dri->tile_y_first;
    int tile_x_last = dri->tile_x_last;
    int tile_y_last = dri->tile_y_last;

    int tiles_stride = game_state.room_tiles_w;
    const struct RAVEN_IMAGE *tileset = mi->map->tileset;
    const uint8_t *tiles = &layer_tiles[tile_y_first * tiles_stride];
    const uint8_t *anim_tiles = &game_state.fx_tiles[tile_y_first * tiles_stride];
    const struct RAVEN_TILE_ANIMATION *anim = mi->map->animation;
    const struct RAVEN_IMAGE *anim_tileset = (anim == NULL) ? NULL : anim->anim_tileset;

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
                int fx_tile = anim_tiles[tile_x]>>4;
                if (fx_tile == fx_tile_base) {
                    int loop_len = anim->loops[tile_num].len;
                    if (loop_len > 0) {
                        tile_num = anim->loops[tile_num].start + (game_state.anim_step>>3) % loop_len;
                        use_tileset = anim_tileset;
                    }
                } else if (fx_tile == fx_tile_base + 1) {
                    int loop_len = anim->loops[tile_num].len;
                    if (loop_len > 0) {
                        tile_num = anim->loops[tile_num].start + (loop_len - 1 - (game_state.anim_step>>3)%loop_len) % loop_len;
                        use_tileset = anim_tileset;
                    }
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

static void draw_parallax_layer(struct DRAW_ROOM_INFO *dri)
{
    int tile_x_first = dri->parallax.tile_x_first;
    int tile_y_first = dri->parallax.tile_y_first;
    int tile_x_last = dri->parallax.tile_x_last;
    int tile_y_last = dri->parallax.tile_y_last;

    int tiles_stride = dri->parallax.room_parallax->width;
    const struct RAVEN_IMAGE *tileset = dri->parallax.room_parallax->tileset;
    const uint8_t *tiles = &dri->parallax.room_parallax->tiles[tile_y_first * tiles_stride];

    int y_pos = dri->parallax.screen_y;
    for (int tile_y = tile_y_first; tile_y <= tile_y_last; tile_y++) {
        int x_pos = dri->parallax.screen_x;
        for (int tile_x = tile_x_first; tile_x <= tile_x_last; tile_x++) {
            uint8_t tile_num = tiles[tile_x];
            if (tile_num != 0xff) {
                vga_image_draw_frame(tileset, tile_num, x_pos, y_pos, false);
            }
            x_pos += TILE_SIZE;
        }
        tiles += tiles_stride;
        y_pos += TILE_SIZE;
    }
}

void draw_room_bg(struct DRAW_ROOM_INFO *dri)
{
    bool has_parallax = dri->parallax.room_parallax->tileset != NULL;
    if (has_parallax) {
        draw_parallax_layer(dri);
    }
    for (int i = 0; i < dri->num_maps; i++) {
        if (dri->maps[i].map->has_bb_layer) {
            draw_map_layer(dri, &dri->maps[i], game_state.bb_tiles, 1, has_parallax);
            draw_map_layer(dri, &dri->maps[i], game_state.bg_tiles, 3, true);
        } else {
            draw_map_layer(dri, &dri->maps[i], game_state.bg_tiles, 3, has_parallax);
        }
    }
}

void draw_room_fg(struct DRAW_ROOM_INFO *dri)
{
    for (int i = 0; i < dri->num_maps; i++) {
        draw_map_layer(dri, &dri->maps[i], game_state.fg_tiles, 5, true);
    }
}
