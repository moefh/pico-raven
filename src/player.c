#include <stdio.h>

#include "player.h"

#include "lib/mem.h"

#define ANIM_ADV_STEPS (1<<4)
#define DX_ACCEL     ((int32_t) 0x100)
#define DX_FRICTION  ((int32_t) 0x0c0)
#define DX_MAX       ((int32_t) 0x700)

#if SPRITE_SHADOW_ENABLE_BITMAP
static uint8_t sprite_shadow_data[(RAVEN_SPRITE_WIDTH_BUNNY+7)/8 * RAVEN_SPRITE_HEIGHT_BUNNY * RAVEN_SPRITE_FRAMES_BUNNY];

static uint8_t *init_player_shadow(const struct RAVEN_IMAGE *sprite)
{
    size_t data_size = ((sprite->width+7)/8) * sprite->height * sprite->num_frames;
    if (data_size > sizeof(sprite_shadow_data)) {
        printf("WARNING: not enough sprite shadow data for player (%zu > %zu)\n", data_size, sizeof(sprite_shadow_data));
    }

    gen_sprite_shadow_image(sprite_shadow_data, sprite);

    return sprite_shadow_data;
}
#endif /* SPRITE_SHADOW_ENABLE_BITMAP */

void player_init(struct RAVEN_PLAYER *pl)
{
    const struct RAVEN_SPRITE_ANIMATION *anim = &raven_sprite_animations[RAVEN_SPRITE_ANIMATION_ID_BUNNY];

    pl->sprite = anim->sprite;
    pl->anim = anim;
    pl->direction = RAVEN_DIR_RIGHT;
    pl->anim_loop = RAVEN_SPRITE_ANIMATION_BUNNY_LOOP_STAND;
    pl->anim_frame = 0;

#if SPRITE_SHADOW_ENABLE_BITMAP
    pl->sprite_shadow = init_player_shadow(pl->sprite);
#endif
}

void player_update_sprite_info(struct RAVEN_PLAYER *pl)
{
    pl->sprite_frame = pl->anim->frame_indices[pl->anim->loops[pl->anim_loop].offset + (pl->anim_frame>>6)];
    if (pl->direction == RAVEN_DIR_LEFT) {
        pl->sprite_frame += pl->sprite->num_frames/2;
        pl->sprite_x = pl->x - (pl->sprite->width - (pl->anim->collision.x + pl->anim->collision.w));
        pl->sprite_y = pl->y - pl->anim->collision.y;
    } else {
        pl->sprite_x = pl->x - pl->anim->collision.x;
        pl->sprite_y = pl->y - pl->anim->collision.y;
    }
    add_sprite_shadow_frame(pl->sprite_frame, pl->sprite_x, pl->sprite_y);
}

void player_advance_state_with_control(struct RAVEN_PLAYER *pl, struct RAVEN_PLAYER_CONTROL *plc)
{
    pl->anim_frame += ANIM_ADV_STEPS;
    if ((pl->anim_frame>>6) >= pl->anim->loops[pl->anim_loop].length) {
        pl->anim_frame = 0;
    }

    pl->x += plc->dx >> 8;
    pl->y += plc->dy >> 8;
}

void player_control_init(struct RAVEN_PLAYER_CONTROL *plc)
{
    plc->dx = 0;
    plc->dy = 0;
}

void player_control_update(struct RAVEN_PLAYER_CONTROL *plc, struct RAVEN_PLAYER *pl)
{
    if (JOY_BTN_PRESSED(&joy, JOY_BTN_RIGHT) || JOY_BTN_PRESSED(&joy, JOY_BTN_LEFT)) {
        pl->direction = JOY_BTN_HELD(&joy, JOY_BTN_RIGHT) ? RAVEN_DIR_RIGHT : RAVEN_DIR_LEFT;
        pl->anim_loop = RAVEN_SPRITE_ANIMATION_BUNNY_LOOP_RUN;
        pl->anim_frame = 0;
    }
    if ((plc->dx != 0 || pl->anim_loop != RAVEN_SPRITE_ANIMATION_BUNNY_LOOP_STAND) && ! JOY_BTN_HELD(&joy, JOY_BTN_RIGHT|JOY_BTN_LEFT)) {
        if (pl->anim_loop != RAVEN_SPRITE_ANIMATION_BUNNY_LOOP_STAND) {
            pl->anim_loop = RAVEN_SPRITE_ANIMATION_BUNNY_LOOP_STAND;
            pl->anim_frame = 0;
        }
        if (plc->dx > 0) {
            plc->dx -= DX_FRICTION;
            if (plc->dx < 0) plc->dx = 0;
        }
        if (plc->dx < 0) {
            plc->dx += DX_FRICTION;
            if (plc->dx > 0) plc->dx = 0;
        }
    }

    if (JOY_BTN_HELD(&joy, JOY_BTN_RIGHT)) {
        plc->dx += DX_ACCEL;
        if (plc->dx >= DX_MAX) plc->dx = DX_MAX;
    }
    if (JOY_BTN_HELD(&joy, JOY_BTN_LEFT)) {
        plc->dx -= DX_ACCEL;
        if (plc->dx <= -DX_MAX) plc->dx = -DX_MAX;
    }

#if 1    // TODO: remove this!
    if (JOY_BTN_HELD(&joy, JOY_BTN_DOWN)) {
        plc->dy += DX_ACCEL;
        if (plc->dy >= DX_MAX) plc->dy = DX_MAX;
    }
    if (JOY_BTN_HELD(&joy, JOY_BTN_UP)) {
        plc->dy -= DX_ACCEL;
        if (plc->dy < -DX_MAX) plc->dy = -DX_MAX;
    }
    if (plc->dy != 0) {
        if (plc->dy > 0) {
            plc->dy -= DX_FRICTION;
            if (plc->dy < 0) plc->dx = 0;
        }
        if (plc->dy < 0) {
            plc->dy += DX_FRICTION;
            if (plc->dy > 0) plc->dy = 0;
        }
    }
#endif
}
