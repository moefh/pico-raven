#ifndef GAME_DATA_H_FILE
#define GAME_DATA_H_FILE

#include <stddef.h>
#include <stdint.h>

#include "lib/mod_data.h"
#include "lib/vga_image_8bit.h"
#include "lib/vga_font_8bit.h"

enum RAVEN_DIRECTION {
    RAVEN_DIR_RIGHT,
    RAVEN_DIR_LEFT,
};

#define RAVEN_SKIP_STRUCTS_MOD
#define RAVEN_SKIP_STRUCTS_IMAGE
#define RAVEN_SKIP_STRUCTS_FONT
#define RAVEN_SKIP_STRUCTS_PROP_FONT
#define RAVEN_ADD_ROOM_SCRIPTS 1

#define RAVEN_MOD_DATA MOD_DATA
#define RAVEN_MOD_CELL MOD_CELL
#define RAVEN_MOD_DATA MOD_DATA
#define RAVEN_IMAGE VGA_IMAGE
#define RAVEN_FONT VGA_FONT
#define RAVEN_PROP_FONT VGA_PROP_FONT
#define RAVEN_STATE GAME_STATE

#include "data/raven.h"
#include "data/raven_data.h"

#define TILE_SIZE 16

#endif /* GAME_DATA_H_FILE */
