#ifndef BASIC_FONT_H
#define BASIC_FONT_H

#include <stdint.h>

#define GLYPH_WHITE_WIDTH   5
#define GLYPH_WHITE_HEIGHT 11
#define GLYPH_GAP           1

#define GLYPH_SML_WHITE_WIDTH    8
#define GLYPH_SML_WHITE_HEIGHT  11
#define GLYPH_SML_GAP            0

void font_init();
void font_flush();
void font_draw_big(const char *text, int16_t vx, int16_t vy);
void font_draw_sm(const char *text, int16_t vx, int16_t vy);

uint16_t font_measurew_big(const char *text);
uint16_t font_measurew_sm(const char *text);

void font_set_color(uint8_t r0, uint8_t g0, uint8_t b0);
void font_reset_color();

#endif