#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"

#include "game.h"

#include "lib/joy_wii_i2c.h"
#include "lib/joystick.h"
#include "lib/vga_8bit.h"
#include "lib/vga_image_8bit.h"

#include "config.h"
#include "core_msg.h"
#include "game_data.h"
#include "player.h"
#include "enemy.h"
#include "collision.h"
#include "screen.h"
#include "sprite_shadow.h"
#include "draw_room.h"
#include "savegame.h"
#include "rooms/default.h"

struct GAME_RUN_STATE game_state;

static struct SAVEGAME tmp_savegame;
static const struct SAVEGAME new_savegame = {
    .room_id = RAVEN_ROOM_ID_WEST__LAB,
};

void game_state_init(void)
{
    memset(&game_state, 0, sizeof(struct GAME_RUN_STATE));

    game_state.mod.index = RAVEN_MOD_ID_BWV_106;
    game_state.mod.volume = 0x40;
}

static int clamp(int val, int min, int max) {
    if (val < min) return min;
    if (val > max) return max;
    return val;
}

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

const struct RAVEN_ROOM_TRIGGER_INFO *game_state_check_player_trigger(uint32_t trigger_type_flags)
{
    int p1_x = game_state.player.x;
    int p1_y = game_state.player.y;
    int p2_x = game_state.player.x + game_state.player.anim->collision.w - 1;
    int p2_y = game_state.player.y + game_state.player.anim->collision.h - 1;

    const struct RAVEN_ROOM *room = &raven_rooms[game_state.room_id];
    for (int tr_index = 0; tr_index < room->num_triggers; tr_index++) {
        const struct RAVEN_ROOM_TRIGGER_INFO *tr = &room->triggers[tr_index];
        if ((trigger_type_flags & (1 << tr->type)) == 0) {
            continue;
        }
        int tr_w = 0;
        int tr_h = 0;
        switch (tr->type) {
        case RAVEN_ROOM_TRIGGER_TYPE_GET_UPGRADE:
        case RAVEN_ROOM_TRIGGER_TYPE_GET_COLLECTABLE:
        case RAVEN_ROOM_TRIGGER_TYPE_GET_PICKUP:
            tr_w = 16;
            tr_h = 16;
            break;
        case RAVEN_ROOM_TRIGGER_TYPE_DOOR:
            tr_w = 16;
            tr_h = 64;
            break;
        case RAVEN_ROOM_TRIGGER_TYPE_WALL_BUTTON:
            tr_w = tr->wall_button.width;
            tr_h = tr->wall_button.height;
            break;
        case RAVEN_ROOM_TRIGGER_TYPE_DETECTOR_BUTTON:
            tr_w = tr->detector_button.width;
            tr_h = tr->detector_button.height;
            break;
        default:
            continue;
        }

        int t1_x = tr->x;
        int t2_x = tr->x + tr_w - 1;
        int t1_y = tr->y;
        int t2_y = tr->y + tr_h - 1;
        if (p1_x <= t2_x && t1_x <= p2_x && p1_y <= t2_y && t1_y <= p2_y) {
            return tr;
        }
    }
    return NULL;
}

static void screen_follow_player(void)
{
    // TODO: add slack
    int32_t screen_width = vga_screen.width;
    int32_t screen_height = vga_screen.height - SCREEN_HEADER_HEIGHT;
    game_state.screen_x = game_state.player.x + game_state.player.sprite->width/2 - screen_width/2;
    game_state.screen_y = game_state.player.y + game_state.player.sprite->height/2 - screen_height/2;
    if (game_state.screen_x < 0) game_state.screen_x = 0;
    if (game_state.screen_y < 0) game_state.screen_y = 0;
    if (game_state.screen_x >= game_state.room_w - screen_width) game_state.screen_x = game_state.room_w - screen_width;
    if (game_state.screen_y >= game_state.room_h - screen_height) game_state.screen_y = game_state.room_h - screen_height;
}

