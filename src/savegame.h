#ifndef SAVEGAME_H_FILE
#define SAVEGAME_H_FILE

#include "game.h"

int savegame_write(const struct GAME_STATE *game, int slot);
int savegame_read(struct GAME_STATE *game, int slot);

#endif /* SAVEGAME_H_FILE */
