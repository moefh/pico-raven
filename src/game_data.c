#include <stdint.h>

#define RAVEN_DATA_BYTES 1

#include "game_data.h"

#if RAVEN_DATA_VGA_BITS_PER_PIXEL != 8
#error "Invalid game data: 8-bit color required"
#endif
