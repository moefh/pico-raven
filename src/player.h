#ifndef PLAYER_H_FILE
#define PLAYER_H_FILE

#include <stdint.h>
#include "game_data.h"
#include "joy.h"
#include "sprite_shadow.h"

enum PLAYER_STATE {
    PLAYER_STATE_STAND,
    PLAYER_STATE_WALK,
    PLAYER_STATE_JUMP,
    PLAYER_STATE_FALL,
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
    uint8_t state;
    uint8_t direction;
    uint8_t shadow_enabled;
    uint8_t anim_loop;
    uint16_t anim_frame;  // 10.6 fixpoint

    int16_t sprite_x;
    int16_t sprite_y;
    uint8_t sprite_frame;
};

void player_init(struct RAVEN_PLAYER *ch);
void player_update_sprite_info(struct RAVEN_PLAYER *ch);
void player_update(struct RAVEN_PLAYER *ch, struct RAVEN_PLAYER_CONTROL *ct);

void player_control_init(struct RAVEN_PLAYER_CONTROL *ct);
void player_control_update(struct RAVEN_PLAYER_CONTROL *ct, struct RAVEN_PLAYER *ch);

#endif /* PLAYER_H_FILE */
