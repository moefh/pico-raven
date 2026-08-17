#ifndef GAME_H_FILE
#define GAME_H_FILE

#include <stdint.h>
#include "game_data.h"
#include "sprite_shadow.h"

#define GAME_MAX_ENEMIES  32

enum PLAYER_STATE {
    PLAYER_STATE_STAND,
    PLAYER_STATE_WALK,
    PLAYER_STATE_JUMP,
    PLAYER_STATE_FALL,
};

enum ENEMY_STATE {
    ENEMY_STATE_STAND,
    ENEMY_STATE_WALK,
    ENEMY_STATE_JUMP,
    ENEMY_STATE_FALL,
};

struct RAVEN_PLAYER_CONTROL {
    int32_t dx;  // 24.8 fixpoint
    int32_t dy;
};

struct RAVEN_ENEMY_CONTROL {
    int8_t enemy_type;
    int8_t ai_state;
    int16_t wait;
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
};

struct JOYSTICK;

void game_main_loop(struct GAME_STATE *game, struct JOYSTICK *joy);
int game_check_player_trigger(struct GAME_STATE *game, uint32_t trigger_type_flags);
void game_spawn_room_enemies(struct GAME_STATE *game);
void game_update_room_enemies(struct GAME_STATE *game);

#endif /* GAME_H_FILE */
