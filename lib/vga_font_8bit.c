#include <stdio.h>
#include <string.h>

#include "vga_font_8bit.h"
#include "vga_8bit.h"

#define FIRST_CHAR 32
#define NUM_CHARS  96

static char print_buf[32];
const struct VGA_FONT *vga_font;
uint32_t vga_font_x, vga_font_y;
enum FONT_ALIGNMENT vga_font_alignment;
uint8_t vga_font_color;
uint8_t vga_font_border[2];

#if VGA_FONT_USE_STDARG
#include <stdarg.h>

void font_printf(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(print_buf, sizeof(print_buf), fmt, ap);
    va_end(ap);

    font_print(print_buf);
}
#endif

void font_set_font(const struct VGA_FONT *new_font)
{
    vga_font = new_font;
}

void font_set_color(uint8_t color)
{
    vga_font_color = color;
}

void font_set_border(bool enable, uint8_t color)
{
    vga_font_border[0] = enable;
    vga_font_border[1] = color;
}

void font_move(uint32_t x, uint32_t y)
{
    vga_font_x = x;
    vga_font_y = y;
}

void font_align(enum FONT_ALIGNMENT alignment)
{
    vga_font_alignment = alignment;
}

void font_print_int(int num)
{
    snprintf(print_buf, sizeof(print_buf), "%d", num);
    font_print(print_buf);
}

void font_print_uint(unsigned int num)
{
    snprintf(print_buf, sizeof(print_buf), "%u", num);
    font_print(print_buf);
}

void font_print_float(float num)
{
    snprintf(print_buf, sizeof(print_buf), "%f", num);
    font_print(print_buf);
}

static int render_text(const char *text, int x, int y, unsigned int color)
{
    while (*text != '\0') {
        char ch = *text++;
        if (ch >= FIRST_CHAR && ch < FIRST_CHAR+NUM_CHARS) {
            ch -= FIRST_CHAR;
            for (int i = 0; i < vga_font->h; i++) {
                uint8_t char_line = vga_font->data[vga_font->h*ch + i];
                uint8_t char_bit = 1;
                for (int j = 0; j < vga_font->w; j++) {
                    if ((char_line & char_bit) != 0 && y+i < vga_screen.height && x+j < vga_screen.width) {
                        vga_screen.lines8[y+i][x+j] = color;
                    }
                    char_bit <<= 1;
                }
            }
        }
        x += vga_font->w;
    }

    return x;
}

void font_print(const char *text)
{
    if (text == NULL) return;

    switch (vga_font_alignment) {
    case FONT_ALIGN_LEFT:   /* nothing to do */ break;
    case FONT_ALIGN_CENTER: vga_font_x -= strlen(text) * vga_font->w / 2; break;
    case FONT_ALIGN_RIGHT:  vga_font_x -= strlen(text) * vga_font->w; break;
    }

    if (vga_font_border[0]) {
        for (int i = -1; i <= 1; i++) {
            for (int j = -1; j <= 1; j++) {
                if (i == 0 && j == 0) continue;
                render_text(text, vga_font_x+i, vga_font_y+j, vga_font_border[1]);
            }
        }
    }
    int new_x = render_text(text, vga_font_x, vga_font_y, vga_font_color);
    if (vga_font_alignment != FONT_ALIGN_RIGHT) {
        vga_font_x = new_x;
    }
}
