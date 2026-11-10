#include <stdio.h>
#include <string.h>

#include "effect.h"
#include "game.h"

void effect_spawn(int x, int y, int direction, int weapon_index)
{
    const struct RAVEN_PLAYER_CHAR_WEAPON *weapon = &game_state.player.player_char->weapons[weapon_index];

    int effect_index = game_state.num_effects;
    if (effect_index >= GAME_STATE_MAX_EFFECTS) { return; }
    game_state.num_effects++;

    struct GAME_EFFECT *effect = &game_state.effects[effect_index];
    effect->x = x - weapon->bullet_hit_anim->collision.w / 2;
    effect->y = y - weapon->bullet_hit_anim->collision.h / 2;
    effect->direction = direction;
    effect->anim.anim = weapon->bullet_hit_anim;
    effect->anim.sprite = weapon->bullet_hit_anim->sprite;
    effect->anim.loop = 0;
    effect->anim.frame = 0;
    game_update_animation(&effect->anim, effect->x, effect->y, effect->direction);
}

static void remove_effect(int index)
{
    memmove(&game_state.effects[index],
            &game_state.effects[index+1],
            sizeof(struct GAME_EFFECT) * (game_state.num_effects-(index+1)));
    game_state.num_effects--;
}

void effect_update_all(void)
{
    for (int i = 0; i < game_state.num_effects; i++) {
        struct GAME_EFFECT *effect = &game_state.effects[i];
        effect->anim.frame += effect->anim.anim->loops[effect->anim.loop].frame_adv + 1; // [0-255] -> [1-256]
        const struct RAVEN_SPRITE_ANIMATION_LOOP *loop = &effect->anim.anim->loops[effect->anim.loop];
        if ((effect->anim.frame >> 8) >= loop->length) {
            remove_effect(i);
            i--;
        } else {
            game_update_animation(&effect->anim, effect->x, effect->y, effect->direction);
        }
    }
}
