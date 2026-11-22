#ifndef GAME_H_FILE
#define GAME_H_FILE

#include <stdint.h>
#include <stdbool.h>
#include "game_data.h"
#include "sprite_shadow.h"
#include "enemy.h"
#include "bullet.h"
#include "effect.h"
#include "darkness.h"

#define GAME_STATE_MAX_EFFECTS      32
#define GAME_STATE_MAX_BULLETS      32
#define GAME_STATE_MAX_ENEMIES      32
#define GAME_STATE_MAX_ROOM_PICKUPS 32

#define SCREEN_HEADER_HEIGHT 32

#define DX_ACCEL          ((int32_t) 0x0e0)
#define DX_FLOOR_FRICTION ((int32_t) 0x0a0)
#define DX_AIR_FRICTION   ((int32_t) 0x010)
#define DX_MAX            ((int32_t) 0x500)

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
    PLAYER_NUM_STATES,
};

struct GAME_PLAYER_CONTROL {
    int32_t dx;  // 24.8 fixpoint
    int32_t dy;
};

struct GAME_ANIMATION {
    const struct RAVEN_SPRITE_ANIMATION *anim;
    const struct VGA_IMAGE *sprite;
    uint16_t loop;
    uint16_t frame;  // 8.8 fixpoint
    int16_t sprite_x;
    int16_t sprite_y;
    uint8_t sprite_frame;
};

struct GAME_PLAYER {
    const struct RAVEN_PLAYER_CHAR *player_char;
    uint16_t suit_index;
    int32_t x;
    int32_t y;
    uint8_t state;
    uint8_t direction;
    uint8_t weapon_frames_left;
    uint8_t shoot_wait_frames;
    uint8_t weapon_index;
    uint8_t shoot_now;
    uint8_t shadow_enabled;
    uint8_t darkness_enabled;
    uint32_t last_shot_frame;
    struct GAME_ANIMATION char_anim;
    struct GAME_ANIMATION weapon_anim;
};

struct GAME_ENEMY_CONTROL {
    int8_t enemy_type;
    int16_t wait;
    int16_t dx;  // 8.8 fixpoint
    int16_t dy;
};

struct GAME_ENEMY {
    int32_t x;
    int32_t y;
    uint8_t state;
    uint8_t direction;
    uint8_t shadow_enabled;
    uint8_t hit_frames_left;
    uint16_t health;
    uint16_t id;
    struct GAME_ANIMATION anim;
};

struct GAME_BULLET {
    int32_t x;   // 24.8 fixpoint
    int32_t y;   // 24.8 fixpoint
    int16_t dx;  // 8.8 fixpoint
    int16_t dy;  // 8.8 fixpoint
    uint8_t direction;
    uint8_t weapon_index;
    struct GAME_ANIMATION anim;
};

struct GAME_EFFECT {
    int32_t x;
    int32_t y;
    uint8_t direction;
    uint16_t set_effect_id;
    uint16_t set_flag_id;
    struct GAME_ANIMATION anim;
};

struct GAME_STATE_LED {
    uint8_t frames;
    uint8_t state;
};

struct GAME_STATE_PERF {
    uint32_t frame_start_us;
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

struct GAME_PICKUP_RESPAWN {
    uint16_t trigger_id;
    uint16_t delay;  // 0 = still in room, 0xffff = no respawn, other = ticking down to respawn
};

struct GAME_PROMPT {
    int16_t x;
    int16_t y;
    const char *text;
};

struct GAME_RUN_STATE {
    struct GAME_STATE_LED led;
    struct GAME_STATE_PERF perf;
    struct GAME_STATE_DISPLAY display;

    struct GAME_STATE_ROOM_TRANSITION room_transition;
    struct GAME_STATE_MOD mod;

    int fps_count;
    unsigned int frame_counter;
    uint32_t joy_cur_state;
    uint32_t joy_last_state;

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

    uint32_t enemy_collision_bits[GAME_STATE_MAX_ENEMIES];
    struct GAME_PROMPT prompt;
    struct DARKNESS darkness[3];

    struct GAME_PLAYER player;
    struct GAME_PLAYER_CONTROL player_control;

    int32_t num_enemies;
    struct GAME_ENEMY enemies[GAME_STATE_MAX_ENEMIES];
    struct GAME_ENEMY_CONTROL enemies_control[GAME_STATE_MAX_ENEMIES];

    int32_t num_bullets;
    struct GAME_BULLET bullets[GAME_STATE_MAX_BULLETS];

    int32_t num_effects;
    struct GAME_EFFECT effects[GAME_STATE_MAX_EFFECTS];

    int num_pickup_respawns;
    struct GAME_PICKUP_RESPAWN pickup_respawn[GAME_STATE_MAX_ROOM_PICKUPS];

