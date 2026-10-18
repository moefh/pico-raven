#ifndef GAME_H_FILE
#define GAME_H_FILE

#include <stdint.h>
#include "game_data.h"
#include "sprite_shadow.h"
#include "run_state.h"

#define SCREEN_HEADER_HEIGHT 32

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

struct GAME_PLAYER_CONTROL {
    int32_t dx;  // 24.8 fixpoint
    int32_t dy;
};

struct GAME_STATE {
    struct GAME_CHARACTER player;
    struct GAME_PLAYER_CONTROL player_control;

    int32_t room_id;
    uint32_t upgrades;
    uint32_t collectables[8];
    uint32_t flags[8];
};

struct JOYSTICK;
struct RAVEN_ROOM_TRIGGER_INFO;

void game_main_loop(struct GAME_STATE *game, struct JOYSTICK *joy);
const struct RAVEN_ROOM_TRIGGER_INFO * game_check_player_trigger(struct GAME_STATE *game, uint32_t trigger_type_flags);

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

#endif /* GAME_H_FILE */
