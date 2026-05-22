#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "mod_play.h"
#include "mod_period_table.h"

//#define ENABLE_ALERT
//#define ENABLE_DEBUG
//#define ENABLE_INFOX

#ifdef ENABLE_ALERT
#define ALERT printf
#else
#define ALERT(...)
#endif

#ifdef ENABLE_DEBUG
#define DEBUG printf
#else
#define DEBUG(...)
#endif

#ifdef ENABLE_INFOX
#define INFOX printf
#else
#define INFOX(...)
#endif

#define MOD_REF_CLOCK           7093789  // 7159090 for NTSC
#define MOD_MAX_CHANNELS        4
#define MOD_NUM_ROWS            64
#define MOD_MAX_VOLUME          256      // we use 0-256 instead of 0-64 for simplicity (all data from MOD is shifted << 2)

#define MOD_DEFAULT_TICKS_PER_ROW 6
#define MOD_DEFAULT_BPM           125
#define MOD_DEFAULT_NOTE_PERIOD   428

// channel
struct PLAY_CHANNEL {
    const struct MOD_SAMPLE *sample;
    uint32_t sample_pos;
    uint32_t sample_len;
    uint32_t sample_advance;
    int32_t  volume;
    uint32_t period;
    uint32_t effect;
    uint32_t playing;
};

struct PLAY_STATE {
    const struct MOD_DATA *mod;
    struct PLAY_CHANNEL channels[MOD_MAX_CHANNELS];

    uint32_t out_frequency;           // output frequency (samples per second)
    uint32_t sample_advance_factor;   // used to calculate sample playing frequency

    uint8_t loop_enabled;             // 1 if mod should be looped
    uint8_t jump_enabled;             // 1 to jump before starting the next row
    uint8_t jump_to_song_pos;         // jump song_pos destination (if jump_enabled)
    uint8_t jump_to_row;              // jump row destination (if jump enabled)

    int32_t ticks_per_row;            // number of ticks per row (defined by mod speed)
    int32_t samples_per_tick;         // number of output samples per tick (defined by mod speed)

    int32_t cur_song_pos;             // current song pos (index into mod->song_positions)
    int32_t cur_pattern;              // current pattern number (as given by mod->song_positions[cur_song_pos])
    int32_t cur_row;                  // current row in current pattern
    int32_t cur_tick;                 // current tick in current row
    int32_t cur_tick_sample;          // current output sample in current tick

    mod_play_event_callback event_callback; // registered function to be called on F8 effect
    void *event_callback_data;              // registered callback data
};

static struct PLAY_STATE state;

// return the number of input samples to advance per output sample for a given note period (fixed point 20.12)
static uint32_t get_sample_advance_for_period(unsigned int period)
{
    return state.sample_advance_factor / period;
}

// return the number of output samples per tick for the given bpm
static uint32_t get_samples_per_tick(unsigned int bpm)
{
    return state.out_frequency * 5 / bpm / 2;
}

static void reset_channels(void)
{
    for (int c = 0; c < state.mod->num_channels; c++) {
        struct PLAY_CHANNEL *ch = &state.channels[c];
        ch->playing        = 0;
        ch->sample         = &state.mod->samples[0];
        ch->sample_pos     = 0;
        ch->sample_len     = 0;
        ch->volume         = MOD_MAX_VOLUME;
        ch->period         = MOD_DEFAULT_NOTE_PERIOD;
        ch->sample_advance = get_sample_advance_for_period(ch->period);
        ch->effect         = 0x000;
    }
}

