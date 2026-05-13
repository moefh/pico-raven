#include <stdio.h>
#include "pico/stdlib.h"

#include "game.h"

#include "lib/joy_wii_i2c.h"
#include "lib/vga_8bit.h"

#include "config.h"
#include "core_msg.h"
#include "game_data.h"
#include "screen.h"
#include "savegame.h"
#include "joy.h"

#define TILE_SIZE 16

static struct GAME_STATE game;

static void load_room(const struct RAVEN_ROOM *room)
{
    game.room = room;

    // calc room width
    game.room_w = 0;
    game.room_h = 0;
    for (int i = 0; i < room->num_maps; i++) {
        int w = (room->maps[i].x + room->maps[i].map->w) * TILE_SIZE;
        int h = (room->maps[i].y + room->maps[i].map->h) * TILE_SIZE;
        if (game.room_w < w) game.room_w = w;
        if (game.room_h < h) game.room_h = h;
    }

    // setup player
    player_init(&game.player);
    player_control_init(&game.player_control);

    const struct RAVEN_ROOM_TRIGGER_INFO *spawn = &game.room->triggers[RAVEN_ROOM_START_TRG_SPAWN_POINT];
    game.player.x = spawn->x;
    game.player.y = spawn->y + spawn->h - game.player.anim->collision.h;
    game.screen_x = 0;
    game.screen_y = 0;
}

static void game_init(void)
{
    game.mod.index = RAVEN_MOD_ID_BWV_106;
    game.mod.volume = 0x20;

    game.display.show_palette = 0;
    game.display.msg_load_frames_left = 0;
    game.display.msg_save_frames_left = 0;
    game.display.msg_mod_event_frames_left = 0;

    load_room(&raven_rooms[RAVEN_ROOM_ID_START]);
}

static void process_mod_event(uint8_t chan, uint8_t event)
{
    game.mod.event_chan = chan;
    game.mod.event_data = event;
    game.display.msg_mod_event_frames_left = 60;
}

static void load_game(void)
{
    static struct GAME_STATE tmp_game;

    tmp_game = game;
    int ret = savegame_read(&tmp_game, 0);
    if (ret == 0) {
        game = tmp_game;
        game.player.anim_loop = RAVEN_SPRITE_ANIMATION_BUNNY_LOOP_STAND;
        game.player.anim_frame = 0;
        game.display.msg_mod_event_frames_left = 0;
        game.display.msg_save_frames_left = 0;
        msg_mod_play(&raven_mods[game.mod.index], game.mod.volume>>4, true);
    }
    game.display.load_success = (ret == 0);
    game.display.msg_load_frames_left = 180;
}

static void save_game(void)
{
    int ret = savegame_write(&game, 0);
    game.display.save_success = (ret == 0);
    game.display.msg_save_frames_left = 180;
    game.display.msg_load_frames_left = 0;
}

static void process_joy_input(void)
{
    if (JOY_BTN_HELD(&joy, JOY_BTN_L2)) {
        game.mod.volume = (game.mod.volume > 1) ? game.mod.volume - 1 : 0;
        msg_mod_set_volume(game.mod.volume>>4);
    }
    if (JOY_BTN_HELD(&joy, JOY_BTN_R2)) {
        game.mod.volume = (game.mod.volume < 0x100) ? game.mod.volume + 1 : 0x100;
        msg_mod_set_volume(game.mod.volume>>4);
    }
    if (JOY_BTN_PRESSED(&joy, JOY_BTN_L1)) {
        game.mod.index = (game.mod.index-1+RAVEN_MOD_COUNT) % RAVEN_MOD_COUNT;
        msg_mod_play(&raven_mods[game.mod.index], game.mod.volume>>4, true);
    }
    if (JOY_BTN_PRESSED(&joy, JOY_BTN_R1)) {
        game.mod.index = (game.mod.index+1) % RAVEN_MOD_COUNT;
        msg_mod_play(&raven_mods[game.mod.index], game.mod.volume>>4, true);
    }
    if (JOY_BTN_PRESSED(&joy, JOY_BTN_C)) {
        msg_sfx_play_once(0, &raven_sfxs[0], 0x10, 0x3<<10);
    }
    if (JOY_BTN_PRESSED(&joy, JOY_BTN_D)) {
        game.display.show_palette = !game.display.show_palette;
    }

    if (JOY_BTN_PRESSED(&joy, JOY_BTN_START)) {
        if (JOY_BTN_HELD(&joy, JOY_BTN_SELECT)) {
            save_game();
        } else {
            load_game();
        }
    }

    player_control_update(&game.player_control, &game.player);
}

static void update_game_state(void)
{
    // blink LED
    if (game.led.frames-- <= 0) {
        game.led.frames = 15;
        game.led.state = !game.led.state;
        gpio_put(LED_PIN, game.led.state);
    }

    // advance message display timers
    if (game.display.msg_mod_event_frames_left > 0) game.display.msg_mod_event_frames_left--;
    if (game.display.msg_load_frames_left > 0) game.display.msg_load_frames_left--;
    if (game.display.msg_save_frames_left > 0) game.display.msg_save_frames_left--;

    // animate character
    player_advance_state_with_control(&game.player, &game.player_control);
    if (game.player.x < 0) game.player.x = 0;
    if (game.player.x + game.player.anim->collision.w >= game.room_w) {
        game.player.x = game.room_w - game.player.anim->collision.w;
    }
    if (game.player.y < 0) game.player.y = 0;
    if (game.player.y + game.player.anim->collision.h >= game.room_h) {
        game.player.y = game.room_h - game.player.anim->collision.h;
    }
    player_update_sprite_info(&game.player);

    // make screen follow player (TODO: add some slack)
    game.screen_x = game.player.x + game.player.sprite->width/2 - 160;
    game.screen_y = game.player.y + game.player.sprite->height/2 - 120;
    if (game.screen_x < 0) game.screen_x = 0;
    if (game.screen_y < 0) game.screen_y = 0;
    if (game.screen_x >= game.room_w - vga_screen.width) game.screen_x = game.room_w - vga_screen.width;
    if (game.screen_y >= game.room_h - vga_screen.height) game.screen_y = game.room_h - vga_screen.height;
}

static void process_core_messages(void)
{
    union CORE_MSG msg;
    while (core0_msg_try_recv(&msg)) {
        switch (msg.msg_header.type) {
        case CORE0_MSG_TYPE_MOD_EVENT:
            process_mod_event(msg.mod_event.chan, msg.mod_event.event);
            return;

        default:
            printf("ERROR processing message: invalid messaged type %d\n", msg.msg_header.type);
            return;
        }
    }
}

void game_main_loop(void)
{
    game_init();

    msg_mod_play(&raven_mods[game.mod.index], game.mod.volume>>4, true);
    while (true) {
        uint32_t update_start = time_us_32();

        // request joy update
        joy.last = joy.cur;
        joy_wii_i2c_request_state(&joy);
        uint32_t joy_req_us = time_us_32();

        // process core messages while waiting for joy update
        process_core_messages();

        // ensure 200us have elapsed since joy update request
        uint32_t joy_read_us = time_us_32();
        while (time_us_32() - joy_req_us < 200) sleep_us(10);
        joy_wii_i2c_read_state(&joy);
        game.perf.joy_read_us = time_us_32() - joy_read_us;

        process_joy_input();
        update_game_state();

        game.perf.update_us = time_us_32() - update_start;
        screen_render(&game);
    }
}
