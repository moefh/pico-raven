#include <stdio.h>

#include "collision.h"

#define VERTICAL_STEP   0x100
#define HORIZONTAL_STEP 0x100
#define FULL_RAMP_STEP  0x16a  // 0x100 * sqrt(2)
#define HALF_RAMP_STEP  0x11e  // 0x100 * sqrt(1.25)

int room_tiles_block_rect(struct COLLISION_RECT *rect)
{
    if (rect->x < 0 || rect->y < 0) { return 1; }
    if (collision_get_room_tile_at(rect->x / TILE_SIZE, rect->y / TILE_SIZE) == TILE_FX_BLOCK) return 1;
    if (collision_get_room_tile_at((rect->x+rect->w) / TILE_SIZE, rect->y / TILE_SIZE) == TILE_FX_BLOCK) return 1;
    if (collision_get_room_tile_at(rect->x / TILE_SIZE, (rect->y+rect->h) / TILE_SIZE) == TILE_FX_BLOCK) return 1;
    if (collision_get_room_tile_at((rect->x+rect->w) / TILE_SIZE, (rect->y+rect->h) / TILE_SIZE) == TILE_FX_BLOCK) return 1;
    return 0;
}

static int h_move(struct COLLISION_RECT *rect, int sx, int *steps)
{
    if (rect->x + sx < 0) {
        rect->x = 0;
        *steps -= HORIZONTAL_STEP;
        return COLLISION_FLAGS_LEFT;
    }

    int rx = (sx < 0) ? rect->x : rect->x + rect->w - 1;
    int16_t tx = (rx + sx) / TILE_SIZE;

    int16_t ty_top = rect->y / TILE_SIZE;
    int16_t ty_bot = (rect->y + rect->h - 1) / TILE_SIZE;
    for (int ty = ty_bot; ty >= ty_top; ty--) {
        uint8_t tile = collision_get_room_tile_at(tx, ty);
        switch (tile) {
        case TILE_FX_BLOCK:
            *steps -= HORIZONTAL_STEP;
            return (sx > 0) ? COLLISION_FLAGS_RIGHT : COLLISION_FLAGS_LEFT;

        case TILE_FX_L_RAMP_FULL:
            if (ty < ty_bot) {   // ramp above foot level
                *steps -= HORIZONTAL_STEP;
                return (sx > 0) ? COLLISION_FLAGS_RIGHT : COLLISION_FLAGS_LEFT;
            }
            {
                int ix = (rx + sx) % TILE_SIZE;
                int iy = (rect->y + rect->h - 1) % TILE_SIZE;
                if (iy >= TILE_SIZE - 1 - ix) {  // under ramp
                    if (sx < 0) {                // ramp from wrong side
                        *steps -= HORIZONTAL_STEP;
                        return COLLISION_FLAGS_LEFT;
                    }
                    rect->x++;
                    rect->y--;
                    *steps -= FULL_RAMP_STEP;
                    return COLLISION_FLAGS_RAMP;
                }
            }
            break;

        case TILE_FX_R_RAMP_FULL:
            if (ty < ty_bot) {   // ramp above foot level
                *steps -= HORIZONTAL_STEP;
                return (sx > 0) ? COLLISION_FLAGS_RIGHT : COLLISION_FLAGS_LEFT;
            }
            {
                int ix = (rx + sx) % TILE_SIZE;
                int iy = (rect->y + rect->h - 1) % TILE_SIZE;
                if (iy >= ix) {      // under ramp
                    if (sx > 0) {    // ramp from wrong side
                        *steps -= HORIZONTAL_STEP;
                        return COLLISION_FLAGS_RIGHT;
                    }
                    rect->x--;
                    rect->y--;
                    *steps -= FULL_RAMP_STEP;
                    return COLLISION_FLAGS_RAMP;
                }
            }
            break;

        case TILE_FX_L_RAMP_TOP:
            if (ty < ty_bot) {  // ramp above foot level
                return (sx > 0) ? COLLISION_FLAGS_RIGHT : COLLISION_FLAGS_LEFT;
            }
            {
                int ix = (rx + sx) % TILE_SIZE;
                int iy = (rect->y + rect->h - 1) % TILE_SIZE;
                if (2*iy >= TILE_SIZE - 1 - ix) {  // under ramp
                    if (sx < 0) {                  // ramp from wrong side
                        *steps -= HORIZONTAL_STEP;
                        return COLLISION_FLAGS_LEFT;
                    }
                    rect->x++;
                    rect->y--;
                    *steps -= HALF_RAMP_STEP;
                    return COLLISION_FLAGS_RAMP;
                }
            }
            break;

        case TILE_FX_L_RAMP_BOT:
            if (ty < ty_bot) {  // ramp above foot level
                return (sx > 0) ? COLLISION_FLAGS_RIGHT : COLLISION_FLAGS_LEFT;
            }
            {
                int ix = (rx + sx) % TILE_SIZE;
                int iy = (rect->y + rect->h - 1) % TILE_SIZE;
                if (2*iy - TILE_SIZE >= TILE_SIZE - 1 - ix) {  // under ramp
                    if (sx < 0) {                              // ramp from wrong side
                        *steps -= HORIZONTAL_STEP;
                        return COLLISION_FLAGS_LEFT;
                    }
                    rect->x++;
                    rect->y--;
                    *steps -= HALF_RAMP_STEP;
                    return COLLISION_FLAGS_RAMP;
                }
            }
            break;

        case TILE_FX_R_RAMP_TOP:
            if (ty < ty_bot) {  // ramp above foot level
                return (sx > 0) ? COLLISION_FLAGS_RIGHT : COLLISION_FLAGS_LEFT;
            }
            {
                int ix = (rx + sx) % TILE_SIZE;
                int iy = (rect->y + rect->h - 1) % TILE_SIZE;
                if (2*iy >= ix) {    // under ramp
                    if (sx > 0) {    // ramp from wrong side
                        *steps -= HORIZONTAL_STEP;
                        return COLLISION_FLAGS_RIGHT;
                    }
                    rect->x--;
                    rect->y--;
                    *steps -= HALF_RAMP_STEP;
                    return COLLISION_FLAGS_RAMP;
                }
            }
            break;

        case TILE_FX_R_RAMP_BOT:
            if (ty < ty_bot) {  // ramp above foot level
                *steps -= HORIZONTAL_STEP;
                return (sx > 0) ? COLLISION_FLAGS_RIGHT : COLLISION_FLAGS_LEFT;
            }
            {
                int ix = (rx + sx) % TILE_SIZE;
                int iy = (rect->y + rect->h - 1) % TILE_SIZE;
                if (2*iy - TILE_SIZE >= ix) {  // under ramp
                    if (sx > 0) {              // ramp from wrong side
                        *steps -= HORIZONTAL_STEP;
                        return COLLISION_FLAGS_RIGHT;
                    }
                    rect->x--;
                    rect->y--;
                    *steps -= HALF_RAMP_STEP;
                    return COLLISION_FLAGS_RAMP;
                }
            }
            break;
        }
    }
    rect->x += sx;
    *steps -= HORIZONTAL_STEP;
    return 0;
}

