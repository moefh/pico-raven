#include <stdio.h>
#include <string.h>

#include "vga_font_8bit.h"
#include "vga_8bit.h"

#define FIRST_CHAR 32
#define NUM_CHARS  96

static char print_buf[32];
const struct VGA_FONT *vga_font;
const struct VGA_PROP_FONT *vga_pfont;
int32_t vga_font_x, vga_font_y;
enum FONT_ALIGNMENT vga_font_alignment;
uint8_t vga_font_is_prop;
uint8_t vga_font_color;
uint8_t vga_font_border_enable;
uint8_t vga_font_border_color;

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

static int render_text_fixed_font(const char *text, int x, int y, unsigned int color)
{
    while (*text != '\0') {
        int ch = *text++;
        if (ch >= FIRST_CHAR && ch < FIRST_CHAR+NUM_CHARS) {
            ch -= FIRST_CHAR;
            const uint8_t *ch_data = &vga_font->data[vga_font->h*ch];
            for (int i = 0; i < vga_font->h; i++) {
                uint8_t ch_data_byte = *ch_data++;
                uint8_t char_bit = 1;
                for (int j = 0; j < vga_font->w; j++) {
                    if ((ch_data_byte & char_bit) != 0 && y+i < vga_screen.height && x+j < vga_screen.width) {
                        vga_screen.lines8[y+i][x+j] = color;
                    }
                    char_bit <<= 1;
                    if (j % 8 == 7) {
                        char_bit = 1;
                        ch_data_byte = *ch_data++;
                    }
                }
            }
        }
        x += vga_font->w;
    }

    return x;
}

static int render_text_prop_font(const char *text, int x, int y, unsigned int color)
{
    while (*text != '\0') {
        int ch = *text++;
        if (ch >= FIRST_CHAR && ch < FIRST_CHAR+NUM_CHARS) {
            ch -= FIRST_CHAR;
            const uint8_t *ch_data = &vga_pfont->data[vga_pfont->char_offset[ch]];
            int ch_width = vga_pfont->char_width[ch];
            for (int i = 0; i < vga_pfont->h; i++) {
                uint8_t ch_data_byte = *ch_data++;
                uint8_t char_bit = 1;
                for (int j = 0; j < ch_width; j++) {
                    if ((ch_data_byte & char_bit) != 0 && y+i < vga_screen.height && x+j < vga_screen.width) {
                        vga_screen.lines8[y+i][x+j] = color;
                    }
                    char_bit <<= 1;
                    if (j % 8 == 7) {
                        char_bit = 1;
                        ch_data_byte = *ch_data++;
                    }
                }
            }
            x += ch_width;
        }
        x++;
    }

    return x;
}

int32_t font_measure(const char *text)
{
    if (! vga_font_is_prop) {
        return vga_font->w * strlen(text);
    }

    if (*text == '\0') return 0;

    int32_t width = 0;
    while (*text != '\0') {
        int ch = *text++;
        if (ch >= FIRST_CHAR && ch < FIRST_CHAR+NUM_CHARS) {
            ch -= FIRST_CHAR;
            width += vga_pfont->char_width[ch];
        }
        width++;  // space between characters
    }
    width--;      // don't include space after last char
    return width;
}

void font_print(const char *text)
{
    if (text == NULL) return;

    int font_width = font_measure(text);
    if (vga_font_alignment == FONT_ALIGN_CENTER) {
        vga_font_x -= font_width / 2;
    } else if (vga_font_alignment == FONT_ALIGN_RIGHT) {
        vga_font_x -= font_width;
    }

    if (vga_font_border_enable) {
        for (int i = -1; i <= 1; i++) {
            for (int j = -1; j <= 1; j++) {
                if (i == 0 && j == 0) continue;
                if (vga_font_is_prop) {
                    render_text_prop_font(text, vga_font_x+i, vga_font_y+j, vga_font_border_color);
                } else {
                    render_text_fixed_font(text, vga_font_x+i, vga_font_y+j, vga_font_border_color);
                }
            }
        }
    }
    int new_x;
    if (vga_font_is_prop) {
        new_x = render_text_prop_font(text, vga_font_x, vga_font_y, vga_font_color);
    } else {
        new_x = render_text_fixed_font(text, vga_font_x, vga_font_y, vga_font_color);
    }
    if (vga_font_alignment != FONT_ALIGN_RIGHT) {
        vga_font_x = new_x;
    }
}
