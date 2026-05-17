/**
 * vga_8bit.c
 *
 * Copyright (C) 2026 MoeFH
 * Released under the MIT License
 */

#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "hardware/pll.h"
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "hardware/structs/bus_ctrl.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#if PICO_PIO_VERSION < 1
#error "PIO version 1 or above is required; RP2040 is not supported"
#endif

#include "vga_8bit.h"
#include "vga_8bit.pio.h"

//                                          clock      hf  hp  hb  hpix    vf  vp  vb  vpix  vdiv (h,v)pol
const struct VGA_MODE vga_mode_320x240  = { 12587500,   8, 48, 24, 320,    10,  2, 33, 480,    2,  1,1 };  //  640x480@60 / 2

static const struct VGA_MODE *vga_mode = NULL;

#define PIX_CLOCK_MHZ (vga_mode->pixel_clock_mhz)
#define H_FRONT_PORCH (vga_mode->h_front_porch)
#define H_SYNC_PULSE  (vga_mode->h_sync_pulse)
#define H_BACK_PORCH  (vga_mode->h_back_porch)
#define H_PIXELS      (vga_mode->h_pixels)
#define V_FRONT_PORCH (vga_mode->v_front_porch)
#define V_SYNC_PULSE  (vga_mode->v_sync_pulse)
#define V_BACK_PORCH  (vga_mode->v_back_porch)
#define V_PIXELS      (vga_mode->v_pixels)
#define V_DIV         (vga_mode->v_div)
#define H_POLARITY    (vga_mode->h_polarity)
#define V_POLARITY    (vga_mode->v_polarity)

#define H_FULL_LINE   (H_FRONT_PORCH+H_SYNC_PULSE+H_BACK_PORCH+H_PIXELS)
#define V_FULL_FRAME  (V_FRONT_PORCH+V_SYNC_PULSE+V_BACK_PORCH+V_PIXELS)

#define HSYNC_ON           (!H_POLARITY)
#define HSYNC_OFF          ( H_POLARITY)
#define VSYNC_ON           (!V_POLARITY)
#define VSYNC_OFF          ( V_POLARITY)
#define HBLANK_BUFFER_LEN  ((H_FRONT_PORCH+H_SYNC_PULSE+H_BACK_PORCH)/4)
#define HPIXELS_BUFFER_LEN (H_PIXELS/4)

#define SCREEN_WIDTH  H_PIXELS
#define SCREEN_HEIGHT (V_PIXELS/V_DIV)

//#define DEBUG(fmt, ...) do { printf(fmt, ##__VA_ARGS__); fflush(stdout); } while (0)
#define DEBUG(...)

static uint32_t *blank_line_buffer;
static uint32_t *framebuffers[2];
static uint32_t **cur_framebuffer_lines;

struct DMA_BUFFER_INFO {
  uintptr_t read_addr;
  uintptr_t write_addr;
  uint32_t  transfer_count;
  uint32_t  ctrl_trig;
};
static struct DMA_BUFFER_INFO *dma_chain;
static void *dma_restart_buffer[1];
static uint dma_control_chan;
static uint dma_data_chan;

static uint pio_vsync_sm;

static volatile uint frame_count;
static uint cur_framebuffer;
struct VGA_SCREEN vga_screen;

static void __isr __time_critical_func(dma_handler)(void)
{
  dma_hw->ints0 = 1u << dma_data_chan;
  frame_count++;
}

static void set_dma_buffer_src(struct DMA_BUFFER_INFO *buf, volatile void *src, uint32_t count)
{
  buf->read_addr = (uintptr_t) src;
  buf->transfer_count = count;
}

static void set_dma_buffer_dst(struct DMA_BUFFER_INFO *buf, volatile void *dest, uint32_t ctrl)
{
  buf->write_addr = (uintptr_t) dest;
  buf->ctrl_trig = ctrl;
}

