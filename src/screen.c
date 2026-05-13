#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"

#include "screen.h"
#include "config.h"
#include "draw_room.h"
#include "sprite_shadow.h"

#include "lib/vga_8bit.h"
#include "lib/vga_font_8bit.h"
#include "lib/compilation_timestamp.h"
#include "lib/mem.h"

#include "game_data.h"
#include "joy.h"

static struct MEM_ARENA mem;

static const uint8_t sprite_shadow_colors[SPRITE_SHADOW_NUM_FRAMES] = {
    0, 1, 2, 3, 4, 5, 6, 7, // red
    //0x00, 0x01, 0x0a, 0x0b, 0x14, 0x15, 0x1e, 0x1f, // orange
    //0x09, 0x0a, 0x13, 0x14, 0x15, 0x0f, 0x27, 0x27, // black->orange->red
    //0<<3, 1<<3, 2<<3, 3<<3, 4<<3, 5<<3, 6<<3, 7<<3, // green
    //0|(0<<6), 1|(0<<6), 2|(1<<6), 3|(1<<6), 4|(2<<6), 5|(2<<6), 6|(3<<6), 7|(3<<6), // magenta
};

int screen_init(void)
{
    if (vga_init(&vga_mode_320x240, VGA_PIO_NUM, VGA_PIN_BASE, true) < 0) {
        printf("ERROR initializing VGA\n");
        fflush(stdout);
        return -1;
    }

    size_t scratch_size = vga_screen.width * vga_screen.height;
    void *scratch = malloc(scratch_size);
    if (scratch == NULL) {
        printf("ERROR allocating screen memory arena\n");
        fflush(stdout);
        return -1;
    }
    mem_init(&mem, scratch, scratch_size);

    font_set_font(&raven_fonts[RAVEN_FONT_ID_FONT_6X8]);
    font_set_color(0xff);

    return 0;
}

static int fps_count(void)
{
    static uint32_t last_millis;
    static uint16_t frame_count;
    static uint16_t last_fps;

    uint32_t cur_millis = to_ms_since_boot(get_absolute_time());
    if (cur_millis/1000 != last_millis/1000) {
        last_fps = frame_count;
        frame_count = 1;
    } else {
        frame_count++;
    }
    last_millis = cur_millis;
    return last_fps;
}

static void draw_palette(void)
{
#define BLOCK 10
    int x_min = vga_screen.width - 10 - 16*BLOCK;
    int y_min = 10;

    for (int y = 0; y < 16; y++) {
        for (int x = 0; x < 16; x++) {
            uint8_t color = (y << 4) | x;
            for (int i = 0; i < BLOCK; i++) {
                uint8_t *d = vga_screen.lines8[y_min+BLOCK*y+i] + x_min+BLOCK*x;
                for (int j = 0; j < BLOCK; j++) {
                    *d++ = color;
                }
            }
        }
    }
#undef BLOCK
}

static void draw_joy_buttons(int y)
{
    font_align(FONT_ALIGN_LEFT);
    if (joy.cur & JOY_BTN_UP)    { font_move(10+0*36, y); font_printf("UP"); }
    if (joy.cur & JOY_BTN_DOWN)  { font_move(10+1*36, y); font_printf("DOWN"); }
    if (joy.cur & JOY_BTN_LEFT)  { font_move(10+2*36, y); font_printf("LEFT"); }
    if (joy.cur & JOY_BTN_RIGHT) { font_move(10+3*36, y); font_printf("RIGHT"); }

    if (joy.cur & JOY_BTN_A) { font_move(10+0*12, y+10); font_printf("A"); }
    if (joy.cur & JOY_BTN_B) { font_move(10+1*12, y+10); font_printf("B"); }
    if (joy.cur & JOY_BTN_C) { font_move(10+2*12, y+10); font_printf("C"); }
    if (joy.cur & JOY_BTN_D) { font_move(10+3*12, y+10); font_printf("D"); }

    if (joy.cur & JOY_BTN_L1) { font_move(10+0*18, y+20); font_printf("L1"); }
    if (joy.cur & JOY_BTN_R1) { font_move(10+1*18, y+20); font_printf("R1"); }
    if (joy.cur & JOY_BTN_L2) { font_move(10+2*18, y+20); font_printf("L2"); }
    if (joy.cur & JOY_BTN_R2) { font_move(10+3*18, y+20); font_printf("R2"); }

    if (joy.cur & JOY_BTN_SELECT) { font_move(10+0*42, y+30); font_printf("SELECT"); }
    if (joy.cur & JOY_BTN_START)  { font_move(10+1*42, y+30); font_printf("START"); }
}

static void draw_player(struct GAME_STATE *game)
{
#if SPRITE_SHADOW_ENABLE_BITMAP
    draw_sprite_shadow_bitmap(game->player.sprite_shadow, game->player.sprite, game->screen_x, game->screen_y, sprite_shadow_colors);
#else
    draw_sprite_shadow(game->player.sprite, game->screen_x, game->screen_y, sprite_shadow_colors);
#endif

    vga_image_draw_frame(game->player.sprite, game->player.sprite_frame,
                         game->player.sprite_x - game->screen_x, game->player.sprite_y - game->screen_y, 1);
}

static void draw_room(struct GAME_STATE *game)
{
    struct DRAW_ROOM_INFO *info = draw_room_init(&mem, game);
    draw_room_bg(info);
    draw_player(game);
    draw_room_fg(info);
}

void screen_render(struct GAME_STATE *game)
{
    uint32_t render_start = time_us_32();
    int fps = fps_count();

    mem_clear(&mem);
    //vga_clear_screen((game->mod.index & 1) ? 0x08 : 0x40);

    draw_room(game);

    font_align(FONT_ALIGN_LEFT);
    font_move(10, 10);
    font_printf("%d fps", fps);

    if (game->display.show_palette) {
        draw_palette();
    }

    font_move(10, 20); font_printf("joy up us: %5lu", game->perf.joy_read_us);
    font_move(10, 30); font_printf("update us: %5lu", game->perf.update_us);
    font_move(10, 40); font_printf("render us: %5lu", game->perf.render_us);
    font_move(10, 50); font_printf("vsync  us: %5lu", game->perf.vsync_us);
    draw_joy_buttons(60);

    int info_y = 60 + 40;
    font_move(10, info_y); font_printf("mod number: %d", game->mod.index);
    font_move(10, info_y+10); font_printf("mod volume: %03x", game->mod.volume);
    if (game->display.msg_mod_event_frames_left) {
        font_move(10, info_y+20); font_printf("mod event: %d, %d", game->mod.event_chan, game->mod.event_data);
    }
    if (game->display.msg_save_frames_left) {
        font_move(10, info_y+30); font_printf("%s", game->display.save_success ? "game saved" : "error saving game");
    }
    if (game->display.msg_load_frames_left) {
        font_move(10, info_y+40); font_printf("%s", game->display.load_success ? "game loaded" : "error loading game");
    }
    font_move(10, 226); font_printf("version %llx", get_compilation_timestamp());

    uint32_t vsync_start = time_us_32();
    game->perf.render_us = vsync_start - render_start;
    vga_swap_buffers(true);
    game->perf.vsync_us = time_us_32() - vsync_start;
}