static int place_player_at_door_transition(void)
{
    const struct RAVEN_ROOM_TRIGGER_INFO *door = game_get_trigger_from_id(game_state.room_id,
                                                                          game_state.room_transition.dst_door_trigger_id);
    if (door == NULL) {
        return -1;
    }

    int tile_x = door->x / TILE_SIZE;
    int tile_y = door->y / TILE_SIZE;
    if (collision_get_room_tile_at(tile_x + 1, tile_y) == TILE_FX_FREE) {
        game_state.player.x = door->x + TILE_SIZE + 2;
        if (game_state.room_transition.player_dx < 0) game_state.room_transition.player_dx = 0;
    } else {
        game_state.player.x = door->x - 2 - game_state.player.anim->collision.w;
        if (game_state.room_transition.player_dx > 0) game_state.room_transition.player_dx = 0;
    }
    game_state.player.y = door->y + game_state.room_transition.player_y;
    game_state.player.state = game_state.room_transition.player_state;
    game_state.player.direction = game_state.room_transition.player_direction;
    game_state.player_control.dx = game_state.room_transition.player_dx;
    game_state.player_control.dy = game_state.room_transition.player_dy;
    player_update_sprite_info();
    return 0;
}

static void load_room_tiles(const struct RAVEN_ROOM *room)
{
    game_state.room_w = 0;
    game_state.room_h = 0;
    game_state.room_tiles_w = 0;
    game_state.room_tiles_h = 0;
    for (int i = 0; i < room->num_maps; i++) {
        int w = room->maps[i].x + room->maps[i].map->w;
        int h = room->maps[i].y + room->maps[i].map->h;
        if (game_state.room_tiles_w < w) game_state.room_tiles_w = w;
        if (game_state.room_tiles_h < h) game_state.room_tiles_h = h;
        if (game_state.room_w < w*TILE_SIZE) game_state.room_w = w*TILE_SIZE;
        if (game_state.room_h < h*TILE_SIZE) game_state.room_h = h*TILE_SIZE;
    }

    memset(game_state.tiles, 0xff, sizeof(game_state.tiles));
    game_state.bg_tiles = &game_state.tiles[0*game_state.room_tiles_w*game_state.room_tiles_h];
    game_state.fg_tiles = &game_state.tiles[1*game_state.room_tiles_w*game_state.room_tiles_h];
    game_state.fx_tiles = &game_state.tiles[2*game_state.room_tiles_w*game_state.room_tiles_h];
    game_state.bb_tiles = &game_state.tiles[3*game_state.room_tiles_w*game_state.room_tiles_h];
    game_state.has_bb = false;

    // copy (bg, fg, fx) from all maps
    for (int i = 0; i < room->num_maps; i++) {
        const struct RAVEN_ROOM_MAP_INFO *room_map = &room->maps[i];

        int map_w = room_map->map->w;
        int map_h = room_map->map->h;
        int src_line = 0;
        int dst_line = game_state.room_tiles_w*room_map->y + room_map->x;
        const uint8_t *map_tiles = room_map->map->tiles;
        for (int y = 0; y < map_h; y++) {
            memcpy(game_state.fg_tiles + dst_line, map_tiles + 0*map_w*map_h + src_line, map_w);
            memcpy(game_state.bg_tiles + dst_line, map_tiles + 1*map_w*map_h + src_line, map_w);
            memcpy(game_state.fx_tiles + dst_line, map_tiles + 2*map_w*map_h + src_line, map_w);
            if (room_map->map->has_bb_layer) {
                memcpy(game_state.bb_tiles + dst_line, map_tiles + 3*map_w*map_h + src_line, map_w);
                game_state.has_bb = true;
            }
            src_line += map_w;
            dst_line += game_state.room_tiles_w;
        }
    }
}

static void load_room(uint32_t room_id)
{
    const struct RAVEN_ROOM *room = &raven_rooms[room_id];
    game_state.room_id = room_id;
    load_room_tiles(room);

    // reset player
    player_init();
    player_control_init();

    // run room init
    game_state.num_enemies = 0;
    game_state.num_pickup_respawns = 0;
    const struct RAVEN_ROOM_SCRIPT *script_table = raven_room_script_table[room_id];
    if (script_table) {
        script_table->init(room_id);
        game_state.update_room = script_table->update;
    } else {
        default_room_init(room_id);
        game_state.update_room = default_room_update;
    }

    screen_follow_player();
}

static void apply_savegame(const struct SAVEGAME *savegame)
{
    game_state_init();
    player_set_sprite_anim(RAVEN_SPRITE_ANIMATION_ID_BUNNY);
    load_room(savegame->room_id);
    msg_mod_play(&raven_mods[game_state.mod.index], game_state.mod.volume>>4, true);
}

