
#include "sprite_shadow.h"
#include "lib/vga_8bit.h"

#pragma GCC optimize ("-O3")

struct SHADOW_FRAME {
    int16_t x;
    int16_t y;
    uint8_t frame;
};

static int n_shadow_frames;
static struct SHADOW_FRAME shadow_frame[SPRITE_SHADOW_NUM_FRAMES];

void add_sprite_shadow_frame(int frame, int x, int y)
{
    int new_shadow = n_shadow_frames-1;
    if (n_shadow_frames < SPRITE_SHADOW_NUM_FRAMES) {
        n_shadow_frames++;
    } else {
        for (int i = 1; i < n_shadow_frames; i++) {
            shadow_frame[i-1] = shadow_frame[i];
        }
    }
    shadow_frame[new_shadow] = (struct SHADOW_FRAME) {
        .x = x,
        .y = y,
        .frame = frame,
    };
    //for (int i = 0; i < n_shadow_frames; i++) printf("|%d,%d,%d", shadow_frame[i].frame, shadow_frame[i].x, shadow_frame[i].y);
    //printf("\n");
}

void gen_sprite_shadow_image(uint8_t *sprite_shadow_data, const struct RAVEN_IMAGE *sprite)
{
    uint8_t *dest = sprite_shadow_data;
    const uint32_t *src = sprite->data;
    int blocks_per_line = (sprite->width + 7) / 8;
    for (int f = 0; f < sprite->num_frames; f++) {
        for (int y = 0; y < sprite->height; y++) {
            const uint8_t *src_line = (const uint8_t *) src;
            int pixels_left = sprite->width;
            for (int b = 0; b < blocks_per_line; b++) {
                int pixels_in_block = (pixels_left > 8) ? 8 : pixels_left;
                uint8_t block = 0;
                for (int i = 0; i < pixels_in_block; i++) {
                    if (*src_line++ != 0x38) {
                        block |= 1<<i;
                    }
                }
                *dest++ = block;
            }
            src += sprite->stride;
        }
    }
}

static void draw_sprite_shadow_frame(int x, int y, int frame, uint8_t color, const uint8_t *data, int w, int h)
{
    if (x + w <= 0 || y + h <= 0 || x >= vga_screen.width || y >= vga_screen.height) return;

    int src_stride = (w + 7) / 8;
    data += src_stride * (frame * h);
    int shift_start = 0;
    if (y < 0) {
        int skip_y = -y;
        y += skip_y;
        h -= skip_y;
        data += skip_y * src_stride;
    }
    if (x < 0) {
        int skip_x = -x;
        x += skip_x;
        w -= skip_x;
        data += skip_x / 8;
        shift_start = skip_x % 8;
    }
    if (y + h > vga_screen.height) {
        h -= y + h - vga_screen.height;
    }
    if (x + w > vga_screen.width) {
        w -= x + w - vga_screen.width;
    }

    for (int iy = 0; iy < h; iy++) {
        uint8_t *dest = vga_screen.lines8[y+iy] + x;
        int shift = shift_start;
        for (int ix = 0; ix < w; ix++) {
            if (*data & (1<<shift)) *dest = color;
            dest++;
            if (++shift == 8) {
                shift = 0;
                data++;
            }
        }
    }
}

void draw_sprite_shadow_bitmap(const uint8_t *sprite_shadow, const struct RAVEN_IMAGE *sprite, int screen_x, int screen_y, const uint8_t *colors)
{
    for (int i = 0; i < n_shadow_frames; i++) {
        draw_sprite_shadow_frame(shadow_frame[i].x - screen_x, shadow_frame[i].y - screen_y, shadow_frame[i].frame, colors[i],
                                 sprite_shadow, sprite->width, sprite->height);
    }
}

void draw_sprite_shadow(const struct RAVEN_IMAGE *sprite, int screen_x, int screen_y, const uint8_t *colors)
{
    for (int i = 0; i < n_shadow_frames; i++) {
        vga_image_draw_frame_mask(sprite, shadow_frame[i].frame, shadow_frame[i].x - screen_x, shadow_frame[i].y - screen_y, colors[i]);
    }
}
