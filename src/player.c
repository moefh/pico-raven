#include <stdio.h>
#include <stdbool.h>

#include "player.h"
#include "collision.h"

#include "lib/joystick.h"
#include "lib/mem.h"

static inline void set_state(struct GAME_PLAYER *pl, int state)
{
    pl->state = state;
    pl->char_anim.frame = 0;
}

static void update_animation(struct GAME_ANIMATION *anim, struct GAME_PLAYER *pl, bool no_arms)
{
    int anim_loop = 0;
    switch (pl->state) {
    case PLAYER_STATE_STAND:  anim_loop = RAVEN_SPRITE_ANIMATION_BUNNY_LOOP_STAND;  break;
    case PLAYER_STATE_WALK:   anim_loop = RAVEN_SPRITE_ANIMATION_BUNNY_LOOP_RUN;    break;
    case PLAYER_STATE_JUMP:   anim_loop = RAVEN_SPRITE_ANIMATION_BUNNY_LOOP_JUMP;   break;
    case PLAYER_STATE_FALL:   anim_loop = RAVEN_SPRITE_ANIMATION_BUNNY_LOOP_FALL;   break;
    case PLAYER_STATE_FALL2:  anim_loop = RAVEN_SPRITE_ANIMATION_BUNNY_LOOP_FALL2;  break;
    case PLAYER_STATE_CROUCH: anim_loop = RAVEN_SPRITE_ANIMATION_BUNNY_LOOP_CROUCH; break;
    }
    anim->loop = anim_loop + ((no_arms) ? PLAYER_NUM_STATES : 0);
    game_update_animation(anim, pl->x, pl->y, pl->direction);
}

void player_update_sprite_info(void)
{
    struct GAME_PLAYER *pl = &game_state.player;

    if (pl->weapon_frames_left != 0) {
        update_animation(&pl->char_anim, pl, true);
        update_animation(&pl->weapon_anim, pl, false);
    } else {
        update_animation(&pl->char_anim, pl, false);
    }
    if (pl->shadow_enabled) {
        sprite_shadow_add_frame(pl->char_anim.sprite_frame, pl->char_anim.sprite_x, pl->char_anim.sprite_y);
    }
}

void player_set_sprite_anim(int animation_index)
{
    const struct RAVEN_SPRITE_ANIMATION *anim = &raven_sprite_animations[animation_index];
    game_state.player.char_anim.sprite = &raven_sprites[anim->sprite_index];
    game_state.player.char_anim.anim = anim;
}

static void set_weapon_sprite_anim(int animation_index)
{
    const struct RAVEN_SPRITE_ANIMATION *anim = &raven_sprite_animations[animation_index];
    if (game_state.player.weapon_anim.anim != anim) {
        game_state.player.weapon_anim.sprite = &raven_sprites[anim->sprite_index];
        game_state.player.weapon_anim.anim = anim;
        game_state.player.weapon_anim.frame = 0;
    }
}

void player_init(void)
{
    struct GAME_PLAYER *pl = &game_state.player;
    pl->char_anim.anim = &raven_sprite_animations[pl->player_char->suits[pl->suit_index].animation_index];
    pl->char_anim.sprite = &raven_sprites[pl->char_anim.anim->sprite_index];

    pl->x = 0;
    pl->y = 0;
    pl->state = PLAYER_STATE_STAND;
    pl->direction = RAVEN_DIR_RIGHT;
    pl->weapon_frames_left = 0;
    pl->shoot_wait_frames = 0;
    pl->weapon_index = 0xff;
    pl->shoot_now = 0;
    pl->shadow_enabled = 0;
    pl->darkness_enabled = 0;
    pl->last_shot_frame = 0;

    pl->char_anim.frame = 0;
    pl->weapon_anim.frame = 0;

    player_update_sprite_info();
}

