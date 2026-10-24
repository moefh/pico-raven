#include <stdio.h>

#include "room.h"

ADD_ROOM_SCRIPT(west__buntown_gate);

static void init_room(uint32_t room_id)
{
    default_room_init(room_id);
}

static void update_room(void)
{
    default_room_update();
}
