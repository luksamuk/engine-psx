#include <stdio.h>

#include "parallax.h"
#include "util.h"
#include "memalloc.h"

extern ArenaAllocator _level_arena;

void
load_parallax(Parallax *parallax, const char *filename)
{
    uint8_t *bytes;
    uint32_t b, length;

    // If we fail to read the file, prepare it so we get no surprises
    // when rendering a parallax structure
    parallax->num_strips = 0;
    parallax->strips = NULL;

    bytes = file_read(filename, &length);
    if(bytes == NULL) {
        printf("Error reading PRL file %s from the CD. Using defaults\n", filename);
        return;
    }

    parallax->num_strips = get_byte(bytes, &b);
    parallax->strips = alloc_arena_malloc(
        &_level_arena,
        sizeof(ParallaxStrip) * parallax->num_strips);
    for(uint8_t i = 0; i < parallax->num_strips; i++) {
        ParallaxStrip *strip = &parallax->strips[i];
        strip->width = 0;

        strip->num_parts = get_byte(bytes, &b);
        strip->is_single = get_byte(bytes, &b);
        strip->scrollx   = get_long_be(bytes, &b);
        strip->y0        = get_short_be(bytes, &b);

        strip->parts = alloc_arena_malloc(
            &_level_arena,
            sizeof(ParallaxPart) * strip->num_parts);
        for(uint8_t j = 0; j < strip->num_parts; j++) {
            ParallaxPart *part = &strip->parts[j];
            part->u0      = get_byte(bytes, &b);
            part->v0      = get_byte(bytes, &b);
            part->bgindex = get_byte(bytes, &b);
            part->width   = get_short_be(bytes, &b);
            part->height  = get_short_be(bytes, &b);
            strip->width += part->width;
        }
    }

    free(bytes);
}
