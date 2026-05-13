#ifndef CORE_MSG_H_FILE
#define CORE_MSG_H_FILE

#include <stdint.h>
#include <stdbool.h>
#include "pico/util/queue.h"

#include "game_data.h"

// message types
enum CORE_MSG_TYPE {
    CORE0_MSG_TYPE_MOD_EVENT,
    CORE1_MSG_TYPE_AUDIO_INIT,
    CORE1_MSG_TYPE_SFX_PLAY,
    CORE1_MSG_TYPE_SFX_STOP,
    CORE1_MSG_TYPE_SFX_SET_VOLUME,
    CORE1_MSG_TYPE_MOD_PLAY,
    CORE1_MSG_TYPE_MOD_STOP,
    CORE1_MSG_TYPE_MOD_SET_VOLUME,
};

struct CORE_MSG_HEADER {
    enum CORE_MSG_TYPE type;
};

struct CORE0_MSG_MOD_EVENT {
    struct CORE_MSG_HEADER msg_header;
    uint8_t chan;
    uint8_t event;
};

struct CORE1_MSG_AUDIO_INIT {
    struct CORE_MSG_HEADER msg_header;
    uint32_t sample_freq;
    uint8_t pio_num;
    uint8_t data_pin;
    uint8_t clock_pin_base;
};

struct CORE1_MSG_SFX_PLAY {
    struct CORE_MSG_HEADER msg_header;
    uint16_t sfx_id;
    uint16_t bits_per_sample;
    const void *samples;
    uint32_t len;
    uint32_t loop_start;
    uint32_t pitch_shift;  // 20.12 fixed point
    uint16_t volume;       // 8.8 fixed point
};

struct CORE1_MSG_SFX_SET_VOLUME {
    struct CORE_MSG_HEADER msg_header;
    uint16_t sfx_id;
    uint16_t volume;  // 8.8 fixed point
};

struct CORE1_MSG_SFX_STOP {
    struct CORE_MSG_HEADER msg_header;
    uint16_t sfx_id;
};

struct CORE1_MSG_MOD_PLAY {
    struct CORE_MSG_HEADER msg_header;
    const struct MOD_DATA *mod;
    uint16_t volume; // 8.8 fixed point
    bool loop;
};

struct CORE1_MSG_MOD_SET_VOLUME {
    struct CORE_MSG_HEADER msg_header;
    uint16_t volume; // 8.8 fixed point
};

struct CORE1_MSG_MOD_STOP {
    struct CORE_MSG_HEADER msg_header;
};

union CORE_MSG {
    struct CORE_MSG_HEADER msg_header;
    struct CORE0_MSG_MOD_EVENT mod_event;
    struct CORE1_MSG_AUDIO_INIT audio_init;
    struct CORE1_MSG_SFX_PLAY sfx_play;
    struct CORE1_MSG_SFX_STOP sfx_stop;
    struct CORE1_MSG_SFX_SET_VOLUME sfx_set_volume;
    struct CORE1_MSG_MOD_PLAY mod_play;
    struct CORE1_MSG_MOD_STOP mod_stop;
    struct CORE1_MSG_MOD_SET_VOLUME mod_set_volume;
};

void core_msg_init(void);
bool core0_msg_try_recv(union CORE_MSG *msg);
bool core1_msg_try_recv(union CORE_MSG *msg);

// =======================================================
// === core0 -> core1:

void msg_audio_init(int pio_num, int data_pin, int clock_pin_base, int sample_freq);

void msg_sfx_play_once(uint16_t sfx_id, const struct RAVEN_SFX *sfx, uint16_t volume, uint32_t pitch_shift);
void msg_sfx_play_loop(uint16_t sfx_id, const struct RAVEN_SFX *sfx, uint16_t volume, uint32_t pitch_shift);
void msg_sfx_stop(uint16_t sfx_id);
void msg_sfx_set_volume(uint16_t sfx_id, uint16_t volume);

void msg_mod_play(const struct MOD_DATA *mod, uint16_t volume, bool loop);
void msg_mod_stop(void);
void msg_mod_set_volume(uint16_t volume);

// =======================================================
// === core1 -> core0:

void msg_mod_event(uint8_t chan, uint8_t event);

#endif /* CORE_MSG_H_FILE */
