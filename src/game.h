#ifndef GAME_H_FILE
#define GAME_H_FILE

#include "player.h"

struct GAME_STATE_LED {
    uint8_t frames;
    uint8_t state;
};

struct GAME_STATE_MOD {
    uint16_t index;
    uint16_t volume;
    uint16_t event_chan;
    uint16_t event_data;
};

struct GAME_STATE_PERF {
    uint32_t frame_start_us;
    uint32_t joy_req_us;
    uint32_t joy_read_us;
    uint32_t update_us;
    uint32_t room_fg_us;
    uint32_t player_us;
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

struct GAME_STATE {
    struct GAME_STATE_MOD mod;
    struct GAME_STATE_LED led;
    struct GAME_STATE_PERF perf;
    struct GAME_STATE_DISPLAY display;

    struct RAVEN_PLAYER player;
    struct RAVEN_PLAYER_CONTROL player_control;

    const struct RAVEN_ROOM *room;
    int room_w;
    int room_h;
    int screen_x;
    int screen_y;
};

void game_main_loop(void);

#define GAME_PERF_START(game)           (game)->perf.frame_start_us = time_us_32()
#define GAME_PERF(game, name)           GAME_PERF_AT((game), name, time_us_32())
#define GAME_PERF_AT(game, name, time)  (game)->perf.name = (time) - (game)->perf.frame_start_us

#endif /* GAME_H_FILE */
