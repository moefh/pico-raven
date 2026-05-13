#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "pico/flash.h"

#include "config.h"
#include "core_msg.h"
#include "lib/sound_i2s.h"
#include "lib/audio_mixer.h"
#include "lib/mod_play.h"

static void mod_event_callback(int chan, int event, void *data)
{
    (void) data;
    msg_mod_event(chan, event);
}

static void mod_mixer_callback(int16_t *samples, uint32_t len)
{
    if (mod_play_step(samples, len) != 0) {
        audio_mixer_set_callback(NULL, 0);
    }
}

static void process_message(union CORE_MSG *msg)
{
    //printf("processing message of type %d\n", msg->msg_header.type);
    switch (msg->msg_header.type) {
    case CORE1_MSG_TYPE_AUDIO_INIT:
        {
            struct CORE1_MSG_AUDIO_INIT *m = &msg->audio_init;
            struct sound_i2s_config cfg = {
                .pio_num = m->pio_num,
                .pin_data = m->data_pin,
                .pin_clock_base = m->clock_pin_base,
                .bits_per_sample = 16,
                .sample_rate = m->sample_freq,
            };
            sound_i2s_init(&cfg);
            sound_i2s_playback_start();
            audio_mixer_init();
            mod_play_init(m->sample_freq);
            mod_play_set_event_callback(mod_event_callback, NULL);
        }
        return;

    case CORE1_MSG_TYPE_SFX_PLAY:
        {
            struct CORE1_MSG_SFX_PLAY *m = &msg->sfx_play;
            int pitch_shift = (m->pitch_shift == 0) ? (1<<12) : m->pitch_shift;
            audio_mixer_play_sfx_loop(m->sfx_id, m->bits_per_sample, m->samples, m->len, m->loop_start, m->volume, pitch_shift);
        }
        return;

    case CORE1_MSG_TYPE_SFX_SET_VOLUME:
        {
            struct CORE1_MSG_SFX_SET_VOLUME *m = &msg->sfx_set_volume;
            audio_mixer_set_sfx_volume(m->sfx_id, m->volume);
        }
        return;

    case CORE1_MSG_TYPE_SFX_STOP:
        {
            struct CORE1_MSG_SFX_STOP *m = &msg->sfx_stop;
            audio_mixer_stop_sfx(m->sfx_id);
        }
        return;

    case CORE1_MSG_TYPE_MOD_PLAY:
        {
            struct CORE1_MSG_MOD_PLAY *m = &msg->mod_play;
            mod_play_start(m->mod, m->loop);
            audio_mixer_set_callback(mod_mixer_callback, m->volume);
        }
        return;

    case CORE1_MSG_TYPE_MOD_STOP:
        {
            audio_mixer_set_callback(NULL, 0);
        }
        return;

    case CORE1_MSG_TYPE_MOD_SET_VOLUME:
        {
            struct CORE1_MSG_MOD_SET_VOLUME *m = &msg->mod_set_volume;
            audio_mixer_set_callback_volume(m->volume);
        }
        return;

    default:
        printf("ERROR processing message: invalid messaged type %d\n", msg->msg_header.type);
        return;
    }
}

void core1_main(void)
{
    // handshake with core0: notify we're ready and wait until core0 is ready:
    multicore_fifo_push_blocking(0);
    multicore_fifo_pop_blocking();

    // required because we'll write to flash on core0:
    flash_safe_execute_core_init();

    while (true) {
        // it takes (SOUND_I2S_BUFFER_NUM_SAMPLES / SND_SAMPLE_FREQUENCY)
        // seconds to play one buffer full of samples; to be safe we sleep
        // for a third of that before checking if the next buffer is ready to
        // prepare
        sleep_ms(1000 * SOUND_I2S_BUFFER_NUM_SAMPLES / SND_SAMPLE_FREQUENCY / 3);

        union CORE_MSG msg;
        while (core1_msg_try_recv(&msg)) {
            process_message(&msg);
        }

        audio_mixer_step();
    }
}