void player_update(struct JOYSTICK *joy)
{
    struct GAME_PLAYER *pl = &game_state.player;
    struct GAME_PLAYER_CONTROL *plc = &game_state.player_control;

    //printf("update player: at (%ld,%ld), delta=0x(%lx,%lx)=(%ld,%ld)\n", pl->x, pl->y, plc->dx, plc->dy, plc->dx>>8, plc->dy>>8);
    int dx = plc->dx >> 8;
    int dy = plc->dy >> 8;
    struct COLLISION_RECT rect = {
        .x = pl->x,
        .y = pl->y,
        .w = pl->char_anim.anim->collision.w,
        .h = pl->char_anim.anim->collision.h,
    };
    int collision = collision_move(&rect, dx, dy);
    if (collision & COLLISION_FLAGS_DOWN) {
        plc->dy = 0;
        if (JOY_BTN_HELD(joy, JOY_BTN_RIGHT|JOY_BTN_LEFT)) {
            set_state(pl, PLAYER_STATE_WALK);
        } else if (pl->state != PLAYER_STATE_CROUCH) {
            set_state(pl, PLAYER_STATE_STAND);
        }
    } else if (collision & COLLISION_FLAGS_UP) {
        if (plc->dy < 0) {
            plc->dy = 0;
        }
        if (pl->state == PLAYER_STATE_JUMP) {
            set_state(pl, PLAYER_STATE_FALL);
        }
    } else if (pl->state == PLAYER_STATE_STAND || pl->state == PLAYER_STATE_WALK || pl->state == PLAYER_STATE_CROUCH) {
        int y = rect.y;
        if (collision_move(&rect, 0, 1) == 0) {
            set_state(pl, PLAYER_STATE_FALL);
        }
        rect.y = y;
    }

    pl->x = rect.x;
    pl->y = rect.y;
    pl->char_anim.frame += pl->char_anim.anim->loops[pl->char_anim.loop].frame_adv + 1; // [0-255] -> [1-256]
    if (pl->weapon_frames_left != 0) {
        pl->weapon_frames_left--;
        pl->weapon_anim.frame += pl->weapon_anim.anim->loops[pl->weapon_anim.loop].frame_adv + 1;
    } else {
        pl->weapon_index = 0xff;
        pl->weapon_anim.frame = 0;
    }
    if (pl->state == PLAYER_STATE_FALL) {
        // reaching end of animation in the "FALL" state moves to the "FALL2" state
        const struct RAVEN_SPRITE_ANIMATION_LOOP *loop = &pl->char_anim.anim->loops[pl->char_anim.loop];
        if ((pl->char_anim.frame >> 8) >= loop->length) {
            set_state(pl, PLAYER_STATE_FALL2);
        }
    }
    player_update_sprite_info();
}

void player_control_init(void)
{
    struct GAME_PLAYER_CONTROL *plc = &game_state.player_control;
    plc->dx = 0;
    plc->dy = 0;
}

