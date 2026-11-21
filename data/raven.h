#ifndef RAVEN_DATA_H_FILE
#define RAVEN_DATA_H_FILE

#include <stddef.h>
#include <stdint.h>

enum RAVEN_FONT_TYPE {
    RAVEN_FONT_TYPE_FIXED,
    RAVEN_FONT_TYPE_PROP,
};

struct RAVEN_FONT_REF {
    uint16_t type;              // RAVEN_FONT_TYPE_xxx
    union {
        uint16_t fixed_index;   // raven_fonts[]
        uint16_t prop_index;    // raven_prop_fonts[]
    };
};

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
    int32_t has_mirrors;
    const uint32_t *data;
};

#endif /* RAVEN_SKIP_STRUCTS_IMAGE */

#ifndef RAVEN_SKIP_STRUCTS_PAL_SPRITE

struct RAVEN_PAL_SPRITE {
    int16_t width;
    int16_t height;
    int16_t num_frames;
    uint8_t has_mirrors;
    uint8_t bpp;
    uint8_t palette[16];
    const uint8_t *data;
};

#endif /* RAVEN_SKIP_STRUCTS_PAL_SPRITE */

#ifndef RAVEN_SKIP_STRUCTS_MAP

struct RAVEN_MAP_TILE_CHANGE {
    uint16_t w;
    uint16_t h;
    const uint32_t *tile_bitmap;
    const uint8_t *tiles;
};

struct RAVEN_MAP {
    int16_t w;
    int16_t h;
    uint8_t has_bb_layer;
    uint16_t num_tile_changes;
    uint16_t tileset_index;        // raven_tilesets[]
    uint16_t tile_animation_index; // raven_tile_animations[]
    const uint8_t *tiles;
    const struct RAVEN_MAP_TILE_CHANGE *tile_changes;
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
    uint16_t sprite_index;  // raven_sprites[]
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
   RAVEN_ROOM_TRIGGER_TYPE_WALL_BUTTON,
   RAVEN_ROOM_TRIGGER_TYPE_DETECTOR_BUTTON,
   RAVEN_ROOM_TRIGGER_TYPE_UNBLOCK_TILES,
   RAVEN_ROOM_TRIGGER_TYPE_DISABLE_ANIMATION,
   RAVEN_ROOM_TRIGGER_TYPE_CHANGE_TILES,
   RAVEN_ROOM_TRIGGER_TYPE_REMOVE_ENEMY,
   RAVEN_ROOM_TRIGGER_TYPE_PLAY_ANIMATION,
   RAVEN_ROOM_TRIGGER_TYPE_PLAY_SFX,
   RAVEN_ROOM_TRIGGER_TYPE_STOP_SFX,
   RAVEN_ROOM_TRIGGER_TYPE_GET_UPGRADE,
   RAVEN_ROOM_TRIGGER_TYPE_GET_COLLECTABLE,
   RAVEN_ROOM_TRIGGER_TYPE_GET_PICKUP,
};

struct RAVEN_ROOM_MAP_INFO {
    uint16_t x;
    uint16_t y;
    uint16_t map_index; // raven_maps[]
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
            uint16_t data4;
            uint16_t data5;
        } any;
        struct {
            uint8_t direction;
        } player_spawn;
        struct {
            uint16_t dest_room_index; // raven_rooms[]
            uint16_t dest_trigger_id;
            uint16_t req_flag_id;
        } door;
        struct {
            uint16_t enemy_id;
            uint16_t enemy_index;  // raven_enemies[]
            uint8_t direction;
            uint16_t req_flag_id;
        } enemy_spawn;
        struct {
            uint16_t width;
            uint16_t height;
            uint16_t interact_prompt_id;
            uint16_t set_effect_id;
            uint16_t set_flag_id;
            uint16_t req_flag_id;
            uint16_t req_collectable_id;
        } wall_button;
        struct {
            uint16_t width;
            uint16_t height;
            uint16_t set_effect_id;
            uint16_t set_flag_id;
            uint16_t req_flag_id;
            uint16_t req_collectable_id;
        } detector_button;
        struct {
            uint16_t width;
            uint16_t height;
            uint16_t on_effect_id;
        } unblock_tiles;
        struct {
            uint16_t width;
            uint16_t height;
            uint16_t on_effect_id;
        } disable_animation;
        struct {
            uint16_t map_index;
            uint16_t tile_change_index;
            uint8_t map_layer;
            uint16_t on_effect_id;
        } change_tiles;
        struct {
            uint16_t enemy_id;
            uint16_t on_effect_id;
        } remove_enemy;
        struct {
            uint16_t anim_index;  // raven_sprite_animations[]
            uint16_t on_effect_id;
            uint16_t set_effect_id;
            uint16_t set_flag_id;
        } play_animation;
        struct {
            uint16_t sfx_index;  // raven_sfxs[]
            uint16_t sfx_id;
            uint16_t volume;
            uint16_t pitch_shift;
            uint16_t loop;
            uint16_t on_effect_id;
        } play_sfx;
        struct {
            uint16_t sfx_id;
            uint16_t on_effect_id;
        } stop_sfx;
        struct {
            uint16_t get_upgrade_id;
            uint16_t req_flag_id;
        } get_upgrade;
        struct {
            uint16_t get_collectable_id;
            uint16_t interact_prompt_id;
            uint16_t req_flag_id;
        } get_collectable;
        struct {
            uint16_t get_pickup_id;
            uint16_t respawn_delay;
            uint16_t sfx_index;  // raven_sfxs[]
        } get_pickup;
    };
};

