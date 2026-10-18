#ifndef VGA_IMAGE_8BIT_H_FILE
#define VGA_IMAGE_8BIT_H_FILE

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct VGA_IMAGE_SCREEN {
    // x_min is always 0
    int32_t x_max;
    int32_t y_min;
    int32_t y_max;
};

struct VGA_IMAGE {
    int32_t width;
    int32_t height;
    int32_t stride;  // number of <uint32_t>s per line
    int32_t num_frames;
    int32_t has_mirrors;
    const uint32_t *data;
};

void vga_image_init(void);
void vga_image_draw(const struct VGA_IMAGE *image, int x, int y, bool transparent);
void vga_image_draw_frame(const struct VGA_IMAGE *img, int frame, int x, int y, bool transparent);
void vga_image_draw_frame_mask(const struct VGA_IMAGE *img, int frame, int x, int y, uint8_t color);

extern struct VGA_IMAGE_SCREEN vga_image_screen;

#ifdef __cplusplus
}
#endif

#endif /* VGA_IMAGE_8BIT_H_FILE */
