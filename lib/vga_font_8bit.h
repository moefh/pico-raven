#ifndef VGA_FONT_8BIT_H_FILE
#define VGA_FONT_8BIT_H_FILE

#include <stdint.h>
#include <stdbool.h>

#define VGA_FONT_USE_STDARG 1

struct VGA_FONT {
    uint8_t w;
    uint8_t h;
    const uint8_t *data;
};

struct VGA_PROP_FONT {
    uint8_t h;
    const uint8_t *data;
    uint8_t char_width[96];
    uint16_t char_offset[96];
};

enum FONT_ALIGNMENT {
    FONT_ALIGN_LEFT,
    FONT_ALIGN_CENTER,
    FONT_ALIGN_RIGHT
};

int32_t font_measure(const char *text);
void font_print_int(int num);
void font_print_uint(unsigned int num);
void font_print_float(float num);
void font_print(const char *text);

#if VGA_FONT_USE_STDARG
void font_printf(const char *fmt, ...)
    __attribute__ ((format (printf, 1, 2)));
#endif

extern const struct VGA_FONT *vga_font;
extern const struct VGA_PROP_FONT *vga_pfont;
extern int32_t vga_font_x, vga_font_y;
extern enum FONT_ALIGNMENT vga_font_alignment;
extern uint8_t vga_font_is_prop;
extern uint8_t vga_font_color;
extern uint8_t vga_font_border_enable;
extern uint8_t vga_font_border_color;

static inline const struct VGA_FONT *font_set_font(const struct VGA_FONT *new_font) {
    const struct VGA_FONT *old_font = vga_font;
    vga_font = new_font;
    return old_font;
}

static inline const struct VGA_PROP_FONT *font_set_prop_font(const struct VGA_PROP_FONT *new_pfont) {
    const struct VGA_PROP_FONT *old_pfont = vga_pfont;
    vga_pfont = new_pfont;
    return old_pfont;
}

static inline bool font_set_is_prop(bool is_prop) {
    bool old_is_prop = vga_font_is_prop;
    vga_font_is_prop = is_prop;
    return old_is_prop;
}

static inline uint8_t font_set_color(uint8_t color) {
    uint8_t old_color = vga_font_color;
    vga_font_color = color;
    return old_color;
}

static inline bool font_set_border(bool enable, uint8_t color) {
    bool old_enable = vga_font_border_enable;
    vga_font_border_enable = enable;
    vga_font_border_color = color;
    return old_enable;
}

static inline void font_move(int32_t x, int32_t y) {
    vga_font_x = x;
    vga_font_y = y;
}

static inline void font_align(enum FONT_ALIGNMENT alignment) {
    vga_font_alignment = alignment;
}

#endif /* VGA_FONT_8BIT_H_FILE */
