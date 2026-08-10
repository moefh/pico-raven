#include <stdio.h>

#include "player.h"
#include "collision.h"

#include "lib/mem.h"

#define DX_ACCEL      ((int32_t) 0x100)
#define DX_FRICTION   ((int32_t) 0x0c0)
#define DX_MAX        ((int32_t) 0x700)

#define DY_GRAVITY    ((int32_t) 0x0c0)
#define DY_MAX        ((int32_t) 0x900)
#define DY_JUMP_START ((int32_t)-0xa00)
#define DY_JUMP_HOLD  ((int32_t)-0x060)

static void update_sprite_info(struct RAVEN_PLAYER *pl)
{
    switch (pl->state) {
    case PLAYER_STATE_STAND: pl->anim_loop = RAVEN_SPRITE_ANIMATION_BUNNY_LOOP_STAND; break;
    case PLAYER_STATE_WALK:  pl->anim_loop = RAVEN_SPRITE_ANIMATION_BUNNY_LOOP_RUN; break;
    case PLAYER_STATE_JUMP:  pl->anim_loop = RAVEN_SPRITE_ANIMATION_BUNNY_LOOP_JUMP; break;
    case PLAYER_STATE_FALL:  pl->anim_loop = RAVEN_SPRITE_ANIMATION_BUNNY_LOOP_FALL; break;
    }
    const struct RAVEN_SPRITE_ANIMATION_LOOP *loop = &pl->anim->loops[pl->anim_loop];
    int loop_frame = pl->anim_frame >> 8;
    if (loop_frame >= loop->length) {
        if (loop->dont_loop) {
            loop_frame = loop->length - 1;
            pl->anim_frame = (loop_frame) << 8;
        } else {
            loop_frame = 0;
            pl->anim_frame &= 0xff;
        }
    }
    pl->sprite_frame = pl->anim->frame_indices[loop->offset + loop_frame];
    if (pl->direction == RAVEN_DIR_LEFT) {
        pl->sprite_frame += pl->sprite->num_frames/2;
        pl->sprite_x = pl->x - (pl->sprite->width - (pl->anim->collision.x + pl->anim->collision.w));
        pl->sprite_y = pl->y - pl->anim->collision.y;
    } else {
        pl->sprite_x = pl->x - pl->anim->collision.x;
        pl->sprite_y = pl->y - pl->anim->collision.y;
    }
    if (pl->shadow_enabled) {
        sprite_shadow_add_frame(pl->sprite_frame, pl->sprite_x, pl->sprite_y);
    }
}

void player_init(struct RAVEN_PLAYER *pl)
{
    const struct RAVEN_SPRITE_ANIMATION *anim = &raven_sprite_animations[RAVEN_SPRITE_ANIMATION_ID_BUNNY];

    pl->sprite = anim->sprite;
    pl->anim = anim;
    pl->x = 0;
    pl->y = 0;
    pl->state = PLAYER_STATE_STAND;
    pl->direction = RAVEN_DIR_RIGHT;
    pl->anim_loop = RAVEN_SPRITE_ANIMATION_BUNNY_LOOP_STAND;
    pl->anim_frame = 0;
    pl->shadow_enabled = 0;

    update_sprite_info(pl);
}

