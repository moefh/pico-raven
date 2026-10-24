#include <stdio.h>

#include "default.h"
#include "room.h"

void default_room_init(uint32_t room_id)
{
    const struct RAVEN_ROOM *room = &raven_rooms[room_id];
    game_state.num_pickup_respawns = 0;
    for (int t = 0; t < room->num_triggers; t++) {
        const struct RAVEN_ROOM_TRIGGER_INFO *tr = &room->triggers[t];
        switch (tr->type) {
        case RAVEN_ROOM_TRIGGER_TYPE_PLAYER_SPAWN:
            game_state.player.x = tr->x + (4*TILE_SIZE - game_state.player.anim->collision.w) / 2;
            game_state.player.y = tr->y + 4*TILE_SIZE - game_state.player.anim->collision.h;
            game_state.player.direction = tr->player_spawn.direction;
            break;

        case RAVEN_ROOM_TRIGGER_TYPE_GET_PICKUP:
            if (game_state.num_pickup_respawns < GAME_STATE_MAX_ROOM_PICKUPS) {
                struct PICKUP_RESPAWN *respawn = &game_state.pickup_respawn[game_state.num_pickup_respawns++];
                respawn->trigger_id = tr->trigger_id;
                respawn->delay = 0;
            }
            break;
        }
    }
    game_state_spawn_room_enemies(0xffff);
}

void default_room_update(void)
{
    game_state_update_room_enemies();
    game_state_advance_pickup_trigger_respawns();
}
