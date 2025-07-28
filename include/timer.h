#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

void     timer_init();
void     timer_update();
int      get_frame_rate();

uint32_t get_elapsed_frames();
void     pause_elapsed_frames();
void     resume_elapsed_frames();
void     reset_elapsed_frames();
uint8_t  elapsed_frames_paused();

uint32_t get_global_frames();

#endif
