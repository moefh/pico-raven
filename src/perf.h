#ifndef PERF_H_FILE
#define PERF_H_FILE

struct GAME_STATE_PERF;
struct JOYSTICK;

void perf_update_history(struct GAME_STATE_PERF *perf);
void perf_draw_info(void);

#endif /* PERF_H_FILE */
