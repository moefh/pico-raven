#include <string.h>
#include "pico/stdlib.h"
#include "pico/flash.h"
#include "hardware/flash.h"

#include "flash_write.h"

struct FLASH_WRITE_PARAMS {
    uint32_t flash_offset;
    const void *data;
    uint32_t size;
};

static void __no_inline_not_in_flash_func(write_flash)(void *data)
{
    struct FLASH_WRITE_PARAMS *params = data;
    flash_range_erase(params->flash_offset, params->size);
    flash_range_program(params->flash_offset, params->data, params->size);
}

int flash_write(uint32_t flash_offset, const void *data, uint32_t size)
{
    if ((flash_offset % FLASH_SECTOR_SIZE != 0) ||
        (size % FLASH_SECTOR_SIZE) != 0) {
        return 1;
    }

    struct FLASH_WRITE_PARAMS params = {
        .flash_offset = flash_offset,
        .data = data,
        .size = size,
    };
    if (flash_safe_execute(write_flash, &params, UINT32_MAX) != PICO_OK) {
        return 1;
    }
    return 0;
}
