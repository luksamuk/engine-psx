#include "input.h"
#include <psxapi.h>
#include <psxpad.h>

static uint8_t  _padbuff[2][34];
static uint16_t _cur_state = 0;
static uint16_t _old_state = 0;

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
        _old_state = _cur_state;
        _cur_state = ~pad->btn;
    }
}

uint16_t
pad_pressing(PadButton b)
{
    return _cur_state & b;
}

uint16_t
pad_pressed(PadButton b)
{
    return !(_old_state & b) && (_cur_state & b);
}
