#ifndef RAVEN_DATA_H_FILE
#define RAVEN_DATA_H_FILE

#include <stddef.h>
#include <stdint.h>

#ifndef RAVEN_SKIP_STRUCTS_MOD

struct RAVEN_MOD_SAMPLE {
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

struct RAVEN_MOD_CELL {
    uint8_t  sample;
    uint8_t  note_index;
    uint16_t effect;
};

struct RAVEN_MOD_DATA {
    struct RAVEN_MOD_SAMPLE samples[31];
    uint8_t num_channels;

    uint8_t num_song_positions;
    uint8_t song_positions[128];

    uint8_t num_patterns;
    const struct RAVEN_MOD_CELL *pattern;
};

#endif /* RAVEN_SKIP_STRUCTS_MOD */

#ifndef RAVEN_SKIP_STRUCTS_SFX

struct RAVEN_SFX {
    int32_t len;
    int32_t loop_start;
    int32_t loop_len;
    int32_t bits_per_sample;
    union {
        const void *samples;
        const int8_t *spl8;
        const int16_t *spl16;
    };
};

#endif /* RAVEN_SKIP_STRUCTS_SFX */

#ifndef RAVEN_SKIP_STRUCTS_IMAGE

struct RAVEN_IMAGE {
    int32_t width;
    int32_t height;
    int32_t stride;
    int32_t num_frames;
    const uint32_t *data;
};

#endif /* RAVEN_SKIP_STRUCTS_IMAGE */

#ifndef RAVEN_SKIP_STRUCTS_MAP

struct RAVEN_MAP {
    int16_t w;
    int16_t h;
    int16_t bg_w;
    int16_t bg_h;
    const struct RAVEN_IMAGE *tileset;
    const uint8_t *tiles;
};

#endif /* #ifndef RAVEN_SKIP_STRUCTS_MAP */

#ifndef RAVEN_SKIP_STRUCT_SPRITE_ANIMATION

struct RAVEN_SPRITE_ANIMATION_LOOP {
    uint16_t offset;   // offset into animation frame_indices
    uint16_t length;   // loop data length
};

struct RAVEN_SPRITE_ANIMATION_COLLISION {
    uint16_t x;
    uint16_t y;
    uint16_t w;
    uint16_t h;
};

struct RAVEN_SPRITE_ANIMATION {
    const uint8_t *frame_indices;
    const struct RAVEN_IMAGE *sprite;
    struct RAVEN_SPRITE_ANIMATION_COLLISION collision;
    int8_t use_foot_frames;
    int8_t foot_overlap;
    struct RAVEN_SPRITE_ANIMATION_LOOP loops[20];
};

#endif /* #ifndef RAVEN_SKIP_STRUCTS_SPRITE_ANIMATION */

#ifndef RAVEN_SKIP_STRUCTS_FONT

struct RAVEN_FONT {
    uint8_t width;
    uint8_t height;
    const uint8_t *data;
};

#endif /* #ifndef RAVEN_SKIP_STRUCTS_FONT */

#ifndef RAVEN_SKIP_STRUCTS_PROP_FONT

struct RAVEN_PROP_FONT {
    uint8_t height;
    const uint8_t *data;
    uint8_t char_width[96];
    uint16_t char_offset[96];
};

#endif /* #ifndef RAVEN_SKIP_STRUCTS_PROP_FONT */

#ifndef RAVEN_SKIP_STRUCTS_ROOM

struct RAVEN_ROOM_MAP_INFO {
    uint16_t x;
    uint16_t y;
    const struct RAVEN_MAP *map;
};

struct RAVEN_ROOM_ENTITY_INFO {
    int16_t x;
    int16_t y;
    const struct RAVEN_SPRITE_ANIMATION *anim;
    uint16_t data0;
    uint16_t data1;
    uint16_t data2;
    uint16_t data3;
};

struct RAVEN_ROOM_TRIGGER_INFO {
    int16_t x;
    int16_t y;
    uint16_t w;
    uint16_t h;
    uint16_t data0;
    uint16_t data1;
    uint16_t data2;
    uint16_t data3;
};

struct RAVEN_ROOM {
    uint8_t num_maps;
    uint8_t num_entities;
    uint8_t num_triggers;
    const struct RAVEN_ROOM_MAP_INFO *maps;
    const struct RAVEN_ROOM_ENTITY_INFO *entities;
    const struct RAVEN_ROOM_TRIGGER_INFO *triggers;
};

#endif /* #ifndef RAVEN_SKIP_STRUCTS_ROOM */

extern const struct RAVEN_FONT raven_fonts[];
extern const struct RAVEN_PROP_FONT raven_prop_fonts[];
extern const struct RAVEN_MOD_DATA raven_mods[];
extern const struct RAVEN_SFX raven_sfxs[];
extern const struct RAVEN_IMAGE raven_tilesets[];
extern const struct RAVEN_IMAGE raven_sprites[];
extern const struct RAVEN_MAP raven_maps[];
extern const struct RAVEN_SPRITE_ANIMATION raven_sprite_animations[];
extern const struct RAVEN_ROOM raven_rooms[];

#endif /* RAVEN_DATA_H_FILE */
