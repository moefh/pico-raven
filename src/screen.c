#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

#define PERF_IMAGE_W 128
#define PERF_IMAGE_H 70  // 16666 >> 8 = 65 plus a few extra pixels
#define PERF_COLOR_JOY_REQ  0b00000111
#define PERF_COLOR_JOY_READ 0b11000000
#define PERF_COLOR_UPDATE   0b11111111
#define PERF_COLOR_ROOM_BG  0b11111000
#define PERF_COLOR_PLAYER   0b11000000
#define PERF_COLOR_ROOM_FG  0b00000111
#define PERF_COLOR_RENDER   0b11111111
#define PERF_COLOR_VSYNC    0b00111000

static struct PERF_HISTORY {
    uint8_t image[PERF_IMAGE_W*PERF_IMAGE_H];
    uint32_t pos;
} perf_history;

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

    font_set_font(&raven_fonts[RAVEN_FONT_ID_6X8]);
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

static uint32_t update_perf_history_counter(uint32_t x, uint32_t start, uint32_t counter, uint8_t color)
{
    uint32_t end = counter >> 8;
    if (end > PERF_IMAGE_H-1) end = PERF_IMAGE_H-1;

    for (uint32_t y = start; y <= end; y++) {
        perf_history.image[x + (PERF_IMAGE_H-y-1)*PERF_IMAGE_W] = color;
    }
    return end;
}

static void update_perf_history(struct GAME_STATE_PERF *perf)
{
    uint32_t x = perf_history.pos;
    uint32_t y = 0;
    y = update_perf_history_counter(x, y, perf->joy_req_us,  0b00000111);
    y = update_perf_history_counter(x, y, perf->joy_read_us, 0b11000000);
    y = update_perf_history_counter(x, y, perf->update_us,   0b11111111);
    y = update_perf_history_counter(x, y, perf->room_bg_us,  0b11111000);
    y = update_perf_history_counter(x, y, perf->player_us,   0b11000000);
    y = update_perf_history_counter(x, y, perf->room_fg_us,  0b00000111);
    y = update_perf_history_counter(x, y, perf->render_us,   0b11111111);
    y = update_perf_history_counter(x, y, perf->vsync_us,    0b00111000);
    for (; y < PERF_IMAGE_H; y++) {
        perf_history.image[x + (PERF_IMAGE_H-y-1)*PERF_IMAGE_W] = 0;
    }
    perf_history.pos = (perf_history.pos + 1) % PERF_IMAGE_W;
}

static void draw_perf_history(uint32_t x, uint32_t y)
{
    uint32_t cur_x = perf_history.pos;
    for (uint32_t i = 0; i < PERF_IMAGE_H; i++) {
        memcpy(vga_screen.lines8[y+i] + x, &perf_history.image[i*PERF_IMAGE_W + cur_x], PERF_IMAGE_W - cur_x);
        memcpy(vga_screen.lines8[y+i] + x + PERF_IMAGE_W - cur_x, &perf_history.image[i*PERF_IMAGE_W], cur_x);
    }
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
    if (game->player.shadow_enabled) {
        sprite_shadow_draw(game->player.sprite, game->screen_x, game->screen_y, sprite_shadow_colors);
    }

    vga_image_draw_frame(game->player.sprite, game->player.sprite_frame,
                         game->player.sprite_x - game->screen_x, game->player.sprite_y - game->screen_y, 1);
}

static void draw_room(struct GAME_STATE *game)
{
    struct DRAW_ROOM_INFO *info = draw_room_init_frame(&mem, game);

    draw_room_bg(info);
    GAME_PERF(game, room_bg_us);

    draw_player(game);
    GAME_PERF(game, player_us);

    draw_room_fg(info);
    GAME_PERF(game, room_fg_us);
}

void screen_render(struct GAME_STATE *game)
{
    int fps = fps_count();

    mem_clear(&mem);
    //vga_clear_screen(0);

    draw_room(game);

    font_align(FONT_ALIGN_LEFT);
    font_move(10, 10);
    font_printf("%d fps", fps);

    int pw = game->player.anim->collision.w;
    int ph = game->player.anim->collision.w;
    font_move(10, 20); font_printf("pos  %4ld,%-4ld | %4ld,%-4ld\n", game->player.x, game->player.y, game->player.x+pw-1, game->player.y);
    font_move(10, 30); font_printf("pos  %4ld,%-4ld | %4ld,%-4ld\n", game->player.x, game->player.y+ph-1, game->player.x+pw-1, game->player.y+ph-1);
    font_move(10, 40); font_printf("vel %5ld,%-5ld| %4ld,%-4ld\n", game->player_control.dx, game->player_control.dy, game->player_control.dx>>8, game->player_control.dy>>8);
    font_move(10, 50); font_printf("state %d\n", game->player.state);

    update_perf_history(&game->perf);
    if (game->display.show_perf) {
        draw_perf_history(180, 10);
        if (game->display.show_perf == 2) {
            for (int y = 85; y < 175; y++) memset(vga_screen.lines8[y] + 210, 0, PERF_IMAGE_W - 30);
            font_set_color(PERF_COLOR_VSYNC);    font_move(216,  90); font_printf("vsync    %5lu", game->perf.vsync_us);
            font_set_color(PERF_COLOR_RENDER);   font_move(216, 100); font_printf("render   %5lu", game->perf.render_us);
            font_set_color(PERF_COLOR_ROOM_FG);  font_move(216, 110); font_printf("room_fg  %5lu", game->perf.room_fg_us);
            font_set_color(PERF_COLOR_PLAYER);   font_move(216, 120); font_printf("player   %5lu", game->perf.player_us);
            font_set_color(PERF_COLOR_ROOM_BG);  font_move(216, 130); font_printf("room_bg  %5lu", game->perf.room_bg_us);
            font_set_color(PERF_COLOR_UPDATE);   font_move(216, 140); font_printf("logic    %5lu", game->perf.update_us);
            font_set_color(PERF_COLOR_JOY_READ); font_move(216, 150); font_printf("joy read %5lu", game->perf.joy_read_us);
            font_set_color(PERF_COLOR_JOY_REQ);  font_move(216, 160); font_printf("joy req  %5lu", game->perf.joy_req_us);
            font_set_color(0xff);
        }
    }

    draw_joy_buttons(80);

    font_move(10, 160); font_printf("mod number: %d", game->mod.index);
    font_move(10, 170); font_printf("mod volume: %03x", game->mod.volume);
    if (game->display.msg_mod_event_frames_left) {
        font_move(10, 180); font_printf("mod event: %d, %d", game->mod.event_chan, game->mod.event_data);
    }
    if (game->display.msg_save_frames_left) {
        font_move(10, 190); font_printf("%s", game->display.save_success ? "game saved" : "error saving game");
    }
    if (game->display.msg_load_frames_left) {
        font_move(10, 200); font_printf("%s", game->display.load_success ? "game loaded" : "error loading game");
    }
    font_move(10, 226); font_printf("version %llx", get_compilation_timestamp());

    GAME_PERF(game, render_us);
    vga_swap_buffers(true);
    GAME_PERF(game, vsync_us);
}
