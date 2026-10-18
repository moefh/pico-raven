#include <stdio.h>

#include "room.h"

ADD_ROOM_SCRIPT(west__buntown_gate);

static void init_room(uint32_t room_id, struct GAME_STATE *game)
{
    default_room_init(room_id, game);
}

static void update_room(struct GAME_STATE *game)
{
    default_room_update(game);
}