static void load_game(void)
{
    int ret = savegame_read(&tmp_savegame, 0);
    if (ret == 0) {
        apply_savegame(&tmp_savegame);
    }
    game_state.display.load_success = (ret == 0);
    game_state.display.msg_load_frames_left = 180;
}

static void save_game(void)
{
    memset(&tmp_savegame, 0, sizeof(struct SAVEGAME));
    tmp_savegame.room_id = game_state.room_id;
    int ret = savegame_write(&tmp_savegame, 0);
    game_state.display.save_success = (ret == 0);
    game_state.display.msg_save_frames_left = 180;
    game_state.display.msg_load_frames_left = 0;
}

static void process_joy_input(struct JOYSTICK *joy)
{
    if (JOY_BTN_HELD(joy, JOY_BTN_L2)) {
        game_state.mod.volume = (game_state.mod.volume > 1) ? game_state.mod.volume - 1 : 0;
        msg_mod_set_volume(game_state.mod.volume>>4);
    }
    if (JOY_BTN_HELD(joy, JOY_BTN_R2)) {
        game_state.mod.volume = (game_state.mod.volume < 0x100) ? game_state.mod.volume + 1 : 0x100;
        msg_mod_set_volume(game_state.mod.volume>>4);
    }
    if (JOY_BTN_PRESSED(joy, JOY_BTN_L1)) {
        game_state.mod.index = (game_state.mod.index-1+RAVEN_MOD_COUNT) % RAVEN_MOD_COUNT;
        msg_mod_play(&raven_mods[game_state.mod.index], game_state.mod.volume>>4, true);
    }
    if (JOY_BTN_PRESSED(joy, JOY_BTN_R1)) {
        game_state.mod.index = (game_state.mod.index+1) % RAVEN_MOD_COUNT;
        msg_mod_play(&raven_mods[game_state.mod.index], game_state.mod.volume>>4, true);
    }
    if (JOY_BTN_PRESSED(joy, JOY_BTN_Y)) {
        game_state.player.shadow_enabled = 1 - game_state.player.shadow_enabled;
        if (game_state.player.shadow_enabled) {
            sprite_shadow_clear();
        }
    }
    if (JOY_BTN_PRESSED(joy, JOY_BTN_Y)) {
        msg_sfx_play_once(0, &raven_sfxs[0], 0x10, 0x3<<10);
    }
    if (JOY_BTN_PRESSED(joy, JOY_BTN_A)) {
        game_state.display.show_perf = (game_state.display.show_perf + 1) % 3;
    }

    if (JOY_BTN_PRESSED(joy, JOY_BTN_START)) {
        if (JOY_BTN_HELD(joy, JOY_BTN_SELECT)) {
            save_game();
        } else {
            load_game();
        }
    }

    player_control_update(joy);
}

static void update_check_room_items(struct JOYSTICK *joy)
{
    uint32_t item_flags = ((1u<<RAVEN_ROOM_TRIGGER_TYPE_GET_UPGRADE) |
                           (1u<<RAVEN_ROOM_TRIGGER_TYPE_GET_PICKUP) |
                           (1u<<RAVEN_ROOM_TRIGGER_TYPE_GET_COLLECTABLE));
    const struct RAVEN_ROOM_TRIGGER_INFO *tr = game_state_check_player_trigger(item_flags);
    if (tr != NULL) {
        switch (tr->type) {
        case RAVEN_ROOM_TRIGGER_TYPE_GET_UPGRADE:
            game_state_set_upgrade(tr->get_upgrade.get_upgrade_id);
            break;
        case RAVEN_ROOM_TRIGGER_TYPE_GET_PICKUP:
            game_state_disable_pickup_trigger(tr->trigger_id, tr->get_pickup.respawn_delay);
            // TODO: pickup effect
            break;
        case RAVEN_ROOM_TRIGGER_TYPE_GET_COLLECTABLE:
            if (JOY_BTN_PRESSED(joy, JOY_BTN_X)) {
                game_state_set_collectable(tr->get_collectable.get_collectable_id);
            }
            break;
        }
    }
}

