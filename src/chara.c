#include "chara.h"
#include "util.h"
#include "render.h"
#include "basic_font.h"
#include <psxgpu.h>
#include <inline_c.h>
#include <stdlib.h>
#include <stdio.h>

extern uint8_t level_fade;

void
load_chara(Chara *chara, const char *filename, TIM_IMAGE *tim)
{
    uint8_t *bytes;
    uint32_t b, length;

    bytes = file_read(filename, &length);
    if(bytes == NULL) {
        printf("Error reading CHARA file %s from the CD.\n", filename);
        return;
    }

    b = 0;

    chara->width = get_short_be(bytes, &b);
    chara->height = get_short_be(bytes, &b);
    chara->numframes = get_short_be(bytes, &b);
    chara->numanims = get_short_be(bytes, &b);

    chara->frames = (CharaFrame *)malloc(chara->numframes * sizeof(CharaFrame));
    for(uint16_t i = 0; i < chara->numframes; i++) {
        chara->frames[i].x = get_byte(bytes, &b);
        chara->frames[i].y = get_byte(bytes, &b);
        chara->frames[i].cols = get_byte(bytes, &b);
        chara->frames[i].rows = get_byte(bytes, &b);
        chara->frames[i].width = get_short_be(bytes, &b);
        chara->frames[i].height = get_short_be(bytes, &b);
        uint16_t numtiles = chara->frames[i].cols * chara->frames[i].rows;
        chara->frames[i].tiles = (uint16_t *)malloc(numtiles * sizeof(uint16_t));
        for(uint16_t j = 0; j < numtiles; j++) {
            chara->frames[i].tiles[j] = (uint16_t) get_short_be(bytes, &b);
        }
    }

    chara->anims = (CharaAnim *)malloc(chara->numanims * sizeof(CharaAnim));
    for(uint16_t i = 0; i < chara->numanims; i++) {
        for(int j = 0; j < 16; j++) {
            chara->anims[i].name[j] = get_byte(bytes, &b);
        }
        chara->anims[i].name[15] = '\0';
        for(int j = 14; j >= 0; j--) {
            if(chara->anims[i].name[j] != ' ')
                break;
            chara->anims[i].name[j] = '\0';
        }
        chara->anims[i].hname = adler32(chara->anims[i].name);
        chara->anims[i].start = get_byte(bytes, &b);
        chara->anims[i].end = get_byte(bytes, &b);
    }

    for(uint16_t i = 0; i < chara->numanims; i++) {
        printf("Animation: %s (h: %08x) [%d -> %d]\n",
               chara->anims[i].name,
               chara->anims[i].hname,
               chara->anims[i].start, chara->anims[i].end);
    }

    /* chara->crectx = tim->crect->x; */
    /* chara->crecty = tim->crect->y; */
    /* chara->prectx = tim->prect->x; */
    /* chara->precty = tim->prect->y; */
    /* printf("prect: %d %d %d %d, %x\n", */
    /*        tim->prect->x, tim->prect->y, */
    /*        tim->prect->w, tim->prect->h, */
    /*        getTPage(tim->mode & 0x3, 1, tim->prect->x, tim->prect->y)); */
    /* printf("crect: %d %d %d %d, %x\n", */
    /*        tim->crect->x, tim->crect->y, */
    /*        tim->crect->w, tim->crect->h, */
    /*        getClut(tim->crect->x, tim->crect->y)); */
    chara->crectx = 0;
    chara->crecty = 480;
    chara->prectx = 320;
    chara->precty = 0; // why not loading correctly???

    free(bytes);
}

void
free_chara(Chara *chara)
{
    if(chara->frames != NULL) {
        for(unsigned short i = 0; i < chara->numframes; i++) {
            free(chara->frames[i].tiles);
        }
        free(chara->frames);
        free(chara->anims);
    }
}

// Extern variables
uint8_t frame_debug = 0;

void
chara_draw_prepare(RECT *render_area, int otz)
{
    /* DR_TPAGE  *tpage_a, *tpage_b; */
    FILL      *pfill;
    DR_AREA   *parea;
    DR_OFFSET *poffs;

    // Clear offscreen area.
    // This is put at the end because this is the very first thing to be done.
    pfill = (FILL *)get_next_prim();
    setFill(pfill);
    setXY0(pfill, render_area->x, render_area->y);
    setWH(pfill, render_area->w, render_area->h);
    setRGB0(pfill, 0, 0, 0);
    sort_sub_prim(pfill, otz);
    increment_prim(sizeof(FILL));

    // Sort draw area primitive.
    parea = (DR_AREA *)get_next_prim();
    setDrawArea(parea, render_area);
    sort_sub_prim(parea, otz);
    increment_prim(sizeof(DR_AREA));

    // Sort draw offset primitive.
    poffs = (DR_OFFSET *)get_next_prim();
    setDrawOffset(poffs, render_area->x, render_area->y);
    sort_sub_prim(poffs, otz);
    increment_prim(sizeof(DR_OFFSET));
}

