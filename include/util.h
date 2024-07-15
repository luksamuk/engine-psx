#ifndef UTIL_H
#define UTIL_H

#include <psxgpu.h>
#include <psxgte.h>

int RotAverageNclip4(SVECTOR *a, SVECTOR *b, SVECTOR *c, SVECTOR *d,
                     long *xy0, long *xy1, long *xy2, long *xy3,
                     int *otz);

char *file_read(const char *filename, unsigned long *length);
void load_texture(char *data, TIM_IMAGE *tim);

char  get_byte(char *bytes, unsigned long *b);
short get_short_be(char *bytes, unsigned long *b);
short get_short_le(char *bytes, unsigned long *b);

#endif