static void update_check_room_buttons(struct JOYSTICK *joy)
{
    uint32_t button_flags = ((1u<<RAVEN_ROOM_TRIGGER_TYPE_WALL_BUTTON) |
                             (1u<<RAVEN_ROOM_TRIGGER_TYPE_DETECTOR_BUTTON));
    const struct RAVEN_ROOM_TRIGGER_INFO *tr = game_state_check_player_trigger(button_flags);
    if (tr != NULL) {
        switch (tr->type) {
        case RAVEN_ROOM_TRIGGER_TYPE_DETECTOR_BUTTON:
            {
                int collectable_ok = (tr->detector_button.req_collectable_id == 0xffff ||
                                      game_state_has_collectable(tr->detector_button.req_collectable_id));
                int flag_ok = (tr->detector_button.req_flag_id == 0xffff ||
                               game_state_has_flag(tr->detector_button.req_flag_id));
                if (collectable_ok && flag_ok) {
                    game_state_activate_effect(tr->detector_button.set_effect_id);
                    if (tr->detector_button.set_flag_id != 0xffff && ! game_state_has_flag(tr->detector_button.set_flag_id)) {
                        game_state_set_flag(tr->detector_button.set_flag_id);
                        game_state_spawn_room_enemies(tr->detector_button.set_flag_id);
                    }
                }
            }
            break;

        case RAVEN_ROOM_TRIGGER_TYPE_WALL_BUTTON:
            if (JOY_BTN_PRESSED(joy, JOY_BTN_X)) {
                int collectable_ok = (tr->wall_button.req_collectable_id == 0xffff ||
                                       game_state_has_collectable(tr->wall_button.req_collectable_id));
                int flag_ok = (tr->wall_button.req_flag_id == 0xffff ||
                               game_state_has_flag(tr->wall_button.req_flag_id));
                if (collectable_ok && flag_ok) {
                    game_state_activate_effect(tr->wall_button.set_effect_id);
                    if (tr->detector_button.set_flag_id != 0xffff && ! game_state_has_flag(tr->detector_button.set_flag_id)) {
                        game_state_set_flag(tr->detector_button.set_flag_id);
                        game_state_spawn_room_enemies(tr->detector_button.set_flag_id);
                    }
                }
            }
            break;
        }
    }
}

static void update_game_state(struct JOYSTICK *joy)
{
    game_state.anim_step++;

    // advance message display timers
    if (game_state.display.msg_mod_event_frames_left > 0) game_state.display.msg_mod_event_frames_left--;
    if (game_state.display.msg_load_frames_left > 0) game_state.display.msg_load_frames_left--;
    if (game_state.display.msg_save_frames_left > 0) game_state.display.msg_save_frames_left--;

    // update character
    player_update(joy);
    screen_follow_player();

    // update room
    update_check_room_items(joy);
    update_check_room_buttons(joy);
    const struct RAVEN_ROOM_TRIGGER_INFO *door = game_state_check_player_trigger(1<<RAVEN_ROOM_TRIGGER_TYPE_DOOR);
    if (door != NULL && (door->door.req_flag_id == 0xffff || game_state_has_flag(door->door.req_flag_id))) {
        game_state.room_transition.enabled = true;
        game_state.room_transition.frame = 0;
        game_state.room_transition.src_room_id = game_state.room_id;
        game_state.room_transition.src_door_trigger_id = door->trigger_id;
        game_state.room_transition.dst_room_id = (uint16_t) (door->door.dest_room - raven_rooms);
        game_state.room_transition.dst_door_trigger_id = door->door.dest_trigger_id;
        game_state.room_transition.player_x = game_state.player.x - door->x;
        game_state.room_transition.player_y = game_state.player.y - door->y;
        game_state.room_transition.player_state = game_state.player.state;
        game_state.room_transition.player_direction = game_state.player.direction;
        game_state.room_transition.player_dx = game_state.player_control.dx;
        game_state.room_transition.player_dy = game_state.player_control.dy;
    }
    if (game_state.update_room) {
        game_state.update_room();
    }
}