void
chara_draw_end(int otz)
{
    DR_AREA   *parea;
    DR_OFFSET *poffs;
    RECT      *clip = render_get_buffer_clip();

    // Revert to original draw area at end.
    parea = (DR_AREA *)get_next_prim();
    setDrawArea(parea, clip);
    sort_sub_prim(parea, otz);
    increment_prim(sizeof(DR_AREA));

    // Revert to original draw offset at end.
    poffs = (DR_OFFSET *)get_next_prim();
    setDrawOffset(poffs, clip->x, clip->y);
    sort_sub_prim(poffs, otz);
    increment_prim(sizeof(DR_OFFSET));
}

void
chara_draw_offscreen(Chara *chara, int16_t framenum, int flipx, int otz)
{
    CharaFrame *frame = &chara->frames[framenum];

    // Start drawing before sub_ot[2] and sub_ot[SUB_OT_LENGTH-2].
    for(uint16_t row = 0; row < frame->rows; row++) {
        for(uint16_t col = 0; col < frame->cols; col++) {
            uint16_t idx = (row * frame->cols) + col;
            idx = frame->tiles[idx];
            if(idx == 0) continue;

            uint16_t precty = chara->precty;

            uint16_t v0idx = idx / 28;
            uint16_t u0idx = idx - (v0idx * 28);
            uint16_t
                u0 = u0idx * 9,
                v0 = v0idx * 9;
            if((v0 + 9) >= 256) {
                // Go to TPAGE right below
                v0idx -= 28;
                v0 = v0idx * 9;
                precty = 256;
            }

            int16_t tilex = (col << 3) + frame->x;
            int16_t tiley = (row << 3) + frame->y - 5 + 8;

            POLY_FT4 *poly = (POLY_FT4 *)get_next_prim();
            increment_prim(sizeof(POLY_FT4));
            setPolyFT4(poly);
            setRGB0(poly, 128, 128, 128);
            setTPage(poly, 1, 1, chara->prectx, precty);
            setClut(poly, chara->crectx, chara->crecty);
            setUV4(poly,
                   u0,     v0,
                   u0 + 8, v0,
                   u0,     v0 + 8,
                   u0 + 8, v0 + 8);
            setXYWH(poly, tilex, tiley, 8, 8);
            sort_sub_prim(poly, otz);
        }
    }
}

void
chara_draw_blit(RECT *render_area,
                int16_t vx, int16_t vy,
                int32_t offsetx, int32_t offsety,
                uint8_t flipx, int32_t angle)
{
    // Calculate offsets
    int32_t asin = rsin(angle);
    int32_t acos = rcos(angle);

    int32_t offsetx_b = offsetx << 12;
    int32_t offsety_b = offsety << 12;
    offsety = ((offsety_b * acos) >> 12);
    offsetx = ((offsety_b * asin) >> 12);
    offsety += (offsetx_b * asin) >> 12;
    offsetx += (offsetx_b * acos) >> 12;
    
    VECTOR pos = {
        .vx = vx - CENTERX + (offsetx >> 12),
        .vy = vy - CENTERY - (offsety >> 12),
        .vz = frame_debug ? 0 : SCREEN_Z,
    };
    SVECTOR rotation = { 0, 0, angle, 0 };
    int otz;

    MATRIX world = { 0 };
    TransMatrix(&world, &pos);
    RotMatrix(&rotation, &world);
    gte_SetTransMatrix(&world);
    gte_SetRotMatrix(&world);

    // Now draw the actual character, where it is supposed to be!
    POLY_FT4 *poly = (POLY_FT4 *)get_next_prim();
    increment_prim(sizeof(POLY_FT4));
    setPolyFT4(poly);
    setRGB0(poly, level_fade, level_fade, level_fade);
    setTPage(poly, 2, 0, render_area->x, render_area->y);
    // Not CLUT! We use 16-bit depth.

    uint8_t umax = render_area->w - 1;
    uint8_t vmax = render_area->h - 1;
    uint8_t vmin = render_area->y;
    poly->clut = 0;
    if(flipx) {
        setUV4(poly,
               umax, vmin,
               0,    vmin,
               umax, vmin + vmax,
               0,    vmin + vmax);
    } else {
        setUV4(poly,
               0,    vmin,
               umax, vmin,
               0,    vmin + vmax,
               umax, vmin + vmax);
    }

    int16_t hw = (render_area->w >> 1);
    int16_t hh = (render_area->h >> 1);

    SVECTOR vertices[] = {
        { -hw - 4, -hh - 4, 0, 0 },
        {  hw - 4, -hh - 4, 0, 0 },
        { -hw - 4,  hh - 4, 0, 0 },
        {  hw - 4,  hh - 4, 0, 0 },
    };

    RotAverageNclip4(
        &vertices[0],
        &vertices[1],
        &vertices[2],
        &vertices[3],
        (uint32_t *)&poly->x0,
        (uint32_t *)&poly->x1,
        (uint32_t *)&poly->x2,
        (uint32_t *)&poly->x3,
        &otz);

    sort_prim(poly, OTZ_LAYER_PLAYER);
}