    uint32_t upgrade_bits;
    uint32_t collectable_bits[8];
    uint32_t flag_bits[8];
    uint32_t effect_bits[8];
};

extern struct GAME_RUN_STATE game_state;

#define RUN_PERF_START()         (game_state.perf.frame_start_us = time_us_32())
#define RUN_PERF(name)           RUN_PERF_AT(name, time_us_32())
#define RUN_PERF_AT(name, time)  (game_state.perf.name = (time) - game_state.perf.frame_start_us)

static inline const struct RAVEN_TILE_ANIMATION *get_tile_animation(int index) {
    if (index == 0xffff) { return NULL; }
    return &raven_tile_animations[index];
}

static inline const struct RAVEN_SPRITE_ANIMATION *get_sprite_animation(int index) {
    if (index == 0xffff) { return NULL; }
    return &raven_sprite_animations[index];
}

static inline const struct RAVEN_SFX *get_sfx(int index) {
    if (index == 0xffff) { return NULL; }
    return &raven_sfxs[index];
}

static inline int game_state_is_pickup_trigger_enabled(uint16_t trigger_id) {
    for (int i = 0; i < game_state.num_pickup_respawns; i++) {
        struct GAME_PICKUP_RESPAWN *respawn = &game_state.pickup_respawn[i];
        if (respawn->trigger_id == trigger_id) {
            return respawn->delay == 0;
        }
    }
    return 0;
}

static inline void game_state_disable_pickup_trigger(uint16_t trigger_id, uint16_t delay) {
    for (int i = 0; i < game_state.num_pickup_respawns; i++) {
        struct GAME_PICKUP_RESPAWN *respawn = &game_state.pickup_respawn[i];
        if (respawn->trigger_id == trigger_id) {
            respawn->delay = (delay == 0) ? 0xffff : delay;
            return;
        }
    }
}

static inline void game_state_advance_pickup_trigger_respawns() {
    for (int i = 0; i < game_state.num_pickup_respawns; i++) {
        struct GAME_PICKUP_RESPAWN *respawn = &game_state.pickup_respawn[i];
        if (respawn->delay != 0 && respawn->delay != 0xffff) {
            respawn->delay--;
        }
    }
}

static inline bool game_state_has_upgrade(uint32_t upgrade_id) {
    return upgrade_id < 32 && (game_state.upgrade_bits & (1<<upgrade_id)) != 0;
}

static inline bool game_state_has_collectable(uint32_t collectable_id) {
    uint32_t index = collectable_id / 32;
    uint32_t bit = collectable_id % 32;
    if (index < sizeof(game_state.collectable_bits)/sizeof(game_state.collectable_bits[0])) {
        return (game_state.collectable_bits[index] & (1<<bit)) != 0;
    }
    return false;
}

static inline bool game_state_has_flag(uint32_t flag_id) {
    uint32_t index = flag_id / 32;
    uint32_t bit = flag_id % 32;
    if (index < sizeof(game_state.flag_bits)/sizeof(game_state.flag_bits[0])) {
        return (game_state.flag_bits[index] & (1<<bit)) != 0;
    }
    return false;
}

static inline bool game_state_has_effect(uint32_t effect_id) {
    uint32_t index = effect_id / 32;
    uint32_t bit = effect_id % 32;
    if (index < sizeof(game_state.effect_bits)/sizeof(game_state.effect_bits[0])) {
        return (game_state.effect_bits[index] & (1<<bit)) != 0;
    }
    return false;
}

static inline void game_state_set_upgrade(uint32_t upgrade_id) {
    if (upgrade_id < 32) {
        game_state.upgrade_bits |= 1u << upgrade_id;
    }
}

static inline void game_state_set_collectable(uint32_t collectable_id) {
    uint32_t index = collectable_id / 32;
    uint32_t bit = collectable_id % 32;
    if (index < sizeof(game_state.collectable_bits)/sizeof(game_state.collectable_bits[0])) {
        game_state.collectable_bits[index] |= 1u << bit;
    }
}

static inline void game_state_set_flag(uint32_t flag_id) {
    uint32_t index = flag_id / 32;
    uint32_t bit = flag_id % 32;
    if (index < sizeof(game_state.flag_bits)/sizeof(game_state.flag_bits[0])) {
        game_state.flag_bits[index] |= 1u << bit;
    }
}

static inline void game_state_set_effect(uint32_t effect_id) {
    uint32_t index = effect_id / 32;
    uint32_t bit = effect_id % 32;
    if (index < sizeof(game_state.effect_bits)/sizeof(game_state.effect_bits[0])) {
        game_state.effect_bits[index] |= 1u << bit;
    }
}

static inline void game_state_update_enemies(void) { enemy_update_all(); }
static inline void game_state_update_bullets(void) { bullet_update_all(); }
static inline void game_state_update_effects(void) { effect_update_all(); }

void game_state_fps_count(void);
void game_state_blink_led(void);
void game_state_spawn_room_enemies(uint16_t req_flag_id);
void game_update_animation(struct GAME_ANIMATION *anim, int x, int y, int direction);
void game_state_activate_effect(uint32_t effect_id);
const struct RAVEN_ROOM_TRIGGER_INFO *game_state_check_player_trigger(uint32_t trigger_type_flags);
void game_main_loop(void);

#endif /* GAME_H_FILE */
