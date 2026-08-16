#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"

#include "game.h"

#include "lib/joy_wii_i2c.h"
#include "lib/joystick.h"
#include "lib/vga_8bit.h"

#include "config.h"
#include "core_msg.h"
#include "game_data.h"
#include "player.h"
#include "collision.h"
#include "screen.h"
#include "sprite_shadow.h"
#include "draw_room.h"
#include "savegame.h"

int game_check_player_trigger(struct GAME_STATE *game, uint32_t trigger_type_flags)
{
    int p1_x = game->player.x + game->player.anim->collision.w/2;
    int p1_y = game->player.y + 8;
    int p2_x = game->player.x + game->player.anim->collision.w/2;
    int p2_y = game->player.y + game->player.anim->collision.h/2 - 9;

    const struct RAVEN_ROOM *room = &raven_rooms[game->room_id];
    for (int tr_index = 0; tr_index < room->num_triggers; tr_index++) {
        const struct RAVEN_ROOM_TRIGGER_INFO *tr = &room->triggers[tr_index];
        if ((trigger_type_flags & (1 << tr->type)) == 0) {
            continue;
        }
        int tr_w = 0;
        int tr_h = 0;
        if (tr->type == RAVEN_ROOM_TRIGGER_TYPE_DOOR) {
            tr_w = 16;
            tr_h = 64;
        } else if (tr->type == RAVEN_ROOM_TRIGGER_TYPE_TRAP) {
            tr_w = tr->trap.width;
            tr_h = tr->trap.height;
        } else {
            continue;
        }
        if ((p1_x >= tr->x && p1_x < tr->x + tr_w && p1_y >= tr->y && p1_y < tr->y + tr_h) ||
            (p2_x >= tr->x && p2_x < tr->x + tr_w && p2_y >= tr->y && p2_y < tr->y + tr_h)) {
            return tr_index;
        }
    }
    return -1;
}

static void screen_follow_player(struct GAME_STATE *game)
{
    // TODO: add slack
    game->screen_x = game->player.x + game->player.sprite->width/2 - 160;
    game->screen_y = game->player.y + game->player.sprite->height/2 - 120;
    if (game->screen_x < 0) game->screen_x = 0;
    if (game->screen_y < 0) game->screen_y = 0;
    if (game->screen_x >= game->room_w - vga_screen.width) game->screen_x = game->room_w - vga_screen.width;
    if (game->screen_y >= game->room_h - vga_screen.height) game->screen_y = game->room_h - vga_screen.height;
}

static int place_player_at_door_exit(struct GAME_STATE *game, int32_t door_trigger_id)
{
    const struct RAVEN_ROOM *room = &raven_rooms[game->room_id];
    const struct RAVEN_ROOM_TRIGGER_INFO *door = NULL;
    for (int i = 0; i < room->num_triggers; i++) {
        if (room->triggers[i].trigger_id == door_trigger_id) {
            door = &room->triggers[i];
            break;
        }
    }
    if (door == NULL) {
        return -1;
    }

    // move player near door
    int tile_x = door->x / TILE_SIZE;
    int tile_y = door->y / TILE_SIZE;
    if (collision_get_room_tile_at(game, tile_x + 1, tile_y) == 0xff) {
        game->player.direction = RAVEN_DIR_RIGHT;
        game->player.x = TILE_SIZE + 2;
        game->player.y = door->y + 4*TILE_SIZE - game->player.anim->collision.h;
    } else {
        game->player.direction = RAVEN_DIR_LEFT;
        game->player.x = door->x - 2 - game->player.anim->collision.w;
        game->player.y = door->y + 4*TILE_SIZE - game->player.anim->collision.h;
    }

    // set player on ground
    struct COLLISION_RECT rect = {
        .x = game->player.x,
        .y = game->player.y,
        .w = game->player.anim->collision.w,
        .h = game->player.anim->collision.h,
    };
    while (collision_move(game, &rect, 0, 128) == 0) {
        // keep going down until we hit the floor
    }
    game->player.x = rect.x;
    game->player.y = rect.y;
    return 0;
}