struct RAVEN_ROOM_PARALLAX {
    uint16_t tileset_index;  // raven_tilesets[]
    uint16_t width;
    uint16_t height;
    const uint8_t *tiles;
};

struct RAVEN_ROOM {
    uint16_t num_maps;
    uint16_t num_triggers;
    const struct RAVEN_ROOM_MAP_INFO *maps;
    const struct RAVEN_ROOM_TRIGGER_INFO *triggers;
    struct RAVEN_ROOM_PARALLAX parallax;
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

#ifndef RAVEN_SKIP_STRUCTS_TILE_ANIMATION

struct RAVEN_TILE_ANIMATION_LOOP {
    uint8_t start;
    uint8_t len;
};

struct RAVEN_TILE_ANIMATION {
    uint16_t parent_tileset_index;  // raven_tilesets[]
    uint16_t anim_tileset_index;    // raven_tilesets[]
    const struct RAVEN_TILE_ANIMATION_LOOP loops[256];
};

#endif /* RAVEN_SKIP_STRUCTS_TILE_ANIMATION */

#ifndef RAVEN_SKIP_STRUCTS_PLAYER_CHAR

struct RAVEN_PLAYER_CHAR_SUIT {
    uint16_t animation_index;  // raven_sprite_animations[]
    uint16_t req_upgrade_id;
};

struct RAVEN_PLAYER_CHAR_WEAPON {
    uint16_t shoot_anim_index;       // raven_sprite_animations[]
    uint16_t shoot_sfx_index;        // raven_sfxs[]
    uint16_t bullet_anim_index;      // raven_sprite_animations[]
    uint16_t bullet_hit_anim_index;  // raven_sprite_animations[]
    uint16_t bullet_hit_sfx_index;   // raven_sfxs[]

    uint16_t req_collectable_id;
    uint16_t ammo_pickup_id;
    int8_t crouch_x;
    int8_t crouch_y;
    int8_t bullet_x;
    int8_t bullet_y;
    int16_t bullet_dx;
    int16_t bullet_dy;
    uint16_t damage;
    uint8_t bullet_initial_delay;
    uint8_t bullet_interval;
    uint8_t shoot_anim_len;
};

struct RAVEN_PLAYER_CHAR {
    struct RAVEN_FONT_REF font;
    uint16_t gamepad_sprite_index;  // raven_sprites[]
    uint16_t num_suits;
    uint16_t num_weapons;
    const struct RAVEN_PLAYER_CHAR_SUIT *suits;
    const struct RAVEN_PLAYER_CHAR_WEAPON *weapons;
};

#endif /* RAVEN_SKIP_STRUCTS_PLAYER_CHAR */

#ifndef RAVEN_SKIP_STRUCTS_ENEMY

struct RAVEN_ENEMY {
    uint16_t type;
    uint16_t anim_index;  // raven_sprite_animations[]
    uint16_t health;
};

#endif /* RAVEN_SKIP_STRUCTS_ENEMY */

#ifndef RAVEN_SKIP_STRUCTS_DATA_TABLES

struct RAVEN_TABLE_ITEM {
    uint16_t sprite_index;  // raven_sprite[]
};

#endif /* RAVEN_SKIP_STRUCTS_DATA_TABLES */

#ifndef RAVEN_SKIP_ROOM_SCRIPT

#ifndef RAVEN_SKIP_ROOM_SCRIPT_FUNCTION_TYPEDEFS
typedef void (*raven_room_init_function)(uint32_t, void *);
typedef void (*raven_room_update_function)(void *);
#endif

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
extern const struct RAVEN_TILE_ANIMATION raven_tile_animations[];
extern const struct RAVEN_ROOM raven_rooms[];
extern const struct RAVEN_PLAYER_CHAR raven_player_chars[];
extern const struct RAVEN_ENEMY raven_enemies[];
extern const struct RAVEN_TABLE_ITEM raven_table_upgrades[];
extern const struct RAVEN_TABLE_ITEM raven_table_collectables[];
extern const struct RAVEN_TABLE_ITEM raven_table_pickups[];
extern const char *raven_table_interact_prompts[];

#if RAVEN_ADD_ROOM_SCRIPTS
extern const struct RAVEN_ROOM_SCRIPT *raven_room_script_table[];
#endif /* RAVEN_ADD_ROOM_SCRIPTS */

#endif /* RAVEN_DATA_H_FILE */
