#include <string.h>

#include "perf.h"
#include "game.h"

#include "lib/vga_8bit.h"
#include "lib/vga_image_8bit.h"
#include "lib/vga_font_8bit.h"
#include "lib/joystick.h"
#include "lib/compilation_timestamp.h"

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

static uint32_t update_perf_history_counter(uint32_t x, uint32_t start, uint32_t counter, uint8_t color)
{
    uint32_t end = counter >> 8;
    if (end > PERF_IMAGE_H-1) end = PERF_IMAGE_H-1;

    for (uint32_t y = start; y <= end; y++) {
        perf_history.image[x + (PERF_IMAGE_H-y-1)*PERF_IMAGE_W] = color;
    }
    return end;
}

void perf_update_history(struct GAME_STATE_PERF *perf)
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

static void draw_joy_buttons(struct JOYSTICK *joy, int y)
{
    font_align(FONT_ALIGN_LEFT);
    if (joy->cur & JOY_BTN_UP)    { font_move(10+0*10, y); font_printf("^"); }
    if (joy->cur & JOY_BTN_DOWN)  { font_move(10+1*10, y); font_printf("V"); }
    if (joy->cur & JOY_BTN_LEFT)  { font_move(10+2*10, y); font_printf("<-"); }
    if (joy->cur & JOY_BTN_RIGHT) { font_move(10+3*10, y); font_printf("->"); }

    if (joy->cur & JOY_BTN_B) { font_move(10+0*12, y+10); font_printf("B"); }
    if (joy->cur & JOY_BTN_Y) { font_move(10+1*12, y+10); font_printf("Y"); }
    if (joy->cur & JOY_BTN_A) { font_move(10+2*12, y+10); font_printf("A"); }
    if (joy->cur & JOY_BTN_X) { font_move(10+3*12, y+10); font_printf("X"); }

    if (joy->cur & JOY_BTN_L1) { font_move(10+0*18, y+20); font_printf("L1"); }
    if (joy->cur & JOY_BTN_R1) { font_move(10+1*18, y+20); font_printf("R1"); }
    if (joy->cur & JOY_BTN_L2) { font_move(10+2*18, y+20); font_printf("L2"); }
    if (joy->cur & JOY_BTN_R2) { font_move(10+3*18, y+20); font_printf("R2"); }

    if (joy->cur & JOY_BTN_SELECT) { font_move(10+0*42, y+30); font_printf("SELECT"); }
    if (joy->cur & JOY_BTN_START)  { font_move(10+1*42, y+30); font_printf("START"); }
}

void perf_draw_info(struct JOYSTICK *joy)
{
    font_align(FONT_ALIGN_LEFT);
    font_move(10, 10);
    font_printf("%d fps", game_state.fps_count);

    draw_perf_history(180, 10);

    if (game_state.display.show_perf >= 2) {
        font_move(10, 20); font_printf("pos  %4ld,%-4ld\n", game_state.player.x, game_state.player.y);
        font_move(10, 30); font_printf("vel %5ld,%-5ld\n", game_state.player_control.dx, game_state.player_control.dy);
        font_move(10, 40); font_printf("state %d\n", game_state.player.state);
        font_move(10, 50); font_printf("frame %d\n", game_state.player.char_anim.frame >> 8);
        font_move(10, 60); font_printf("joy %08lx\n", joy->cur);

        draw_joy_buttons(joy, 80);

        font_move(10, 160); font_printf("mod number: %d", game_state.mod.index);
        font_move(10, 170); font_printf("mod volume: %03x", game_state.mod.volume);
        if (game_state.display.msg_mod_event_frames_left) {
            font_move(10, 180); font_printf("mod event: %d, %d", game_state.mod.event_chan, game_state.mod.event_data);
        }
        if (game_state.display.msg_save_frames_left) {
            font_move(10, 190); font_printf("%s", game_state.display.save_success ? "game saved" : "error saving game");
        }
        if (game_state.display.msg_load_frames_left) {
            font_move(10, 200); font_printf("%s", game_state.display.load_success ? "game loaded" : "error loading game");
        }
        font_move(10, 226); font_printf("v%llx", get_compilation_timestamp());
    }

    if (game_state.display.show_perf >= 3) {
        for (int y = 85; y < 175; y++) memset(vga_screen.lines8[y] + 210, 0, PERF_IMAGE_W - 30);
        font_set_color(PERF_COLOR_VSYNC);    font_move(216,  90); font_printf("vsync    %5lu", game_state.perf.vsync_us);
        font_set_color(PERF_COLOR_RENDER);   font_move(216, 100); font_printf("render   %5lu", game_state.perf.render_us);
        font_set_color(PERF_COLOR_ROOM_FG);  font_move(216, 110); font_printf("room_fg  %5lu", game_state.perf.room_fg_us);
        font_set_color(PERF_COLOR_PLAYER);   font_move(216, 120); font_printf("player   %5lu", game_state.perf.player_us);
        font_set_color(PERF_COLOR_ROOM_BG);  font_move(216, 130); font_printf("room_bg  %5lu", game_state.perf.room_bg_us);
        font_set_color(PERF_COLOR_UPDATE);   font_move(216, 140); font_printf("logic    %5lu", game_state.perf.update_us);
        font_set_color(PERF_COLOR_JOY_READ); font_move(216, 150); font_printf("joy read %5lu", game_state.perf.joy_read_us);
        font_set_color(PERF_COLOR_JOY_REQ);  font_move(216, 160); font_printf("joy req  %5lu", game_state.perf.joy_req_us);
        font_set_color(0xff);
    }
}
