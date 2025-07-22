#ifndef BASIC_FONT_H
#define BASIC_FONT_H

#include <stdint.h>

#define GLYPH_WHITE_WIDTH   5
#define GLYPH_WHITE_HEIGHT 11
#define GLYPH_GAP           1

#define GLYPH_MD_WHITE_WIDTH  14
#define GLYPH_MD_WHITE_HEIGHT 18
#define GLYPH_MD_GAP           0

#define GLYPH_SML_WHITE_WIDTH    8
#define GLYPH_SML_WHITE_HEIGHT  11
#define GLYPH_SML_GAP            0

#define GLYPH_HG_WHITE_WIDTH    16
#define GLYPH_HG_WHITE_HEIGHT   24
#define GLYPH_HG_GAP             0

void font_init();
void font_flush();
void font_draw_big(const char *text, int16_t vx, int16_t vy);
void font_draw_md(const char *text, int16_t vx, int16_t vy);
void font_draw_sm(const char *text, int16_t vx, int16_t vy);
void font_draw_hg(const char *text, int16_t vx, int16_t vy);

uint16_t font_measurew_big(const char *text);
uint16_t font_measurew_md(const char *text);
uint16_t font_measurew_sm(const char *text);
uint16_t font_measurew_hg(const char *text);

void font_set_color(uint8_t r0, uint8_t g0, uint8_t b0);
void font_set_color_sonic();
void font_set_color_miles();
void font_set_color_knuckles();
void font_set_color_super();
void font_reset_color();

#endif
