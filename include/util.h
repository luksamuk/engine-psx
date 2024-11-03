#ifndef UTIL_H
#define UTIL_H

#include <psxgpu.h>
#include <psxgte.h>

// These definitions should be given by CMake
#ifndef GIT_SHA1
#define GIT_SHA1 "UNKNOWN"
#endif

#ifndef GIT_REFSPEC
#define GIT_REFSPEC "UNKNOWN"
#endif

#ifndef GIT_COMMIT
#define GIT_COMMIT "UNKNOWN"
#endif

#ifndef GIT_VERSION
#define GIT_VERSION "UNKNOWN"
#endif

#define BCD_TO_DEC(x) (((x & 0xF0) >> 4) * 10 + (x & 0x0F))
#define SIGNUM(x) (x < 0 ? -1 : 1)
#define MAX(x, y) (x > y ? x : y)
#define MIN(x, y) (x < y ? x : y)

int RotTransPers(SVECTOR *v, uint32_t *xy0);
int RotAverageNclip3(SVECTOR *a, SVECTOR *b, SVECTOR *c,
                     uint32_t *xy0, uint32_t *xy1, uint32_t *xy2,
                     int *otz);
int RotAverageNclip4(SVECTOR *a, SVECTOR *b, SVECTOR *c, SVECTOR *d,
                     uint32_t *xy0, uint32_t *xy1, uint32_t *xy2, uint32_t *xy3,
                     int *otz);
void CrossProduct0(VECTOR *v0, VECTOR *v1, VECTOR *out);
void CrossProduct12(VECTOR *v0, VECTOR *v1, VECTOR *out);

uint8_t *file_read(const char *filename, uint32_t *length);
void load_texture(uint8_t *data, TIM_IMAGE *tim);

uint8_t  get_byte(uint8_t *bytes, uint32_t *b);
uint16_t get_short_be(uint8_t *bytes, uint32_t *b);
uint16_t get_short_le(uint8_t *bytes, uint32_t *b);
uint32_t get_long_be(uint8_t *bytes, uint32_t *b);

uint32_t adler32(const char *s);
int32_t  div12(int32_t a, int32_t b); // Division ignoring remainder
int32_t  floor12(int32_t a);

#endif
