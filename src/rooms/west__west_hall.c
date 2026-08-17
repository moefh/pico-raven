#include "room.h"

ADD_ROOM_SCRIPT(west__west_hall);

static void init_room(uint32_t room_id, struct GAME_STATE *game)
{
    (void)room_id;
    game_spawn_room_enemies(game);
}

static void update_room(struct GAME_STATE *game)
{
    game_update_room_enemies(game);
}
