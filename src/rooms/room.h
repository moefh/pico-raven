#ifndef ROOM_H_FILE
#define ROOM_H_FILE

#include "../game.h"
#include "../game_data.h"

#define ADD_ROOM_SCRIPT(name)                                         \
    static void init_room(uint32_t, struct GAME_STATE *);             \
    static void update_room(struct GAME_STATE *);                     \
    const struct RAVEN_ROOM_SCRIPT raven_room_script_table_##name = { \
        .init = init_room,                                            \
        .update = update_room,                                        \
    }

#endif