void player_control_update(struct JOYSTICK *joy)
{
    struct GAME_PLAYER *pl = &game_state.player;
    struct GAME_PLAYER_CONTROL *plc = &game_state.player_control;

    if (JOY_BTN_HELD(joy, JOY_BTN_A) || JOY_BTN_HELD(joy, JOY_BTN_Y)) {
        int weapon_index = JOY_BTN_HELD(joy, JOY_BTN_A) ? 1 : 0;
        const struct RAVEN_PLAYER_CHAR_WEAPON *weapon = &pl->player_char->weapons[weapon_index];
        if (pl->weapon_index != weapon_index) {
            pl->weapon_index = weapon_index;
            set_weapon_sprite_anim(weapon->shoot_anim_index);
            pl->shoot_wait_frames = weapon->bullet_initial_delay;
        }
        pl->weapon_frames_left = weapon->shoot_anim_len;
        if (pl->shoot_wait_frames == 0 && game_state.frame_counter - pl->last_shot_frame > weapon->bullet_interval) {
            if (weapon->bullet_initial_delay > 0) {
                pl->shoot_wait_frames = weapon->bullet_initial_delay;
            } else {
                pl->shoot_wait_frames = 1;  // will shoot immediatelly below
            }
        }
    }

    if (pl->shoot_wait_frames > 0) {
        pl->shoot_wait_frames -= 1;
        if (pl->shoot_wait_frames == 0) {
            pl->last_shot_frame = game_state.frame_counter;
            pl->shoot_now = 1;
        }
    }

    // walk
    if (pl->state == PLAYER_STATE_STAND || pl->state == PLAYER_STATE_WALK || pl->state == PLAYER_STATE_CROUCH) {
        if (pl->state != PLAYER_STATE_WALK && JOY_BTN_HELD(joy, JOY_BTN_RIGHT|JOY_BTN_LEFT)) {
            set_state(pl, PLAYER_STATE_WALK);
        } else if (pl->state == PLAYER_STATE_WALK && ! JOY_BTN_HELD(joy, JOY_BTN_RIGHT|JOY_BTN_LEFT)) {
            set_state(pl, PLAYER_STATE_STAND);
            //pl->char_anim.loop = RAVEN_SPRITE_ANIMATION_BUNNY_LOOP_STAND; // TODO: is this needed?
        }
        if (plc->dx != 0) {
            if (plc->dx > 0) {
                plc->dx -= DX_FLOOR_FRICTION;
                if (plc->dx < 0) plc->dx = 0;
            }
            if (plc->dx < 0) {
                plc->dx += DX_FLOOR_FRICTION;
                if (plc->dx > 0) plc->dx = 0;
            }
        }
    } else if (plc->dx != 0) {
        if (plc->dx > 0) {
            plc->dx -= DX_AIR_FRICTION;
            if (plc->dx < 0) plc->dx = 0;
        }
        if (plc->dx < 0) {
            plc->dx += DX_AIR_FRICTION;
            if (plc->dx > 0) plc->dx = 0;
        }
    }

    // change direction
    if (JOY_BTN_HELD(joy, JOY_BTN_RIGHT|JOY_BTN_LEFT)) {
        pl->direction = JOY_BTN_HELD(joy, JOY_BTN_RIGHT) ? RAVEN_DIR_RIGHT : RAVEN_DIR_LEFT;
    }

    // crouch
    if (pl->state == PLAYER_STATE_STAND && JOY_BTN_HELD(joy, JOY_BTN_DOWN)) {
        set_state(pl, PLAYER_STATE_CROUCH);
    }
    if (pl->state == PLAYER_STATE_CROUCH && JOY_BTN_HELD(joy, JOY_BTN_UP|JOY_BTN_B)) {
        set_state(pl, PLAYER_STATE_STAND);
    }

    // limit x speed
    if (JOY_BTN_HELD(joy, JOY_BTN_RIGHT)) {
        plc->dx += DX_ACCEL;
        if (plc->dx >= DX_MAX) plc->dx = DX_MAX;
    }
    if (JOY_BTN_HELD(joy, JOY_BTN_LEFT)) {
        plc->dx -= DX_ACCEL;
        if (plc->dx <= -DX_MAX) plc->dx = -DX_MAX;
    }

    // jump
    if ((pl->state == PLAYER_STATE_STAND || pl->state == PLAYER_STATE_WALK) && JOY_BTN_PRESSED(joy, JOY_BTN_B)) {
        plc->dy = DY_JUMP_START;
        set_state(pl, PLAYER_STATE_JUMP);
    }

    // hold jump / start fall
    if (pl->state == PLAYER_STATE_JUMP) {
        if (JOY_BTN_HELD(joy, JOY_BTN_B) && plc->dy < 0) {
            plc->dy += DY_JUMP_HOLD;
        } else {
            set_state(pl, PLAYER_STATE_FALL);
        }
    }

    // apply gravity
    if (pl->state != PLAYER_STATE_STAND && pl->state != PLAYER_STATE_WALK && pl->state != PLAYER_STATE_CROUCH) {
        plc->dy += DY_GRAVITY;
    }
    if (plc->dy >= DY_MAX) plc->dy = DY_MAX;
}
