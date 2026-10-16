#ifndef GAME_H_FILE
#define GAME_H_FILE

#include <stdint.h>
#include "game_data.h"
#include "sprite_shadow.h"

#define GAME_MAX_ENEMIES  32

#define DX_ACCEL          ((int32_t) 0x100)
#define DX_FLOOR_FRICTION ((int32_t) 0x0c0)
#define DX_AIR_FRICTION   ((int32_t) 0x010)
#define DX_MAX            ((int32_t) 0x700)

#define DY_GRAVITY    ((int32_t) 0x0c0)
#define DY_MAX        ((int32_t) 0x900)
#define DY_JUMP_START ((int32_t)-0xa00)
#define DY_JUMP_HOLD  ((int32_t)-0x060)

enum PLAYER_STATE {
    PLAYER_STATE_STAND,
    PLAYER_STATE_WALK,
    PLAYER_STATE_JUMP,
    PLAYER_STATE_FALL,
    PLAYER_STATE_FALL2,
    PLAYER_STATE_CROUCH,
};

struct RAVEN_PLAYER_CONTROL {
    int32_t dx;  // 24.8 fixpoint
    int32_t dy;
};

struct RAVEN_ENEMY_CONTROL {
    int8_t enemy_type;
    int16_t wait;
    int16_t dx;  // 8.8 fixpoint
    int16_t dy;
};

struct RAVEN_CHARACTER {
    const struct VGA_IMAGE *sprite;
    const struct RAVEN_SPRITE_ANIMATION *anim;
    int32_t x;
    int32_t y;
    uint8_t state;
    uint8_t direction;
    uint8_t shadow_enabled;
    uint8_t anim_loop;
    uint16_t anim_frame;  // 8.8 fixpoint

    int16_t sprite_x;
    int16_t sprite_y;
    uint8_t sprite_frame;
};

struct GAME_STATE {
    struct RAVEN_CHARACTER player;
    struct RAVEN_PLAYER_CONTROL player_control;

    int32_t num_enemies;
    struct RAVEN_CHARACTER enemies[GAME_MAX_ENEMIES];
    struct RAVEN_ENEMY_CONTROL enemies_control[GAME_MAX_ENEMIES];

    int32_t room_id;
    uint32_t upgrades;
    uint32_t collectables[8];
    uint32_t flags[8];
    uint32_t pickups[8];
};

struct JOYSTICK;
struct RAVEN_ROOM_TRIGGER_INFO;

void game_main_loop(struct GAME_STATE *game, struct JOYSTICK *joy);
const struct RAVEN_ROOM_TRIGGER_INFO * game_check_player_trigger(struct GAME_STATE *game, uint32_t trigger_type_flags);
void game_spawn_room_enemies(struct GAME_STATE *game);
void game_update_room_enemies(struct GAME_STATE *game);

void game_activate_effect(struct GAME_STATE *game, uint32_t effect_id);

static inline bool game_has_upgrade(struct GAME_STATE *game, uint32_t upgrade_id) {
    return (game->upgrades & (1<<upgrade_id)) != 0;
}
static inline bool game_has_collectable(struct GAME_STATE *game, uint32_t collectable_id) {
    uint32_t index = collectable_id / 32;
    uint32_t bit = collectable_id % 32;
    if (index < sizeof(game->collectables)/sizeof(game->collectables[0])) {
        return (game->collectables[index] & (1<<bit)) != 0;
    }
    return false;
}
static inline bool game_has_flag(struct GAME_STATE *game, uint32_t flag_id) {
    uint32_t index = flag_id / 32;
    uint32_t bit = flag_id % 32;
    if (index < sizeof(game->flags)/sizeof(game->flags[0])) {
        return (game->flags[index] & (1<<bit)) != 0;
    }
    return false;
}
static inline bool game_has_pickup(struct GAME_STATE *game, uint32_t pickup_id) {
    uint32_t index = pickup_id / 32;
    uint32_t bit = pickup_id % 32;
    if (index < sizeof(game->pickups)/sizeof(game->pickups[0])) {
        return (game->pickups[index] & (1<<bit)) != 0;
    }
    return false;
}

static inline void game_set_upgrade(struct GAME_STATE *game, uint32_t upgrade_id) {
    game->upgrades |= 1u << upgrade_id;
}
static inline void game_set_collectable(struct GAME_STATE *game, uint32_t collectable_id) {
    uint32_t index = collectable_id / 32;
    uint32_t bit = collectable_id % 32;
    if (index < sizeof(game->collectables)/sizeof(game->collectables[0])) {
        game->collectables[index] |= 1u << bit;
    }
}
static inline void game_set_flag(struct GAME_STATE *game, uint32_t flag_id) {
    uint32_t index = flag_id / 32;
    uint32_t bit = flag_id % 32;
    if (index < sizeof(game->flags)/sizeof(game->flags[0])) {
        game->flags[index] |= 1u << bit;
    }
}
static inline void game_set_pickup(struct GAME_STATE *game, uint32_t pickup_id) {
    uint32_t index = pickup_id / 32;
    uint32_t bit = pickup_id % 32;
    if (index < sizeof(game->pickups)/sizeof(game->pickups[0])) {
        game->pickups[index] |= 1u << bit;
    }
}

#endif /* GAME_H_FILE */
