#include <stdio.h>

#include "enemy.h"
#include "collision.h"

#include "lib/joystick.h"
#include "lib/mem.h"

enum ENEMY_TYPE {
    ENEMY_TYPE_WALKER,
    ENEMY_TYPE_CHILLER,
};

enum ENEMY_ANIM_LOOP {
    ENEMY_ANIM_LOOP_STAND,
    ENEMY_ANIM_LOOP_RUN,
    ENEMY_ANIM_LOOP_JUMP,
    ENEMY_ANIM_LOOP_FALL,
    ENEMY_ANIM_LOOP_LOOK,
};

static void init_walker(struct GAME_STATE *game, int enemy_index)
{
    struct RAVEN_CHARACTER *enemy = &game->enemies[enemy_index];
    enemy->state = ENEMY_STATE_WALK;

    struct RAVEN_ENEMY_CONTROL *control = &game->enemies_control[enemy_index];
    control->ai_state = 0;
    control->wait = 0;
}

static void update_walker(struct GAME_STATE *game, int enemy_index)
{
    struct RAVEN_CHARACTER *enemy = &game->enemies[enemy_index];

    int dx = (enemy->direction == RAVEN_DIR_LEFT) ? -1 : 1;
    struct COLLISION_RECT rect = {
        .x = enemy->x,
        .y = enemy->y,
        .w = enemy->anim->collision.w,
        .h = enemy->anim->collision.h,
    };
    if (collision_move(game, &rect, 0, 1) == 0) {
        enemy->x -= dx;
        enemy->direction = ! enemy->direction;
        enemy->anim_frame = 0;
    } else {
        int collision = collision_move(game, &rect, dx, 0);
        if (collision != 0 && collision != COLLISION_FLAGS_RAMP) {
            enemy->direction = ! enemy->direction;
            enemy->anim_frame = 0;
        }
        enemy->x = rect.x;
        enemy->y = rect.y;
    }
}

static void init_chiller(struct GAME_STATE *game, int enemy_index)
{
    struct RAVEN_CHARACTER *enemy = &game->enemies[enemy_index];
    enemy->state = ENEMY_STATE_STAND;

    struct RAVEN_ENEMY_CONTROL *control = &game->enemies_control[enemy_index];
    control->ai_state = 0;
    control->wait = 0;
}

static void update_chiller(struct GAME_STATE *game, int enemy_index)
{
    struct RAVEN_CHARACTER *enemy = &game->enemies[enemy_index];
    struct RAVEN_ENEMY_CONTROL *control = &game->enemies_control[enemy_index];

    if (enemy->state == ENEMY_STATE_WALK) {
        int dx = (enemy->direction == RAVEN_DIR_LEFT) ? -1 : 1;
        struct COLLISION_RECT rect = {
            .x = enemy->x,
            .y = enemy->y,
            .w = enemy->anim->collision.w,
            .h = enemy->anim->collision.h,
        };
        if (collision_move(game, &rect, 0, 1) == 0) {
            enemy->x -= dx;
            enemy->direction = ! enemy->direction;
            enemy->anim_frame = 0;
            control->wait = 120;
        } else {
            int collision = collision_move(game, &rect, dx, 0);
            if (collision != 0 && collision != COLLISION_FLAGS_RAMP) {
                enemy->direction = ! enemy->direction;
                enemy->anim_frame = 0;
                control->wait = 120;
            }
            enemy->x = rect.x;
            enemy->y = rect.y;
        }
    }

    if (control->wait-- == 0) {
        if (enemy->state == ENEMY_STATE_WALK) {
            enemy->state = ENEMY_STATE_STAND;
            control->wait = 300;
        } else {
            enemy->state = ENEMY_STATE_WALK;
            control->wait = 120;
        }
    }
}

static void update_sprite_info(struct RAVEN_CHARACTER *enemy)
{
    switch (enemy->state) {
    case ENEMY_STATE_STAND: enemy->anim_loop = ENEMY_ANIM_LOOP_STAND; break;
    case ENEMY_STATE_WALK:  enemy->anim_loop = ENEMY_ANIM_LOOP_RUN; break;
    case ENEMY_STATE_JUMP:  enemy->anim_loop = ENEMY_ANIM_LOOP_JUMP; break;
    case ENEMY_STATE_FALL:  enemy->anim_loop = ENEMY_ANIM_LOOP_FALL; break;
    }
    const struct RAVEN_SPRITE_ANIMATION_LOOP *loop = &enemy->anim->loops[enemy->anim_loop];
    int loop_frame = enemy->anim_frame >> 8;
    if (loop_frame >= loop->length) {
        if (loop->dont_loop) {
            loop_frame = loop->length - 1;
            enemy->anim_frame = (loop_frame) << 8;
        } else {
            loop_frame = 0;
            enemy->anim_frame &= 0xff;
        }
    }
    enemy->sprite_frame = enemy->anim->frame_indices[loop->offset + loop_frame];
    if (enemy->direction == RAVEN_DIR_LEFT) {
        enemy->sprite_frame += enemy->sprite->num_frames/2;
        enemy->sprite_x = enemy->x - (enemy->sprite->width - (enemy->anim->collision.x + enemy->anim->collision.w));
        enemy->sprite_y = enemy->y - enemy->anim->collision.y;
    } else {
        enemy->sprite_x = enemy->x - enemy->anim->collision.x;
        enemy->sprite_y = enemy->y - enemy->anim->collision.y;
    }
}

void enemy_init(struct GAME_STATE *game, int enemy_index, const struct RAVEN_ROOM_TRIGGER_INFO *spawn)
{
    struct RAVEN_CHARACTER *enemy = &game->enemies[enemy_index];
    enemy->anim = spawn->enemy_spawn.animation;
    enemy->sprite = enemy->anim->sprite;
    enemy->x = spawn->x;
    enemy->y = spawn->y;
    enemy->state = ENEMY_STATE_STAND;
    enemy->direction = spawn->enemy_spawn.direction;
    enemy->anim_frame = 0;
    enemy->shadow_enabled = 0;

    struct RAVEN_ENEMY_CONTROL *control = &game->enemies_control[enemy_index];
    control->enemy_type = spawn->enemy_spawn.enemy_type;
    switch (control->enemy_type) {
    case ENEMY_TYPE_WALKER: init_walker(game, enemy_index); break;
    case ENEMY_TYPE_CHILLER: init_chiller(game, enemy_index); break;
    default:
        control->ai_state = 0;
        control->wait = 0;
        break;
    }

    update_sprite_info(enemy);
}

void enemy_update(struct GAME_STATE *game, int enemy_index)
{
    struct RAVEN_CHARACTER *enemy = &game->enemies[enemy_index];
    struct RAVEN_ENEMY_CONTROL *control = &game->enemies_control[enemy_index];

    switch (control->enemy_type) {
    case ENEMY_TYPE_WALKER: update_walker(game, enemy_index); break;
    case ENEMY_TYPE_CHILLER: update_chiller(game, enemy_index); break;
    }

    enemy->anim_frame += enemy->anim->loops[enemy->anim_loop].frame_adv + 1; // [0-255] -> [1-256]
    update_sprite_info(enemy);
}
