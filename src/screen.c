#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pico/stdlib.h"

#include "screen.h"
#include "config.h"
#include "perf.h"
#include "draw_room.h"
#include "sprite_shadow.h"
#include "darkness.h"
#include "gamepad.h"

#include "lib/vga_8bit.h"
#include "lib/vga_image_8bit.h"
#include "lib/vga_font_8bit.h"
#include "lib/mem.h"

#include "game_data.h"

static struct MEM_ARENA draw_frame_arena;

static const uint8_t sprite_shadow_colors[SPRITE_SHADOW_NUM_FRAMES] = {
    0, 1, 2, 3, 4, 5, 6, 7, // red
    //0x00, 0x01, 0x0a, 0x0b, 0x14, 0x15, 0x1e, 0x1f, // orange
    //0x09, 0x0a, 0x13, 0x14, 0x15, 0x0f, 0x27, 0x27, // black->orange->red
    //0<<3, 1<<3, 2<<3, 3<<3, 4<<3, 5<<3, 6<<3, 7<<3, // green
    //0|(0<<6), 1|(0<<6), 2|(1<<6), 3|(1<<6), 4|(2<<6), 5|(2<<6), 6|(3<<6), 7|(3<<6), // magenta
};

int screen_init(void)
{
    if (vga_init(&vga_mode_320x240, VGA_PIO_NUM, VGA_PIN_BASE, true) < 0) {
        printf("ERROR initializing VGA\n");
        fflush(stdout);
        return -1;
    }
    vga_image_init();

    size_t scratch_size = vga_screen.width * vga_screen.height;
    void *scratch = malloc(scratch_size);
    if (scratch == NULL) {
        printf("ERROR allocating screen memory arena\n");
        fflush(stdout);
        return -1;
    }
    mem_init(&draw_frame_arena, scratch, scratch_size);

    font_set_font(&raven_fonts[RAVEN_FONT_ID_6X8]);
    font_set_color(0xff);

    return 0;
}

static void draw_player(void)
{
    if (game_state.player.shadow_enabled) {
        sprite_shadow_draw(game_state.player.char_anim.sprite, sprite_shadow_colors);
    }

    vga_image_draw_frame(game_state.player.char_anim.sprite,
                         game_state.player.char_anim.sprite_frame,
                         game_state.player.char_anim.sprite_x - game_state.screen_x,
                         game_state.player.char_anim.sprite_y - game_state.screen_y + vga_image_screen.y_min,
                         1);
    if (game_state.player.weapon_frames_left) {
        vga_image_draw_frame(game_state.player.weapon_anim.sprite,
                             game_state.player.weapon_anim.sprite_frame,
                             game_state.player.weapon_anim.sprite_x - game_state.screen_x,
                             game_state.player.weapon_anim.sprite_y - game_state.screen_y + vga_image_screen.y_min,
                             1);
    }
}

static void draw_enemies(void)
{
    for (int i = 0; i < game_state.num_enemies; i++) {
        struct GAME_ENEMY *enemy = &game_state.enemies[i];
        if (enemy->hit_frames_left != 0) {
            vga_image_draw_frame_mask(enemy->anim.sprite,
                                      enemy->anim.sprite_frame,
                                      enemy->anim.sprite_x - game_state.screen_x,
                                      enemy->anim.sprite_y - game_state.screen_y + vga_image_screen.y_min,
                                      0x07);
        } else {
            vga_image_draw_frame(enemy->anim.sprite,
                                 enemy->anim.sprite_frame,
                                 enemy->anim.sprite_x - game_state.screen_x,
                                 enemy->anim.sprite_y - game_state.screen_y + vga_image_screen.y_min,
                                 1);
        }
    }
}

static void draw_bullets(void)
{
    for (int i = 0; i < game_state.num_bullets; i++) {
        struct GAME_BULLET *bullet = &game_state.bullets[i];
        vga_image_draw_frame(bullet->anim.sprite,
                             bullet->anim.sprite_frame,
                             bullet->anim.sprite_x - game_state.screen_x,
                             bullet->anim.sprite_y - game_state.screen_y + vga_image_screen.y_min,
                             1);
    }
}