static void setup_pio_dma(PIO pio, uint sm)
{
    uint pio_dreq = pio_get_dreq(pio, sm, true);
    dma_control_chan = dma_claim_unused_channel(true);
    dma_data_chan    = dma_claim_unused_channel(true);

    // DMA control channel config
    dma_channel_config cfg = dma_channel_get_default_config(dma_control_chan);
    channel_config_set_transfer_data_size(&cfg, DMA_SIZE_32);
    channel_config_set_read_increment(&cfg, true);
    channel_config_set_write_increment(&cfg, true);
    channel_config_set_ring(&cfg, true, 4);    // loop write address every 1<<4 = 16 bytes

    dma_channel_configure(dma_control_chan,
                          &cfg,
                          &dma_hw->ch[dma_data_chan].read_addr,     // dest (update data channel and trigger it)
                          &dma_chain[0],                            // source
                          4,                                        // num words for each transfer
                          false                                     // don't start now
                          );

    // all blocks of dma_chain except last are set to trigger dma_data_chan to copy data to PIO
    for (int i = 0; i < V_FULL_FRAME; i++) {
        // src will be set by init_buffers() and vga_swap_buffers()
        set_dma_buffer_dst(&dma_chain[i],
                           &pio->txf[sm],                                              // write to PIO
                           DMA_CH0_CTRL_TRIG_INCR_READ_BITS                         |  // increment read ptr
                           (pio_dreq            << DMA_CH0_CTRL_TRIG_TREQ_SEL_LSB)  |  // as fast as PIO requires
                           (dma_control_chan    << DMA_CH0_CTRL_TRIG_CHAIN_TO_LSB)  |  // chain to dma_control_chan
                           (((uint)DMA_SIZE_32) << DMA_CH0_CTRL_TRIG_DATA_SIZE_LSB) |  // copy 32 bits per count
                           DMA_CH0_CTRL_TRIG_IRQ_QUIET_BITS                         |  // suppress IRQ
                           DMA_CH0_CTRL_TRIG_EN_BITS);
    }

    // last block of dma_chain is set to trigger dma_data_chan to copy the dma_chain start address to the control chain (restarting it)
    set_dma_buffer_src(&dma_chain[V_FULL_FRAME], dma_restart_buffer, 1);
    set_dma_buffer_dst(&dma_chain[V_FULL_FRAME],
                       &dma_hw->ch[dma_control_chan].al3_read_addr_trig,           // write to dma_control_chan read address trigger
                       (DREQ_FORCE          << DMA_CH0_CTRL_TRIG_TREQ_SEL_LSB)  |  // as fast as possible
                       (dma_data_chan       << DMA_CH0_CTRL_TRIG_CHAIN_TO_LSB)  |  // chain to itself (don't chain)
                       (((uint)DMA_SIZE_32) << DMA_CH0_CTRL_TRIG_DATA_SIZE_LSB) |  // copy 32 bits per count
                       0                                                        |  // trigger IRQ
                       DMA_CH0_CTRL_TRIG_EN_BITS);

    dma_channel_set_irq0_enabled(dma_data_chan, true);
    irq_set_exclusive_handler(DMA_IRQ_0, dma_handler);
    irq_set_priority(DMA_IRQ_0, 0xff);
    irq_set_enabled(DMA_IRQ_0, true);
}

/**
 * Try to find a system clock frequency that's an integer multiple of
 * the pixel clock for the given mode. Return a frequency that will be
 * accepted by `set_sys_clock_hz()`, or 0 if not found.
 */
static uint32_t get_nearest_sys_clock_hz(const struct VGA_MODE *mode, uint32_t mult_boost)
{
    uint mult = clock_get_hz(clk_sys) / (mode->pixel_clock_mhz * 2);
    if (mult % (mode->pixel_clock_mhz * 2) != 0) mult++;
    uint freq_hz = mode->pixel_clock_mhz * 2 * (mult + mult_boost);
    if (freq_hz > 200000000) return 0;  // refuse to consider anything above 200MHz
    uint best_freq = 0;
    uint best_diff = UINT_MAX;

    // this was taken from the pico sdk function `check_sys_clock_hz()`:
    uint reference_freq_hz = XOSC_HZ / PLL_SYS_REFDIV;
    for (uint fbdiv = 320; fbdiv >= 16; fbdiv--) {
        uint vco_hz = fbdiv * reference_freq_hz;
        if (vco_hz < PICO_PLL_VCO_MIN_FREQ_HZ || vco_hz > PICO_PLL_VCO_MAX_FREQ_HZ) continue;
        for (uint postdiv1 = 7; postdiv1 >= 1; postdiv1--) {
            for (uint postdiv2 = postdiv1; postdiv2 >= 1; postdiv2--) {
                uint out = vco_hz / (postdiv1 * postdiv2);
                if (vco_hz % (postdiv1 * postdiv2) == 0) {
                    uint32_t diff = (out > freq_hz) ? out - freq_hz : freq_hz - out;
                    if (diff < best_diff) {
                        best_diff = diff;
                        best_freq = out;
                        if (diff == 0) {
                            return best_freq;
                        }
                    }
                }
            }
        }
    }
    return best_freq;
}