static void process_mod_event(uint8_t chan, uint8_t event)
{
    game_state.mod.event_chan = chan;
    game_state.mod.event_data = event;
    game_state.display.msg_mod_event_frames_left = 60;
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

static const struct RAVEN_MAP_TILE_CHANGE *get_map_tile_change(int map_index, int tile_change_index)
{
    const struct RAVEN_ROOM *room = &raven_rooms[game_state.room_id];
    if (map_index >= room->num_maps) return NULL;
    const struct RAVEN_MAP *map = room->maps[map_index].map;
    if (tile_change_index >= map->num_tile_changes) return NULL;
    return &map->tile_changes[tile_change_index];
}

void game_state_activate_effect(uint32_t effect_id)
{
    const struct RAVEN_ROOM *room = &raven_rooms[game_state.room_id];
    for (int tr_index = 0; tr_index < room->num_triggers; tr_index++) {
        const struct RAVEN_ROOM_TRIGGER_INFO *tr = &room->triggers[tr_index];
        switch (tr->type) {
        case RAVEN_ROOM_TRIGGER_TYPE_UNBLOCK_TILES:
            if (tr->unblock_tiles.on_effect_id == effect_id) {
                int xs = clamp(tr->x / TILE_SIZE, 0, game_state.room_tiles_w-1);
                int ys = clamp(tr->y / TILE_SIZE, 0, game_state.room_tiles_h-1);
                int xe = clamp((tr->x + tr->unblock_tiles.width - 1) / TILE_SIZE, 0, game_state.room_tiles_w-1);
                int ye = clamp((tr->y + tr->unblock_tiles.height - 1) / TILE_SIZE, 0, game_state.room_tiles_h-1);
                for (int y = ys; y <= ye; y++) {
                    for (int x = xs; x <= xe; x++) {
                        game_state.fx_tiles[game_state.room_tiles_w*y + x] |= 0x0f;
                    }
                }
            }
            break;
        case RAVEN_ROOM_TRIGGER_TYPE_CHANGE_TILES:
            if (tr->change_tiles.on_effect_id == effect_id) {
                const struct RAVEN_MAP_TILE_CHANGE *change = get_map_tile_change(tr->change_tiles.map_index,
                                                                                 tr->change_tiles.tile_change_index);
                if (change != NULL) {
                    int xs = tr->x / TILE_SIZE;
                    int ys = tr->y / TILE_SIZE;
                    uint8_t *tiles;
                    switch (tr->change_tiles.map_layer) {
                    case 0:  tiles = game_state.bb_tiles; break;
                    case 1:  tiles = game_state.bg_tiles; break;
                    default: tiles = game_state.fg_tiles; break;
                    }
                    int bit_index = 0;
                    const uint8_t *change_tile = change->tiles;
                    for (int y = 0; y < change->h; y++) {
                        for (int x = 0; x < change->w; x++) {
                            int bit = change->tile_bitmap[bit_index / 32] & (1 << (bit_index % 32));
                            if (bit) {
                                if (x+xs >= 0 && x+xs < game_state.room_tiles_w && y+ys >= 0 && y+ys < game_state.room_tiles_h) {
                                    tiles[game_state.room_tiles_w*(y+ys) + x+xs] = *change_tile;
                                }
                                change_tile++;
                            }
                            bit_index++;
                        }
                    }
                }
            }
            break;
        case RAVEN_ROOM_TRIGGER_TYPE_DISABLE_ANIMATION:
            if (tr->disable_animation.on_effect_id == effect_id) {
                int xs = clamp(tr->x / TILE_SIZE, 0, game_state.room_tiles_w-1);
                int ys = clamp(tr->y / TILE_SIZE, 0, game_state.room_tiles_h-1);
                int xe = clamp((tr->x + tr->disable_animation.width - 1) / TILE_SIZE, 0, game_state.room_tiles_w-1);
                int ye = clamp((tr->y + tr->disable_animation.height - 1) / TILE_SIZE, 0, game_state.room_tiles_h-1);
                for (int y = ys; y <= ye; y++) {
                    for (int x = xs; x <= xe; x++) {
                        game_state.fx_tiles[game_state.room_tiles_w*y + x] |= 0xf0;
                    }
                }
            }
            break;

        case RAVEN_ROOM_TRIGGER_TYPE_REMOVE_ENEMY:
            if (tr->remove_enemy.on_effect_id == effect_id) {
                for (int i = 0; i < game_state.num_enemies; i++) {
                    struct GAME_ENEMY *enemy = &game_state.enemies[i];
                    if (enemy->id == tr->remove_enemy.enemy_id) {
                        memmove(&game_state.enemies[i],
                                &game_state.enemies[i+1],
                                sizeof(struct GAME_ENEMY) * (game_state.num_enemies-(i+1)));
                        game_state.num_enemies--;
                        i--;
                    }
                }
            }
            break;
        }
    }
}

void game_state_spawn_room_enemies(uint16_t spawn_for_flag_id)
{
    const struct RAVEN_ROOM *room = &raven_rooms[game_state.room_id];
    for (int tr_index = 0; tr_index < room->num_triggers; tr_index++) {
        const struct RAVEN_ROOM_TRIGGER_INFO *tr = &room->triggers[tr_index];
        if (tr->type == RAVEN_ROOM_TRIGGER_TYPE_ENEMY_SPAWN && game_state.num_enemies < GAME_STATE_MAX_ENEMIES) {
            int has_req_flag = (tr->enemy_spawn.req_flag_id == 0xffff || game_state_has_flag(tr->enemy_spawn.req_flag_id));
            if ((spawn_for_flag_id == 0xffff && has_req_flag) ||
                (spawn_for_flag_id != 0xffff && spawn_for_flag_id == tr->enemy_spawn.req_flag_id)) {
                int enemy_index = game_state.num_enemies++;
                enemy_init(enemy_index, tr);
            }
        }
    }
}

void game_state_fps_count(void)
{
    static uint32_t last_millis;
    static uint16_t frame_count;

    uint32_t cur_millis = to_ms_since_boot(get_absolute_time());
    if (cur_millis/1000 != last_millis/1000) {
        game_state.fps_count = frame_count;
        frame_count = 1;
    } else {
        frame_count++;
    }
    last_millis = cur_millis;
}

void game_state_blink_led(void)
{
    if (game_state.led.frames-- <= 0) {
        game_state.led.frames = 15;
        game_state.led.state = !game_state.led.state;
        gpio_put(LED_PIN, game_state.led.state);
    }
}

void game_state_update_room_enemies(void)
{
    for (int i = 0; i < game_state.num_enemies; i++) {
        enemy_update(i);
    }
}

static void advance_room_transition(struct JOYSTICK *joy)
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
    if (game_state.room_transition.frame == START_FADE_OUT) {
        // render and capture exiting old room
        RUN_PERF(update_us);
        screen_render(joy);
        screen_save_to_scratch(0);
    } else if (game_state.room_transition.frame == START_FADE_IN) {
        // render and capture entering new room
        load_room(game_state.room_transition.dst_room_id);
        place_player_at_door_transition();
        screen_follow_player();
        RUN_PERF(update_us);
        screen_render(joy);
        screen_save_to_scratch(1);
    } else {
        RUN_PERF(update_us);
        if (game_state.room_transition.frame <= END_FADE_OUT) {
            // fading out
            screen_fade_from_scratch(2 - (game_state.room_transition.frame-START_FADE_OUT) / (FADE_OUT_FRAMES/2));
        } else if (game_state.room_transition.frame <= END_BLACKOUT) {
            // blacking out
            screen_fade_from_scratch(0);
        } else if (game_state.room_transition.frame <= END_FADE_IN) {
            // fading in
            screen_fade_from_scratch(1 + (game_state.room_transition.frame-START_FADE_IN) / (FADE_IN_FRAMES/2));
        }
    }

    if (++game_state.room_transition.frame >= END_TRANSITION) {
        // end door transition (current frame will be the last)
        game_state.room_transition.enabled = 0;
    }
}

static void advance_game_state(struct JOYSTICK *joy)
{
    process_joy_input(joy);
    update_game_state(joy);
    RUN_PERF(update_us);

    screen_render(joy);
}

void game_main_loop(struct JOYSTICK *joy)
{
    game_state_init();
    apply_savegame(&new_savegame);

    while (true) {
        RUN_PERF_START();

        // request joy update
        joy->last = joy->cur;
        joy_wii_i2c_request_state(joy);
        uint32_t joy_req_us = time_us_32();
        RUN_PERF_AT(joy_req_us, joy_req_us);

        // process core messages while waiting for joy update
        process_core_messages();
        game_state_fps_count();
        game_state_blink_led();

        // ensure 200us have elapsed since joy update request
        while (time_us_32() - joy_req_us < 200) sleep_us(10);
        joy_wii_i2c_read_state(joy);
        RUN_PERF(joy_read_us);

        // advance game
        if (game_state.room_transition.enabled) {
            advance_room_transition(joy);
        } else {
            advance_game_state(joy);
        }

        vga_swap_buffers(true);
        RUN_PERF(vsync_us);
    }
}