void
chara_draw_fb(Chara *chara, int16_t framenum,
              RECT *render_area,
              int16_t vx, int16_t vy,
              uint8_t flipx, int32_t angle)
{
    CharaFrame *frame = &chara->frames[framenum];
    int16_t left = frame->x >> 3;
    int16_t right = 7 - (frame->width >> 3) - left;

    // Start drawing before sub_ot[2] and sub_ot[SUB_OT_LENGTH-2].

    // We're gonna use two TPAGE's: one for the top texture, one for the bottom
    // texture. Both use the same CLUT.

    // Prepare TPAGE's. Both use 8-bit CLUT always.
    /* tpage_a = get_next_prim(); */
    /* setDrawTPage(tpage_a, 0, 1, getTPage(1, 1, chara->prectx, 0)); // Upper page */
    /* sort_sub_prim(tpage_a, 2); // sub_otz = 2 */
    /* increment_prim(sizeof(DR_TPAGE)); */

    /* tpage_b = get_next_prim(); */
    /* setDrawTPage(tpage_b, 0, 1, getTPage(1, 1, chara->prectx, 256)); // Lower page */
    /* sort_sub_prim(tpage_b, 3); // sub_otz = 3 */
    /* increment_prim(sizeof(DR_TPAGE)); */

    for(uint16_t row = 0; row < frame->rows; row++) {
        for(uint16_t col = 0; col < frame->cols; col++) {
            uint16_t idx = (row * frame->cols) + col;
            idx = frame->tiles[idx];
            if(idx == 0) continue;

            uint32_t otz = 2;
            uint16_t precty = chara->precty;

            uint16_t v0idx = idx / 28;
            uint16_t u0idx = idx - (v0idx * 28);
            uint16_t
                u0 = u0idx * 9,
                v0 = v0idx * 9;
            if((v0 + 9) >= 256) {
                // Go to TPAGE right below
                v0idx -= 28;
                v0 = v0idx * 9;
                /* otz = SUB_OT_LENGTH - 50; */
                precty = 256;
            }

            //int16_t tilex = (col << 3) + (flipx ? (right << 3) : frame->x) + 5;
            int16_t tilex = (col << 3) + frame->x + 5;
            int16_t tiley = (row << 3) + frame->y - 5 + 8;

            /* SPRT_8 *sprt = (SPRT_8 *)get_next_prim(); */
            /* increment_prim(sizeof(SPRT_8)); */
            /* setSprt8(sprt); */
            /* setUV0(sprt, u0, v0); */
            /* setXY0(sprt, tilex, tiley); */
            /* setRGB0(sprt, level_fade, level_fade, level_fade); */
            /* setClut(sprt, chara->crectx, chara->crecty); */
            /* sort_sub_prim(sprt, otz); */

            POLY_FT4 *poly = (POLY_FT4 *)get_next_prim();
            increment_prim(sizeof(POLY_FT4));
            setPolyFT4(poly);
            setRGB0(poly, 128, 128, 128);
            setTPage(poly, 1, 1, chara->prectx, precty);
            setClut(poly, chara->crectx, chara->crecty);
            setUV4(poly,
                   u0,     v0,
                   u0 + 8, v0,
                   u0,     v0 + 8,
                   u0 + 8, v0 + 8);
            setXYWH(poly, tilex, tiley, 8, 8);
            sort_sub_prim(poly, otz);
        }
    }

    // Prepare position
    VECTOR pos = {
        .vx = vx - CENTERX,
        .vy = vy - CENTERY,
        .vz = frame_debug ? 0 : SCREEN_Z,
    };
    SVECTOR rotation = { 0, 0, angle, 0 };
    int otz;

    MATRIX world = { 0 };
    TransMatrix(&world, &pos);
    RotMatrix(&rotation, &world);
    gte_SetTransMatrix(&world);
    gte_SetRotMatrix(&world);

    // Now draw the actual character, where it is supposed to be!
    POLY_FT4 *poly = (POLY_FT4 *)get_next_prim();
    increment_prim(sizeof(POLY_FT4));
    setPolyFT4(poly);
    setRGB0(poly, level_fade, level_fade, level_fade);
    setTPage(poly, 2, 0, render_area->x, render_area->y);
    // Not CLUT! We use 16-bit depth.

    uint8_t umax = render_area->w - 1;
    uint8_t vmax = render_area->h - 1;
    poly->clut = 0;
    if(flipx) {
        setUV4(poly,
               umax, 0,
               0,    0,
               umax, vmax,
               0,    vmax);
    } else {
        setUV4(poly,
               0,    0,
               umax, 0,
               0,    vmax,
               umax, vmax);
    }

    int16_t hw = (render_area->w >> 1) + 4;
    int16_t hh = (render_area->h >> 1) + 4;

    SVECTOR vertices[] = {
        { -hw, -hh, 0, 0 },
        {  hw, -hh, 0, 0 },
        { -hw,  hh, 0, 0 },
        {  hw,  hh, 0, 0 },
    };

    RotAverageNclip4(
        &vertices[0],
        &vertices[1],
        &vertices[2],
        &vertices[3],
        (uint32_t *)&poly->x0,
        (uint32_t *)&poly->x1,
        (uint32_t *)&poly->x2,
        (uint32_t *)&poly->x3,
        &otz);

    sort_prim(poly, OTZ_LAYER_PLAYER);
}