static void init_pio(uint32_t pio_num, uint32_t pin_out_base, bool adjust_sys_clock)
{
    DEBUG("vga mode: pixel_clock=%d, full_w=%d, full_h=%d\n", (int)PIX_CLOCK_MHZ, H_FULL_LINE, V_FULL_FRAME);

    uint32_t sys_freq;
    if (adjust_sys_clock) {
        sys_freq = get_nearest_sys_clock_hz(vga_mode, 0);
        if (sys_freq == 0) {
            sys_freq = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_SYS) * 1000;
            DEBUG("can't get good clock for VGA, proceeding with current clock: %d MHz\n", (int) sys_freq);
        } else {
            DEBUG("setting system clock to %d MHz for best VGA output\n", (int) sys_freq);
            set_sys_clock_hz(sys_freq, true);
        }
    } else {
        sys_freq = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_SYS) * 1000;
    }

    float clock_div = (float)sys_freq / (float)PIX_CLOCK_MHZ / 2.0;
    clock_div = round(clock_div);
    DEBUG("system clock: %d MHz\n", (int) sys_freq);
    DEBUG("PIO clock div: %f\n", clock_div);
    DEBUG("VGA pixel clock: 2 * %f = %f (%f%% off)\n",
          sys_freq / clock_div / 2.0,
          sys_freq / clock_div,
          100.0 * fabs(((sys_freq / clock_div / 2.0) - (float)PIX_CLOCK_MHZ)) / PIX_CLOCK_MHZ);

    PIO pio = pio_get_instance(pio_num);
    uint vsync_sm = pio_claim_unused_sm(pio, true);
    uint hsync_sm = pio_claim_unused_sm(pio, true);
    uint pixels_sm = pio_claim_unused_sm(pio, true);

    // pixels program
    uint pixels_offset = pio_add_program(pio, &pixels_program);
    pixels_program_init(pio, pixels_sm, pixels_offset, pin_out_base, clock_div);
    pio_sm_set_enabled(pio, pixels_sm, true);  // stall waiting for irq from hsync program
    pio_sm_put_blocking(pio, pixels_sm, H_PIXELS-1);
    pio_sm_set_enabled(pio, pixels_sm, false);
    setup_pio_dma(pio, pixels_sm);
    pio_sm_set_enabled(pio, pixels_sm, true);
    DEBUG("initialized pixels sm\n");

    // hsync program
    uint hsync_offset = pio_add_program(pio, &hsync_program);
    hsync_program_init(pio, hsync_sm, hsync_offset, pin_out_base+8, H_POLARITY, clock_div);
    pio->rxf_putget[hsync_sm][0] = H_SYNC_PULSE-3;
    pio->rxf_putget[hsync_sm][1] = H_BACK_PORCH-2; //H_BACK_PORCH-3;
    pio_sm_set_enabled(pio, hsync_sm, true);  // stall waiting for irq from vsync program
    DEBUG("initialized hsync sm: %d, %d\n", H_SYNC_PULSE, H_BACK_PORCH);

    // vsync program
    uint vsync_offset = pio_add_program(pio, &vsync_program);
    vsync_program_init(pio, vsync_sm, vsync_offset, pin_out_base+9, V_POLARITY, clock_div);
    pio->rxf_putget[vsync_sm][0] = V_SYNC_PULSE - 1;
    pio->rxf_putget[vsync_sm][1] = V_FRONT_PORCH+V_BACK_PORCH+V_PIXELS - 1;
    pio->rxf_putget[vsync_sm][2] = H_FULL_LINE - 3;
    DEBUG("initialized vsync sm: %d+%d=%d, %d\n",
          V_SYNC_PULSE,
          (V_FRONT_PORCH+V_BACK_PORCH+V_PIXELS),
          V_SYNC_PULSE * H_FULL_LINE + (V_FRONT_PORCH+V_BACK_PORCH+V_PIXELS),
          H_FULL_LINE);

    pio_vsync_sm = vsync_sm;
}

