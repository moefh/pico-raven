#ifndef SPRITE_SHADOW_H_FILE
#define SPRITE_SHADOW_H_FILE

#include <stdint.h>
#include "game_data.h"

#define SPRITE_SHADOW_ENABLE_BITMAP 0  // 1 to enable bitmap generation in RAM, 0 to use original sprite transparency
#define SPRITE_SHADOW_NUM_FRAMES    8

void add_sprite_shadow_frame(int frame, int x, int y);

#if SPRITE_SHADOW_ENABLE_BITMAP
void gen_sprite_shadow_image(uint8_t *sprite_shadow_image, const struct RAVEN_IMAGE *sprite);
void draw_sprite_shadow_bitmap(const uint8_t *sprite_shadow_image, const struct RAVEN_IMAGE *sprite,
                               int scr_x, int scr_y, const uint8_t *colors);
#else
void draw_sprite_shadow(const struct RAVEN_IMAGE *sprite, int scr_x, int scr_y, const uint8_t *colors);
#endif

#endif /* SPRITE_SHADOW_H_FILE */
