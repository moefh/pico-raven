#include <stdio.h>

#include "room.h"

ADD_ROOM_SCRIPT(intro);

static void init_intro(uint32_t room_id, struct GAME_STATE *game)
{
    printf("init room: id=%d, %p\n", (int)room_id, game);
}