static void load_room(struct GAME_STATE *game, uint32_t room_id)
{
    const struct RAVEN_ROOM *room = &raven_rooms[room_id];
    game->room_id = room_id;

    // calc room width
    game->room_w = 0;
    game->room_h = 0;
    for (int i = 0; i < room->num_maps; i++) {
        int w = (room->maps[i].x + room->maps[i].map->w) * TILE_SIZE;
        int h = (room->maps[i].y + room->maps[i].map->h) * TILE_SIZE;
        if (game->room_w < w) game->room_w = w;
        if (game->room_h < h) game->room_h = h;
    }

    // setup player
    player_init(game);
    player_control_init(game);

    // setup room
    draw_room_init_room(game);
    game->room_doors_enabled = 1;
    const struct RAVEN_ROOM_SCRIPT *script_table = raven_room_script_table[room_id];
    if (script_table) {
        script_table->init(room_id, game);
        game->update_room = script_table->update;
    } else {
        game->update_room = NULL;
    }
}

static void game_init(struct GAME_STATE *game)
{
    game->mod.index = RAVEN_MOD_ID_BWV_106;
    game->mod.volume = 0x40;

    game->display.show_perf = 1;
    game->display.msg_load_frames_left = 0;
    game->display.msg_save_frames_left = 0;
    game->display.msg_mod_event_frames_left = 0;

    player_set_sprite_anim(game, RAVEN_SPRITE_ANIMATION_ID_BUNNY);
    load_room(game, RAVEN_ROOM_ID_WEST__BUNTOWN_GATE);
}

static void load_game(struct GAME_STATE *game)
{
    static struct GAME_STATE tmp_game;

    memcpy(&tmp_game, game, sizeof(struct GAME_STATE));
    int ret = savegame_read(&tmp_game, 0);
    if (ret == 0) {
        memcpy(game, &tmp_game, sizeof(struct GAME_STATE));
        game->player.state = PLAYER_STATE_STAND;
        game->player.anim_frame = 0;
        player_control_init(game);
        game->display.msg_mod_event_frames_left = 0;
        game->display.msg_save_frames_left = 0;
        msg_mod_play(&raven_mods[game->mod.index], game->mod.volume>>4, true);
    }
    game->display.load_success = (ret == 0);
    game->display.msg_load_frames_left = 180;
}

static void save_game(struct GAME_STATE *game)
{
    int ret = savegame_write(game, 0);
    game->display.save_success = (ret == 0);
    game->display.msg_save_frames_left = 180;
    game->display.msg_load_frames_left = 0;
}

static void process_joy_input(struct GAME_STATE *game, struct JOYSTICK *joy)
{
    if (JOY_BTN_HELD(joy, JOY_BTN_L2)) {
        game->mod.volume = (game->mod.volume > 1) ? game->mod.volume - 1 : 0;
        msg_mod_set_volume(game->mod.volume>>4);
    }
    if (JOY_BTN_HELD(joy, JOY_BTN_R2)) {
        game->mod.volume = (game->mod.volume < 0x100) ? game->mod.volume + 1 : 0x100;
        msg_mod_set_volume(game->mod.volume>>4);
    }
    if (JOY_BTN_PRESSED(joy, JOY_BTN_L1)) {
        game->mod.index = (game->mod.index-1+RAVEN_MOD_COUNT) % RAVEN_MOD_COUNT;
        msg_mod_play(&raven_mods[game->mod.index], game->mod.volume>>4, true);
    }
    if (JOY_BTN_PRESSED(joy, JOY_BTN_R1)) {
        game->mod.index = (game->mod.index+1) % RAVEN_MOD_COUNT;
        msg_mod_play(&raven_mods[game->mod.index], game->mod.volume>>4, true);
    }
    if (JOY_BTN_PRESSED(joy, JOY_BTN_B)) {
        game->player.shadow_enabled = 1 - game->player.shadow_enabled;
        if (game->player.shadow_enabled) {
            sprite_shadow_clear();
        }
    }
    if (JOY_BTN_PRESSED(joy, JOY_BTN_C)) {
        msg_sfx_play_once(0, &raven_sfxs[0], 0x10, 0x3<<10);
    }
    if (JOY_BTN_PRESSED(joy, JOY_BTN_D)) {
        game->display.show_perf = (game->display.show_perf + 1) % 3;
    }

    if (JOY_BTN_PRESSED(joy, JOY_BTN_START)) {
        if (JOY_BTN_HELD(joy, JOY_BTN_SELECT)) {
            save_game(game);
        } else {
            load_game(game);
        }
    }

    player_control_update(game, joy);
}

