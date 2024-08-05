#ifndef CAMERA_H
#define CAMERA_H

#include <psxgte.h>

#include "player.h"

typedef struct {
    VECTOR pos;
} Camera;

void camera_init(Camera *);
void camera_update(Camera *, Player *);

#endif
