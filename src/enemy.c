#include <stdio.h>
#include <string.h>

#include "enemy.h"
#include "collision.h"
#include "game.h"

#include "lib/vga_8bit.h"
#include "lib/mem.h"

enum ENEMY_TYPE {
    ENEMY_TYPE_WALKER,
    ENEMY_TYPE_CHILLER,
    ENEMY_TYPE_HOPPER,
    ENEMY_TYPE_FLOATER,
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

enum FLOATER_STATE {
    FLOATER_STATE_FLOAT,
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
    ENEMY_ANIM_LOOP_EXPLODE,
    ENEMY_ANIM_LOOP_FLOAT,
    ENEMY_ANIM_LOOP_DIE,
};

static int is_at_animation_end(struct GAME_ENEMY *enemy)
{
    const struct RAVEN_SPRITE_ANIMATION_LOOP *loop = &enemy->anim.anim->loops[enemy->anim.loop];
    return (((enemy->anim.frame + loop->frame_adv + 1) >> 8) >= loop->length);
}

static int is_player_seen(struct GAME_ENEMY *enemy)
{
    struct COLLISION_RECT watched_area = {
        .x = enemy->x - 5*TILE_SIZE,
        .y = enemy->y - 2*TILE_SIZE,
        .w = enemy->anim.anim->collision.w + 10*TILE_SIZE,
        .h = enemy->anim.anim->collision.h + 10*TILE_SIZE,
    };

    struct GAME_PLAYER *pl = &game_state.player;
    int px = pl->x + pl->char_anim.anim->collision.w / 2;
    int py = pl->y + pl->char_anim.anim->collision.h / 2;

    return collision_is_point_in_rect(px, py, &watched_area);
}

static int fall(int enemy_index)
{
    struct GAME_ENEMY *enemy = &game_state.enemies[enemy_index];
    struct GAME_ENEMY_CONTROL *control = &game_state.enemies_control[enemy_index];

    struct COLLISION_RECT rect = {
        .x = enemy->x,
        .y = enemy->y,
        .w = enemy->anim.anim->collision.w,
        .h = enemy->anim.anim->collision.h,
    };
    int flags = collision_move(&rect, control->dx>>8, control->dy>>8);
    enemy->x = rect.x;
    enemy->y = rect.y;

    return flags;
}

static int walk_but_turn_on_bump_or_edge(int enemy_index)
{
    struct GAME_ENEMY *enemy = &game_state.enemies[enemy_index];
    struct GAME_ENEMY_CONTROL *control = &game_state.enemies_control[enemy_index];

    // check if we're close to an edge
    struct COLLISION_RECT check_fall_rect = {
        .x = enemy->x + (((control->dx < 0) ? -1 : 1) * enemy->anim.anim->collision.w),
        .y = enemy->y,
        .w = enemy->anim.anim->collision.w,
        .h = enemy->anim.anim->collision.h,
    };
    if (collision_move(&check_fall_rect, 0, 1) == 0) {
        enemy->direction = ! enemy->direction;
        enemy->anim.frame = 0;
        return 1;
    }

    // walk
    struct COLLISION_RECT rect = {
        .x = enemy->x,
        .y = enemy->y,
        .w = enemy->anim.anim->collision.w,
        .h = enemy->anim.anim->collision.h,
    };
    int collision = collision_move(&rect, control->dx >> 8, 0);
    if (collision != 0 && collision != COLLISION_FLAGS_RAMP) {
        enemy->direction = ! enemy->direction;
        enemy->anim.frame = 0;
        enemy->x = rect.x;
        enemy->y = rect.y;
        return 1;
    }
    enemy->x = rect.x;
    enemy->y = rect.y;
    return 0;
}

static inline void set_state(struct GAME_ENEMY *enemy, int state)
{
    enemy->state = state;
    enemy->anim.frame = 0;
}

/* ================================================================ */
/* === WALKER ===================================================== */
/* ================================================================ */

static void set_walker_anim_loop(struct GAME_ENEMY *enemy)
{
    switch (enemy->state) {
    case WALKER_STATE_WALK: enemy->anim.loop = ENEMY_ANIM_LOOP_RUN; break;
    default: enemy->anim.loop = ENEMY_ANIM_LOOP_STAND; break;
    }
}

static void init_walker(int enemy_index)
{
    struct GAME_ENEMY *enemy = &game_state.enemies[enemy_index];
    enemy->state = WALKER_STATE_WALK;

    struct GAME_ENEMY_CONTROL *control = &game_state.enemies_control[enemy_index];
    control->dx = (enemy->direction == RAVEN_DIR_LEFT) ? -0x100 : 0x100;
    control->dy = 0;
    control->wait = 0;

    set_walker_anim_loop(enemy);
}

static void update_walker(int enemy_index)
{
    struct GAME_ENEMY *enemy = &game_state.enemies[enemy_index];
    struct GAME_ENEMY_CONTROL *control = &game_state.enemies_control[enemy_index];

    if (walk_but_turn_on_bump_or_edge(enemy_index) != 0) {
        control->dx = (enemy->direction == RAVEN_DIR_LEFT) ? -0x100 : 0x100;
    }

    set_walker_anim_loop(enemy);
}

/* ================================================================ */
/* === CHILLER ==================================================== */
/* ================================================================ */

static void set_chiller_anim_loop(struct GAME_ENEMY *enemy)
{
    switch (enemy->state) {
    case CHILLER_STATE_WALK:  enemy->anim.loop = ENEMY_ANIM_LOOP_RUN; break;
    case CHILLER_STATE_LOOK:  enemy->anim.loop = ENEMY_ANIM_LOOP_LOOK; break;
    case CHILLER_STATE_BLINK:  enemy->anim.loop = ENEMY_ANIM_LOOP_BLINK; break;
    default: enemy->anim.loop = ENEMY_ANIM_LOOP_STAND; break;
    }
}

static void init_chiller(int enemy_index)
{
    struct GAME_ENEMY *enemy = &game_state.enemies[enemy_index];
    enemy->state = CHILLER_STATE_LOOK;

    struct GAME_ENEMY_CONTROL *control = &game_state.enemies_control[enemy_index];
    control->dx = (enemy->direction == RAVEN_DIR_LEFT) ? -0x100 : 0x100;
    control->dy = 0;
    control->wait = 0;

    set_chiller_anim_loop(enemy);
}

static void update_chiller(int enemy_index)
{
    struct GAME_ENEMY *enemy = &game_state.enemies[enemy_index];
    struct GAME_ENEMY_CONTROL *control = &game_state.enemies_control[enemy_index];

    switch (enemy->state) {
    case CHILLER_STATE_WALK:
        if (control->wait-- <= 0 || walk_but_turn_on_bump_or_edge(enemy_index) != 0) {
            set_state(enemy, CHILLER_STATE_BLINK);
            control->dx = (enemy->direction == RAVEN_DIR_LEFT) ? -0x100 : 0x100;
        }
        break;

    case CHILLER_STATE_BLINK:
        if (is_at_animation_end(enemy)) {
            set_state(enemy, CHILLER_STATE_LOOK);
        }
    break;

    case CHILLER_STATE_LOOK:
        if (is_at_animation_end(enemy)) {
            set_state(enemy, CHILLER_STATE_WALK);
            control->wait = 120;
        }
        break;
    }

    set_chiller_anim_loop(enemy);
}

/* ================================================================ */
/* === HOPPER ===================================================== */
/* ================================================================ */

static void set_hopper_anim_loop(struct GAME_ENEMY *enemy)
{
    switch (enemy->state) {
    case HOPPER_STATE_AMBUSH_WAIT:   enemy->anim.loop = ENEMY_ANIM_LOOP_STAND; break;
    case HOPPER_STATE_AMBUSH_PLUNGE: enemy->anim.loop = ENEMY_ANIM_LOOP_FALL; break;
    case HOPPER_STATE_SPLAT:         enemy->anim.loop = ENEMY_ANIM_LOOP_SPLAT; break;
    case HOPPER_STATE_PATROL_WALK:   enemy->anim.loop = ENEMY_ANIM_LOOP_RUN; break;
    case HOPPER_STATE_PATROL_BLINK:  enemy->anim.loop = ENEMY_ANIM_LOOP_BLINK; break;
    case HOPPER_STATE_PATROL_LOOK:   enemy->anim.loop = ENEMY_ANIM_LOOP_LOOK; break;
    case HOPPER_STATE_POUNCE_PREP:   enemy->anim.loop = ENEMY_ANIM_LOOP_PREP_JUMP; break;
    case HOPPER_STATE_POUNCE_JUMP:   enemy->anim.loop = ENEMY_ANIM_LOOP_JUMP; break;
    default: enemy->anim.loop = ENEMY_ANIM_LOOP_STAND; break;
    }
}

static void init_hopper(int enemy_index)
{
    struct GAME_ENEMY *enemy = &game_state.enemies[enemy_index];
    enemy->state = HOPPER_STATE_AMBUSH_WAIT;

    struct GAME_ENEMY_CONTROL *control = &game_state.enemies_control[enemy_index];
    control->dx = 0;
    control->dy = 0;
    control->wait = 0;

    set_hopper_anim_loop(enemy);
}

static void update_hopper(int enemy_index)
{
    struct GAME_ENEMY *enemy = &game_state.enemies[enemy_index];
    struct GAME_ENEMY_CONTROL *control = &game_state.enemies_control[enemy_index];

    switch (enemy->state) {
    case HOPPER_STATE_AMBUSH_WAIT:
        if (is_player_seen(enemy) != 0) {
            set_state(enemy, HOPPER_STATE_AMBUSH_PLUNGE);
            control->dx = 0;
            control->dy = 0;
        }
        break;

    case HOPPER_STATE_AMBUSH_PLUNGE:
        control->dy += DY_GRAVITY;
        if (control->dy >= DY_MAX) control->dy = DY_MAX;
        if ((fall(enemy_index) & COLLISION_FLAGS_DOWN) != 0) {
            set_state(enemy, HOPPER_STATE_SPLAT);
        }
        break;

    case HOPPER_STATE_SPLAT:
        if (is_at_animation_end(enemy)) {
            if (is_player_seen(enemy)) {
                set_state(enemy, HOPPER_STATE_POUNCE_PREP);
                enemy->direction = (game_state.player.x < enemy->x) ? RAVEN_DIR_LEFT : RAVEN_DIR_RIGHT;
                control->dx = (enemy->direction == RAVEN_DIR_LEFT) ? -0x100 : 0x100;
            } else {
                set_state(enemy, HOPPER_STATE_PATROL_WALK);
                control->dx = (enemy->direction == RAVEN_DIR_LEFT) ? -0x100 : 0x100;
                control->wait = 120;
            }
        }
        break;

    case HOPPER_STATE_PATROL_WALK:
        if (is_player_seen(enemy)) {
            set_state(enemy, HOPPER_STATE_POUNCE_PREP);
            enemy->direction = (game_state.player.x < enemy->x) ? RAVEN_DIR_LEFT : RAVEN_DIR_RIGHT;
        } else if (control->wait-- <= 0 || walk_but_turn_on_bump_or_edge(enemy_index) != 0) {
            set_state(enemy, HOPPER_STATE_PATROL_BLINK);
        }
        break;

    case HOPPER_STATE_PATROL_BLINK:
        if (is_player_seen(enemy)) {
            set_state(enemy, HOPPER_STATE_POUNCE_PREP);
            enemy->direction = (game_state.player.x < enemy->x) ? RAVEN_DIR_LEFT : RAVEN_DIR_RIGHT;
        } else if (is_at_animation_end(enemy)) {
            set_state(enemy, HOPPER_STATE_PATROL_LOOK);
        }
        break;

    case HOPPER_STATE_PATROL_LOOK:
        if (is_player_seen(enemy)) {
            set_state(enemy, HOPPER_STATE_POUNCE_PREP);
            enemy->direction = (game_state.player.x < enemy->x) ? RAVEN_DIR_LEFT : RAVEN_DIR_RIGHT;
        } else if (is_at_animation_end(enemy)) {
            set_state(enemy, HOPPER_STATE_PATROL_WALK);
            control->wait = 120;
        }
        control->dx = (enemy->direction == RAVEN_DIR_LEFT) ? -0x100 : 0x100;
        break;

    case HOPPER_STATE_POUNCE_PREP:
        if (is_at_animation_end(enemy)) {
            set_state(enemy, HOPPER_STATE_POUNCE_JUMP);
            enemy->direction = (game_state.player.x < enemy->x) ? RAVEN_DIR_LEFT : RAVEN_DIR_RIGHT;
            control->dx = (enemy->direction == RAVEN_DIR_LEFT) ? -0x200 : 0x200;
            control->dy = -0xa00;
        }
        break;

    case HOPPER_STATE_POUNCE_JUMP:
        control->dy += DY_GRAVITY;
        if ((fall(enemy_index) & COLLISION_FLAGS_DOWN) != 0) {
            set_state(enemy, HOPPER_STATE_SPLAT);
        }
        break;
    }

    set_hopper_anim_loop(enemy);
}

/* ================================================================ */
/* === FLOATER ==================================================== */
/* ================================================================ */

static void init_floater(int enemy_index)
{
    struct GAME_ENEMY *enemy = &game_state.enemies[enemy_index];
    enemy->state = FLOATER_STATE_FLOAT;

    struct GAME_ENEMY_CONTROL *control = &game_state.enemies_control[enemy_index];
    control->dx = 0;
    control->dy = 0;
    control->wait = enemy->y;

    enemy->anim.loop = ENEMY_ANIM_LOOP_FLOAT;
}

static void update_floater(int enemy_index)
{
    struct GAME_ENEMY *enemy = &game_state.enemies[enemy_index];
    struct GAME_ENEMY_CONTROL *control = &game_state.enemies_control[enemy_index];

    static const int8_t float_delta[] = { 0, -1, -2, -2, -1, 0, 1, 2, 2, 1 };
    enemy->y = control->wait + float_delta[(game_state.frame_counter / 16) % (sizeof(float_delta) / sizeof(*float_delta))];
}

/* ================================================================ */
/* ================================================================ */
/* ================================================================ */

void enemy_init(int enemy_index, const struct RAVEN_ROOM_TRIGGER_INFO *spawn)
{
    const struct RAVEN_ENEMY *raven_enemy = &raven_enemies[spawn->enemy_spawn.enemy_index];
    struct GAME_ENEMY *enemy = &game_state.enemies[enemy_index];
    enemy->id = spawn->enemy_spawn.enemy_id;
    enemy->anim.anim = &raven_sprite_animations[raven_enemy->anim_index];
    enemy->health = raven_enemy->health;
    enemy->anim.sprite = &raven_sprites[enemy->anim.anim->sprite_index];
    enemy->x = spawn->x;
    enemy->y = spawn->y;
    enemy->direction = spawn->enemy_spawn.direction;
    enemy->anim.frame = 0;
    enemy->shadow_enabled = 0;
    enemy->hit_frames_left = 0;

    struct GAME_ENEMY_CONTROL *control = &game_state.enemies_control[enemy_index];
    control->enemy_type = raven_enemy->type;
    switch (control->enemy_type) {
    case ENEMY_TYPE_WALKER: init_walker(enemy_index); break;
    case ENEMY_TYPE_CHILLER: init_chiller(enemy_index); break;
    case ENEMY_TYPE_HOPPER: init_hopper(enemy_index); break;
    case ENEMY_TYPE_FLOATER: init_floater(enemy_index); break;
    default: enemy->anim.loop = ENEMY_ANIM_LOOP_STAND; break;
    }

    game_update_animation(&enemy->anim, enemy->x, enemy->y, enemy->direction);
}

static void update_enemy(int enemy_index)
{
    struct GAME_ENEMY *enemy = &game_state.enemies[enemy_index];
    struct GAME_ENEMY_CONTROL *control = &game_state.enemies_control[enemy_index];

    if (enemy->health == 0) {
        enemy->hit_frames_left = 0;
        enemy->anim.loop = ENEMY_ANIM_LOOP_DIE;
        enemy->anim.frame = 0;
        game_update_animation(&enemy->anim, enemy->x, enemy->y, enemy->direction);
        game_state.enemy_collision_bits[enemy_index] = 0;
        return;
    }

    switch (control->enemy_type) {
    case ENEMY_TYPE_WALKER: update_walker(enemy_index); break;
    case ENEMY_TYPE_CHILLER: update_chiller(enemy_index); break;
    case ENEMY_TYPE_HOPPER: update_hopper(enemy_index); break;
    case ENEMY_TYPE_FLOATER: update_floater(enemy_index); break;
    }

    if (enemy->hit_frames_left != 0) enemy->hit_frames_left--;
    enemy->anim.frame += enemy->anim.anim->loops[enemy->anim.loop].frame_adv + 1; // [0-255] -> [1-256]
    game_update_animation(&enemy->anim, enemy->x, enemy->y, enemy->direction);

    if (enemy->health != 0xffff &&
        enemy->y > game_state.screen_y - 64 &&
        enemy->y < game_state.screen_y + vga_screen.height - SCREEN_HEADER_HEIGHT + 64) {
        int x_start = enemy->x >> 12;
        int x_end = (enemy->x + enemy->anim.anim->collision.w) >> 12;
        if (x_start < 0) x_start = 0; else if (x_start >= 32) x_start = 31;
        if (x_end   < 0) x_end   = 0; else if (x_end   >= 32) x_end   = 31;
        game_state.enemy_collision_bits[enemy_index] = (1<<x_start) | (1<<x_end);
    }
}

void enemy_remove_id(uint16_t enemy_id)
{
    for (int i = 0; i < game_state.num_enemies; i++) {
        struct GAME_ENEMY *enemy = &game_state.enemies[i];
        if (enemy->id == enemy_id) {
            memmove(&game_state.enemies[i],
                    &game_state.enemies[i+1],
                    sizeof(struct GAME_ENEMY) * (game_state.num_enemies-(i+1)));
            game_state.num_enemies--;
            i--;
        }
    }
}

void enemy_update_all(void)
{
    for (int i = 0; i < game_state.num_enemies; i++) {
        update_enemy(i);
    }
}