static void process_channel_effect(int chan_num)
{
    struct PLAY_CHANNEL *ch = &state.channels[chan_num];
    if (ch->effect == 0) return;
    switch (ch->effect >> 8) {
    case 0x0: // arpeggio [x][y]
        ALERT("                                                ch[%d]: ARPEGGIO %02x !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n", chan_num, ch->effect & 0xff);
        break;

    case 0x1: // slide up [z]
        ALERT("                                                ch[%d]: SLIDE UP %02x !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n", chan_num, ch->effect & 0xff);
        break;

    case 0x2: // slide down [z]
        ALERT("                                                ch[%d]: SLIDE DOWN %02x !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n", chan_num, ch->effect & 0xff);
        break;

    case 0x3: // slide to note [z] [period]
        ALERT("                                                ch[%d]: SLIDE TO NOTE %02x !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n", chan_num, ch->effect & 0xff);
        break;

    case 0x4: // vibrato [x][y] adjust pitch: (x=frequency, y=amplitude)
        ALERT("                                                ch[%d]: VIBRATO %02x !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n", chan_num, ch->effect & 0xff);
        break;

    case 0x5: break; // slide to note into volume slide (processed in tick)

    case 0x6: break; // vibrato into volume slide (processed in tick)

    case 0x7: // tremolo [x][y] adjust volume: (x=frequency, y=amplitude)
        ALERT("                                                ch[%d]: TREMOLO %02x !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n", chan_num, ch->effect & 0xff);
        break;

    case 0x8: // set pan [z]
        ALERT("                                                ch[%d]: SET PAN %02x !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n", chan_num, ch->effect & 0xff);
        break;

    case 0x9: // set sample offset [z] offset = z<<8
        ch->sample_pos = ((ch->effect & 0xff) << 8) << 12;
        DEBUG("                                                ch[%d]: sample_pos=%d\n", chan_num, ch->sample_pos>>12);
        break;

    case 0xA: break; // volume slide (processed in tick)

    case 0xB: // jump to song position
        state.jump_enabled = 1;
        state.jump_to_song_pos = ch->effect & 0xff;
        state.jump_to_row = 0;
        DEBUG("                                                ch[%d]: jump to song pos %d\n", chan_num, state.jump_to_song_pos);
        break;

    case 0xC: // set volume
        ch->volume = (ch->effect & 0xff) << 2;
        if (ch->volume > MOD_MAX_VOLUME) ch->volume = MOD_MAX_VOLUME;
        DEBUG("                                                ch[%d]: set volume %d\n", chan_num, ch->volume);
        break;

    case 0xD: // break pattern [x][y]: jump to next pattern's row x*10+y
        state.jump_enabled = 1;
        state.jump_to_song_pos = state.cur_song_pos + 1;
        state.jump_to_row = ((ch->effect & 0xf0) >> 4) * 10 + (ch->effect & 0x0f);
        DEBUG("                                                ch[%d]: break pattern %d %d\n", chan_num, state.jump_to_song_pos, state.jump_to_row);
        break;

    case 0xE: // multiple effects
        switch ((ch->effect>>4)&0x0f) {
        case 0x0: // set filter on/off
            ALERT("                                                ch[%d]: SET FILTER %02x !!!!!!!!!!!!!!!!!!!!!\n", chan_num, ch->effect&0x0f);
            break;

        case 0x1: // fineslide up
            ALERT("                                                ch[%d]: FINESLIDE UP %02x !!!!!!!!!!!!!!!!!!!\n", chan_num, ch->effect&0x0f);
            break;

        case 0x2: // fineslide down
            ALERT("                                                ch[%d]: FINESLIDE DOWN %02x !!!!!!!!!!!!!!!!!\n", chan_num, ch->effect&0x0f);
            break;

        case 0x3: // glissando on/off
            ALERT("                                                ch[%d]: GLISSANDO %02x !!!!!!!!!!!!!!!!!!!!!!\n", chan_num, ch->effect&0x0f);
            break;

        case 0x4: // set vibrato waveform
            ALERT("                                                ch[%d]: SET VIBRATO WAVEFORM %02x !!!!!!!!!!!\n", chan_num, ch->effect&0x0f);
            break;

        case 0x5: // set finetune value
            ALERT("                                                ch[%d]: SET FINETUNE VALUE %02x !!!!!!!!!!!!!\n", chan_num, ch->effect&0x0f);
            break;

        case 0x6: // loop pattern
            ALERT("                                                ch[%d]: LOOP PATTERN %02x !!!!!!!!!!!!!!!!!!!\n", chan_num, ch->effect&0x0f);
            break;

        case 0x7: // set tremolo waveform
            ALERT("                                                ch[%d]: SET TREMOLO WAVEFORM %02x !!!!!!!!!!!\n", chan_num, ch->effect&0x0f);
            break;

        case 0x8: // custom callback (standard says unused, sometimes used as pan)
            if (state.event_callback) {
                state.event_callback(chan_num, ch->effect & 0x0f, state.event_callback_data);
            }
            ALERT("                                                ch[%d]: CALLBACK %02x\n", chan_num, ch->effect&0x0f);
            break;

        case 0x9: break; // retrigger sample (processed in tick)

        case 0xA: // fine volume slide up
            ch->volume += (ch->effect & 0x0f) << 2;
            if (ch->volume > MOD_MAX_VOLUME) ch->volume = MOD_MAX_VOLUME;
            DEBUG("                                                ch[%d]: volume UP %d to %d\n", chan_num, ch->effect&0x0f, ch->volume);
            break;

        case 0xB: // fine volume slide down
            ch->volume -= (ch->effect & 0x0f) << 2;
            if (ch->volume < 0) ch->volume = 0;
            DEBUG("                                                ch[%d]: volume DOWN %d to %d\n", chan_num, ch->effect&0x0f, ch->volume);
            break;

        case 0xC: break; // cut sample (processed in tick)

        case 0xD: break; // delay sample (processed in tick)

        case 0xE: // delay pattern
            ALERT("                                                ch[%d]: DELAY PATTERN %02x !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n", chan_num, ch->effect&0x0f);
            break;

        case 0xF: // invert loop
            ALERT("                                                ch[%d]: INVERT LOOP %02x !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n", chan_num, ch->effect&0x0f);
            break;
        }
        break;

    case 0xF: // set speed
        {
            unsigned int speed = ch->effect & 0xff;
            if (speed <= 32) {  // set ticks per row
                state.ticks_per_row = (speed == 0) ? 1 : speed;
            } else {            // set beats per minute
                state.samples_per_tick = get_samples_per_tick(speed);
            }
        }
        DEBUG("                                                ch[%d]: set speed to %d\n", chan_num, ch->effect & 0xff);
        break;
    }
}

