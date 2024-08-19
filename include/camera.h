#ifndef CAMERA_H
#define CAMERA_H

#include <psxgte.h>

#include "player.h"

typedef struct {
    VECTOR pos;
    VECTOR realpos;
    int32_t extension_x;
    int32_t extension_y;
} Camera;

void camera_init(Camera *);
void camera_update(Camera *, Player *);
void camera_set(Camera *, int32_t vx, int32_t vy);

#endif
