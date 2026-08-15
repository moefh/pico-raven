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

#ifndef RAVEN_SKIP_STRUCTS_PAL_SPRITE

struct RAVEN_PAL_SPRITE {
    int16_t width;
    int16_t height;
    int16_t num_frames;
    uint16_t bpp;
    uint8_t palette[16];
    const uint8_t *data;
};

#endif /* RAVEN_SKIP_STRUCTS_PAL_SPRITE */

#ifndef RAVEN_SKIP_STRUCTS_MAP

struct RAVEN_MAP {
    int16_t w;
    int16_t h;
    int16_t para_w;
    int16_t para_h;
    const struct RAVEN_IMAGE *tileset;
    const uint8_t *tiles;
};

#endif /* RAVEN_SKIP_STRUCTS_MAP */

#ifndef RAVEN_SKIP_STRUCTS_SPRITE_ANIMATION

struct RAVEN_SPRITE_ANIMATION_LOOP {
    uint16_t offset;   // offset into animation frame_indices
    uint16_t length;   // loop data length
    uint8_t dont_loop;
    uint8_t frame_adv;
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

#endif /* RAVEN_SKIP_STRUCTS_SPRITE_ANIMATION */

#ifndef RAVEN_SKIP_STRUCTS_FONT

struct RAVEN_FONT {
    uint8_t width;
    uint8_t height;
    const uint8_t *data;
};

#endif /* RAVEN_SKIP_STRUCTS_FONT */

#ifndef RAVEN_SKIP_STRUCTS_PROP_FONT

struct RAVEN_PROP_FONT {
    uint8_t height;
    const uint8_t *data;
    uint8_t char_width[96];
    uint16_t char_offset[96];
};

#endif /* RAVEN_SKIP_STRUCTS_PROP_FONT */

#ifndef RAVEN_SKIP_STRUCTS_ROOM

enum RAVEN_ROOM_TRIGGER_TYPE {
   RAVEN_ROOM_TRIGGER_TYPE_UNKNOWN,
   RAVEN_ROOM_TRIGGER_TYPE_DOOR,
   RAVEN_ROOM_TRIGGER_TYPE_PLAYER_SPAWN,
   RAVEN_ROOM_TRIGGER_TYPE_ENEMY_SPAWN,
   RAVEN_ROOM_TRIGGER_TYPE_TRAP,
};

struct RAVEN_ROOM_MAP_INFO {
    uint16_t x;
    uint16_t y;
    const struct RAVEN_MAP *map;
};

struct RAVEN_ROOM_TRIGGER_INFO {
    uint16_t type; // enum RAVEN_ROOM_TRIGGER_TYPE
    uint16_t trigger_id;
    int16_t x;
    int16_t y;
    union {
        struct {
            uint16_t data0;
            uint16_t data1;
            uint16_t data2;
            uint16_t data3;
        } any;
        struct {
            uint8_t direction;
        } player_spawn;
        struct {
             const struct RAVEN_ROOM *dest_room;
             uint16_t dest_trigger_id;
        } door;
        struct {
             const struct RAVEN_SPRITE_ANIMATION *animation;
        } enemy_spawn;
        struct {
             uint16_t width;
             uint16_t height;
             uint16_t type;
        } trap;
    };
};

struct RAVEN_ROOM {
    uint16_t num_maps;
    uint16_t num_triggers;
    const struct RAVEN_ROOM_MAP_INFO *maps;
    const struct RAVEN_ROOM_TRIGGER_INFO *triggers;
};

#endif /* RAVEN_SKIP_STRUCTS_ROOM */

#ifndef RAVEN_SKIP_STRUCTS_WORLD

struct RAVEN_WORLD_REGION {
    uint8_t x;
    uint8_t y;
    uint8_t width;
    uint8_t height;
    const uint32_t *block_bitmap;
    const uint8_t *blocks;
    const uint16_t *room_indices;
};

struct RAVEN_WORLD {
    uint16_t num_regions;
    const struct RAVEN_WORLD_REGION *regions;
};

#endif /* RAVEN_SKIP_STRUCTS_WORLD */

#ifndef RAVEN_SKIP_ROOM_SCRIPT

struct RAVEN_STATE;
typedef void (*raven_room_init_function)(uint32_t, struct RAVEN_STATE *);
typedef void (*raven_room_update_function)(struct RAVEN_STATE *);

struct RAVEN_ROOM_SCRIPT {
    raven_room_init_function init;
    raven_room_update_function update;
};

#endif /* RAVEN_SKIP_ROOM_SCRIPT */

extern const struct RAVEN_FONT raven_fonts[];
extern const struct RAVEN_PROP_FONT raven_prop_fonts[];
extern const struct RAVEN_MOD_DATA raven_mods[];
extern const struct RAVEN_SFX raven_sfxs[];
extern const struct RAVEN_IMAGE raven_tilesets[];
extern const struct RAVEN_IMAGE raven_sprites[];
extern const struct RAVEN_MAP raven_maps[];
extern const struct RAVEN_SPRITE_ANIMATION raven_sprite_animations[];
extern const struct RAVEN_ROOM raven_rooms[];

#if RAVEN_ADD_ROOM_SCRIPTS
extern const struct RAVEN_ROOM_SCRIPT *raven_room_script_table[];
#endif /* RAVEN_ADD_ROOM_SCRIPTS */

#endif /* RAVEN_DATA_H_FILE */