static void process_tick_channel_effect(int chan_num)
{
    struct PLAY_CHANNEL *ch = &state.channels[chan_num];
    if (ch->effect == 0) return;
    switch (ch->effect >> 8) {
    case 0x5: // slide to note into volume slide
        {
            int add = (ch->effect >> 4) & 0x0f;
            int sub = (ch->effect     ) & 0x0f;
            if (add) {
                ch->volume += add<<2;
                if (ch->volume > MOD_MAX_VOLUME) ch->volume = MOD_MAX_VOLUME;
                INFOX("                                                    * ch[%d] vol slide 5 UP %d to %d\n", chan_num, add, ch->volume);
            } else {
                ch->volume -= sub<<2;
                if (ch->volume < 0) ch->volume = 0;
                INFOX("                                                    * ch[%d] vol slide 5 DOWN %d to %d\n", chan_num, sub, ch->volume);
            }
        }
        break;

    case 0x6: // vibrato into volume slide
        {
            int add = (ch->effect >> 4) & 0x0f;
            int sub = (ch->effect     ) & 0x0f;
            if (add) {
                ch->volume += add<<2;
                if (ch->volume > MOD_MAX_VOLUME) ch->volume = MOD_MAX_VOLUME;
                INFOX("                                                    * ch[%d] vol slide 6 UP %d to %d\n", chan_num, add, ch->volume);
            } else {
                ch->volume -= sub<<2;
                if (ch->volume < 0) ch->volume = 0;
                INFOX("                                                    * ch[%d] vol slide 6 DOWN %d to %d\n", chan_num, sub, ch->volume);
            }
        }
        break;

    case 0xA: // volume slide
        {
            int add = (ch->effect >> 4) & 0x0f;
            int sub = (ch->effect     ) & 0x0f;
            if (add) {
                ch->volume += add<<2;
                if (ch->volume > MOD_MAX_VOLUME) ch->volume = MOD_MAX_VOLUME;
                INFOX("                                                    * ch[%d] vol slide A UP %d to %d\n", chan_num, add, ch->volume);
            } else {
                ch->volume -= sub<<2;
                if (ch->volume < 0) ch->volume = 0;
                INFOX("                                                    * ch[%d] vol slide A DOWN %d to %d\n", chan_num, sub, ch->volume);
            }
        }
        break;

    case 0xE:
        switch ((ch->effect>>4)&0x0f) {
        case 0x0: break; // set filter
        case 0x1: break; // fine slide up
        case 0x2: break; // fine slide down
        case 0x3: break; // glissando control
        case 0x4: break; // set vibrato waveform
        case 0x5: break; // set finetune
        case 0x6: break; // set/jump to loop
        case 0x7: break; // set tremolo waveform
        case 0x8: break; // UNUSED (we use it as callback)

        case 0x9: // retrigger sample
            {
                int num_ticks = ch->effect & 0x0f;
                if (num_ticks > 0 && state.cur_tick % num_ticks == 0) {
                    if (ch->sample->loop_len > 2) {
                        ch->sample_pos = ch->sample->loop_start;
                        ch->sample_len = ch->sample->loop_start + ch->sample->loop_len;
                    } else {
                        ch->sample_pos = 0;
                        ch->sample_len = ch->sample->len;
                    }
                    ch->playing = 1;
                    INFOX("                                                    * ch[%d]: retrigger sample %2d (%d) vol=%d/%d\n",
                          chan_num, ch->effect&0x0f, state.cur_tick, ch->volume, ch->sample->volume);
                }
            }
            break;

        case 0xA: break; // fine volume slide up
        case 0xB: break; // fine volume slide down

        case 0xC: // cut sample
            {
                int tick = ch->effect & 0x0f;
                if (state.cur_tick == tick) {
                    ch->volume = 0;
                    DEBUG("                                                    * ch[%d] cut sample\n", chan_num);
                }
            }
            break;

        case 0xD: // delay note
            {
                int tick = ch->effect & 0x0f;
                if (state.cur_tick == tick) {
                    ch->sample_advance = get_sample_advance_for_period(ch->period);
                    if (ch->sample->loop_len > 2) {
                        ch->sample_pos = ch->sample->loop_start;
                        ch->sample_len = ch->sample->loop_start + ch->sample->loop_len;
                    } else {
                        ch->sample_pos = 0;
                        ch->sample_len = ch->sample->len;
                    }
                    ch->playing = 1;
                    DEBUG("                                                    * ch[%d] delay note\n", chan_num);
                }
            }
            break;

        case 0xE: break; // pattern delay
        case 0xF: break; // invert loop
        }
        break;
    }
}

