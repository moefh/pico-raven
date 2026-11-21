#ifndef DARKNESS_H_FILE
#define DARKNESS_H_FILE

#include <stdint.h>

#define DARKNESS_MAX_ROWS 256

struct DARKNESS {
    int num_rows;
    struct {
        int16_t xs;
        int16_t xe;
    } row[DARKNESS_MAX_ROWS];
};

void darkness_load_circle(struct DARKNESS *dark, int radius);
void darkness_clear(struct DARKNESS *dark, int x, int y, uint8_t color);
void darkness_darken(struct DARKNESS *inner, struct DARKNESS *outer, int x, int y, int inner_dx, int inner_dy, int level);

#endif /* DARKNESS_H_FILE */
