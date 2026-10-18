#include <stdio.h>

#include "default.h"
#include "room.h"

void default_room_init(uint32_t room_id, struct GAME_STATE *game)
{
    const struct RAVEN_ROOM *room = &raven_rooms[room_id];
    run_state.num_pickup_respawns = 0;
    for (int t = 0; t < room->num_triggers; t++) {
        const struct RAVEN_ROOM_TRIGGER_INFO *tr = &room->triggers[t];
        switch (tr->type) {
        case RAVEN_ROOM_TRIGGER_TYPE_PLAYER_SPAWN:
            game->player.x = tr->x + (4*TILE_SIZE - game->player.anim->collision.w) / 2;
            game->player.y = tr->y + 4*TILE_SIZE - game->player.anim->collision.h;
            game->player.direction = tr->player_spawn.direction;
            break;

        case RAVEN_ROOM_TRIGGER_TYPE_GET_PICKUP:
            if (run_state.num_pickup_respawns < RUN_STATE_MAX_ROOM_PICKUPS) {
                struct PICKUP_RESPAWN *respawn = &run_state.pickup_respawn[run_state.num_pickup_respawns++];
                respawn->trigger_id = tr->trigger_id;
                respawn->delay = 0;
            }
            break;
        }
    }
    run_state_spawn_room_enemies(game, 0xffff);
}

void default_room_update(struct GAME_STATE *game)
{
    run_state_update_room_enemies(game);
    run_state_advance_pickup_trigger_respawns();
}
