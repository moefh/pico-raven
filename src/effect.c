#include <stdio.h>
#include <string.h>

#include "effect.h"
#include "game.h"

static struct GAME_EFFECT *effect_spawn(void)
{
    int effect_index = game_state.num_effects;
    if (effect_index >= GAME_STATE_MAX_EFFECTS) {
        return NULL;
    }
    game_state.num_effects++;
    return &game_state.effects[effect_index];
}

void effect_spawn_animation(int x, int y, uint16_t set_effect_id, uint16_t set_flag_id,
                            const struct RAVEN_SPRITE_ANIMATION *anim)
{
    struct GAME_EFFECT *effect = effect_spawn();
    if (effect == NULL) { return; }

    effect->x = x;
    effect->y = y;
    effect->direction = RAVEN_DIR_RIGHT;
    effect->set_effect_id = set_effect_id;
    effect->set_flag_id = set_flag_id;
    effect->anim.anim = anim;
    effect->anim.sprite = &raven_sprites[effect->anim.anim->sprite_index];
    effect->anim.loop = 0;
    effect->anim.frame = 0;
    game_update_animation(&effect->anim, effect->x, effect->y, effect->direction);
}

void effect_spawn_for_weapon(int x, int y, int direction, int weapon_index)
{
    struct GAME_EFFECT *effect = effect_spawn();
    if (effect == NULL) { return; }
    const struct RAVEN_PLAYER_CHAR_WEAPON *weapon = &game_state.player.player_char->weapons[weapon_index];
    const struct RAVEN_SPRITE_ANIMATION *hit_anim = get_sprite_animation(weapon->bullet_hit_anim_index);
    if (hit_anim == NULL) return;

    effect->x = x - hit_anim->collision.w / 2;
    effect->y = y - hit_anim->collision.h / 2;
    effect->direction = direction;
    effect->set_effect_id = 0xffff;
    effect->set_flag_id = 0xffff;
    effect->anim.anim = hit_anim;
    effect->anim.sprite = &raven_sprites[hit_anim->sprite_index];
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
            if (effect->set_effect_id != 0xffff) {
                game_state_activate_effect(effect->set_effect_id);
            }
            if (effect->set_flag_id != 0xffff) {
                game_state_set_flag(effect->set_flag_id);
            }
            remove_effect(i);
            i--;
        } else {
            game_update_animation(&effect->anim, effect->x, effect->y, effect->direction);
        }
    }
}