static void draw_effects(void)
{
    for (int i = 0; i < game_state.num_effects; i++) {
        struct GAME_EFFECT *effect = &game_state.effects[i];
        vga_image_draw_frame(effect->anim.sprite,
                             effect->anim.sprite_frame,
                             effect->anim.sprite_x - game_state.screen_x,
                             effect->anim.sprite_y - game_state.screen_y + vga_image_screen.y_min,
                             1);
    }
}

static void draw_room_items(const struct RAVEN_ROOM *room)
{
    for (int i = 0; i < room->num_triggers; i++) {
        const struct RAVEN_IMAGE *sprite = NULL;
        const struct RAVEN_ROOM_TRIGGER_INFO *trg = &room->triggers[i];
        switch (trg->type) {
        case RAVEN_ROOM_TRIGGER_TYPE_GET_UPGRADE:
            if (trg->get_upgrade.get_upgrade_id != 0xffff &&
                (trg->get_upgrade.req_flag_id == 0xffff || game_state_has_flag(trg->get_upgrade.req_flag_id)) &&
                ! game_state_has_upgrade(trg->get_upgrade.get_upgrade_id)) {
                sprite = &raven_sprites[raven_table_upgrades[trg->get_upgrade.get_upgrade_id].sprite_index];
            }
            break;
        case RAVEN_ROOM_TRIGGER_TYPE_GET_COLLECTABLE:
            if (trg->get_collectable.get_collectable_id != 0xffff
                && ! game_state_has_collectable(trg->get_collectable.get_collectable_id)) {
                sprite = &raven_sprites[raven_table_collectables[trg->get_collectable.get_collectable_id].sprite_index];
            }
            break;
        case RAVEN_ROOM_TRIGGER_TYPE_GET_PICKUP:
            if (trg->get_pickup.get_pickup_id != 0xffff && game_state_is_pickup_trigger_enabled(trg->trigger_id)) {
                sprite = &raven_sprites[raven_table_pickups[trg->get_pickup.get_pickup_id].sprite_index];
            }
            break;
        }
        if (sprite != NULL) {
            vga_image_draw_frame(sprite, 0, trg->x - game_state.screen_x, trg->y - game_state.screen_y + vga_image_screen.y_min, 1);
        }
    }
}

static void draw_texts(void)
{
    if (game_state.prompt.text != NULL) {
        const char *text = game_state.prompt.text;
        const struct VGA_IMAGE *sprite = NULL;
        int sprite_frame = 0;
        if (text[0] == '!') {
            sprite = &raven_sprites[game_state.player.player_char->gamepad_sprite_index];
            switch (text[1]) {
            case 'B': sprite_frame = 0; text += 2; break;
            case 'A': sprite_frame = 1; text += 2; break;
            case 'Y': sprite_frame = 2; text += 2; break;
            case 'X': sprite_frame = 3; text += 2; break;
            default: sprite = NULL;
            }
            if (*text == ' ') text++;
        }

        int font_height;
        const void *old_font;
        bool old_is_prop;
        const struct RAVEN_FONT_REF *font = &game_state.player.player_char->font;
        if (font->type == RAVEN_FONT_TYPE_FIXED) {
            const struct VGA_FONT *ffont = &raven_fonts[font->fixed_index];
            old_font = font_set_font(ffont);
            old_is_prop = font_set_is_prop(false);
            font_height = ffont->h;
        } else {
            const struct VGA_PROP_FONT *pfont = &raven_prop_fonts[font->prop_index];
            old_font = font_set_prop_font(pfont);
            old_is_prop = font_set_is_prop(true);
            font_height = pfont->h;
        }

        int x = game_state.prompt.x - font_measure(text) / 2 - game_state.screen_x;
        int y = game_state.prompt.y - 6 - font_height - game_state.screen_y + vga_image_screen.y_min;
        if (sprite != NULL) {
            int sx = x - sprite->width/2 - 3;
            int sy = y - sprite->height/2 + font_height/2;
            vga_image_draw_frame(sprite, sprite_frame, sx, sy, 1);
            x += sprite->width / 2 + 3;
        }

        bool old_border = font_set_border(true, 0x00);
        uint8_t old_color = font_set_color(0xff);
        font_move(x, y);
        font_print(text);

        font_set_border(old_border, 0x00);
        font_set_color(old_color);
        font_set_is_prop(old_is_prop);
        if (font->type == RAVEN_FONT_TYPE_FIXED) {
            font_set_font(old_font);
        } else {
            font_set_prop_font(old_font);
        }
    }
}

