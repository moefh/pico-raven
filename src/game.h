#ifndef GAME_H_FILE
#define GAME_H_FILE

#include <stdint.h>
#include "game_data.h"
#include "sprite_shadow.h"

struct JOYSTICK;

#define GAME_STATE_MAX_ENEMIES      32
#define GAME_STATE_MAX_ROOM_PICKUPS 32
#define SCREEN_HEADER_HEIGHT 32

#define DX_ACCEL          ((int32_t) 0x100)
#define DX_FLOOR_FRICTION ((int32_t) 0x0c0)
#define DX_AIR_FRICTION   ((int32_t) 0x010)
#define DX_MAX            ((int32_t) 0x700)

#define DY_GRAVITY    ((int32_t) 0x0c0)
#define DY_MAX        ((int32_t) 0x900)
#define DY_JUMP_START ((int32_t)-0xa00)
#define DY_JUMP_HOLD  ((int32_t)-0x060)

enum PLAYER_STATE {
    PLAYER_STATE_STAND,
    PLAYER_STATE_WALK,
    PLAYER_STATE_JUMP,
    PLAYER_STATE_FALL,
    PLAYER_STATE_FALL2,
    PLAYER_STATE_CROUCH,
};

struct GAME_PLAYER_CONTROL {
    int32_t dx;  // 24.8 fixpoint
    int32_t dy;
};

struct GAME_CHARACTER {
    const struct VGA_IMAGE *sprite;
    const struct RAVEN_SPRITE_ANIMATION *anim;
    int32_t x;
    int32_t y;
    uint8_t state;
    uint8_t direction;
    uint8_t shadow_enabled;
    uint8_t anim_loop;
    uint16_t anim_frame;  // 8.8 fixpoint

    int16_t sprite_x;
    int16_t sprite_y;
    uint8_t sprite_frame;
};

struct GAME_ENEMY_CONTROL {
    int8_t enemy_type;
    int16_t wait;
    int16_t dx;  // 8.8 fixpoint
    int16_t dy;
};

struct GAME_ENEMY {
    const struct VGA_IMAGE *sprite;
    const struct RAVEN_SPRITE_ANIMATION *anim;
    int32_t x;
    int32_t y;
    uint8_t state;
    uint8_t direction;
    uint8_t shadow_enabled;
    uint8_t anim_loop;
    uint16_t anim_frame;  // 8.8 fixpoint
    uint16_t id;

    int16_t sprite_x;
    int16_t sprite_y;
    uint8_t sprite_frame;
};

struct GAME_STATE_LED {
    uint8_t frames;
    uint8_t state;
};

struct GAME_STATE_PERF {
    uint32_t frame_start_us;
    uint32_t joy_req_us;
    uint32_t joy_read_us;
    uint32_t update_us;
    uint32_t room_fg_us;
    uint32_t player_us;
    uint32_t enemies_us;
    uint32_t room_bg_us;
    uint32_t render_us;
    uint32_t vsync_us;
};

struct GAME_STATE_DISPLAY {
    uint16_t msg_mod_event_frames_left;
    uint16_t msg_save_frames_left;
    uint16_t msg_load_frames_left;
    uint8_t save_success;
    uint8_t load_success;
    uint8_t show_perf;
};

struct GAME_STATE_ROOM_TRANSITION {
    uint16_t enabled;
    uint16_t frame;
    uint32_t dst_room_id;
    uint32_t src_room_id;
    uint16_t dst_door_trigger_id;
    uint16_t src_door_trigger_id;
    int8_t player_x;
    int8_t player_y;
    uint8_t player_state;
    uint8_t player_direction;
    int32_t player_dx;
    int32_t player_dy;
};

struct GAME_STATE_MOD {
    uint16_t index;
    uint16_t volume;
    uint16_t event_chan;
    uint16_t event_data;
};

struct PICKUP_RESPAWN {
    uint16_t trigger_id;
    uint16_t delay;  // 0 = still in room, 0xffff = no respawn, other = ticking down to respawn
};

struct GAME_RUN_STATE {
    struct GAME_STATE_LED led;
    struct GAME_STATE_PERF perf;
    struct GAME_STATE_DISPLAY display;

    struct GAME_STATE_ROOM_TRANSITION room_transition;
    struct GAME_STATE_MOD mod;

    int fps_count;
    unsigned int anim_step;

    void (*update_room)(void);

    int32_t room_id;
    int32_t room_w;
    int32_t room_h;
    int32_t screen_x;
    int32_t screen_y;

