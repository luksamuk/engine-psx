#include <stdio.h>

#include "parallax.h"
#include "util.h"
#include "memalloc.h"
#include "camera.h"
#include "render.h"

extern ArenaAllocator _level_arena;
extern uint8_t level_fade;

void
load_parallax(Parallax *parallax, const char *filename)
{
    uint8_t *bytes;
    uint32_t b, length;

    // If we fail to read the file, prepare it so we get no surprises
    // when rendering a parallax structure
    parallax->num_strips = 0;
    parallax->strips = NULL;

    b = 0;
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
            part->offsetx = (j == 0)
                ? 0
                : (strip->parts[j-1].offsetx + strip->parts[j-1].width);
            strip->width += part->width;
        }
    }

    free(bytes);
}

void
parallax_draw(Parallax *prl, Camera *camera,
              uint8_t tx_mode, int32_t px, int32_t py, int32_t cx, int32_t cy)
{
    // Camera left boundary (fixed 20.12 format)
    int32_t camera_vx = (camera->pos.vx - (CENTERX << 12));

    for(uint8_t si = 0; si < prl->num_strips; si++) {
        ParallaxStrip *strip = &prl->strips[si];
        // Cast multiplication to avoid sign extension on bit shift
        // This gets the mult. result but also removes the decimal part
        uint32_t stripx = (uint32_t)(camera_vx * strip->scrollx) >> 24;
        for(uint8_t pi = 0; pi < strip->num_parts; pi++) {
            ParallaxPart *part = &strip->parts[pi];

            // Calculate part X position based on factor and camera (int format)
            int32_t vx = ((int32_t)part->offsetx) - stripx;

            // Given that each part is a horizontal piece of a strip, we assume
            // that these parts repeat at every (strip width), so just draw
            // all equal parts now at once, until we exhaust the screen width
            for(int32_t wx = vx;
                wx < (int32_t)(SCREEN_XRES + part->width);
                wx += part->width)
            {
                // TODO: 6 or 8 Depends on CLUT!!!
                uint16_t curr_px = (uint16_t)(px + ((uint32_t)part->bgindex << 6));
                uint16_t curr_cy = (uint16_t)(cy + part->bgindex);

                POLY_FT4 *poly = (POLY_FT4 *)get_next_prim();
                increment_prim(sizeof(POLY_FT4));
                setPolyFT4(poly);
                setRGB0(poly, level_fade, level_fade, level_fade);
                poly->tpage = getTPage(tx_mode & 0x3, 0, curr_px, py);
                poly->clut = getClut(cx, curr_cy);
                setXYWH(poly, wx, strip->y0, part->width, part->height);
                setUVWH(poly, part->u0, part->v0, part->width - 1, part->height - 1);
                sort_prim(poly, OT_LENGTH - 2); // Layer 5: Background

                // If drawing a single time, stop now
                if(strip->is_single) break;
            }
        }
    }
}
