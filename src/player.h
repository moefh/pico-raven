#ifndef PLAYER_H_FILE
#define PLAYER_H_FILE

#include <stdint.h>
#include "game_data.h"
#include "joy.h"
#include "sprite_shadow.h"

enum RAVEN_DIRECTION {
    RAVEN_DIR_RIGHT,
    RAVEN_DIR_LEFT,
};

struct RAVEN_PLAYER_CONTROL {
    int32_t dx;  // 24.8 fixpoint
    int32_t dy;
};

struct RAVEN_PLAYER {
    const struct VGA_IMAGE *sprite;
    const struct RAVEN_SPRITE_ANIMATION *anim;
    int32_t x;
    int32_t y;
    uint8_t direction;
    uint8_t anim_loop;
    uint16_t anim_frame;  // 10.6 fixpoint

    int16_t sprite_x;
    int16_t sprite_y;
    uint8_t sprite_frame;
#if SPRITE_SHADOW_ENABLE_BITMAP
    uint8_t *sprite_shadow;
#endif
};

void player_init(struct RAVEN_PLAYER *ch);
void player_update_sprite_info(struct RAVEN_PLAYER *ch);
void player_advance_state_with_control(struct RAVEN_PLAYER *ch, struct RAVEN_PLAYER_CONTROL *ct);

void player_control_init(struct RAVEN_PLAYER_CONTROL *ct);
void player_control_update(struct RAVEN_PLAYER_CONTROL *ct, struct RAVEN_PLAYER *ch);

#endif /* PLAYER_H_FILE */
