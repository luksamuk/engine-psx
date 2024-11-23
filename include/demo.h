#ifndef DEMO_H
#define DEMO_H

#include "input.h"

// Tools for recording and playing game demos.

typedef struct {
    uint16_t state;
    uint32_t num_frames;
} DemoPlaybackSample;

typedef struct {
    InputState state;
    uint32_t   timer;
    uint32_t   sample;
} DemoState;

void demo_init();

// The way we record input is by actually measuring the number of frames
// that the current state has taken a certain value, then we output that
// value in hex along with the number of frames (1-based) that it stayed
// that way.
void demo_record();

void demo_update_playback(int level, InputState *s);


#endif
