#ifndef SPRITE_SHADOW_H_FILE
#define SPRITE_SHADOW_H_FILE

#include <stdint.h>
#include "game_data.h"

#define SPRITE_SHADOW_NUM_FRAMES 8

void sprite_shadow_clear(void);
void sprite_shadow_add_frame(int frame, int x, int y);
void sprite_shadow_draw(const struct RAVEN_IMAGE *sprite, int scr_x, int scr_y, const uint8_t *colors);

#endif /* SPRITE_SHADOW_H_FILE */