void player_update(struct RAVEN_PLAYER *pl, struct RAVEN_PLAYER_CONTROL *plc)
{
    //printf("update player: at (%ld,%ld), delta=0x(%lx,%lx)=(%ld,%ld)\n", pl->x, pl->y, plc->dx, plc->dy, plc->dx>>8, plc->dy>>8);
    int dx = plc->dx >> 8;
    int dy = plc->dy >> 8;
    struct COLLISION_RECT rect = {
        .x = pl->x,
        .y = pl->y,
        .w = pl->anim->collision.w,
        .h = pl->anim->collision.h,
    };
    int collision = collision_move(&rect, dx, dy);
    if (collision & COLLISION_FLAGS_DOWN) {
        plc->dy = 0;
        if (JOY_BTN_HELD(&joy, JOY_BTN_RIGHT|JOY_BTN_LEFT)) {
            pl->state = PLAYER_STATE_WALK;
        } else {
            pl->state = PLAYER_STATE_STAND;
        }
        pl->anim_frame = 0;
    } else if (collision & COLLISION_FLAGS_UP) {
        if (plc->dy < 0) {
            plc->dy = 0;
        }
        if (pl->state == PLAYER_STATE_JUMP) {
            pl->state = PLAYER_STATE_FALL;
            pl->anim_frame = 0;
        }
    } else if (pl->state == PLAYER_STATE_STAND || pl->state == PLAYER_STATE_WALK) {
        int y = rect.y;
        if (collision_move(&rect, 0, 1) == 0) {
            pl->state = PLAYER_STATE_FALL;
            pl->anim_frame = 0;
        }
        rect.y = y;
    }

    pl->x = rect.x;
    pl->y = rect.y;
    pl->anim_frame += pl->anim->loops[pl->anim_loop].frame_adv + 1; // [0-255] -> [1-256]
    update_sprite_info(pl);
}

void player_control_init(struct RAVEN_PLAYER_CONTROL *plc)
{
    plc->dx = 0;
    plc->dy = 0;
}

void player_control_update(struct RAVEN_PLAYER_CONTROL *plc, struct RAVEN_PLAYER *pl)
{
    // walk
    if (pl->state == PLAYER_STATE_STAND || pl->state == PLAYER_STATE_WALK) {
        if (JOY_BTN_PRESSED(&joy, JOY_BTN_RIGHT) || JOY_BTN_PRESSED(&joy, JOY_BTN_LEFT)) {
            pl->state = PLAYER_STATE_WALK;
            pl->anim_frame = 0;
        } else if (pl->state != PLAYER_STATE_STAND && ! JOY_BTN_HELD(&joy, JOY_BTN_RIGHT|JOY_BTN_LEFT)) {
            pl->state = PLAYER_STATE_STAND;
            pl->anim_loop = RAVEN_SPRITE_ANIMATION_BUNNY_LOOP_STAND;
            pl->anim_frame = 0;
        }
        if (plc->dx != 0) {
            if (plc->dx > 0) {
                plc->dx -= DX_FRICTION;
                if (plc->dx < 0) plc->dx = 0;
            }
            if (plc->dx < 0) {
                plc->dx += DX_FRICTION;
                if (plc->dx > 0) plc->dx = 0;
            }
        }
    }

    // change direction
    if (JOY_BTN_PRESSED(&joy, JOY_BTN_RIGHT) || JOY_BTN_PRESSED(&joy, JOY_BTN_LEFT)) {
        pl->direction = JOY_BTN_HELD(&joy, JOY_BTN_RIGHT) ? RAVEN_DIR_RIGHT : RAVEN_DIR_LEFT;
    }

    // limit x speed
    if (JOY_BTN_HELD(&joy, JOY_BTN_RIGHT)) {
        plc->dx += DX_ACCEL;
        if (plc->dx >= DX_MAX) plc->dx = DX_MAX;
    }
    if (JOY_BTN_HELD(&joy, JOY_BTN_LEFT)) {
        plc->dx -= DX_ACCEL;
        if (plc->dx <= -DX_MAX) plc->dx = -DX_MAX;
    }

    // jump
    if ((pl->state == PLAYER_STATE_STAND || pl->state == PLAYER_STATE_WALK) && JOY_BTN_PRESSED(&joy, JOY_BTN_A)) {
        plc->dy = DY_JUMP_START;
        pl->state = PLAYER_STATE_JUMP;
        pl->anim_frame = 0;
    }

    // hold jump / start fall
    if (pl->state == PLAYER_STATE_JUMP) {
        if (JOY_BTN_HELD(&joy, JOY_BTN_A) && plc->dy < 0) {
            plc->dy += DY_JUMP_HOLD;
        } else {
            pl->state = PLAYER_STATE_FALL;
            pl->anim_frame = 0;
        }
    }

    // apply gravity
    if (pl->state != PLAYER_STATE_STAND && pl->state != PLAYER_STATE_WALK) {
        plc->dy += DY_GRAVITY;
    }
    if (plc->dy >= DY_MAX) plc->dy = DY_MAX;
}
