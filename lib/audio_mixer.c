#include <string.h>
#include <stdio.h>

#include "audio_mixer.h"
#include "sound_i2s.h"

#define SPL_TYPE_8BIT  0
#define SPL_TYPE_16BIT 1

struct DATA_SOURCE {
    uint16_t id;
    uint16_t spl_type;    // SPL_TYPE_xxx
    union {
        const void *spl;
        const int8_t *spl8;
        const int16_t *spl16;
    };
    uint32_t pos;         // 20.12 fixed point
    uint32_t len;         // 20.12 fixed point
    uint32_t loop_start;  // 20.12 fixed point
    uint32_t spl_advance; // 20.12 fixed point (1.0 = 1<<12 = original pitch)
    int16_t volume;       // 8.8 fixed point (1.0 = 1<<8 = 100%)
};

static int16_t *prev_sound_buffer;

static struct DATA_SOURCE data_sources[AUDIO_MIXER_MAX_SOURCES];

static audio_callback callback_source;
static int16_t callback_volume;
static int16_t callback_buffer[SOUND_I2S_BUFFER_NUM_SAMPLES];

static int find_unused_source(void)
{
    for (int i = 0; i < AUDIO_MIXER_MAX_SOURCES; i++) {
        if (! data_sources[i].spl) {
            return i;
        }
    }
    return -1;
}

static int find_source_by_id(uint16_t id)
{
    for (int i = 0; i < AUDIO_MIXER_MAX_SOURCES; i++) {
        if (data_sources[i].id == id) {
            return i;
        }
    }
    return -1;
}

static int add_source(uint16_t id, uint16_t spl_type, const void *spl, uint32_t len, uint32_t loop_start, uint16_t volume, uint32_t pitch_shift)
{
    int src_index = find_unused_source();
    if (src_index < 0) {
        return -1;
    }
    struct DATA_SOURCE *source = &data_sources[src_index];

    source->id = id;
    source->spl_type = spl_type;
    switch (spl_type) {
    case SPL_TYPE_8BIT:  source->spl8 = spl; break;
    case SPL_TYPE_16BIT: source->spl16 = spl; break;
    default: source->spl = NULL; break;
    }
    source->spl_advance = pitch_shift;
    source->volume = volume;
    
    source->pos = 0;
    source->loop_start = loop_start << 12;
    source->len = len << 12;
    
    return 0;
}

int audio_mixer_play_sfx_once(uint16_t id, uint16_t bits_per_sample, const void *samples, uint32_t len,
                              uint16_t volume, uint32_t pitch_shift)
{
    int16_t spl_type = (bits_per_sample == 8) ? SPL_TYPE_8BIT : SPL_TYPE_16BIT;
    return add_source(id, spl_type, samples, len, len, volume, pitch_shift);
}

int audio_mixer_play_sfx_loop(uint16_t id, uint16_t bits_per_sample, const void *samples, uint32_t len,
                              uint32_t loop_start, uint16_t volume, uint32_t pitch_shift)
{
    int16_t spl_type = (bits_per_sample == 8) ? SPL_TYPE_8BIT : SPL_TYPE_16BIT;
    return add_source(id, spl_type, samples, len, loop_start, volume, pitch_shift);
}

void audio_mixer_stop_sfx(uint16_t id)
{
    int src_index = find_source_by_id(id);
    if (src_index >= 0) {
        data_sources[src_index].spl = NULL;
    }
}

void audio_mixer_set_sfx_volume(uint16_t id, int16_t volume)
{
    int src_index = find_source_by_id(id);
    if (src_index >= 0) {
        data_sources[src_index].volume = volume;
    }
}

bool audio_mixer_check_sfx_playing(uint16_t id)
{
    int src_index = find_source_by_id(id);
    if (src_index >= 0) {
        return data_sources[src_index].spl != NULL;
    }
    return false;
}

void audio_mixer_set_callback(audio_callback func, int16_t volume)
{
    callback_source = func;
    callback_volume = volume;
    if (! callback_source) {
        memset(callback_buffer, 0, sizeof(callback_buffer));
    }
}

void audio_mixer_set_callback_volume(int16_t volume)
{
    callback_volume = volume;
}

static int16_t clamp(int32_t sample)
{
    if (sample < -32768) return -32768;
    if (sample >  32767) return  32767;
    return sample;
}

void audio_mixer_init(void)
{
    audio_mixer_set_callback(NULL, 0);
    prev_sound_buffer = NULL;
}

void audio_mixer_step(void)
{
    int16_t *sound_buffer = sound_i2s_get_next_buffer();
    if (sound_buffer == prev_sound_buffer) return;
    prev_sound_buffer = sound_buffer;

    // run callback source if we have one
    int16_t callback_volume_local = 0;
    if (callback_source) {
        callback_source(callback_buffer, SOUND_I2S_BUFFER_NUM_SAMPLES);
        callback_volume_local = callback_volume;
    }

    for (int i = 0; i < SOUND_I2S_BUFFER_NUM_SAMPLES; i++) {
        int32_t sample = 0;

        // mix sample sources
        for (int s = 0; s < AUDIO_MIXER_MAX_SOURCES; s++) {
            struct DATA_SOURCE *source = &data_sources[s];
            if (! source->spl) continue;

            switch (source->spl_type) {
            case SPL_TYPE_8BIT:  sample += source->spl8[source->pos>>12] * source->volume; break;
            case SPL_TYPE_16BIT: sample += (source->spl16[source->pos>>12] * source->volume) >> 8; break;
            }
            source->pos += source->spl_advance;
            if (source->pos >= source->len) {
                if (source->loop_start < source->len) {
                    source->pos -= source->len - source->loop_start;
                } else {
                    source->spl = NULL;
                }
            }
        }

        // mix callback source
        if (callback_source) {
            sample += (callback_buffer[i] * callback_volume_local) >> 8;
        }
    
        // clamp result
        sample = clamp(sample);
        *sound_buffer++ = sample;
        *sound_buffer++ = sample;
    }
}
