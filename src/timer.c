#include "timer.h"
#include <psxapi.h>
#include <psxetc.h>
#include <stdio.h>

volatile int timer_counter = 0;
volatile int frame_counter = 0;
volatile int frame_rate = 0;

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
}

int
get_frame_rate()
{
    return frame_rate;
}
