#ifndef SAVEGAME_H_FILE
#define SAVEGAME_H_FILE

#include <stdint.h>

struct SAVEGAME {
    uint16_t room_id;
};

int savegame_write(const struct SAVEGAME *savegame, int slot);
int savegame_read(struct SAVEGAME *savegame, int slot);

#endif /* SAVEGAME_H_FILE */
