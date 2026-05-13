
#include "pico/multicore.h"

#include "core_msg.h"

#define MSG_QUEUE_LENGTH 4  // max messages in queue

static queue_t core0_msg_queue;
static queue_t core1_msg_queue;

void core_msg_init(void)
{
  queue_init(&core0_msg_queue, sizeof(union CORE_MSG), MSG_QUEUE_LENGTH);
  queue_init(&core1_msg_queue, sizeof(union CORE_MSG), MSG_QUEUE_LENGTH);
}

bool core0_msg_try_recv(union CORE_MSG *msg)
{
  return queue_try_remove(&core0_msg_queue, msg);
}

bool core1_msg_try_recv(union CORE_MSG *msg)
{
  return queue_try_remove(&core1_msg_queue, msg);
}

static void send_to_core0(union CORE_MSG *msg)
{
  return queue_add_blocking(&core0_msg_queue, msg);
}

static void send_to_core1(union CORE_MSG *msg)
{
  return queue_add_blocking(&core1_msg_queue, msg);
}

// ========================================================================
// === CORE0 -> CORE1
// ========================================================================

void msg_audio_init(int pio_num, int data_pin, int clock_pin_base, int sample_freq)
{
  union CORE_MSG msg;
  msg.msg_header.type            = CORE1_MSG_TYPE_AUDIO_INIT;
  msg.audio_init.pio_num         = pio_num;
  msg.audio_init.data_pin        = data_pin;
  msg.audio_init.clock_pin_base  = clock_pin_base;
  msg.audio_init.sample_freq     = sample_freq;
  send_to_core1(&msg);
}

void msg_sfx_play_once(uint16_t sfx_id, const struct RAVEN_SFX *sfx, uint16_t volume, uint32_t pitch_shift)
{
  union CORE_MSG msg;
  msg.msg_header.type          = CORE1_MSG_TYPE_SFX_PLAY;
  msg.sfx_play.sfx_id          = sfx_id;
  msg.sfx_play.bits_per_sample = sfx->bits_per_sample;
  msg.sfx_play.samples         = sfx->samples;
  msg.sfx_play.len             = sfx->len;
  msg.sfx_play.loop_start      = sfx->len;
  msg.sfx_play.pitch_shift     = pitch_shift;
  msg.sfx_play.volume          = volume;
  send_to_core1(&msg);
}

void msg_sfx_play_loop(uint16_t sfx_id, const struct RAVEN_SFX *sfx, uint16_t volume, uint32_t pitch_shift)
{
  union CORE_MSG msg;
  msg.msg_header.type          = CORE1_MSG_TYPE_SFX_PLAY;
  msg.sfx_play.sfx_id          = sfx_id;
  msg.sfx_play.bits_per_sample = sfx->bits_per_sample;
  msg.sfx_play.samples         = sfx->samples;
  msg.sfx_play.len             = sfx->len;
  msg.sfx_play.loop_start      = sfx->loop_start;
  msg.sfx_play.pitch_shift     = pitch_shift;
  msg.sfx_play.volume          = volume;
  send_to_core1(&msg);
}

void msg_sfx_stop(uint16_t sfx_id)
{
  union CORE_MSG msg;
  msg.msg_header.type = CORE1_MSG_TYPE_SFX_STOP;
  msg.sfx_stop.sfx_id = sfx_id;
  send_to_core1(&msg);
}

void msg_sfx_set_volume(uint16_t sfx_id, uint16_t volume)
{
  union CORE_MSG msg;
  msg.msg_header.type       = CORE1_MSG_TYPE_SFX_SET_VOLUME;
  msg.sfx_set_volume.sfx_id = sfx_id;
  msg.sfx_set_volume.volume = volume;
  send_to_core1(&msg);
}

void msg_mod_play(const struct MOD_DATA *mod, uint16_t volume, bool loop)
{
  union CORE_MSG msg;
  msg.msg_header.type = CORE1_MSG_TYPE_MOD_PLAY;
  msg.mod_play.mod    = mod;
  msg.mod_play.volume = volume;
  msg.mod_play.loop   = loop;
  send_to_core1(&msg);
}

void msg_mod_set_volume(uint16_t volume)
{
  union CORE_MSG msg;
  msg.msg_header.type       = CORE1_MSG_TYPE_MOD_SET_VOLUME;
  msg.mod_set_volume.volume = volume;
  send_to_core1(&msg);
}

void msg_mod_stop(void)
{
  union CORE_MSG msg;
  msg.msg_header.type = CORE1_MSG_TYPE_MOD_STOP;
  send_to_core1(&msg);
}

// ========================================================================
// === CORE1 -> CORE0
// ========================================================================

void msg_mod_event(uint8_t chan, uint8_t event)
{
  union CORE_MSG msg;
  msg.msg_header.type = CORE0_MSG_TYPE_MOD_EVENT;
  msg.mod_event.chan = chan;
  msg.mod_event.event = event;
  send_to_core0(&msg);
}
