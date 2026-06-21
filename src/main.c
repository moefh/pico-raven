#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "tusb.h"

#include "config.h"

#include "lib/joystick.h"
#include "lib/joy_wii_i2c.h"
#include "lib/sysinfo.h"
#include "lib/compilation_timestamp.h"

#include "core_msg.h"
#include "screen.h"
#include "game.h"

void core1_main(void);

struct JOYSTICK joy;

static void led_init(void)
{
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
}

static void wait_for_usb(uint32_t timeout)
{
    uint32_t start = to_ms_since_boot(get_absolute_time());
    while (! tud_cdc_connected()) {
        uint32_t cur = to_ms_since_boot(get_absolute_time());
        if (cur > start + timeout) {
            break;
        }
        sleep_ms(250);
    }
}

int main(void)
{
    stdio_init_all();
    wait_for_usb(3000);
    printf("\n==== starting ================\n");
    printf("version: %llx\n", get_compilation_timestamp());
    printf("pre-init free mem: %lu/%lu\n", sysinfo_heap_free(), sysinfo_heap_total());

    core_msg_init();
    multicore_launch_core1(core1_main);
    multicore_fifo_pop_blocking();  // wait for core1 to start
    multicore_fifo_push_blocking(0);

    led_init();
    joy_wii_i2c_init(&joy, JOY_I2C_NUM, JOY_SDA_PIN, JOY_SCL_PIN);
    msg_audio_init(SND_PIO_NUM, SND_PIN_BASE, SND_PIN_BASE+1, SND_SAMPLE_FREQUENCY);

    if (screen_init() < 0) {
        return 1;
    }

    printf("pos-init free mem: %lu/%lu\n", sysinfo_heap_free(), sysinfo_heap_total());
    game_main_loop();
}
