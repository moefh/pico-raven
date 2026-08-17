#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"

#include "run_state.h"
#include "game.h"

struct GAME_RUN_STATE run_state;

void run_state_init(void)
{
    run_state.display.show_perf = 0;
    run_state.display.msg_load_frames_left = 0;
    run_state.display.msg_save_frames_left = 0;
    run_state.display.msg_mod_event_frames_left = 0;

    run_state.mod.index = RAVEN_MOD_ID_BWV_106;
    run_state.mod.volume = 0x40;
}
