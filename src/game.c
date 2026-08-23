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
#include "run_state.h"
#include "player.h"
#include "enemy.h"
#include "collision.h"
#include "screen.h"
#include "sprite_shadow.h"
#include "draw_room.h"
#include "savegame.h"

static struct SAVEGAME tmp_savegame;
static const struct SAVEGAME new_savegame = {
    .room_id = RAVEN_ROOM_ID_WEST__BUNTOWN_GATE,
};

const struct RAVEN_ROOM_TRIGGER_INFO *game_get_trigger_from_id(int room_id, int trigger_id)
{
    const struct RAVEN_ROOM *room = &raven_rooms[room_id];
    for (int i = 0; i < room->num_triggers; i++) {
        if (room->triggers[i].trigger_id == trigger_id) {
            return &room->triggers[i];
        }
    }
    return NULL;
}

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

void game_spawn_room_enemies(struct GAME_STATE *game)
{
    const struct RAVEN_ROOM *room = &raven_rooms[game->room_id];
    for (int tr_index = 0; tr_index < room->num_triggers; tr_index++) {
        const struct RAVEN_ROOM_TRIGGER_INFO *tr = &room->triggers[tr_index];
        if (tr->type == RAVEN_ROOM_TRIGGER_TYPE_ENEMY_SPAWN && game->num_enemies < GAME_MAX_ENEMIES) {
            int enemy_index = game->num_enemies++;
            enemy_init(game, enemy_index, tr);
        }
    }
}

void game_update_room_enemies(struct GAME_STATE *game)
{
    for (int i = 0; i < game->num_enemies; i++) {
        enemy_update(game, i);
    }
}

static void screen_follow_player(struct GAME_STATE *game)
{
    // TODO: add slack
    run_state.screen_x = game->player.x + game->player.sprite->width/2 - 160;
    run_state.screen_y = game->player.y + game->player.sprite->height/2 - 120;
    if (run_state.screen_x < 0) run_state.screen_x = 0;
    if (run_state.screen_y < 0) run_state.screen_y = 0;
    if (run_state.screen_x >= run_state.room_w - vga_screen.width) run_state.screen_x = run_state.room_w - vga_screen.width;
    if (run_state.screen_y >= run_state.room_h - vga_screen.height) run_state.screen_y = run_state.room_h - vga_screen.height;
}

static int place_player_at_door_transition(struct GAME_STATE *game)
{
    const struct RAVEN_ROOM_TRIGGER_INFO *door = game_get_trigger_from_id(game->room_id, run_state.room_transition.dst_door_trigger_id);
    if (door == NULL) {
        return -1;
    }

    int tile_x = door->x / TILE_SIZE;
    int tile_y = door->y / TILE_SIZE;
    if (collision_get_room_tile_at(game, tile_x + 1, tile_y) == 0xff) {
        game->player.x = TILE_SIZE + 2;
        if (run_state.room_transition.player_dx < 0) run_state.room_transition.player_dx = 0;
    } else {
        game->player.x = door->x - 2 - game->player.anim->collision.w;
        if (run_state.room_transition.player_dx > 0) run_state.room_transition.player_dx = 0;
    }
    game->player.y = door->y + run_state.room_transition.player_y;
    game->player.state = run_state.room_transition.player_state;
    game->player.direction = run_state.room_transition.player_direction;
    game->player_control.dx = run_state.room_transition.player_dx;
    game->player_control.dy = run_state.room_transition.player_dy;
    return 0;
}

static void load_room(struct GAME_STATE *game, uint32_t room_id)
{
    const struct RAVEN_ROOM *room = &raven_rooms[room_id];
    game->room_id = room_id;

    // calc room width
    run_state.room_w = 0;
    run_state.room_h = 0;
    for (int i = 0; i < room->num_maps; i++) {
        int w = (room->maps[i].x + room->maps[i].map->w) * TILE_SIZE;
        int h = (room->maps[i].y + room->maps[i].map->h) * TILE_SIZE;
        if (run_state.room_w < w) run_state.room_w = w;
        if (run_state.room_h < h) run_state.room_h = h;
    }

    // setup player
    player_init(game);
    player_control_init(game);

    // setup room
    draw_room_init_room(game);
    game->num_enemies = 0;
    run_state.room_doors_enabled = 1;
    const struct RAVEN_ROOM_SCRIPT *script_table = raven_room_script_table[room_id];
    if (script_table) {
        script_table->init(room_id, game);
        run_state.update_room = script_table->update;
    } else {
        run_state.update_room = NULL;
    }

    screen_follow_player(game);
}

