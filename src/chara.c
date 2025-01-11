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
