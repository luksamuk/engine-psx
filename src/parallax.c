#include <stdio.h>
#include <assert.h>

#include "parallax.h"
#include "util.h"
#include "memalloc.h"
#include "camera.h"
#include "render.h"

#include "screens/level.h"

extern ArenaAllocator _level_arena;
extern uint8_t level_fade;
extern int32_t level_water_y;

// Pre-allocated parallax polygons
uint8_t prl_current_buffer = 0;

// Sorry for this mess, but it's needed:
// prl_pols[i]: Array of polygons (POLY_FT4**) for current buffer (double buffering)
// prl_pols[i][j]: Pointer to list of polygons (POLY_FT4*) for the parallax strip #j
// prl_pols[i][j][k]: A single polygon for current strip
POLY_FT4 **prl_pols[2];

void
load_parallax(Parallax *parallax, const char *filename,
              uint8_t tx_mode, int32_t px, int32_t py,
              int32_t cx, int32_t cy)
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

    // Prepare polygon lists
    prl_pols[0] = alloc_arena_malloc(
        &_level_arena,
        sizeof(POLY_FT4 **) * parallax->num_strips);
    prl_pols[1] = alloc_arena_malloc(
        &_level_arena,
        sizeof(POLY_FT4 **) * parallax->num_strips);
    
    for(uint8_t i = 0; i < parallax->num_strips; i++) {
        ParallaxStrip *strip = &parallax->strips[i];
        uint8_t u0 = get_byte(bytes, &b);
        uint8_t v0 = get_byte(bytes, &b);
        strip->width = get_short_be(bytes, &b);
        strip->height = get_short_be(bytes, &b);
        uint8_t bgindex = get_byte(bytes, &b);
        strip->is_single = get_byte(bytes, &b);
        strip->scrollx   = get_long_be(bytes, &b);
        strip->speedx    = get_long_be(bytes, &b);
        strip->y0        = get_short_be(bytes, &b);

        strip->rposx     = 0;

        /* RENDERING OPTIMIZATION */
        // 1. Calculate number of polygons needed for this strip, round up
        uint32_t polygons_per_strip = (SCREEN_XRES / strip->width) + 3;
        
        // 2. Allocate polygons for this strip
        prl_pols[0][i] = alloc_arena_malloc(
            &_level_arena,
            sizeof(POLY_FT4) * polygons_per_strip);
        prl_pols[1][i] = alloc_arena_malloc(
            &_level_arena,
            sizeof(POLY_FT4) * polygons_per_strip);

        // 3. Preload and prepare polygons for this strip
        // TODO: 6 or 8 Depends on CLUT!!!
        uint16_t curr_px = (uint16_t)(px + ((uint32_t)bgindex << 6));
        uint16_t curr_cy = (uint16_t)(cy + bgindex);

        for(uint32_t p = 0; p < polygons_per_strip; p++) {
            POLY_FT4 *poly0 = &prl_pols[0][i][p];
            POLY_FT4 *poly1 = &prl_pols[1][i][p];
            
            setPolyFT4(poly0);
            setRGB0(poly0, 0, 0, 0);
            poly0->tpage = getTPage(tx_mode & 0x3, 0, curr_px, py);
            poly0->clut = getClut(cx, curr_cy);
            //setXYWH(poly0, 0, strip->y0, strip->width, strip->height);
            setUVWH(poly0, u0, v0, strip->width - 1, strip->height - 1);
            
            setPolyFT4(poly1);
            setRGB0(poly1, 0, 0, 0);
            poly1->tpage = getTPage(tx_mode & 0x3, 0, curr_px, py);
            poly1->clut = getClut(cx, curr_cy);
            //setXYWH(poly1, 0, strip->y0, strip->width, strip->height);
            setUVWH(poly1, u0, v0, strip->width - 1, strip->height - 1);
        }
        
    }

    free(bytes);
}

void
parallax_draw(Parallax *prl, Camera *camera)
{
    int32_t start_y = 0;

    {
        uint8_t level = screen_level_getlevel();
        if(level == 10 || level == 11) {
            // This is R5, and the water surface is at y = 177.
            // Compensate that, but do not allow the background to
            // go much beyond that
            // Use level_water_y.

            int32_t ymin = -(SCREEN_YRES << 12) + (CENTERY << 11);

            start_y = level_water_y - camera->pos.vy - (CENTERY << 11);
            start_y = MAX(start_y, ymin);
        }
    }

    start_y = start_y >> 12;
    
    // Camera left boundary (fixed 20.12 format)
    int32_t camera_vx = (camera->pos.vx - (CENTERX << 12));

    // Strips are draw bottom to top so we can have further stuff
    // (e.g. clouds) drawn on back
    for(int8_t si = prl->num_strips - 1; si >= 0; si--) {
        ParallaxStrip *strip = &prl->strips[si];
        // Cast multiplication to avoid sign extension on bit shift
        // This gets the mult. result but also removes the decimal part
        int32_t stripx = (uint32_t)(camera_vx * -strip->scrollx) >> 24;

        // Coordinates currently start drawing at screen center, so
        // push them back one screem
        stripx -= SCREEN_XRES + SCREEN_XRES;

        // Update strip relative position when there's speed involved
        strip->rposx -= strip->speedx;

        // Calculate part X position based on factor and camera (int format)
        int32_t vx = stripx + (strip->rposx >> 12);
        int32_t vy = strip->y0 + start_y;

        if((vy + strip->height < 0) || (vy > SCREEN_YRES))
            continue;

        // Given that each part is a horizontal piece of a strip, we assume
        // that these parts repeat at every (strip width), so just draw
        // all equal parts now at once, until we exhaust the screen width
        uint32_t poly_n = 0;
        for(int32_t wx = vx;
            wx < (int32_t)(SCREEN_XRES + strip->width);
            wx += strip->width)
        {
            // Don't draw if strip is outside screen
            if((wx + strip->width) < 0) continue;

            // If this is a single strip, adjust it and move it forward a bit
            if(strip->is_single) wx += SCREEN_XRES >> 1;

            POLY_FT4 *poly = &prl_pols[prl_current_buffer][si][poly_n];
            setRGB0(poly, level_fade, level_fade, level_fade);
            setXYWH(poly, wx, vy, strip->width, strip->height);
            sort_prim(poly, OTZ_LAYER_LEVEL_BG);

            poly_n++;
            // If drawing a single time, stop now
            if(strip->is_single) break;
        }
    }

    prl_current_buffer ^= 1;
}
