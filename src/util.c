#include "util.h"
#include <inline_c.h>
#include <psxcd.h>
#include <psxgpu.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int
RotAverageNclip4(
    SVECTOR *a, SVECTOR *b, SVECTOR *c, SVECTOR *d,
    uint32_t *xy0, uint32_t *xy1, uint32_t *xy2, uint32_t *xy3,
    int *otz)
{
    int nclip = 0;

    gte_ldv0(a);
    gte_ldv1(b);
    gte_ldv2(c);
    gte_rtpt();
    gte_nclip();
    gte_stopz(&nclip);
    if(nclip <= 0) goto exit;
    gte_stsxy0(xy0);
    gte_ldv0(d);
    gte_rtps();
    gte_stsxy3(xy1, xy2, xy3);
    gte_avsz4();
    gte_stotz(otz);
exit:
    return nclip;
}

int
RotTransPers(SVECTOR *v, uint32_t *xy0)
{
    int otz = 0;
    gte_ldv0(v);
    gte_rtps();
    gte_stsxy0(xy0);
    gte_stotz(&otz);
    return otz;
}

uint8_t *
file_read(const char *filename, uint32_t *length)
{
    CdlFILE filepos;
    int numsectors;
    uint8_t *buffer;
    buffer = NULL;

    if(CdSearchFile(&filepos, filename) == NULL) {
        printf("File %s not found!\n", filename);
        return NULL;
    }

    numsectors = (filepos.size + 2047) / 2048;
    buffer = (uint8_t *) malloc(2048 * numsectors);
    if(!buffer) {
        printf("Error allocating %d sectors.\n", numsectors);
        return NULL;
    }

    CdControl(CdlSetloc, (uint8_t *) &filepos.pos, 0);
    CdRead(numsectors, (uint32_t *) buffer, CdlModeSpeed);
    CdReadSync(0, 0);

    *length = filepos.size;
    return buffer;
}

void
load_texture(uint8_t *data, TIM_IMAGE *tim)
{
    GetTimInfo((const uint32_t *)data, tim);
    LoadImage(tim->prect, tim->paddr);
    if(tim->mode & 0x8) {
        LoadImage(tim->crect, tim->caddr);
    }
}


uint8_t
get_byte(uint8_t *bytes, uint32_t *b)
{
    return (uint8_t) bytes[(*b)++];
}

uint16_t
get_short_be(uint8_t *bytes, uint32_t *b)
{
    uint16_t value = 0;
    value |= bytes[(*b)++] << 8;
    value |= bytes[(*b)++];
    return value;
}

uint16_t
get_short_le(uint8_t *bytes, uint32_t *b)
{
    uint16_t value = 0;
    value |= bytes[(*b)++];
    value |= bytes[(*b)++] << 8;
    return value;
}


uint32_t
adler32(const char *s)
{
    uint32_t a = 0x0001, b = 0x0000;
    const char *i = s;
    while(*i != '\0') {
        a = (a + *i) % 0xfff1;
        b = (a + b) % 0xfff1;
        i++;
    }
    return (b << 16) | a;
}

/* cityhash32 */

#define ROTATE32(v, n) (n == 0 ? v : ((v >> n) | (v << (32 - n))))
// This macro expects a big endian value, which we already have
#define FETCH32(p) ((uint32_t)(*p))

#define MAGIC_C1 0xcc9e2d51
#define MAGIC_C2 0x1b873593

uint32_t
_ch32_mur(uint32_t a, uint32_t h)
{
    a *= MAGIC_C1;
    a = ROTATE32(a, 17);
    a *= MAGIC_C2;
    h ^= a;
    h = ROTATE32(h, 19);
    return h * 5 + 0xe6546b64;
}

uint32_t
_ch32_fmix(uint32_t h)
{
    h ^= h >> 16;
    h *= 0x85ebca6b;
    h ^= h >> 13;
    h *= 0xc2b2ae35;
    h ^= h >> 16;
    return h;
}

uint32_t
_ch32_0_4(const char *s, size_t len)
{
    uint32_t b = 0, c = 9;
    for(size_t i = 0; i < len; i++) {
        int8_t v = (int8_t)s[i];
        b = b * MAGIC_C1 + ((uint32_t)v);
        c ^= b;
    }
    return _ch32_fmix(_ch32_mur(b,  _ch32_mur((uint32_t)len, c)));
}

uint32_t
_ch32_5_12(const char *s, size_t len)
{
    uint32_t
        a = (uint32_t)len,
        b = a * 5,
        c = 9,
        d = b;
    a += FETCH32(s);
    b += FETCH32(s + len - 4);
    c += FETCH32(s + ((len >> 1) & 4));
    return _ch32_fmix(_ch32_mur(c, _ch32_mur(b, _ch32_mur(a, d))));
}

uint32_t
_ch32_13_24(const char *s, size_t len)
{
    uint32_t
        a = FETCH32(s - 4 + (len >> 1)),
        b = FETCH32(s + 4),
        c = FETCH32(s + len - 8),
        d = FETCH32(s + (len >> 1)),
        e = FETCH32(s),
        f = FETCH32(s + len - 4),
        h = (uint32_t)len;
    return _ch32_fmix(
        _ch32_mur(
            f,
            _ch32_mur(
                e,
                _ch32_mur(
                    d,
                    _ch32_mur(
                        c,
                        _ch32_mur(
                            b,
                            _ch32_mur(a, h)))))));
}

// Perform cityhash32 on a string. Notice that this quick variant
// expects a string below 24 characters.
// Source:
// https://github.com/google/cityhash/blob/f5dc54147fcce12cefd16548c8e760d68ac04226/src/city.cc#L189
uint32_t
cityhash32_sub24(const char *s)
{
    size_t len = strlen(s);
    len = (len > 24) ? 24 : len;
    return (len <= 12)
        ? (len <= 4 ? _ch32_0_4(s, len) : _ch32_5_12(s, len))
        : _ch32_13_24(s, len);
}