static void update_game_state(struct GAME_STATE *game, struct JOYSTICK *joy)
{
    // blink LED
    if (game->led.frames-- <= 0) {
        game->led.frames = 15;
        game->led.state = !game->led.state;
        gpio_put(LED_PIN, game->led.state);
    }

    // advance message display timers
    if (game->display.msg_mod_event_frames_left > 0) game->display.msg_mod_event_frames_left--;
    if (game->display.msg_load_frames_left > 0) game->display.msg_load_frames_left--;
    if (game->display.msg_save_frames_left > 0) game->display.msg_save_frames_left--;

    // update character
    player_update(game, joy);
    screen_follow_player(game);

    // update room
    if (game->room_doors_enabled) {
        int door_index = game_check_player_trigger(game, 1<<RAVEN_ROOM_TRIGGER_TYPE_DOOR);
        if (door_index >= 0) {
            const struct RAVEN_ROOM_TRIGGER_INFO *door = &raven_rooms[game->room_id].triggers[door_index];
            game->room_transition.src_room_id = game->room_id;
            game->room_transition.src_door_trigger_id = door->trigger_id;
            game->room_transition.dst_room_id = (uint16_t) (door->door.dest_room - raven_rooms);
            game->room_transition.dst_door_trigger_id = door->door.dest_trigger_id;
            game->room_transition.enabled = true;
        }
    }
    if (game->update_room) {
        game->update_room(game);
    }
}

static void process_mod_event(struct GAME_STATE *game, uint8_t chan, uint8_t event)
{
    game->mod.event_chan = chan;
    game->mod.event_data = event;
    game->display.msg_mod_event_frames_left = 60;
}

static void process_core_messages(struct GAME_STATE *game)
{
    union CORE_MSG msg;
    while (core0_msg_try_recv(&msg)) {
        switch (msg.msg_header.type) {
        case CORE0_MSG_TYPE_MOD_EVENT:
            process_mod_event(game, msg.mod_event.chan, msg.mod_event.event);
            return;

        default:
            printf("ERROR processing message: invalid messaged type %d\n", msg.msg_header.type);
            return;
        }
    }
}

void game_main_loop(struct GAME_STATE *game, struct JOYSTICK *joy)
{
    game_init(game);

    msg_mod_play(&raven_mods[game->mod.index], game->mod.volume>>4, true);
    while (true) {
        GAME_PERF_START(game);

        // request joy update
        joy->last = joy->cur;
        joy_wii_i2c_request_state(joy);
        uint32_t joy_req_us = time_us_32();
        GAME_PERF_AT(game, joy_req_us, joy_req_us);

        // process core messages while waiting for joy update
        process_core_messages(game);

        // ensure 200us have elapsed since joy update request
        while (time_us_32() - joy_req_us < 200) sleep_us(10);
        joy_wii_i2c_read_state(joy);
        GAME_PERF(game, joy_read_us);

        if (game->room_transition.enabled) {
            // TODO: fade
            load_room(game, game->room_transition.dst_room_id);
            place_player_at_door_exit(game, game->room_transition.dst_door_trigger_id);
            screen_follow_player(game);
            game->room_transition.enabled = 0;
        } else {
            process_joy_input(game, joy);
            update_game_state(game, joy);
        }

        GAME_PERF(game, update_us);
        screen_render(game, joy);
    }
}
