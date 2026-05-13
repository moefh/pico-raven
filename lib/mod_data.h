#ifndef MOD_DATA_H_FILE
#define MOD_DATA_H_FILE

#include <stdint.h>

struct MOD_SAMPLE {
    uint32_t len;
    uint32_t loop_start;
    uint32_t loop_len;
    uint8_t  finetune;
    uint8_t  volume;
    uint16_t bits_per_sample;
    union {
        const void *data;
        const int8_t *data8;
        const int16_t *data16;
    };
};

struct MOD_CELL {
    uint8_t  sample;
    uint8_t  note_index;
    uint16_t effect;
};

struct MOD_DATA {
    struct MOD_SAMPLE samples[31];
    uint8_t num_channels;

    uint8_t num_song_positions;
    uint8_t song_positions[128];

    uint8_t num_patterns;
    const struct MOD_CELL *pattern;
};

#endif /* MOD_DATA_H_FILE */
