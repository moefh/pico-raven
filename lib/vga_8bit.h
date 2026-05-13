#ifndef VGA_8BIT_H_FILE
#define VGA_8BIT_H_FILE

#include <stdbool.h>
#include <stdint.h>

#define VGA_ERROR_ALLOC     (-1)

#ifdef __cplusplus
extern "C" {
#endif

struct VGA_MODE {
    uint32_t pixel_clock_mhz;

    uint16_t h_front_porch;
    uint16_t h_sync_pulse;
    uint16_t h_back_porch;
    uint16_t h_pixels;

    uint16_t v_front_porch;
    uint16_t v_sync_pulse;
    uint16_t v_back_porch;
    uint16_t v_pixels;
    uint8_t  v_div;

    uint8_t  h_polarity;
    uint8_t  v_polarity;
};

struct VGA_SCREEN {
    int32_t width;
    int32_t height;
    union {
        uint32_t **lines;
        uint8_t **lines8;
    };
    union {
        uint32_t *framebuffer;
        uint8_t *framebuffer8;
    };
};

int vga_init(const struct VGA_MODE *mode, uint32_t pio_num, uint32_t pin_out_base, bool round_clock_div);
void vga_clear_screen(uint8_t color);
void vga_swap_buffers(bool wait_sync);
uint32_t vga_get_nearest_sys_clock_hz(const struct VGA_MODE *mode, uint32_t mult_boost);

extern struct VGA_SCREEN vga_screen;

extern const struct VGA_MODE vga_mode_320x240;

#ifdef __cplusplus
}
#endif

#endif /* VGA_8BIT_H_FILE */
