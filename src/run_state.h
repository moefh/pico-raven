#ifndef RUN_STATE_H_FILE
#define RUN_STATE_H_FILE

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
    uint32_t dst_room_id;
    uint32_t src_room_id;
    uint16_t dst_door_trigger_id;
    uint16_t src_door_trigger_id;
    int8_t enabled;
};

struct GAME_STATE_MOD {
    uint16_t index;
    uint16_t volume;
    uint16_t event_chan;
    uint16_t event_data;
};

struct GAME_STATE;

struct GAME_RUN_STATE {
    struct GAME_STATE_LED led;
    struct GAME_STATE_PERF perf;
    struct GAME_STATE_DISPLAY display;

    struct GAME_STATE_ROOM_TRANSITION room_transition;
    struct GAME_STATE_MOD mod;

    int32_t room_w;
    int32_t room_h;
    int32_t screen_x;
    int32_t screen_y;
    uint8_t room_doors_enabled;
    void (*update_room)(struct GAME_STATE *);
};

void run_state_init(void);

#define RUN_PERF_START()         (run_state.perf.frame_start_us = time_us_32())
#define RUN_PERF(name)           RUN_PERF_AT(name, time_us_32())
#define RUN_PERF_AT(name, time)  (run_state.perf.name = (time) - run_state.perf.frame_start_us)

extern struct GAME_RUN_STATE run_state;

#endif /* RUN_STATE_H_FILE */
