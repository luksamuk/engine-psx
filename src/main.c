#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <psxgte.h>
#include <inline_c.h>

#include "render.h"
#include <stdio.h>

static int  x = 32,  y = 32;
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

void
engine_init()
{
    setup_context();
    set_clear_color(63, 0, 127);
}

void
engine_update()
{
    if(x < 32 || x > (SCREEN_XRES - 32)) dx = -dx;
    if(y < 32 || y > (SCREEN_YRES - 32)) dy = -dy;

    x += dx;
    y += dy;

    rotation.vx += 6;
    rotation.vy -= 8;
    rotation.vz -= 12;
}

void
engine_draw()
{
    // Gouraud-shaded triangle
    POLY_G4 *poly = (POLY_G4 *) get_next_prim();
    setPolyG4(poly);
    setXY4(poly,
           x - 32, y - 32,
           x + 32, y - 32,
           x - 32, y + 32,
           x + 32, y + 32);
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

        gte_ldv0(&vertices[faces[i]]);
        gte_ldv1(&vertices[faces[i + 1]]);
        gte_ldv2(&vertices[faces[i + 2]]);
        gte_rtpt();
        gte_nclip();
        gte_stopz(&nclip);
        if(nclip <= 0) continue;

        gte_stsxy0(&poly->x0);
        gte_ldv0(&vertices[faces[i + 3]]);
        gte_rtps();
        gte_stsxy3(&poly->x1, &poly->x2, &poly->x3);
        gte_avsz4();
        gte_stotz(&otz);

        if((otz > 0) && (otz < OT_LENGTH)) {
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
