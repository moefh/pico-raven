#include <stdio.h>
#include <string.h>

#include "bullet.h"
#include "effect.h"
#include "collision.h"
#include "game.h"

#include "lib/vga_8bit.h"

void bullet_spawn(int weapon_index)
{
    const struct RAVEN_PLAYER_CHAR_WEAPON *weapon = &game_state.player.player_char->weapons[weapon_index];

    int bullet_index = game_state.num_bullets;
    if (bullet_index >= GAME_STATE_MAX_BULLETS) { return; }
    game_state.num_bullets++;

    struct GAME_PLAYER *pl = &game_state.player;

    struct GAME_BULLET *bullet = &game_state.bullets[bullet_index];
    int x = pl->x + ((pl->direction != 0) ? -weapon->bullet_x : weapon->bullet_x);
    int y = pl->y + weapon->bullet_y;
    if (pl->state == PLAYER_STATE_CROUCH) {
        x += (pl->direction != 0) ? -weapon->crouch_x : weapon->crouch_x;
        y += weapon->crouch_y;
    }
    bullet->weapon_index = weapon_index;
    bullet->x = x << 8;
    bullet->y = y << 8;
    bullet->dx = weapon->bullet_dx * ((pl->direction != 0) ? -1 : 1);
    bullet->dy = weapon->bullet_dy;
    bullet->direction = pl->direction;
    bullet->anim.anim = weapon->bullet_anim;
    bullet->anim.sprite = weapon->bullet_anim->sprite;
    bullet->anim.loop = 0;
    bullet->anim.frame = 0;
    game_update_animation(&bullet->anim, bullet->x>>8, bullet->y>>8, bullet->direction);
}

static void remove_bullet(int index)
{
    memmove(&game_state.bullets[index],
            &game_state.bullets[index+1],
            sizeof(struct GAME_BULLET) * (game_state.num_bullets-(index+1)));
    game_state.num_bullets--;
}

void bullet_update_all(void)
{
    for (int i = 0; i < game_state.num_bullets; i++) {
        struct GAME_BULLET *bullet = &game_state.bullets[i];
        bullet->x += bullet->dx;
        bullet->y += bullet->dy;
        struct COLLISION_RECT rect = {
            .x = bullet->x >> 8,
            .y = bullet->y >> 8,
            .w = bullet->anim.anim->collision.w,
            .h = bullet->anim.anim->collision.h,
        };
        int collided = 0;

        // check collision with enemies
        if ((bullet->y>>8) > game_state.screen_y - 64 &&
            (bullet->y>>8) < game_state.screen_y + vga_screen.height - SCREEN_HEADER_HEIGHT + 64) {
            int x_start = bullet->x >> (8 + 12);
            int x_end = (bullet->x + bullet->anim.anim->collision.w) >> (8 + 12);
            if (x_start < 0) x_start = 0; else if (x_start >= 32) x_start = 31;
            if (x_end   < 0) x_end   = 0; else if (x_end   >= 32) x_end   = 31;
            uint32_t bullet_collision_bits = (1<<x_start) | (1<<x_end);
            for (int ei = 0; ei < game_state.num_enemies; ei++) {
                if ((game_state.enemy_collision_bits[ei] & bullet_collision_bits) != 0) {
                    struct GAME_ENEMY *enemy = &game_state.enemies[ei];
                    struct COLLISION_RECT enemy_rect = {
                        .x = enemy->x,
                        .y = enemy->y,
                        .w = enemy->anim.anim->collision.w,
                        .h = enemy->anim.anim->collision.h,
                    };
                    if (collision_rects_intersect(&rect, &enemy_rect)) {
                        const struct RAVEN_PLAYER_CHAR_WEAPON *weapon = &game_state.player.player_char->weapons[bullet->weapon_index];
                        int health = enemy->health - weapon->damage;
                        enemy->health = (health < 0) ? 0 : health;
                        enemy->hit_frames_left = 6;
                        collided = 1;
                        break;
                    }
                }
            }
        }

        // check collision with room
        if (! collided && room_tiles_block_rect(&rect)) {
            collided = 1;
        }

        if (collided) {
            int x = (bullet->x >> 8) + bullet->anim.anim->collision.w / 2;
            int y = (bullet->y >> 8) + bullet->anim.anim->collision.h / 2;
            effect_spawn(x, y, 0, bullet->weapon_index);
            remove_bullet(i);
            i--;
        } else {
            bullet->anim.frame += bullet->anim.anim->loops[bullet->anim.loop].frame_adv + 1; // [0-255] -> [1-256]
            game_update_animation(&bullet->anim, bullet->x>>8, bullet->y>>8, bullet->direction);
        }
    }
}
