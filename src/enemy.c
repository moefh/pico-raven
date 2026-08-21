#include <stdio.h>

#include "enemy.h"
#include "collision.h"

#include "lib/joystick.h"
#include "lib/mem.h"

enum ENEMY_TYPE {
    ENEMY_TYPE_WALKER,
    ENEMY_TYPE_CHILLER,
    ENEMY_TYPE_HOPPER,
};

enum WALKER_STATE {
    WALKER_STATE_WALK,
};

enum CHILLER_STATE {
    CHILLER_STATE_WALK,
    CHILLER_STATE_LOOK,
    CHILLER_STATE_BLINK,
};

enum HOPPER_STATE {
    HOPPER_STATE_AMBUSH_WAIT,
    HOPPER_STATE_AMBUSH_PLUNGE,
    HOPPER_STATE_SPLAT,
    HOPPER_STATE_PATROL_WALK,
    HOPPER_STATE_PATROL_BLINK,
    HOPPER_STATE_PATROL_LOOK,
    HOPPER_STATE_POUNCE_PREP,
    HOPPER_STATE_POUNCE_JUMP,
};

enum ENEMY_ANIM_LOOP {
    ENEMY_ANIM_LOOP_STAND,
    ENEMY_ANIM_LOOP_RUN,
    ENEMY_ANIM_LOOP_JUMP,
    ENEMY_ANIM_LOOP_FALL,
    ENEMY_ANIM_LOOP_LOOK,
    ENEMY_ANIM_LOOP_BLINK,
    ENEMY_ANIM_LOOP_SPLAT,
    ENEMY_ANIM_LOOP_PREP_JUMP,
};

static int is_at_animation_end(struct RAVEN_CHARACTER *enemy)
{
    const struct RAVEN_SPRITE_ANIMATION_LOOP *loop = &enemy->anim->loops[enemy->anim_loop];
    return (((enemy->anim_frame + loop->frame_adv + 1) >> 8) >= loop->length);
}

static int is_player_seen(struct GAME_STATE *game, struct RAVEN_CHARACTER *enemy)
{
    struct COLLISION_RECT watched_area = {
        .x = enemy->x - 5*TILE_SIZE,
        .y = enemy->y - 2*TILE_SIZE,
        .w = enemy->anim->collision.w + 10*TILE_SIZE,
        .h = enemy->anim->collision.h + 10*TILE_SIZE,
    };

    struct RAVEN_CHARACTER *pl = &game->player;
    int px = pl->x + pl->anim->collision.w / 2;
    int py = pl->y + pl->anim->collision.h / 2;

    return collision_is_point_in_rect(px, py, &watched_area);
}

static int fall(struct GAME_STATE *game, int enemy_index)
{
    struct RAVEN_CHARACTER *enemy = &game->enemies[enemy_index];
    struct RAVEN_ENEMY_CONTROL *control = &game->enemies_control[enemy_index];

    struct COLLISION_RECT rect = {
        .x = enemy->x,
        .y = enemy->y,
        .w = enemy->anim->collision.w,
        .h = enemy->anim->collision.h,
    };
    int flags = collision_move(game, &rect, control->dx>>8, control->dy>>8);
    enemy->x = rect.x;
    enemy->y = rect.y;

    return flags;
}

