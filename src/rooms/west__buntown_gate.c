#include <stdio.h>

#include "room.h"

ADD_ROOM_SCRIPT(west__buntown_gate);

static void init_room(uint32_t room_id, struct GAME_STATE *game)
{
    const struct RAVEN_ROOM *room = &raven_rooms[room_id];
    int num_triggers = room->num_triggers;
    for (int t = 0; t < num_triggers; t++) {
        const struct RAVEN_ROOM_TRIGGER_INFO *tr = &room->triggers[t];
        if (tr->type == RAVEN_ROOM_TRIGGER_TYPE_PLAYER_SPAWN) {
            game->player.x = tr->x;
            game->player.y = tr->y + 4*TILE_SIZE - game->player.anim->collision.h;
            game->player.direction = tr->player_spawn.direction;
            break;
        }
    }

    game_spawn_room_enemies(game);
}

static void update_room(struct GAME_STATE *game)
{
    game_update_room_enemies(game);
}
