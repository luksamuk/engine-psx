#include "screens/modeltest.h"

#include <stdio.h>

#include "object.h"
#include "screen.h"
#include "input.h"
#include "render.h"

typedef struct {
    char buffer[255];
    Model *ring;
} screen_modeltest_data;

void
screen_modeltest_load()
{
    screen_modeltest_data *data = screen_alloc(sizeof(screen_modeltest_data));

    data->ring = screen_alloc(sizeof(Model));
    load_model(data->ring, "\\OBJS\\COMMON\\PLANET.MDL");
    data->ring->pos.vz = 4800;
    data->ring->rot.vx = ONE >> 2;

    printf("Vertices: %d, normals: %d, polygons: %d\n",
           data->ring->num_vertices,
           data->ring->num_normals,
           data->ring->num_polygons);
}

void
screen_modeltest_unload(void *)
{
    screen_free();
}

void
screen_modeltest_update(void *d)
{
    screen_modeltest_data *data = (screen_modeltest_data *) d;

    int32_t spd = 10;
    if(pad_pressing(PAD_CIRCLE)) spd = 50;

    if(pad_pressing(PAD_L1)) {
        data->ring->pos.vz += spd;
    }

    if(pad_pressing(PAD_R1)) {
        data->ring->pos.vz -= spd;
    }

    if(pad_pressing(PAD_UP)) {
        data->ring->rot.vx += spd;
    }

    if(pad_pressing(PAD_DOWN)) {
        data->ring->rot.vx -= spd;
    }

    if(pad_pressing(PAD_RIGHT)) {
        data->ring->rot.vz -= 30;
    }

    if(pad_pressing(PAD_LEFT)) {
        data->ring->rot.vz += 30;
    }

    if(pad_pressing(PAD_TRIANGLE)) {
        data->ring->scl.vx += spd;
        data->ring->scl.vy += spd;
        data->ring->scl.vz += spd;
    }

    if(pad_pressing(PAD_CROSS)) {
        data->ring->scl.vx -= spd;
        data->ring->scl.vy -= spd;
        data->ring->scl.vz -= spd;
    }

    if(pad_pressed(PAD_SELECT)) {
        scene_change(SCREEN_TITLE);
    }
}

void
screen_modeltest_draw(void *d)
{
    screen_modeltest_data *data = (screen_modeltest_data *) d;
    snprintf(data->buffer, 255,
             "ZPOS %08x\n"
             "ROT  %08x %08x %08x\n"
             "SCL  %08x %08x %08x\n",
             data->ring->pos.vz,
             data->ring->rot.vx,
             data->ring->rot.vy,
             data->ring->rot.vz,
             data->ring->scl.vx,
             data->ring->scl.vy,
             data->ring->scl.vz);
    draw_text(8, 12, 0, data->buffer);
    render_model(data->ring);
}
