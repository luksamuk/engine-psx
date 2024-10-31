#ifndef BASIC_FONT_H
#define BASIC_FONT_H

#include <stdint.h>

void font_init();
void font_flush();
void font_draw_big(const char *text, int16_t vx, int16_t vy);
void font_draw_sm(const char *text, int16_t vx, int16_t vy);

void font_set_color(uint8_t r0, uint8_t g0, uint8_t b0);
void font_reset_color();

#endif