static void draw_room(void)
{
    struct DRAW_ROOM_INFO *info = draw_room_init_frame(&draw_frame_arena);
    const struct RAVEN_ROOM *room = &raven_rooms[game_state.room_id];

    draw_room_bg(info);
    RUN_PERF(room_bg_us);

    draw_bullets();
    draw_room_items(room);
    draw_player();
    RUN_PERF(player_us);

    draw_enemies();
    RUN_PERF(enemies_us);

    draw_room_fg(info);

    if (game_state.player.darkness_enabled) {
        int x = game_state.player.char_anim.sprite_x + game_state.player.char_anim.sprite->width/2;
        int y = game_state.player.char_anim.sprite_y + game_state.player.char_anim.sprite->height/2 + 4;
        darkness_darken(&game_state.darkness[1], &game_state.darkness[2], x, y, 4, 4, 2);
        darkness_darken(&game_state.darkness[0], &game_state.darkness[1], x, y, 4, 4, 1);
        darkness_clear(&game_state.darkness[0], x, y, 0x00);
    }

    draw_effects();
    draw_texts();
    RUN_PERF(room_fg_us);

    if (game_state.display.show_perf) {
        int sx = game_state.player.x - game_state.screen_x;
        int sy = game_state.player.y - game_state.screen_y;
        int w = game_state.player.char_anim.anim->collision.w;
        int h = game_state.player.char_anim.anim->collision.h;
        for (int x = 0; x < w; x++) {
            vga_screen.lines8[SCREEN_HEADER_HEIGHT+sy][sx+x] = 0x38;
            vga_screen.lines8[SCREEN_HEADER_HEIGHT+sy+h-1][sx+x] = 0x38;
        }
        for (int y = 0; y < h; y++) {
            vga_screen.lines8[SCREEN_HEADER_HEIGHT+sy+y][sx] = 0x38;
            vga_screen.lines8[SCREEN_HEADER_HEIGHT+sy+y][sx+w-1] = 0x38;
        }
    }
}

static void draw_header(void)
{
    memset(vga_screen.framebuffer, 0, vga_screen.width * SCREEN_HEADER_HEIGHT);

    const struct RAVEN_IMAGE *energy = &raven_sprites[RAVEN_SPRITE_ID_SCREEN__ENERGY];
    vga_image_draw_frame(energy, 0, 64, 0, 0);

    const struct RAVEN_IMAGE *explosive = &raven_sprites[RAVEN_SPRITE_ID_SCREEN__EXPLOSIVE];
    vga_image_draw_frame(explosive, 0, vga_screen.width - explosive->width - 4, 16, 0);
    font_move(vga_screen.width - explosive->width - 4 - 2*vga_font->w - 1, 20);
    font_printf("%2d", 3);
}

void screen_render(void)
{
    mem_clear(&draw_frame_arena);
    //vga_clear_screen(0);

    vga_image_screen.y_min = 0;
    draw_header();
    vga_image_screen.y_min = SCREEN_HEADER_HEIGHT;
    draw_room();
    vga_image_screen.y_min = 0;

    perf_update_history(&game_state.perf);
    if (game_state.display.show_perf) {
        perf_draw_info();
    }

    RUN_PERF(render_us);
}

void screen_save_to_scratch(int clear_after)
{
    int len = vga_screen.width * vga_screen.height;
    memcpy(draw_frame_arena.data, vga_screen.framebuffer, len);
    if (clear_after) {
        memset(vga_screen.framebuffer, 0, len);
    }
}

void screen_fade_from_scratch(int level)
{
    int len = vga_screen.width * vga_screen.height;
    uint32_t *src = draw_frame_arena.data;
    uint32_t *dst = vga_screen.framebuffer;
    switch (level) {
    case 2:
        for (int i = len/4; i > 0; i--) {
            *dst++ = (*src++ >> 1) & 0x5b5b5b5b;
        }
        break;

    case 1:
        for (int i = len/4; i > 0; i--) {
            *dst++ = (*src++ >> 2) & 0x09090909;
        }
        break;

    default:
        memset(dst, 0, len);
        break;
    }
}