static int walk_but_turn_on_bump_or_edge(struct GAME_STATE *game, int enemy_index)
{
    struct RAVEN_CHARACTER *enemy = &game->enemies[enemy_index];
    struct RAVEN_ENEMY_CONTROL *control = &game->enemies_control[enemy_index];

    // check if we're close to an edge
    struct COLLISION_RECT check_fall_rect = {
        .x = enemy->x + (((control->dx < 0) ? -1 : 1) * enemy->anim->collision.w),
        .y = enemy->y,
        .w = enemy->anim->collision.w,
        .h = enemy->anim->collision.h,
    };
    if (collision_move(game, &check_fall_rect, 0, 1) == 0) {
        enemy->direction = ! enemy->direction;
        enemy->anim_frame = 0;
        return 1;
    }

    // walk
    struct COLLISION_RECT rect = {
        .x = enemy->x,
        .y = enemy->y,
        .w = enemy->anim->collision.w,
        .h = enemy->anim->collision.h,
    };
    int collision = collision_move(game, &rect, control->dx >> 8, 0);
    if (collision != 0 && collision != COLLISION_FLAGS_RAMP) {
        enemy->direction = ! enemy->direction;
        enemy->anim_frame = 0;
        enemy->x = rect.x;
        enemy->y = rect.y;
        return 1;
    }
    enemy->x = rect.x;
    enemy->y = rect.y;
    return 0;
}

/* ================================================================ */
/* === WALKER ===================================================== */
/* ================================================================ */

static void set_walker_anim_loop(struct RAVEN_CHARACTER *enemy)
{
    switch (enemy->state) {
    case WALKER_STATE_WALK: enemy->anim_loop = ENEMY_ANIM_LOOP_RUN; break;
    default: enemy->anim_loop = ENEMY_ANIM_LOOP_STAND; break;
    }
}

static void init_walker(struct GAME_STATE *game, int enemy_index)
{
    struct RAVEN_CHARACTER *enemy = &game->enemies[enemy_index];
    enemy->state = WALKER_STATE_WALK;

    struct RAVEN_ENEMY_CONTROL *control = &game->enemies_control[enemy_index];
    control->dx = 0;
    control->dy = 0;
    control->wait = 0;

    set_walker_anim_loop(enemy);
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

    set_walker_anim_loop(enemy);
}

/* ================================================================ */
/* === CHILLER ==================================================== */
/* ================================================================ */

static void set_chiller_anim_loop(struct RAVEN_CHARACTER *enemy)
{
    switch (enemy->state) {
    case CHILLER_STATE_WALK:  enemy->anim_loop = ENEMY_ANIM_LOOP_RUN; break;
    case CHILLER_STATE_LOOK:  enemy->anim_loop = ENEMY_ANIM_LOOP_LOOK; break;
    case CHILLER_STATE_BLINK:  enemy->anim_loop = ENEMY_ANIM_LOOP_BLINK; break;
    default: enemy->anim_loop = ENEMY_ANIM_LOOP_STAND; break;
    }
}

static void init_chiller(struct GAME_STATE *game, int enemy_index)
{
    struct RAVEN_CHARACTER *enemy = &game->enemies[enemy_index];
    enemy->state = CHILLER_STATE_LOOK;

    struct RAVEN_ENEMY_CONTROL *control = &game->enemies_control[enemy_index];
    control->dx = 0x100;
    control->dy = 0;
    control->dx = (enemy->direction == RAVEN_DIR_LEFT) ? -0x100 : 0x100;

    set_chiller_anim_loop(enemy);
}

static void update_chiller(struct GAME_STATE *game, int enemy_index)
{
    struct RAVEN_CHARACTER *enemy = &game->enemies[enemy_index];
    struct RAVEN_ENEMY_CONTROL *control = &game->enemies_control[enemy_index];

    switch (enemy->state) {
    case CHILLER_STATE_WALK:
        if (control->wait-- <= 0 || walk_but_turn_on_bump_or_edge(game, enemy_index) != 0) {
            enemy->state = CHILLER_STATE_BLINK;
            enemy->anim_frame = 0;
            control->dx = (enemy->direction == RAVEN_DIR_LEFT) ? -0x100 : 0x100;
        }
        break;

    case CHILLER_STATE_BLINK:
        if (is_at_animation_end(enemy)) {
            enemy->state = CHILLER_STATE_LOOK;
            enemy->anim_frame = 0;
        }
    break;

    case CHILLER_STATE_LOOK:
        if (is_at_animation_end(enemy)) {
            enemy->state = CHILLER_STATE_WALK;
            enemy->anim_frame = 0;
            control->wait = 120;
        }
        break;
    }

    set_chiller_anim_loop(enemy);
}

