#include "timer.h"
#include <psxapi.h>
#include <psxetc.h>
#include <stdio.h>

extern uint8_t paused;

volatile int      timer_counter = 0;
volatile int      frame_counter = 0;
volatile int      frame_rate = 0;
volatile uint8_t  counting_frames = 0;
volatile uint32_t frame_count = 0;
volatile uint32_t global_count = 0;

void
timer_tick()
{
    timer_counter--;
    if(timer_counter == 0) {
        timer_counter = 100;
        frame_rate    = frame_counter;
        frame_counter = 0;
    }
}

void
timer_init()
{
    timer_counter = 100;
    EnterCriticalSection();
    TIMER_CTRL(2) = 0X0258;              // CLK/8 input, IRQ on reload
    TIMER_RELOAD(2) = (F_CPU / 8) / 100; // 100 Hz

    // Configure timer 2 IRQ
    ChangeClearRCnt(2, 0);
    InterruptCallback(6, &timer_tick);
    ExitCriticalSection();
}

inline void
timer_update()
{
    frame_counter++;
    if(counting_frames && !paused)
        frame_count++;
    global_count++;
}

int
get_frame_rate()
{
    return frame_rate;
}

uint32_t
get_elapsed_frames()
{
    return frame_count;
}

uint32_t
get_global_frames()
{
    return global_count;
}

void
pause_elapsed_frames()
{
    counting_frames = 0;
}

void
resume_elapsed_frames()
{
    counting_frames = 1;
}

void
reset_elapsed_frames()
{
    frame_count = 0;
    counting_frames = 1;
}

uint8_t
elapsed_frames_paused()
{
    return !counting_frames;
}