static void clear_framebuffer(uint fb_num, uint8_t color)
{
  memset(framebuffers[fb_num], color, SCREEN_WIDTH*SCREEN_HEIGHT);
}

static int alloc_buffers(int num_framebuffers)
{
  for (int i = 0; i < num_framebuffers; i++) {
    framebuffers[i] = NULL;
  }
  cur_framebuffer_lines = NULL;
  blank_line_buffer     = NULL;
  dma_chain             = NULL;

#define ALLOC(p, size)  p = malloc(size); if (! p) goto error
  for (int i = 0; i < num_framebuffers; i++) {
    ALLOC(framebuffers[i], SCREEN_WIDTH * SCREEN_HEIGHT);
  }
  ALLOC(cur_framebuffer_lines, SCREEN_HEIGHT      * sizeof(uint32_t *));
  ALLOC(blank_line_buffer,     HPIXELS_BUFFER_LEN * sizeof(uint32_t));
  ALLOC(dma_chain,             (V_FULL_FRAME+1)   * sizeof(struct DMA_BUFFER_INFO));
#undef ALLOC

  return 0;

 error:
  for (int i = 0; i < num_framebuffers; i++) {
    free(framebuffers[i]);
  }
  free(cur_framebuffer_lines);
  free(blank_line_buffer);
  free(dma_chain);
  return -1;
}

static int init_buffers(int num_framebuffers)
{
  if (alloc_buffers(num_framebuffers) < 0) {
    return VGA_ERROR_ALLOC;
  }

  // blank pixel line
  memset(blank_line_buffer, 0, H_PIXELS);

  // framebuffers
  for (int i = 0; i < num_framebuffers; i++) {
    clear_framebuffer(i, 0);
  }

  // setup DMA chain buffers
  struct DMA_BUFFER_INFO *buf = &dma_chain[0];
  for (int i = 0; i < V_FULL_FRAME; i++) {
    if (i < V_SYNC_PULSE + V_BACK_PORCH || i >= V_SYNC_PULSE + V_BACK_PORCH + V_PIXELS) {
      // blank line
      set_dma_buffer_src(buf++, blank_line_buffer, HPIXELS_BUFFER_LEN);
    } else {
      // pixel data
      set_dma_buffer_src(buf++, NULL, HPIXELS_BUFFER_LEN);  // set by vga_swap_buffers()
    }
  }

  // setup DMA restart buffer
  dma_restart_buffer[0] = &dma_chain[0];

  return 0;
}

// === INTERFACE ====================================================

void vga_swap_buffers(bool wait_sync)
{
  if (wait_sync) {
    uint start_frame_count = frame_count;
    while (frame_count == start_frame_count) {
      //sleep_ms(1);  // should we remove this?
    }
  }

  // inject new framebuffer in DMA chain
  for (int i = 0; i < V_PIXELS; i++) {
    struct DMA_BUFFER_INFO *buf = &dma_chain[V_SYNC_PULSE + V_BACK_PORCH + i];
    set_dma_buffer_src(buf, &framebuffers[cur_framebuffer][i/V_DIV*HPIXELS_BUFFER_LEN], HPIXELS_BUFFER_LEN);
  }

  // setup old framebuffer for drawing
  cur_framebuffer = !cur_framebuffer;
  for (int i = 0; i < SCREEN_HEIGHT; i++) {
    cur_framebuffer_lines[i] = &framebuffers[cur_framebuffer][i*HPIXELS_BUFFER_LEN];
  }
  vga_screen.framebuffer = cur_framebuffer_lines[0];
}

void vga_clear_screen(unsigned char color)
{
  clear_framebuffer(cur_framebuffer, color);
}

int vga_init(const struct VGA_MODE *mode, uint32_t pio_num, uint32_t pin_out_base, bool round_clock_div)
{
    vga_mode = mode;

    int err = init_buffers(2);
    if (err < 0) return err;

    init_pio(pio_num, pin_out_base, round_clock_div);

    vga_screen.width  = SCREEN_WIDTH;
    vga_screen.height = SCREEN_HEIGHT;
    vga_screen.lines  = cur_framebuffer_lines;

    // setup first framebuffer
    cur_framebuffer = 0;
    vga_swap_buffers(false);

    dma_channel_start(dma_control_chan);
    pio_sm_set_enabled(pio_get_instance(pio_num), pio_vsync_sm, true);  // start everything
    return 0;
}