/* ================================================================ */
/* === HOPPER ===================================================== */
/* ================================================================ */

static void set_hopper_anim_loop(struct RAVEN_CHARACTER *enemy)
{
    switch (enemy->state) {
    case HOPPER_STATE_AMBUSH_WAIT:   enemy->anim_loop = ENEMY_ANIM_LOOP_STAND; break;
    case HOPPER_STATE_AMBUSH_PLUNGE: enemy->anim_loop = ENEMY_ANIM_LOOP_FALL; break;
    case HOPPER_STATE_SPLAT:         enemy->anim_loop = ENEMY_ANIM_LOOP_SPLAT; break;
    case HOPPER_STATE_PATROL_WALK:   enemy->anim_loop = ENEMY_ANIM_LOOP_RUN; break;
    case HOPPER_STATE_PATROL_BLINK:  enemy->anim_loop = ENEMY_ANIM_LOOP_BLINK; break;
    case HOPPER_STATE_PATROL_LOOK:   enemy->anim_loop = ENEMY_ANIM_LOOP_LOOK; break;
    case HOPPER_STATE_POUNCE_PREP:   enemy->anim_loop = ENEMY_ANIM_LOOP_PREP_JUMP; break;
    case HOPPER_STATE_POUNCE_JUMP:   enemy->anim_loop = ENEMY_ANIM_LOOP_JUMP; break;
    default: enemy->anim_loop = ENEMY_ANIM_LOOP_STAND; break;
    }
}

static void init_hopper(struct GAME_STATE *game, int enemy_index)
{
    struct RAVEN_CHARACTER *enemy = &game->enemies[enemy_index];
    enemy->state = HOPPER_STATE_AMBUSH_WAIT;

    struct RAVEN_ENEMY_CONTROL *control = &game->enemies_control[enemy_index];
    control->dx = 0;
    control->dy = 0;
    control->wait = 0;

    set_hopper_anim_loop(enemy);
}

