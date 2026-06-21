#include <stdio.h>

#include "room.h"

ADD_ROOM_SCRIPT(intro);

static void intro_init(uint32_t room_id, struct GAME_STATE *game)
{
    printf("init room: id=%d, %p\n", (int)room_id, game);

    int num_triggers = game->room->num_triggers;
    for (int t = 0; t < num_triggers; t++) {
        const struct RAVEN_ROOM_TRIGGER_INFO *tr = &game->room->triggers[t];
        if (tr->type == RAVEN_ROOM_TRIGGER_TYPE_PLAYER_SPAWN) {
            printf("spawning player\n");
            game->player.x = tr->x;
            game->player.y = tr->y + 64 - game->player.anim->collision.h;
            game->player.direction = tr->player_spawn.direction;
            game->screen_x = 0;
            game->screen_y = 0;
            break;
        }
    }
}
