#ifndef EFFECT_H_FILE
#define EFFECT_H_FILE

#include <stdint.h>

struct RAVEN_SPRITE_ANIMATION;

void effect_spawn_animation(int x, int y, uint16_t set_effect_id, uint16_t set_flag_id,
                            const struct RAVEN_SPRITE_ANIMATION *anim);
void effect_spawn_for_weapon(int x, int y, int direction, int weapon_index);
void effect_update_all(void);

#endif /* EFFECT_H_FILE */
