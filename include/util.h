#ifndef UTIL_H
#define UTIL_H

#include <psxgpu.h>
#include <psxgte.h>

int RotTransPers(SVECTOR *v, uint32_t *xy0);
int RotAverageNclip4(SVECTOR *a, SVECTOR *b, SVECTOR *c, SVECTOR *d,
                     uint32_t *xy0, uint32_t *xy1, uint32_t *xy2, uint32_t *xy3,
                     int *otz);

uint8_t *file_read(const char *filename, uint32_t *length);
void load_texture(uint8_t *data, TIM_IMAGE *tim);

uint8_t  get_byte(uint8_t *bytes, uint32_t *b);
uint16_t get_short_be(uint8_t *bytes, uint32_t *b);
uint16_t get_short_le(uint8_t *bytes, uint32_t *b);

#endif
