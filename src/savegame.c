#include <string.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "pico/flash.h"
#include "hardware/flash.h"

#include "savegame.h"

#include "lib/flash_write.h"

extern char __flash_binary_end;  // from the SDK linker script

static uint8_t scratch_buffer[FLASH_SECTOR_SIZE];
static const uint8_t savegame_magic[4] = { 'R', 'A', 'V', 0x04 };

static uint32_t get_flash_offset_for_slot(int slot)
{
    if (slot < 0 || slot > 3) return 0;

    uint32_t flash_offset = PICO_FLASH_SIZE_BYTES - (slot+1) * FLASH_SECTOR_SIZE;
    if (flash_offset < ((uintptr_t) &__flash_binary_end) - XIP_BASE) return 0;
    return flash_offset;
}

// ========================================================================
// ==== READ
// ========================================================================

struct READ_SAVE_DATA {
    const uint8_t *data;
    const uint8_t *end;
    int error;
};

static struct READ_SAVE_DATA init_read(const uint8_t *data, int32_t size)
{
    return (struct READ_SAVE_DATA) {
        .data = data,
        .end = data + size,
        .error = 0,
    };
}

static uint8_t read_u8(struct READ_SAVE_DATA *save)
{
    if (save->data + 1 > save->end) { save->error = 1; return 0; }
    return *save->data++;
}

static uint16_t read_u16(struct READ_SAVE_DATA *save)
{
    if (save->data + 2 > save->end) { save->error = 1; return 0; }
    uint16_t ret = (((uint16_t) save->data[0] << 0) |
                    ((uint16_t) save->data[1] << 8));
    save->data += 2;
    return ret;
}

static uint32_t read_u32(struct READ_SAVE_DATA *save)
{
    if (save->data + 4 > save->end) { save->error = 1; return 0; }
    uint32_t ret = (((uint32_t) save->data[0] <<  0) |
                    ((uint32_t) save->data[1] <<  8) |
                    ((uint32_t) save->data[2] << 16) |
                    ((uint32_t) save->data[3] << 24));
    save->data += 4;
    return ret;
}

static void read_bytes(struct READ_SAVE_DATA *save, void *bytes, uint32_t len)
{
    if (save->data + len > save->end) {
        save->error = 1;
        memset(bytes, 0, len);
        return;
    }
    memcpy(bytes, save->data, len);
    save->data += len;
}

// ========================================================================
// ==== WRITE
// ========================================================================

struct WRITE_SAVE_DATA {
    uint8_t *data;
    uint8_t *end;
    int error;
};

static struct WRITE_SAVE_DATA init_write(uint8_t *data, int32_t size)
{
    return (struct WRITE_SAVE_DATA) {
        .data = data,
        .end = data + size,
        .error = 0,
    };
}

static void write_u8(struct WRITE_SAVE_DATA *save, uint8_t data)
{
    if (save->data + 1 > save->end) { save->error = 1; return; }
    *save->data++ = data;
}

static void write_u16(struct WRITE_SAVE_DATA *save, uint16_t data)
{
    if (save->data + 2 > save->end) { save->error = 1; return; }
    *save->data++ = (data >> 0) & 0xff;
    *save->data++ = (data >> 8) & 0xff;
}

static void write_u32(struct WRITE_SAVE_DATA *save, uint32_t data)
{
    if (save->data + 4 > save->end) { save->error = 1; return; }
    *save->data++ = (data >>  0) & 0xff;
    *save->data++ = (data >>  8) & 0xff;
    *save->data++ = (data >> 16) & 0xff;
    *save->data++ = (data >> 24) & 0xff;
}

static void write_bytes(struct WRITE_SAVE_DATA *save, const void *bytes, uint32_t len)
{
    if (save->data + len > save->end) { save->error = 1; return; }
    memcpy(save->data, bytes, len);
    save->data += len;
}

// ========================================================================
// ==== SERIALIZE/DESERIALIZE
// ========================================================================

static int deserialize_game(struct READ_SAVE_DATA *save, struct GAME_STATE *game)
{
    uint8_t magic[4] = {0};
    read_bytes(save, magic, 4);
    if (memcmp(magic, savegame_magic, 4) != 0) return 1;

    game->room_id = read_u16(save);
    game->player.x = read_u32(save);
    game->player.y = read_u32(save);
    game->player.direction = read_u8(save);
    return save->error;
}

static int serialize_game(struct WRITE_SAVE_DATA *save, const struct GAME_STATE *game)
{
    write_bytes(save, savegame_magic, 4);
    write_u16(save, game->room_id);
    write_u32(save, game->player.x);
    write_u32(save, game->player.y);
    write_u8(save, game->player.direction);
    return save->error;
}

int savegame_read(struct GAME_STATE *game, int slot)
{
    uint32_t flash_offset = get_flash_offset_for_slot(slot);
    if (flash_offset == 0) return 1;

    const uint8_t *save_data = (const uint8_t *) (XIP_BASE + flash_offset);
    struct READ_SAVE_DATA save = init_read(save_data, FLASH_SECTOR_SIZE);
    return deserialize_game(&save, game);
}

int savegame_write(const struct GAME_STATE *game, int slot)
{
    uint32_t flash_offset = get_flash_offset_for_slot(slot);
    if (flash_offset == 0) return 1;

    struct WRITE_SAVE_DATA save = init_write(scratch_buffer, FLASH_SECTOR_SIZE);
    memset(scratch_buffer, 0, FLASH_SECTOR_SIZE);
    if (serialize_game(&save, game) != 0) {
        return 1;
    }

    return flash_write(flash_offset, scratch_buffer, FLASH_SECTOR_SIZE);
}
