#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"

#include "run_state.h"
#include "game.h"
#include "config.h"
#include "enemy.h"

struct GAME_RUN_STATE run_state;

void run_state_init(void)
{
    memset(&run_state, 0, sizeof(struct GAME_RUN_STATE));

    run_state.mod.index = RAVEN_MOD_ID_BWV_106;
    run_state.mod.volume = 0x40;
}

void run_state_fps_count(void)
{
    static uint32_t last_millis;
    static uint16_t frame_count;

    uint32_t cur_millis = to_ms_since_boot(get_absolute_time());
    if (cur_millis/1000 != last_millis/1000) {
        run_state.fps_count = frame_count;
        frame_count = 1;
    } else {
        frame_count++;
    }
    last_millis = cur_millis;
}

void run_state_blink_led(void)
{
    if (run_state.led.frames-- <= 0) {
        run_state.led.frames = 15;
        run_state.led.state = !run_state.led.state;
        gpio_put(LED_PIN, run_state.led.state);
    }
}

void run_state_spawn_room_enemies(struct GAME_STATE *game, uint16_t spawn_for_flag_id)
{
    const struct RAVEN_ROOM *room = &raven_rooms[game->room_id];
    for (int tr_index = 0; tr_index < room->num_triggers; tr_index++) {
        const struct RAVEN_ROOM_TRIGGER_INFO *tr = &room->triggers[tr_index];
        if (tr->type == RAVEN_ROOM_TRIGGER_TYPE_ENEMY_SPAWN && run_state.num_enemies < RUN_STATE_MAX_ENEMIES) {
            int has_req_flag = (tr->enemy_spawn.req_flag_id == 0xffff || game_has_flag(game, tr->enemy_spawn.req_flag_id));
            if ((spawn_for_flag_id == 0xffff && has_req_flag) ||
                (spawn_for_flag_id != 0xffff && spawn_for_flag_id == tr->enemy_spawn.req_flag_id)) {
                int enemy_index = run_state.num_enemies++;
                enemy_init(enemy_index, tr);
            }
        }
    }
}

void run_state_update_room_enemies(struct GAME_STATE *game)
{
    for (int i = 0; i < run_state.num_enemies; i++) {
        enemy_update(game, i);
    }
}