static void update_hopper(struct GAME_STATE *game, int enemy_index)
{
    struct RAVEN_CHARACTER *enemy = &game->enemies[enemy_index];
    struct RAVEN_ENEMY_CONTROL *control = &game->enemies_control[enemy_index];

    switch (enemy->state) {
    case HOPPER_STATE_AMBUSH_WAIT:
        if (is_player_seen(game, enemy) != 0) {
            enemy->state = HOPPER_STATE_AMBUSH_PLUNGE;
            enemy->anim_frame = 0;
            control->dx = 0;
            control->dy = 0;
        }
        break;

    case HOPPER_STATE_AMBUSH_PLUNGE:
        control->dy += DY_GRAVITY;
        if (control->dy >= DY_MAX) control->dy = DY_MAX;
        if ((fall(game, enemy_index) & COLLISION_FLAGS_DOWN) != 0) {
            enemy->state = HOPPER_STATE_SPLAT;
            enemy->anim_frame = 0;
        }
        break;

    case HOPPER_STATE_SPLAT:
        if (is_at_animation_end(enemy)) {
            if (is_player_seen(game, enemy)) {
                enemy->state = HOPPER_STATE_POUNCE_PREP;
                enemy->anim_frame = 0;
                enemy->direction = (game->player.x < enemy->x) ? RAVEN_DIR_LEFT : RAVEN_DIR_RIGHT;
                control->dx = (enemy->direction == RAVEN_DIR_LEFT) ? -0x100 : 0x100;
            } else {
                enemy->state = HOPPER_STATE_PATROL_WALK;
                enemy->anim_frame = 0;
                control->dx = (enemy->direction == RAVEN_DIR_LEFT) ? -0x100 : 0x100;
                control->wait = 120;
            }
        }
        break;

    case HOPPER_STATE_PATROL_WALK:
        if (is_player_seen(game, enemy)) {
            enemy->state = HOPPER_STATE_POUNCE_PREP;
            enemy->anim_frame = 0;
            enemy->direction = (game->player.x < enemy->x) ? RAVEN_DIR_LEFT : RAVEN_DIR_RIGHT;
        } else if (control->wait-- <= 0 || walk_but_turn_on_bump_or_edge(game, enemy_index) != 0) {
            enemy->state = HOPPER_STATE_PATROL_BLINK;
            enemy->anim_frame = 0;
        }
        break;

    case HOPPER_STATE_PATROL_BLINK:
        if (is_player_seen(game, enemy)) {
            enemy->state = HOPPER_STATE_POUNCE_PREP;
            enemy->anim_frame = 0;
            enemy->direction = (game->player.x < enemy->x) ? RAVEN_DIR_LEFT : RAVEN_DIR_RIGHT;
        } else if (is_at_animation_end(enemy)) {
            enemy->state = HOPPER_STATE_PATROL_LOOK;
            enemy->anim_frame = 0;
        }
        break;

    case HOPPER_STATE_PATROL_LOOK:
        if (is_player_seen(game, enemy)) {
            enemy->state = HOPPER_STATE_POUNCE_PREP;
            enemy->anim_frame = 0;
            enemy->direction = (game->player.x < enemy->x) ? RAVEN_DIR_LEFT : RAVEN_DIR_RIGHT;
        } else if (is_at_animation_end(enemy)) {
            enemy->state = HOPPER_STATE_PATROL_WALK;
            enemy->anim_frame = 0;
            control->wait = 120;
        }
        control->dx = (enemy->direction == RAVEN_DIR_LEFT) ? -0x100 : 0x100;
        break;

    case HOPPER_STATE_POUNCE_PREP:
        if (is_at_animation_end(enemy)) {
            enemy->state = HOPPER_STATE_POUNCE_JUMP;
            enemy->anim_frame = 0;
            enemy->direction = (game->player.x < enemy->x) ? RAVEN_DIR_LEFT : RAVEN_DIR_RIGHT;
            control->dx = (enemy->direction == RAVEN_DIR_LEFT) ? -0x200 : 0x200;
            control->dy = -0xa00;
        }
        break;

    case HOPPER_STATE_POUNCE_JUMP:
        control->dy += DY_GRAVITY;
        if ((fall(game, enemy_index) & COLLISION_FLAGS_DOWN) != 0) {
            enemy->state = HOPPER_STATE_SPLAT;
            enemy->anim_frame = 0;
        }
        break;
    }

    set_hopper_anim_loop(enemy);
}

static void update_sprite_info(struct RAVEN_CHARACTER *enemy)
{
    const struct RAVEN_SPRITE_ANIMATION_LOOP *loop = &enemy->anim->loops[enemy->anim_loop];
    int loop_frame = enemy->anim_frame >> 8;
    if (loop_frame >= loop->length) {
        if (loop->dont_loop) {
            loop_frame = loop->length - 1;
            enemy->anim_frame = loop_frame << 8;
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
    enemy->direction = spawn->enemy_spawn.direction;
    enemy->anim_frame = 0;
    enemy->shadow_enabled = 0;

    struct RAVEN_ENEMY_CONTROL *control = &game->enemies_control[enemy_index];
    control->enemy_type = spawn->enemy_spawn.enemy_type;
    switch (control->enemy_type) {
    case ENEMY_TYPE_WALKER: init_walker(game, enemy_index); break;
    case ENEMY_TYPE_CHILLER: init_chiller(game, enemy_index); break;
    case ENEMY_TYPE_HOPPER: init_hopper(game, enemy_index); break;
    default: enemy->anim_loop = ENEMY_ANIM_LOOP_STAND; break;
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
    case ENEMY_TYPE_HOPPER: update_hopper(game, enemy_index); break;
    }

    enemy->anim_frame += enemy->anim->loops[enemy->anim_loop].frame_adv + 1; // [0-255] -> [1-256]
    update_sprite_info(enemy);
}
