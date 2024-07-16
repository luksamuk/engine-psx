#include "util.h"
#include <inline_c.h>
#include <psxcd.h>
#include <psxgpu.h>
#include <stdlib.h>
#include <stdio.h>

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
