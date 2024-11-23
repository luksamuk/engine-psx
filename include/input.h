#ifndef INPUT_H
#define INPUT_H

#include <psxpad.h>

typedef struct {
    uint16_t current;
    uint16_t old;
} InputState;

void pad_init(void);
void pad_update(void);

void input_get_state(InputState *);

uint16_t input_pressing(InputState *, PadButton);
uint16_t input_pressed(InputState *, PadButton);

uint16_t pad_pressing(PadButton b);
uint16_t pad_pressed(PadButton b);
uint16_t pad_pressed_any();

#endif