    bool has_bb;
    int32_t room_tiles_w;
    int32_t room_tiles_h;
    uint8_t *fg_tiles;
    uint8_t *bg_tiles;
    uint8_t *fx_tiles;
    uint8_t *bb_tiles;
    uint8_t tiles[0x10000];

    struct GAME_CHARACTER player;
    struct GAME_PLAYER_CONTROL player_control;

    int32_t num_enemies;
    struct GAME_ENEMY enemies[GAME_STATE_MAX_ENEMIES];
    struct GAME_ENEMY_CONTROL enemies_control[GAME_STATE_MAX_ENEMIES];

    int num_pickup_respawns;
    struct PICKUP_RESPAWN pickup_respawn[GAME_STATE_MAX_ROOM_PICKUPS];

    uint32_t upgrades;
    uint32_t collectables[8];
    uint32_t flags[8];
};

extern struct GAME_RUN_STATE game_state;

void game_state_fps_count(void);
void game_state_blink_led(void);
void game_state_spawn_room_enemies(uint16_t req_flag_id);
void game_state_update_room_enemies(void);
void game_state_activate_effect(uint32_t effect_id);
const struct RAVEN_ROOM_TRIGGER_INFO *game_state_check_player_trigger(uint32_t trigger_type_flags);
void game_main_loop(struct JOYSTICK *joy);

#define RUN_PERF_START()         (game_state.perf.frame_start_us = time_us_32())
#define RUN_PERF(name)           RUN_PERF_AT(name, time_us_32())
#define RUN_PERF_AT(name, time)  (game_state.perf.name = (time) - game_state.perf.frame_start_us)

static inline int game_state_is_pickup_trigger_enabled(uint16_t trigger_id) {
    for (int i = 0; i < game_state.num_pickup_respawns; i++) {
        struct PICKUP_RESPAWN *respawn = &game_state.pickup_respawn[i];
        if (respawn->trigger_id == trigger_id) {
            return respawn->delay == 0;
        }
    }
    return 0;
}

static inline void game_state_disable_pickup_trigger(uint16_t trigger_id, uint16_t delay) {
    for (int i = 0; i < game_state.num_pickup_respawns; i++) {
        struct PICKUP_RESPAWN *respawn = &game_state.pickup_respawn[i];
        if (respawn->trigger_id == trigger_id) {
            respawn->delay = (delay == 0) ? 0xffff : delay;
            return;
        }
    }
}

static inline void game_state_advance_pickup_trigger_respawns() {
    for (int i = 0; i < game_state.num_pickup_respawns; i++) {
        struct PICKUP_RESPAWN *respawn = &game_state.pickup_respawn[i];
        if (respawn->delay != 0 && respawn->delay != 0xffff) {
            respawn->delay--;
        }
    }
}

static inline bool game_state_has_upgrade(uint32_t upgrade_id) {
    return (game_state.upgrades & (1<<upgrade_id)) != 0;
}
static inline bool game_state_has_collectable(uint32_t collectable_id) {
    uint32_t index = collectable_id / 32;
    uint32_t bit = collectable_id % 32;
    if (index < sizeof(game_state.collectables)/sizeof(game_state.collectables[0])) {
        return (game_state.collectables[index] & (1<<bit)) != 0;
    }
    return false;
}
static inline bool game_state_has_flag(uint32_t flag_id) {
    uint32_t index = flag_id / 32;
    uint32_t bit = flag_id % 32;
    if (index < sizeof(game_state.flags)/sizeof(game_state.flags[0])) {
        return (game_state.flags[index] & (1<<bit)) != 0;
    }
    return false;
}

static inline void game_state_set_upgrade(uint32_t upgrade_id) {
    game_state.upgrades |= 1u << upgrade_id;
}

static inline void game_state_set_collectable(uint32_t collectable_id) {
    uint32_t index = collectable_id / 32;
    uint32_t bit = collectable_id % 32;
    if (index < sizeof(game_state.collectables)/sizeof(game_state.collectables[0])) {
        game_state.collectables[index] |= 1u << bit;
    }
}

static inline void game_state_set_flag(uint32_t flag_id) {
    uint32_t index = flag_id / 32;
    uint32_t bit = flag_id % 32;
    if (index < sizeof(game_state.flags)/sizeof(game_state.flags[0])) {
        game_state.flags[index] |= 1u << bit;
    }
}

#endif /* GAME_H_FILE */
