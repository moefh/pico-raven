#include <stdio.h>

#include "collision.h"

#define TILE_SIZE 16

#define TILE_BLOCK       0
#define TILE_L_RAMP_FULL 1
#define TILE_R_RAMP_FULL 2
#define TILE_L_RAMP_BOT  3
#define TILE_L_RAMP_TOP  4
#define TILE_R_RAMP_BOT  5
#define TILE_R_RAMP_TOP  6

static const struct RAVEN_ROOM *room;

void collision_init_frame(uint16_t room_id)
{
    room = &raven_rooms[room_id];
}

static uint8_t get_room_tile_at(int tx, int ty)
{
    for (int m = 0; m < room->num_maps; m++) {
        struct RAVEN_ROOM_MAP_INFO mi = room->maps[m];
        int map_tx = tx - mi.x;
        int map_ty = ty - mi.y;
        if (map_tx >= 0 && map_ty >= 0 && map_tx < mi.map->w && map_ty < mi.map->h) {
            return mi.map->tiles[mi.map->w*mi.map->h*2 + mi.map->w*map_ty + map_tx];
        }
    }
    return TILE_BLOCK;
}

// ========================================================================

static int h_move(struct COLLISION_RECT *rect, int sx)
{
    if (rect->x + sx < 0) {
        rect->x = 0;
        return COLLISION_FLAGS_LEFT;
    }

    int rx = (sx < 0) ? rect->x : rect->x + rect->w - 1;
    int16_t tx = (rx + sx) / TILE_SIZE;

    int16_t ty_top = rect->y / TILE_SIZE;
    int16_t ty_bot = (rect->y + rect->h - 1) / TILE_SIZE;
    for (int ty = ty_bot; ty >= ty_top; ty--) {
        uint8_t tile = get_room_tile_at(tx, ty);
        switch (tile) {
        case TILE_BLOCK:
            return (sx > 0) ? COLLISION_FLAGS_RIGHT : COLLISION_FLAGS_LEFT;

        case TILE_L_RAMP_FULL:
            if (ty < ty_bot) {
                // ramp above foot level
                return (sx > 0) ? COLLISION_FLAGS_RIGHT : COLLISION_FLAGS_LEFT;
            }
            {
                int ix = (rx + sx) % TILE_SIZE;
                int iy = (rect->y + rect->h - 1) % TILE_SIZE;
                if (iy >= TILE_SIZE - 1 - ix) {  // under ramp
                    if (sx < 0) {
                        return COLLISION_FLAGS_LEFT;
                    }
                    rect->x++;
                    rect->y--;
                    return COLLISION_FLAGS_RAMP;
                }
            }
            break;

        case TILE_R_RAMP_FULL:
            if (ty < ty_bot) {
                // ramp above foot level
                return (sx > 0) ? COLLISION_FLAGS_RIGHT : COLLISION_FLAGS_LEFT;
            }
            {
                int ix = (rx + sx) % TILE_SIZE;
                int iy = (rect->y + rect->h - 1) % TILE_SIZE;
                if (iy >= ix) {  // under ramp
                    if (sx > 0) {
                        // ramp from wrong side
                        return COLLISION_FLAGS_RIGHT;
                    }
                    rect->x--;
                    rect->y--;
                    return COLLISION_FLAGS_RAMP;
                }
            }
            break;

        case TILE_L_RAMP_TOP:
            if (ty < ty_bot) {
                // ramp above foot level
                return (sx > 0) ? COLLISION_FLAGS_RIGHT : COLLISION_FLAGS_LEFT;
            }
            {
                int ix = (rx + sx) % TILE_SIZE;
                int iy = (rect->y + rect->h - 1) % TILE_SIZE;
                if (2*iy >= TILE_SIZE - 1 - ix) {  // under ramp
                    if (sx < 0) {
                        return COLLISION_FLAGS_LEFT;
                    }
                    rect->x++;
                    rect->y--;
                    return COLLISION_FLAGS_RAMP;
                }
            }
            break;

        case TILE_L_RAMP_BOT:
            if (ty < ty_bot) {
                // ramp above foot level
                return (sx > 0) ? COLLISION_FLAGS_RIGHT : COLLISION_FLAGS_LEFT;
            }
            {
                int ix = (rx + sx) % TILE_SIZE;
                int iy = (rect->y + rect->h - 1) % TILE_SIZE;
                if (2*iy - TILE_SIZE >= TILE_SIZE - 1 - ix) {  // under ramp
                    if (sx < 0) {
                        return COLLISION_FLAGS_LEFT;
                    }
                    rect->x++;
                    rect->y--;
                    return COLLISION_FLAGS_RAMP;
                }
            }
            break;

        case TILE_R_RAMP_TOP:
            if (ty < ty_bot) {
                // ramp above foot level
                return (sx > 0) ? COLLISION_FLAGS_RIGHT : COLLISION_FLAGS_LEFT;
            }
            {
                int ix = (rx + sx) % TILE_SIZE;
                int iy = (rect->y + rect->h - 1) % TILE_SIZE;
                if (2*iy >= ix) {  // under ramp
                    if (sx > 0) {
                        // ramp from wrong side
                        return COLLISION_FLAGS_RIGHT;
                    }
                    rect->x--;
                    rect->y--;
                    return COLLISION_FLAGS_RAMP;
                }
            }
            break;

        case TILE_R_RAMP_BOT:
            if (ty < ty_bot) {
                // ramp above foot level
                return (sx > 0) ? COLLISION_FLAGS_RIGHT : COLLISION_FLAGS_LEFT;
            }
            {
                int ix = (rx + sx) % TILE_SIZE;
                int iy = (rect->y + rect->h - 1) % TILE_SIZE;
                if (2*iy - TILE_SIZE >= ix) {  // under ramp
                    if (sx > 0) {
                        // ramp from wrong side
                        return COLLISION_FLAGS_RIGHT;
                    }
                    rect->x--;
                    rect->y--;
                    return COLLISION_FLAGS_RAMP;
                }
            }
            break;
        }
    }
    rect->x += sx;
    return 0;
}