void
chara_draw_gte(Chara *chara, int16_t framenum,
               int16_t vx, int16_t vy,
               uint8_t flipx, int32_t angle)
{
    CharaFrame *frame = &chara->frames[framenum];
    int16_t left = frame->x >> 3;
    int16_t right = 7 - (frame->width >> 3) - left;

    // Prepare position
    VECTOR pos = {
        .vx = vx - CENTERX,
        .vy = vy - CENTERY,
        .vz = frame_debug ? 0 : SCREEN_Z,
    };
    SVECTOR rotation = { 0, 0, angle, 0 };
    int otz;

    MATRIX world = { 0 };
    TransMatrix(&world, &pos);
    RotMatrix(&rotation, &world);
    gte_SetTransMatrix(&world);
    gte_SetRotMatrix(&world);

    for(uint16_t row = 0; row < frame->rows; row++) {
        for(uint16_t colx = 0; colx < frame->cols; colx++) {
            uint16_t col = flipx ? frame->cols - colx - 1 : colx;
            uint16_t idx = (row * frame->cols) + col;
            idx = frame->tiles[idx];
            if(idx == 0) continue;

            uint16_t precty = chara->precty;

            // Get upper left UV from tile index on tileset
            uint16_t v0idx = idx / 28;
            uint16_t u0idx = idx - (v0idx * 28);
            uint16_t
                u0 = u0idx * 9,
                v0 = v0idx * 9;
            if((v0 + 9) >= 256) {
                // Go to TPAGE right below
                v0idx -= 28;
                v0 = v0idx * 9;
                precty = 256;
            }

            POLY_FT4 *poly = (POLY_FT4 *)get_next_prim();
            increment_prim(sizeof(POLY_FT4));
            setPolyFT4(poly);
            setRGB0(poly, level_fade, level_fade, level_fade);
            // 8-bit CLUT
            setTPage(poly, 1, 1, chara->prectx, precty);
            setClut(poly, chara->crectx, chara->crecty);
    
            if(flipx) {
                if(u0 > 0) u0--;
                setUV4(poly,
                       u0 + 8, v0,
                       u0,     v0,
                       u0 + 8, v0 + 8,
                       u0,     v0 + 8);
            } else {
                setUV4(poly,
                       u0,     v0,
                       u0 + 8, v0,
                       u0,     v0 + 8,
                       u0 + 8, v0 + 8);
            }

            int16_t tilex = (colx << 3) + (flipx ? (right << 3) : frame->x) - (chara->width >> 1) + 5;
            int16_t tiley = (row << 3) + frame->y - (chara->height >> 1) - 5;

            SVECTOR vertices[] = {
                { -4 + tilex, -4 + tiley, 0, 0 },
                {  4 + tilex, -4 + tiley, 0, 0 },
                { -4 + tilex,  4 + tiley, 0, 0 },
                {  4 + tilex,  4 + tiley, 0, 0 },
            };

            RotAverageNclip4(
                &vertices[0],
                &vertices[1],
                &vertices[2],
                &vertices[3],
                (uint32_t *)&poly->x0,
                (uint32_t *)&poly->x1,
                (uint32_t *)&poly->x2,
                (uint32_t *)&poly->x3,
                &otz);

            sort_prim(poly, OTZ_LAYER_PLAYER);

            if(frame_debug) {
                char buffer[10];
                sprintf(buffer, "%d\n", idx);
                if(!(idx % 2)) font_set_color(0, 128, 0);
                else font_set_color(0, 128, 128);
                font_draw_sm(buffer, poly->x0 + 4, poly->y0 + 4);
            }
        }
    }
}
