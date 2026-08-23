#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pico/stdlib.h"

#include "screen.h"
#include "config.h"
#include "run_state.h"
#include "draw_room.h"
#include "sprite_shadow.h"

#include "lib/vga_8bit.h"
#include "lib/vga_font_8bit.h"
#include "lib/compilation_timestamp.h"
#include "lib/joystick.h"
#include "lib/mem.h"

#include "game_data.h"

static struct MEM_ARENA draw_frame_arena;

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
    mem_init(&draw_frame_arena, scratch, scratch_size);

    font_set_font(&raven_fonts[RAVEN_FONT_ID_6X8]);
    font_set_color(0xff);

    return 0;
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

static void draw_joy_buttons(struct JOYSTICK *joy, int y)
{
    font_align(FONT_ALIGN_LEFT);
    if (joy->cur & JOY_BTN_UP)    { font_move(10+0*10, y); font_printf("^"); }
    if (joy->cur & JOY_BTN_DOWN)  { font_move(10+1*10, y); font_printf("V"); }
    if (joy->cur & JOY_BTN_LEFT)  { font_move(10+2*10, y); font_printf("<-"); }
    if (joy->cur & JOY_BTN_RIGHT) { font_move(10+3*10, y); font_printf("->"); }

    if (joy->cur & JOY_BTN_A) { font_move(10+0*12, y+10); font_printf("B"); }
    if (joy->cur & JOY_BTN_B) { font_move(10+1*12, y+10); font_printf("Y"); }
    if (joy->cur & JOY_BTN_C) { font_move(10+2*12, y+10); font_printf("A"); }
    if (joy->cur & JOY_BTN_D) { font_move(10+3*12, y+10); font_printf("X"); }

    if (joy->cur & JOY_BTN_L1) { font_move(10+0*18, y+20); font_printf("L1"); }
    if (joy->cur & JOY_BTN_R1) { font_move(10+1*18, y+20); font_printf("R1"); }
    if (joy->cur & JOY_BTN_L2) { font_move(10+2*18, y+20); font_printf("L2"); }
    if (joy->cur & JOY_BTN_R2) { font_move(10+3*18, y+20); font_printf("R2"); }

    if (joy->cur & JOY_BTN_SELECT) { font_move(10+0*42, y+30); font_printf("SELECT"); }
    if (joy->cur & JOY_BTN_START)  { font_move(10+1*42, y+30); font_printf("START"); }
}

static void draw_player(struct GAME_STATE *game)
{
    if (game->player.shadow_enabled) {
        sprite_shadow_draw(game->player.sprite, run_state.screen_x, run_state.screen_y, sprite_shadow_colors);
    }

    vga_image_draw_frame(game->player.sprite, game->player.sprite_frame,
                         game->player.sprite_x - run_state.screen_x, game->player.sprite_y - run_state.screen_y, 1);
}

static void draw_room(struct GAME_STATE *game)
{
    struct DRAW_ROOM_INFO *info = draw_room_init_frame(&draw_frame_arena, game);

    draw_room_bg(info);
    RUN_PERF(room_bg_us);

    draw_player(game);
    RUN_PERF(player_us);

    for (int i = 0; i < game->num_enemies; i++) {
        struct RAVEN_CHARACTER *enemy = &game->enemies[i];
        vga_image_draw_frame(enemy->sprite, enemy->sprite_frame,
                             enemy->sprite_x - run_state.screen_x, enemy->sprite_y - run_state.screen_y, 1);
    }
    RUN_PERF(enemies_us);

    draw_room_fg(info);
    RUN_PERF(room_fg_us);

    if (run_state.display.show_perf) {
        // collision rect
        int sx = game->player.x - run_state.screen_x;
        int sy = game->player.y - run_state.screen_y;
        int w = game->player.anim->collision.w;
        int h = game->player.anim->collision.h;
        for (int x = 0; x < w; x++) {
            vga_screen.lines8[sy][sx+x] = 0x38;
            vga_screen.lines8[sy+h-1][sx+x] = 0x38;
        }
        for (int y = 0; y < h; y++) {
            vga_screen.lines8[sy+y][sx] = 0x38;
            vga_screen.lines8[sy+y][sx+w-1] = 0x38;
        }
    }
}

