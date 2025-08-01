#ifndef CAMERA_H
#define CAMERA_H

#include <psxgte.h>

#include "player.h"

typedef struct {
    VECTOR  pos;
    VECTOR  realpos;
    int32_t extension_x;
    int32_t extension_y;
    uint8_t delay;
    uint8_t lag;
    int32_t max_x;
    int32_t min_x;

    VECTOR  focus;
    uint8_t follow_player;
} Camera;

void camera_init(Camera *);
void camera_update(Camera *, Player *);
void camera_set(Camera *, int32_t vx, int32_t vy);
void camera_set_right_bound(Camera *, int32_t vx);
void camera_set_left_bound(Camera *, int32_t vx);

void camera_follow_player(Camera *);
void camera_stop_following_player(Camera *);
void camera_focus(Camera *, int32_t vx, int32_t vy);

#endif
