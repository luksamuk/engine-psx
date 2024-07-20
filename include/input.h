#ifndef INPUT_H
#define INPUT_H

#include <psxpad.h>

void pad_init(void);
void pad_update(void);

uint16_t pad_pressing(PadButton b);
uint16_t pad_pressed(PadButton b);

#endif