static void apply_savegame(struct GAME_STATE *game, const struct SAVEGAME *savegame)
{
    run_state_init();
    player_set_sprite_anim(game, RAVEN_SPRITE_ANIMATION_ID_BUNNY);
    load_room(game, savegame->room_id);
    msg_mod_play(&raven_mods[run_state.mod.index], run_state.mod.volume>>4, true);
}

static void load_game(struct GAME_STATE *game)
{
    int ret = savegame_read(&tmp_savegame, 0);
    if (ret == 0) {
        apply_savegame(game, &tmp_savegame);
    }
    run_state.display.load_success = (ret == 0);
    run_state.display.msg_load_frames_left = 180;
}

static void save_game(struct GAME_STATE *game)
{
    memset(&tmp_savegame, 0, sizeof(struct SAVEGAME));
    tmp_savegame.room_id = game->room_id;
    int ret = savegame_write(&tmp_savegame, 0);
    run_state.display.save_success = (ret == 0);
    run_state.display.msg_save_frames_left = 180;
    run_state.display.msg_load_frames_left = 0;
}

static void process_joy_input(struct GAME_STATE *game, struct JOYSTICK *joy)
{
    if (JOY_BTN_HELD(joy, JOY_BTN_L2)) {
        run_state.mod.volume = (run_state.mod.volume > 1) ? run_state.mod.volume - 1 : 0;
        msg_mod_set_volume(run_state.mod.volume>>4);
    }
    if (JOY_BTN_HELD(joy, JOY_BTN_R2)) {
        run_state.mod.volume = (run_state.mod.volume < 0x100) ? run_state.mod.volume + 1 : 0x100;
        msg_mod_set_volume(run_state.mod.volume>>4);
    }
    if (JOY_BTN_PRESSED(joy, JOY_BTN_L1)) {
        run_state.mod.index = (run_state.mod.index-1+RAVEN_MOD_COUNT) % RAVEN_MOD_COUNT;
        msg_mod_play(&raven_mods[run_state.mod.index], run_state.mod.volume>>4, true);
    }
    if (JOY_BTN_PRESSED(joy, JOY_BTN_R1)) {
        run_state.mod.index = (run_state.mod.index+1) % RAVEN_MOD_COUNT;
        msg_mod_play(&raven_mods[run_state.mod.index], run_state.mod.volume>>4, true);
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
        run_state.display.show_perf = (run_state.display.show_perf + 1) % 3;
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
    // advance message display timers
    if (run_state.display.msg_mod_event_frames_left > 0) run_state.display.msg_mod_event_frames_left--;
    if (run_state.display.msg_load_frames_left > 0) run_state.display.msg_load_frames_left--;
    if (run_state.display.msg_save_frames_left > 0) run_state.display.msg_save_frames_left--;

    // update character
    player_update(game, joy);
    screen_follow_player(game);

    // update room
    if (run_state.room_doors_enabled) {
        int door_index = game_check_player_trigger(game, 1<<RAVEN_ROOM_TRIGGER_TYPE_DOOR);
        if (door_index >= 0) {
            const struct RAVEN_ROOM_TRIGGER_INFO *door = &raven_rooms[game->room_id].triggers[door_index];
            run_state.room_transition.enabled = true;
            run_state.room_transition.frame = 0;
            run_state.room_transition.src_room_id = game->room_id;
            run_state.room_transition.src_door_trigger_id = door->trigger_id;
            run_state.room_transition.dst_room_id = (uint16_t) (door->door.dest_room - raven_rooms);
            run_state.room_transition.dst_door_trigger_id = door->door.dest_trigger_id;
            run_state.room_transition.player_x = game->player.x - door->x;
            run_state.room_transition.player_y = game->player.y - door->y;
            run_state.room_transition.player_state = game->player.state;
            run_state.room_transition.player_direction = game->player.direction;
            run_state.room_transition.player_dx = game->player_control.dx;
            run_state.room_transition.player_dy = game->player_control.dy;
        }
    }
    if (run_state.update_room) {
        run_state.update_room(game);
    }
}

static void process_mod_event(struct GAME_STATE *game, uint8_t chan, uint8_t event)
{
    (void)game;
    run_state.mod.event_chan = chan;
    run_state.mod.event_data = event;
    run_state.display.msg_mod_event_frames_left = 60;
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

static int advance_room_transition(struct GAME_STATE *game, struct JOYSTICK *joy)
{
#define FADE_OUT_FRAMES 16
#define BLACKOUT_FRAMES 8
#define FADE_IN_FRAMES  16
#define START_FADE_OUT  0
#define END_FADE_OUT    (START_FADE_OUT+FADE_OUT_FRAMES-1)
#define START_BLACKOUT  (END_FADE_OUT+1)
#define END_BLACKOUT    (START_BLACKOUT+BLACKOUT_FRAMES-1)
#define START_FADE_IN   (END_BLACKOUT+1)
#define END_FADE_IN     (START_FADE_IN+FADE_IN_FRAMES)
#define END_TRANSITION  (END_FADE_IN+1)
    if (run_state.room_transition.frame == START_FADE_OUT) {
        // render and capture exiting old room
        RUN_PERF(update_us);
        screen_render(game, joy);
        screen_save_to_scratch(0);
        vga_swap_buffers(true);
        RUN_PERF(vsync_us);
    } else if (run_state.room_transition.frame == START_FADE_IN) {
        // render and capture entering new room
        load_room(game, run_state.room_transition.dst_room_id);
        place_player_at_door_transition(game);
        screen_follow_player(game);
        RUN_PERF(update_us);
        screen_render(game, joy);
        screen_save_to_scratch(1);
    } else {
        if (run_state.room_transition.frame >= END_TRANSITION) {
            // transition done, resume game
            run_state.room_transition.enabled = 0;
            return 0;
        }

        RUN_PERF(update_us);
        if (run_state.room_transition.frame <= END_FADE_OUT) {
            // fading out
            screen_fade_from_scratch(2 - (run_state.room_transition.frame-START_FADE_OUT) / (FADE_OUT_FRAMES/2));
        } else if (run_state.room_transition.frame <= END_BLACKOUT) {
            // blacking out
            screen_fade_from_scratch(0);
        } else if (run_state.room_transition.frame <= END_FADE_IN) {
            // fading in
            screen_fade_from_scratch(1 + (run_state.room_transition.frame-START_FADE_IN) / (FADE_IN_FRAMES/2));
        }
    }

    vga_swap_buffers(true);
    RUN_PERF(vsync_us);
    run_state.room_transition.frame++;
    return 1;
}

void game_main_loop(struct GAME_STATE *game, struct JOYSTICK *joy)
{
    apply_savegame(game, &new_savegame);

    while (true) {
        RUN_PERF_START();

        // request joy update
        joy->last = joy->cur;
        joy_wii_i2c_request_state(joy);
        uint32_t joy_req_us = time_us_32();
        RUN_PERF_AT(joy_req_us, joy_req_us);

        // process core messages while waiting for joy update
        process_core_messages(game);

        // ensure 200us have elapsed since joy update request
        while (time_us_32() - joy_req_us < 200) sleep_us(10);
        joy_wii_i2c_read_state(joy);
        RUN_PERF(joy_read_us);

        // update run state
        run_state_fps_count();
        run_state_blink_led();

        // door transition
        if (run_state.room_transition.enabled && advance_room_transition(game, joy)) {
            continue;
        }

        // advance game
        process_joy_input(game, joy);
        update_game_state(game, joy);
        RUN_PERF(update_us);

        // render screen
        screen_render(game, joy);
        vga_swap_buffers(true);
        RUN_PERF(vsync_us);
    }
}