static int v_move(struct COLLISION_RECT *rect, int sy, int *steps)
{
    *steps -= VERTICAL_STEP;
    if (rect->y + sy < 0) {
        rect->y = 0;
        return COLLISION_FLAGS_UP;
    }

    int ry = (sy < 0) ? rect->y : rect->y + rect->h - 1;
    int16_t ty = (ry + sy) / TILE_SIZE;

    int16_t tx_left = rect->x / TILE_SIZE;
    int16_t tx_right = (rect->x + rect->w - 1) / TILE_SIZE;

    for (int tx = tx_left; tx <= tx_right; tx++) {
        uint8_t tile = collision_get_room_tile_at(tx, ty);
        switch (tile) {
        case TILE_FX_BLOCK:
            return (sy < 0) ? COLLISION_FLAGS_UP : COLLISION_FLAGS_DOWN;

        case TILE_FX_L_RAMP_FULL:
            if (sy < 0) {
                return COLLISION_FLAGS_UP;
            }

            if (tx < tx_right) {
                // hanging over ramp
                return COLLISION_FLAGS_DOWN;
            }

            {
                int ix = (rect->x + rect->w - 1) % TILE_SIZE;
                int iy = (ry + 1) % TILE_SIZE;
                if (iy >= TILE_SIZE - 1 - ix) {
                    return COLLISION_FLAGS_DOWN;
                }
            }
            break;

        case TILE_FX_R_RAMP_FULL:
            if (sy < 0) {
                return COLLISION_FLAGS_UP;
            }

            if (tx > tx_left) {
                // hanging over ramp
                return COLLISION_FLAGS_DOWN;
            }

            {
                int ix = rect->x % TILE_SIZE;
                int iy = (ry + 1) % TILE_SIZE;
                if (iy >= ix) {
                    return COLLISION_FLAGS_DOWN;
                }
            }
            break;

        case TILE_FX_L_RAMP_TOP:
            if (sy < 0) {
                return COLLISION_FLAGS_UP;
            }

            if (tx < tx_right) {
                // hanging over ramp
                return COLLISION_FLAGS_DOWN;
            }

            {
                int ix = (rect->x + rect->w - 1) % TILE_SIZE;
                int iy = (ry + 1) % TILE_SIZE;
                if (2*iy >= TILE_SIZE - 1 - ix) {
                    return COLLISION_FLAGS_DOWN;
                }
            }
            break;

        case TILE_FX_L_RAMP_BOT:
            if (sy < 0) {
                return COLLISION_FLAGS_UP;
            }

            if (tx < tx_right && (ry + 1) % TILE_SIZE >= TILE_SIZE/2) {
                // hanging over ramp
                return COLLISION_FLAGS_DOWN;
            }

            {
                int ix = (rect->x + rect->w - 1) % TILE_SIZE;
                int iy = (ry + 1) % TILE_SIZE;
                if (2*iy - TILE_SIZE >= TILE_SIZE - 1 - ix) {
                    return COLLISION_FLAGS_DOWN;
                }
            }
            break;

        case TILE_FX_R_RAMP_TOP:
            if (sy < 0) {
                return COLLISION_FLAGS_UP;
            }

            if (tx > tx_left) {
                // hanging over ramp
                return COLLISION_FLAGS_DOWN;
            }

            {
                int ix = rect->x % TILE_SIZE;
                int iy = (ry + 1) % TILE_SIZE;
                if (2*iy >= ix) {
                    return COLLISION_FLAGS_DOWN;
                }
            }
            break;

        case TILE_FX_R_RAMP_BOT:
            if (sy < 0) {
                return COLLISION_FLAGS_UP;
            }

            if (tx > tx_left && (ry + 1) % TILE_SIZE >= TILE_SIZE/2) {
                // hanging over ramp
                return COLLISION_FLAGS_DOWN;
            }

            {
                int ix = rect->x % TILE_SIZE;
                int iy = (ry + 1) % TILE_SIZE;
                if (2*iy - TILE_SIZE >= ix) {
                    return COLLISION_FLAGS_DOWN;
                }
            }
            break;
        }
    }
    rect->y += sy;
    return 0;
}

int collision_move(struct COLLISION_RECT *rect, int dx, int dy)
{
    if (dx == 0 && dy == 0) return 0;
    int sx = (dx < 0) ? -1 : 1;
    int sy = (dy < 0) ? -1 : 1;

    if (dx == 0) {
        int flags = 0;
        int steps = ((dy < 0) ? -dy : dy) << 8;
        while (steps > 0) {
            flags |= v_move(rect, sy, &steps);
        }
        return flags;
    }
    if (dy == 0) {
        int flags = 0;
        int steps = ((dx < 0) ? -dx : dx) << 8;
        while (steps > 0) {
            flags |= h_move(rect, sx, &steps);
        }
        return flags;
    }

    if (dx < 0) dx = -dx;
    if (dy > 0) dy = -dy;
    int error = dx + dy;
    int flags = 0;
    int steps = (dx - dy) << 8;
    while (steps > 0) {
        int e2 = 2 * error;
        if (e2 >= dy) {
            error += dy;
            flags |= h_move(rect, sx, &steps);
        }
        if (e2 <= dx) {
            error += dx;
            flags |= v_move(rect, sy, &steps);
        }
    }
    return flags;
}
