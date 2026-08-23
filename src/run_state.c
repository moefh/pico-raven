#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"

#include "run_state.h"
#include "game.h"
#include "config.h"

struct GAME_RUN_STATE run_state;

void run_state_init(void)
{
    memset(&run_state, 0, sizeof(struct GAME_RUN_STATE));

    run_state.mod.index = RAVEN_MOD_ID_BWV_106;
    run_state.mod.volume = 0x40;
}

void run_state_fps_count(void)
{
    static uint32_t last_millis;
    static uint16_t frame_count;

    uint32_t cur_millis = to_ms_since_boot(get_absolute_time());
    if (cur_millis/1000 != last_millis/1000) {
        run_state.fps_count = frame_count;
        frame_count = 1;
    } else {
        frame_count++;
    }
    last_millis = cur_millis;
}

void run_state_blink_led(void)
{
    if (run_state.led.frames-- <= 0) {
        run_state.led.frames = 15;
        run_state.led.state = !run_state.led.state;
        gpio_put(LED_PIN, run_state.led.state);
    }
}
