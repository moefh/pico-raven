#ifndef RUN_STATE_H_FILE
#define RUN_STATE_H_FILE

#define RUN_STATE_MAX_ENEMIES      32
#define RUN_STATE_MAX_ROOM_PICKUPS 32

struct VGA_IMAGE;
struct RAVEM_SPRITE_ANIMATION;

struct GAME_ENEMY_CONTROL {
    int8_t enemy_type;
    int16_t wait;
    int16_t dx;  // 8.8 fixpoint
    int16_t dy;
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

struct GAME_STATE;

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

    void (*update_room)(struct GAME_STATE *);

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

    int32_t num_enemies;
    struct GAME_ENEMY enemies[RUN_STATE_MAX_ENEMIES];
    struct GAME_ENEMY_CONTROL enemies_control[RUN_STATE_MAX_ENEMIES];

    int num_pickup_respawns;
    struct PICKUP_RESPAWN pickup_respawn[RUN_STATE_MAX_ROOM_PICKUPS];
};

void run_state_init(void);
void run_state_fps_count(void);
void run_state_blink_led(void);
void run_state_spawn_room_enemies(struct GAME_STATE *game, uint16_t req_flag_id);
void run_state_update_room_enemies(struct GAME_STATE *game);

#define RUN_PERF_START()         (run_state.perf.frame_start_us = time_us_32())
#define RUN_PERF(name)           RUN_PERF_AT(name, time_us_32())
#define RUN_PERF_AT(name, time)  (run_state.perf.name = (time) - run_state.perf.frame_start_us)

extern struct GAME_RUN_STATE run_state;

static inline int run_state_is_pickup_trigger_enabled(uint16_t trigger_id) {
    for (int i = 0; i < run_state.num_pickup_respawns; i++) {
        struct PICKUP_RESPAWN *respawn = &run_state.pickup_respawn[i];
        if (respawn->trigger_id == trigger_id) {
            return respawn->delay == 0;
        }
    }
    return 0;
}

static inline void run_state_disable_pickup_trigger(uint16_t trigger_id, uint16_t delay) {
    for (int i = 0; i < run_state.num_pickup_respawns; i++) {
        struct PICKUP_RESPAWN *respawn = &run_state.pickup_respawn[i];
        if (respawn->trigger_id == trigger_id) {
            respawn->delay = (delay == 0) ? 0xffff : delay;
            return;
        }
    }
}

static inline void run_state_advance_pickup_trigger_respawns() {
    for (int i = 0; i < run_state.num_pickup_respawns; i++) {
        struct PICKUP_RESPAWN *respawn = &run_state.pickup_respawn[i];
        if (respawn->delay != 0 && respawn->delay != 0xffff) {
            respawn->delay--;
        }
    }
}

#endif /* RUN_STATE_H_FILE */
