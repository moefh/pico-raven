#ifndef VGA_FONT_8BIT_H_FILE
#define VGA_FONT_8BIT_H_FILE

#include <stdint.h>
#include <stdbool.h>

#define VGA_FONT_USE_STDARG 1

#ifdef __cplusplus
extern "C" {
#endif

struct VGA_FONT {
    uint8_t w;
    uint8_t h;
    const uint8_t *data;
};

struct VGA_PROP_FONT {
    uint8_t height;
    const uint8_t *data;
    uint8_t char_width[96];
    uint16_t char_offset[96];
};

enum FONT_ALIGNMENT {
    FONT_ALIGN_LEFT,
    FONT_ALIGN_CENTER,
    FONT_ALIGN_RIGHT
};

void font_set_font(const struct VGA_FONT *font);
void font_set_color(uint8_t color);
void font_set_border(bool enable, uint8_t color);
void font_move(uint32_t x, uint32_t y);
void font_align(enum FONT_ALIGNMENT alignment);

void font_print_int(int num);
void font_print_uint(unsigned int num);
void font_print_float(float num);
void font_print(const char *text);

#if VGA_FONT_USE_STDARG
void font_printf(const char *fmt, ...)
    __attribute__ ((format (printf, 1, 2)));
#endif

#ifdef __cplusplus
}
#endif

#endif /* VGA_FONT_8BIT_H_FILE */