void screen_render(struct GAME_STATE *game, struct JOYSTICK *joy)
{
    mem_clear(&draw_frame_arena);
    //vga_clear_screen(0);

    draw_room(game);

    if (run_state.display.show_perf) {
        font_align(FONT_ALIGN_LEFT);
        font_move(10, 10);
        font_printf("%d fps", run_state.fps_count);

        font_move(10, 20); font_printf("pos  %4ld,%-4ld\n", game->player.x, game->player.y);
        font_move(10, 30); font_printf("vel %5ld,%-5ld\n", game->player_control.dx, game->player_control.dy);
        font_move(10, 40); font_printf("state %d\n", game->player.state);
        font_move(10, 50); font_printf("frame %d\n", game->player.anim_frame >> 8);
    }

    update_perf_history(&run_state.perf);
    if (run_state.display.show_perf) {
        draw_perf_history(180, 10);
        if (run_state.display.show_perf == 2) {
            for (int y = 85; y < 175; y++) memset(vga_screen.lines8[y] + 210, 0, PERF_IMAGE_W - 30);
            font_set_color(PERF_COLOR_VSYNC);    font_move(216,  90); font_printf("vsync    %5lu", run_state.perf.vsync_us);
            font_set_color(PERF_COLOR_RENDER);   font_move(216, 100); font_printf("render   %5lu", run_state.perf.render_us);
            font_set_color(PERF_COLOR_ROOM_FG);  font_move(216, 110); font_printf("room_fg  %5lu", run_state.perf.room_fg_us);
            font_set_color(PERF_COLOR_PLAYER);   font_move(216, 120); font_printf("player   %5lu", run_state.perf.player_us);
            font_set_color(PERF_COLOR_ROOM_BG);  font_move(216, 130); font_printf("room_bg  %5lu", run_state.perf.room_bg_us);
            font_set_color(PERF_COLOR_UPDATE);   font_move(216, 140); font_printf("logic    %5lu", run_state.perf.update_us);
            font_set_color(PERF_COLOR_JOY_READ); font_move(216, 150); font_printf("joy read %5lu", run_state.perf.joy_read_us);
            font_set_color(PERF_COLOR_JOY_REQ);  font_move(216, 160); font_printf("joy req  %5lu", run_state.perf.joy_req_us);
            font_set_color(0xff);
        }
    }

    if (run_state.display.show_perf) {
        draw_joy_buttons(joy, 80);

        font_move(10, 160); font_printf("mod number: %d", run_state.mod.index);
        font_move(10, 170); font_printf("mod volume: %03x", run_state.mod.volume);
        if (run_state.display.msg_mod_event_frames_left) {
            font_move(10, 180); font_printf("mod event: %d, %d", run_state.mod.event_chan, run_state.mod.event_data);
        }
        if (run_state.display.msg_save_frames_left) {
            font_move(10, 190); font_printf("%s", run_state.display.save_success ? "game saved" : "error saving game");
        }
        if (run_state.display.msg_load_frames_left) {
            font_move(10, 200); font_printf("%s", run_state.display.load_success ? "game loaded" : "error loading game");
        }
        font_move(10, 226); font_printf("version %llx", get_compilation_timestamp());
    }

    RUN_PERF(render_us);
}

void screen_save_to_scratch(int clear_after)
{
    int len = vga_screen.width * vga_screen.height;
    memcpy(draw_frame_arena.data, vga_screen.framebuffer, len);
    if (clear_after) {
        memset(vga_screen.framebuffer, 0, len);
    }
}

void screen_fade_from_scratch(int level)
{
    int len = vga_screen.width * vga_screen.height;
    uint32_t *src = draw_frame_arena.data;
    uint32_t *dst = vga_screen.framebuffer;
    switch (level) {
    case 2:
        for (int i = len/4; i > 0; i--) {
            *dst++ = (*src++ >> 1) & 0x5b5b5b5b;
        }
        break;

    case 1:
        for (int i = len/4; i > 0; i--) {
            *dst++ = (*src++ >> 2) & 0x09090909;
        }
        break;

    default:
        memset(dst, 0, len);
        break;
    }
}
