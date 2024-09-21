#include "screens/modeltest.h"

#include <stdio.h>

#include "object.h"
#include "screen.h"
#include "input.h"

typedef struct {
    Model *ring;
} screen_modeltest_data;

void
screen_modeltest_load()
{
    screen_modeltest_data *data = screen_alloc(sizeof(screen_modeltest_data));

    data->ring = screen_alloc(sizeof(Model));
    load_model(data->ring, "\\OBJS\\COMMON\\RING.MDL");
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

    if(pad_pressing(PAD_L1)) {
        data->ring->pos.vz += 10;
    }

    if(pad_pressing(PAD_R1)) {
        data->ring->pos.vz -= 10;
    }

    if(pad_pressing(PAD_UP)) {
        data->ring->rot.vx += 10;
    }

    if(pad_pressing(PAD_DOWN)) {
        data->ring->rot.vx -= 10;
    }

    if(pad_pressing(PAD_RIGHT)) {
        data->ring->rot.vz -= 30;
    }

    if(pad_pressing(PAD_LEFT)) {
        data->ring->rot.vz += 30;
    }

    if(pad_pressing(PAD_TRIANGLE)) {
        data->ring->scl.vx += 10;
        data->ring->scl.vy += 10;
        data->ring->scl.vz += 10;
    }

    if(pad_pressing(PAD_CROSS)) {
        data->ring->scl.vx -= 10;
        data->ring->scl.vy -= 10;
        data->ring->scl.vz -= 10;
    }

    if(pad_pressed(PAD_SELECT)) {
        scene_change(SCREEN_TITLE);
    }
}

void
screen_modeltest_draw(void *d)
{
    screen_modeltest_data *data = (screen_modeltest_data *) d;
    render_model(data->ring);
}
