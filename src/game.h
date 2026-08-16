#ifndef GAME_H_FILE
#define GAME_H_FILE

#include <stdint.h>
#include "game_data.h"
#include "sprite_shadow.h"

enum PLAYER_STATE {
    PLAYER_STATE_STAND,
    PLAYER_STATE_WALK,
    PLAYER_STATE_JUMP,
    PLAYER_STATE_FALL,
};

struct RAVEN_PLAYER_CONTROL {
    int32_t dx;  // 24.8 fixpoint
    int32_t dy;
};

struct RAVEN_PLAYER {
    const struct VGA_IMAGE *sprite;
    const struct RAVEN_SPRITE_ANIMATION *anim;
    int32_t x;
    int32_t y;
    uint8_t state;
    uint8_t direction;
    uint8_t shadow_enabled;
    uint8_t anim_loop;
    uint16_t anim_frame;  // 10.6 fixpoint

    int16_t sprite_x;
    int16_t sprite_y;
    uint8_t sprite_frame;
};

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

struct GAME_STATE_ROOM_TRANSITION {
    uint32_t dst_room_id;
    uint32_t src_room_id;
    uint16_t dst_door_trigger_id;
    uint16_t src_door_trigger_id;
    int8_t enabled;
};

struct GAME_STATE {
    struct GAME_STATE_MOD mod;
    struct GAME_STATE_LED led;
    struct GAME_STATE_PERF perf;
    struct GAME_STATE_DISPLAY display;

    struct RAVEN_PLAYER player;
    struct RAVEN_PLAYER_CONTROL player_control;

    struct GAME_STATE_ROOM_TRANSITION room_transition;

    int room_id;
    int room_w;
    int room_h;
    int screen_x;
    int screen_y;

    uint8_t room_doors_enabled;
    void (*update_room)(struct GAME_STATE *);
};

struct JOYSTICK;

void game_main_loop(struct GAME_STATE *game, struct JOYSTICK *joy);
int game_check_player_trigger(struct GAME_STATE *game, uint32_t trigger_type_flags);

#define GAME_PERF_START(game)           (game)->perf.frame_start_us = time_us_32()
#define GAME_PERF(game, name)           GAME_PERF_AT((game), name, time_us_32())
#define GAME_PERF_AT(game, name, time)  (game)->perf.name = (time) - (game)->perf.frame_start_us

#endif /* GAME_H_FILE */
