#ifndef AUDIO_MIXER_H_FILE
#define AUDIO_MIXER_H_FILE

#include <stdint.h>
#include <stdbool.h>

#define AUDIO_MIXER_MAX_SOURCES 4

#define AUDIO_MIXER_DEFAULT_VOLUME      (1u<<8)
#define AUDIO_MIXER_DEFAULT_PITCH_SHIFT (1u<<12)

typedef void (*audio_callback)(int16_t *samples, uint32_t len);

#ifdef __cplusplus
extern "C" {
#endif

void audio_mixer_init(void);
void audio_mixer_step(void);

void audio_mixer_set_callback(audio_callback func, int16_t volume);
void audio_mixer_set_callback_volume(int16_t volume);

int audio_mixer_play_sfx_once(uint16_t id, uint16_t bits_per_sample, const void *samples, uint32_t len,
                              uint16_t volume, uint32_t pitch_shift);
int audio_mixer_play_sfx_loop(uint16_t id, uint16_t bits_per_sample, const void *samples, uint32_t len,
                              uint32_t loop_start, uint16_t volume, uint32_t pitch_shift);

void audio_mixer_stop_sfx(uint16_t id);
void audio_mixer_set_sfx_volume(uint16_t id, int16_t volume);
bool audio_mixer_check_sfx_playing(uint16_t id);

#ifdef __cplusplus
}
#endif

#endif /* AUDIO_MIXER_H_FILE */
