#ifndef ROOM_H_FILE
#define ROOM_H_FILE

#include "default.h"
#include "../game.h"
#include "../game_data.h"

#define ADD_ROOM_SCRIPT(name)                                         \
    static void init_room(uint32_t);                                  \
    static void update_room();                                        \
    const struct RAVEN_ROOM_SCRIPT raven_room_script_table_##name = { \
        .init = init_room,                                            \
        .update = update_room,                                        \
    }

#endif
