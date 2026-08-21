#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"

#include "run_state.h"
#include "game.h"

struct GAME_RUN_STATE run_state;

void run_state_init(void)
{
    memset(&run_state, 0, sizeof(struct GAME_RUN_STATE));

    run_state.mod.index = RAVEN_MOD_ID_BWV_106;
    run_state.mod.volume = 0x40;
}