static int start_row(void)
{
    if (state.jump_enabled) {    // process jump to new song position
        // only jump back if looping is enabled
        if (state.jump_to_song_pos > state.cur_song_pos || state.loop_enabled) {
            state.cur_song_pos = state.jump_to_song_pos;
            if (state.cur_song_pos >= state.mod->num_song_positions) {
                if (! state.loop_enabled) {
                    state.mod = NULL;
                    return 1;
                }
                state.cur_song_pos = 0;
            }
            state.cur_row = state.jump_to_row;
            state.cur_pattern = state.mod->song_positions[state.cur_song_pos];
            state.jump_enabled = 0;
        }
    }

    if (state.cur_row >= MOD_NUM_ROWS) {   // scroll to next song position
        state.cur_row = 0;
        if (++state.cur_song_pos >= state.mod->num_song_positions) {
            if (! state.loop_enabled) {
                state.mod = NULL;
                return 1;
            }
            // restart from beginning
            state.samples_per_tick = get_samples_per_tick(MOD_DEFAULT_BPM);
            state.ticks_per_row = MOD_DEFAULT_TICKS_PER_ROW;
            state.cur_song_pos = 0;
            reset_channels();
        }
        state.cur_pattern = state.mod->song_positions[state.cur_song_pos];
        DEBUG("--- pattern ------------\n");
    }

    DEBUG("patt %2d row %2d\n", state.cur_pattern, state.cur_row);
    const struct MOD_CELL *cell = &state.mod->pattern[state.mod->num_channels * (MOD_NUM_ROWS*state.cur_pattern + state.cur_row)];
    for (int c = 0; c < state.mod->num_channels; c++) {
        struct PLAY_CHANNEL *ch = &state.channels[c];
        if (cell->sample > 0) {
            ch->sample = &state.mod->samples[cell->sample-1];
            ch->sample_pos = 0;
            ch->sample_len = (ch->sample->loop_len > 2) ? (ch->sample->loop_start+ch->sample->loop_len) : ch->sample->len;
            ch->playing = 1;
            ch->volume = ch->sample->volume<<2;
            DEBUG("                  ch=%d sample=%2d period=%3d vol=%2d\n", c, cell->sample, cell->period, ch->sample->volume);
        }
        if (cell->note_index != 0xff) {
            ch->period = period_table[ch->sample->finetune][cell->note_index];
            if ((cell->effect>>4) != 0xed) {  // for effect 0xed this is done on tick processing
                ch->sample_advance = get_sample_advance_for_period(ch->period);
            }
        }
        ch->effect = cell->effect;
        process_channel_effect(c);
        cell++;
    }

    return 0;
}

