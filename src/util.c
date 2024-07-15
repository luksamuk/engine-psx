#include "util.h"
#include <inline_c.h>
#include <psxcd.h>
#include <psxgpu.h>
#include <stdlib.h>
#include <stdio.h>

int
RotAverageNclip4(
    SVECTOR *a, SVECTOR *b, SVECTOR *c, SVECTOR *d,
    long *xy0, long *xy1, long *xy2, long *xy3,
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


char *
file_read(const char *filename, unsigned long *length)
{
    CdlFILE filepos;
    int numsectors;
    char *buffer;
    buffer = NULL;

    if(CdSearchFile(&filepos, filename) == NULL) {
        printf("File %s not found!\n", filename);
        return NULL;
    }

    numsectors = (filepos.size + 2047) / 2048;
    buffer = (char *) malloc(2048 * numsectors);
    if(!buffer) {
        printf("Error allocating %d sectors.\n", numsectors);
        return NULL;
    }

    CdControl(CdlSetloc, (unsigned char *) &filepos.pos, 0);
    CdRead(numsectors, (uint32_t *) buffer, CdlModeSpeed);
    CdReadSync(0, 0);

    *length = filepos.size;
    return buffer;
}

void
load_texture(char *data, TIM_IMAGE *tim)
{
    GetTimInfo((const uint32_t *)data, tim);
    LoadImage(tim->prect, tim->paddr);
    if(tim->mode & 0x8) {
        LoadImage(tim->crect, tim->caddr);
    }
}


char
get_byte(char *bytes, unsigned long *b)
{
    return (char) bytes[(*b)++];
}

short
get_short_be(char *bytes, unsigned long *b)
{
    unsigned short value = 0;
    value |= bytes[(*b)++] << 8;
    value |= bytes[(*b)++];
    return (short) value;
}

short
get_short_le(char *bytes, unsigned long *b)
{
    unsigned short value = 0;
    value |= bytes[(*b)++];
    value |= bytes[(*b)++] << 8;
    return (short) value;
}
