#include <string.h>

#include "darkness.h"
#include "game.h"

#include "lib/vga_8bit.h"

void darkness_load_circle(struct DARKNESS *dark, int radius)
{
    int center = (radius <= DARKNESS_MAX_ROWS/2-1) ? radius : DARKNESS_MAX_ROWS/2-1;
    int x = radius;
    int y = 0;
    int t1 = radius / 16;

    while (x >= y) {
        if (y <= center) {
            dark->row[center - y].xs = center - x;
            dark->row[center - y].xe = center + x;
            dark->row[center + y].xs = center - x;
            dark->row[center + y].xe = center + x;
        }
        if (x <= center) {
            dark->row[center - x].xs = center - y;
            dark->row[center - x].xe = center + y;
            dark->row[center + x].xs = center - y;
            dark->row[center + x].xe = center + y;
        }

        y++;
        t1 += y;
        int t2 = t1 - x;
        if (t2 >= 0) {
            t1 = t2;
            x--;
        }
    }
    dark->num_rows = center * 2 + 1;
}

void darkness_clear(struct DARKNESS *dark, int x, int y, uint8_t color)
{
    int dark_x = x - game_state.screen_x - dark->num_rows/2;
    int dark_y = y - game_state.screen_y + vga_image_screen.y_min - dark->num_rows/2;

    if (dark_y > SCREEN_HEADER_HEIGHT) {
        memset(vga_screen.lines[SCREEN_HEADER_HEIGHT], color, vga_screen.width*(dark_y - SCREEN_HEADER_HEIGHT));
    }
    if (dark_y + dark->num_rows < vga_screen.height) {
        memset(vga_screen.lines[dark_y + dark->num_rows],
               color,
               vga_screen.width*(vga_screen.height - (dark_y + dark->num_rows)));
    }
    for (int y = 0; y < dark->num_rows; y++) {
        if (y + dark_y < SCREEN_HEADER_HEIGHT || y + dark_y >= vga_screen.height) continue;
        int xs = dark->row[y].xs + dark_x;
        int xe = dark->row[y].xe + dark_x;
        if (xs > 0) memset(vga_screen.lines8[y + dark_y], color, xs);
        if (xe < vga_screen.width) memset(vga_screen.lines8[y + dark_y] + xe, color, vga_screen.width - xe);
    }
}

static void darken_screen_line(int x, int y, int len, int level)
{
    uint8_t *p = vga_screen.lines8[y] + x;
    if (level == 2) {
        for (int i = len; i > 0; i--) {
            *p = (*p >> 1) & 0x5b;
            p++;
        }
    } else {
        for (int i = len; i > 0; i--) {
            *p = (*p >> 2) & 0x09;
            p++;
        }
    }
}

void darkness_darken(struct DARKNESS *outer, struct DARKNESS *inner, int x, int y, int inner_dx, int inner_dy, int level)
{
    int outer_x = x - game_state.screen_x - outer->num_rows/2;
    int outer_y = y - game_state.screen_y + vga_image_screen.y_min - outer->num_rows/2;

    for (int i = 0; i < inner_dy; i++) {
        int y = outer_y + i;
        if (y < SCREEN_HEADER_HEIGHT || y >= vga_screen.height) continue;
        int x = outer->row[i].xs + outer_x;
        int len = outer->row[i].xe - outer->row[i].xs;
        darken_screen_line(x, y, len, level);
    }
    for (int i = inner->num_rows + inner_dy; i < outer->num_rows; i++) {
        int y = outer_y + i;
        if (y < SCREEN_HEADER_HEIGHT || y >= vga_screen.height) continue;
        int x = outer->row[i].xs + outer_x;
        int len = outer->row[i].xe - outer->row[i].xs;
        darken_screen_line(x, y, len, level);
    }
    for (int i = 0; i < inner->num_rows; i++) {
        int y = outer_y + inner_dx + i;
        if (y < SCREEN_HEADER_HEIGHT || y >= vga_screen.height) continue;

        int x1 = outer->row[i + inner_dy].xs + outer_x;
        int len1 = inner->row[i].xs + inner_dx - outer->row[i + inner_dy].xs;
        darken_screen_line(x1, y, len1, level);

        int x2 = inner->row[i].xe + inner_dx + outer_x;
        int len2 = outer->row[i + inner_dy].xe - inner->row[i].xe - inner_dx;
        darken_screen_line(x2, y, len2, level);
    }
}