static int start_tick(void)
{
    if (state.cur_tick >= state.ticks_per_row) {
        // advance to next row and process first tick
        state.cur_tick = 0;
        state.cur_row++;
        if (start_row()) {
            return 1;
        }
    }

    if (state.cur_tick > 0) {
        // process mid-row ticks
        for (int c = 0; c < state.mod->num_channels; c++) {
            process_tick_channel_effect(c);
        }
    }

    return 0;
}

static int16_t clamp(int32_t sample)
{
    if (sample < -32768) return -32768;
    if (sample >  32767) return 32767;
    return sample;
}

static void play_samples(int16_t *out, unsigned int n_samples)
{
    for (unsigned int i = 0; i < n_samples; i++) {
        int32_t out_sample = 0;
        for (int c = 0; c < state.mod->num_channels; c++) {
            struct PLAY_CHANNEL *ch = &state.channels[c];
            const struct MOD_SAMPLE *sample = ch->sample;

            if (ch->playing && sample->data && (ch->sample_pos>>12) < ch->sample_len) {
                if (sample->bits_per_sample == 16) {
                    out_sample += (sample->data16[ch->sample_pos>>12] * ch->volume) >> 8;
                } else {
                    out_sample += sample->data8[ch->sample_pos>>12] * ch->volume;
                }
                ch->sample_pos += ch->sample_advance;
                if ((ch->sample_pos>>12) >= ch->sample_len) {
                    if (sample->loop_len > 2) {
                        ch->sample_pos = sample->loop_start<<12;
                    } else {
                        ch->playing = 0;
                    }
                }
            }
        }
        *out++ = clamp(out_sample >> 2);
    }
}

int mod_play_step(int16_t *out, unsigned int len)
{
    if (! state.mod) {
        memset(out, 0, sizeof(int16_t) * len);
        return 1;
    }

    unsigned int samples_left_to_play = len;
    while (samples_left_to_play > 0) {
        if (state.cur_tick_sample >= state.samples_per_tick) {
            state.cur_tick_sample = 0;
            state.cur_tick++;
            if (start_tick()) {
                memset(out, 0, sizeof(int16_t) * samples_left_to_play);
                return 1;
            }
        }

        unsigned int num_samples = state.samples_per_tick - state.cur_tick_sample;
        if (num_samples > samples_left_to_play) num_samples = samples_left_to_play;

        play_samples(out, num_samples);
        state.cur_tick_sample += num_samples;
        samples_left_to_play -= num_samples;
        out += num_samples;
    }
    return 0;
}

void mod_play_start(const struct MOD_DATA *mod_data, int loop)
{
    if (mod_data->num_channels > MOD_MAX_CHANNELS) return;

    state.mod = mod_data;

    // MOD_REF_CLOCK/(2*out_frequency) in 20.12 fixed point:
    state.sample_advance_factor = (((((uint32_t)MOD_REF_CLOCK)/2)<<10) / state.out_frequency)<<2;

    state.samples_per_tick = get_samples_per_tick(MOD_DEFAULT_BPM);
    state.ticks_per_row = MOD_DEFAULT_TICKS_PER_ROW;

    state.cur_song_pos = 0;
    state.cur_pattern = state.mod->song_positions[state.cur_song_pos];
    state.cur_row = 0;

    state.cur_tick = 0;
    state.cur_tick_sample = 0;

    state.loop_enabled = loop;
    state.jump_enabled = 0;

    reset_channels();
    start_row();
}

void mod_play_set_event_callback(mod_play_event_callback callback, void *callback_data)
{
    state.event_callback = callback;
    state.event_callback_data = callback_data;
}

void mod_play_init(unsigned int out_frequency)
{
    state.out_frequency = out_frequency;
}