static int v_move(struct COLLISION_RECT *rect, int sy)
{
    if (rect->y + sy < 0) {
        rect->y = 0;
        return COLLISION_FLAGS_UP;
    }

    int ry = (sy < 0) ? rect->y : rect->y + rect->h - 1;
    int16_t ty = (ry + sy) / TILE_SIZE;

    int16_t tx_left = rect->x / TILE_SIZE;
    int16_t tx_right = (rect->x + rect->w - 1) / TILE_SIZE;

    for (int tx = tx_left; tx <= tx_right; tx++) {
        uint8_t tile = get_room_tile_at(tx, ty);
        switch (tile) {
        case TILE_BLOCK:
            return (sy < 0) ? COLLISION_FLAGS_UP : COLLISION_FLAGS_DOWN;

        case TILE_L_RAMP_FULL:
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

        case TILE_R_RAMP_FULL:
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

        case TILE_L_RAMP_TOP:
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

        case TILE_L_RAMP_BOT:
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

        case TILE_R_RAMP_TOP:
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

        case TILE_R_RAMP_BOT:
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
        if (dy < 0) dy = -dy;
        for (int y = 0; y < dy; y++) {
            flags |= v_move(rect, sy);
        }
        return flags;
    }
    if (dy == 0) {
        if (dx < 0) dx = -dx;
        int flags = 0;
        for (int x = 0; x < dx; x++) {
            flags |= h_move(rect, sx);
        }
        return flags;
    }

    int x_end = dx;
    int y_end = dy;
    if (dx < 0) dx = -dx;
    if (dy > 0) dy = -dy;
    int error = dx + dy;
    int x = 0;
    int y = 0;
    int flags = 0;
    while (42) {
        if (x == x_end && y == y_end) { break; }
        int e2 = 2 * error;
        if (e2 >= dy) {
            error += dy;
            flags |= h_move(rect, sx);
            x += sx;
        }
        if (e2 <= dx) {
            error += dx;
            flags |= v_move(rect, sy);
            y += sy;
        }
    }
    return flags;
}
