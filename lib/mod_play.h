#ifndef MOD_PLAY_H_FILE
#define MOD_PLAY_H_FILE

#include "mod_data.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*mod_play_event_callback)(int chan, int event, void *callback_data);

void mod_play_init(unsigned int out_frequency);
void mod_play_set_event_callback(mod_play_event_callback callback, void *callback_data);
void mod_play_start(const struct MOD_DATA *mod_data, int loop);
int  mod_play_step(int16_t *out, unsigned int len);

#ifdef __cplusplus
}
#endif

#endif /* MOD_PLAY_H_FILE */
