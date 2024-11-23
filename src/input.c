#include "input.h"
#include <psxapi.h>
#include <psxpad.h>

static uint8_t  _padbuff[2][34];
static InputState _gstate = { 0 };

void
pad_init(void)
{
    InitPAD(_padbuff[0], 34, _padbuff[1], 34);
    StartPAD();
    ChangeClearPAD(0);
}

void
pad_update(void)
{
    PADTYPE *pad = ((PADTYPE*)_padbuff[0]);
    if((pad->stat == 0) &&
       ((pad->type == PAD_ID_DIGITAL)         // Digital
        || (pad->type == PAD_ID_ANALOG_STICK) // DualShock in digital mode
        || (pad->type == PAD_ID_ANALOG))) {   // DualShock in analog mode
        _gstate.old = _gstate.current;
        _gstate.current = ~pad->btn;
    }
}

void
input_get_state(InputState *s)
{
    *s = _gstate;
}

uint16_t
input_pressing(InputState *s, PadButton b)
{
    return s->current & b;
}

uint16_t
input_pressed(InputState *s, PadButton b)
{
    return !(s->old & b) && (s->current & b);
}

uint16_t
pad_pressing(PadButton b)
{
    return input_pressing(&_gstate, b);
}

uint16_t
pad_pressed(PadButton b)
{
    return input_pressed(&_gstate, b);
}

uint16_t
pad_pressed_any()
{
    return _gstate.current != 0x0000;
}
