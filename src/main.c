#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <psxgte.h>
#include <psxcd.h>
#include <inline_c.h>

#include "render.h"
#include "util.h"
#include "chara.h"
#include <stdlib.h>
#include <stdio.h>

#define SPRTSZ 56

static int  x = (SPRTSZ >> 1),  y = (SPRTSZ >> 1);
static int dx = 1,  dy = 1;

static SVECTOR vertices[] = {
    { -64, -64, -64, 0 },
    {  64, -64, -64, 0 },
    {  64, -64,  64, 0 },
    { -64, -64,  64, 0 },
    { -64,  64, -64, 0 },
    {  64,  64, -64, 0 },
    {  64,  64,  64, 0 },
    { -64,  64,  64, 0 }
};

static short faces[] = {
    2, 1, 3, 0, // top
    1, 5, 0, 4, // front
    5, 6, 4, 7, // bottomn
    2, 6, 1, 5, // right
    7, 6, 3, 2, // back
    7, 3, 4, 0  // left
};

static SVECTOR rotation = { 0 };
static VECTOR  vel      = { 0 };
static VECTOR  acc      = { 0 };
static VECTOR  pos      = { 0, 0, 450 };
static VECTOR  scale    = { ONE, ONE, ONE };
static MATRIX  world    = { 0 };

static Chara *sonic_chara = NULL;

void
engine_init()
{
    setup_context();
    set_clear_color(63, 0, 127);
    CdInit();

    uint32_t filelength;
    TIM_IMAGE tim;
    uint8_t *timfile = file_read("\\SPRITES\\SONIC.TIM;1", &filelength);
    if(timfile) {
        load_texture(timfile, &tim);
        free(timfile);
    }

    sonic_chara = load_chara("\\SPRITES\\SONIC.CHARA;1", &tim);
}

void
engine_update()
{
    if(x < (SPRTSZ >> 1) || x > (SCREEN_XRES - 32)) dx = -dx;
    if(y < (SPRTSZ >> 1) || y > (SCREEN_YRES - 32)) dy = -dy;

    x += dx;
    y += dy;

    rotation.vx += 6;
    rotation.vy -= 8;
    rotation.vz -= 12;
}

void
engine_draw()
{
    chara_render_test(sonic_chara);
    
    // Gouraud-shaded SQUARE
    POLY_G4 *poly = (POLY_G4 *) get_next_prim();
    setPolyG4(poly);
    setXY4(poly,
           x - (SPRTSZ >> 1), y - (SPRTSZ >> 1),
           x + (SPRTSZ >> 1), y - (SPRTSZ >> 1),
           x - (SPRTSZ >> 1), y + (SPRTSZ >> 1),
           x + (SPRTSZ >> 1), y + (SPRTSZ >> 1));
    setRGB0(poly, 255, 0, 0);
    setRGB1(poly, 0, 255, 0);
    setRGB2(poly, 0, 0, 255);
    setRGB3(poly, 255, 255, 0);
    sort_prim(poly, 1);
    increment_prim(sizeof(POLY_G4));

    // Gouraud-shaded cube
    RotMatrix(&rotation, &world);
    TransMatrix(&world, &pos);
    ScaleMatrix(&world, &scale);
    gte_SetRotMatrix(&world);
    gte_SetTransMatrix(&world);

    for(int i = 0; i < 24; i += 4) {
        int nclip, otz;
        POLY_G4 *poly = (POLY_G4 *) get_next_prim();
        setPolyG4(poly);
        setRGB0(poly, 128,   0,   0);
        setRGB1(poly,   0, 128,   0);
        setRGB2(poly,   0,   0, 128);
        setRGB3(poly, 128, 128,   0);

        nclip = RotAverageNclip4(
            &vertices[faces[i]],
            &vertices[faces[i + 1]],
            &vertices[faces[i + 2]],
            &vertices[faces[i + 3]],
            (long *)&poly->x0,
            (long *)&poly->x1,
            (long *)&poly->x2,
            (long *)&poly->x3,
            &otz);

        if((nclip > 0) && (otz > 0) && (otz < OT_LENGTH)) {
            sort_prim(poly, otz);
            increment_prim(sizeof(POLY_G4));
        }
    }

    // Draw some text in front of the square (Z = 0, primitives with higher
    // Z indices are drawn first).
    draw_text(8, 8, 0,
              "Hello world!\n"
              "Running on PSX.\n"
              "Built using PSn00bSDK.");
}

int
main(void)
{
    engine_init();

    while(1) {
        engine_update();
        engine_draw();
        swap_buffers();
    }

    return 0;
}
