#include "camera.h"
#include "render.h"
#include "level.h"
#include "util.h"
#include <stdlib.h>
#include <stdio.h>

#define CENTERX_FIXP          (CENTERX << 12)
#define CENTERY_FIXP          (CENTERY << 12)
#define SCREENX_BORDER_RADIUS (8 << 12)
#define SCREENY_BORDER_RADIUS (32 << 12)
#define SPEEDX_CAP            (16 << 12)
#define SPEEDY_CAP            (24 << 12)
#define CAMERAX_MAX           ((LEVEL_MAX_X_CHUNKS << 19) - CENTERX_FIXP)
#define CAMERAY_MAX           ((LEVEL_MAX_Y_CHUNKS << 19) - CENTERY_FIXP)
#define CAMERA_EXTEND_X_MAX   (64 << 12)
#define CAMERA_STEP           (2 << 12)
#define CAMERA_EXTEND_Y_UP    (104 << 12)
#define CAMERA_EXTEND_Y_DOWN  (88 << 12)
#define CAMERA_MOVE_DELAY     120

void
camera_init(Camera *c)
{
    camera_set(c, CENTERX_FIXP, CENTERY_FIXP);
    c->pos.vz = c->realpos.vz = 0;
    c->extension_x = c->extension_y = 0;
    c->delay = 0;
    c->lag = 0;
    c->max_x = CAMERAX_MAX;
}

void
camera_update(Camera *c, Player *player)
{   
    if(player) {
        VECTOR *center = &player->pos;
        int32_t deltax = 0;
        int32_t deltay = 0;

        if(c->lag > 0)
            c->lag--;
        else {
            // X movement
            int32_t left_border  = c->realpos.vx - SCREENX_BORDER_RADIUS;
            int32_t right_border = c->realpos.vx + SCREENX_BORDER_RADIUS;

            if(right_border < center->vx) {
                deltax = center->vx - right_border;
                deltax = deltax > SPEEDX_CAP ? SPEEDX_CAP : deltax;
            } else if(left_border > center->vx) {
                deltax = center->vx - left_border;
                deltax = deltax < -SPEEDX_CAP ? -SPEEDX_CAP : deltax;
            }

            // Y movement
            if(!player->grnd) {
                int32_t top_border = c->realpos.vy - SCREENY_BORDER_RADIUS;
                int32_t bottom_border = c->realpos.vy + SCREENY_BORDER_RADIUS;

                if(top_border > center->vy) {
                    deltay = center->vy - top_border;
                    deltay = deltay < -SPEEDY_CAP ? -SPEEDY_CAP : deltay;
                } else if(bottom_border < center->vy) {
                    deltay = center->vy - bottom_border;
                    deltay = deltay > SPEEDY_CAP ? SPEEDY_CAP : deltay;
                }
            } else {
                deltay = center->vy - c->realpos.vy;
                int32_t cap = (6 << 12);
                deltay = (deltay > cap)
                    ? cap
                    : (deltay < -cap)
                    ? -cap
                    : deltay;
            }
        }

        c->realpos.vx += deltax;
        c->realpos.vy += deltay;

        if(c->lag == 0) {
            // Extended camera
            if(abs(player->vel.vz) >= 0x6000) {
                if(abs(c->extension_x) < CAMERA_EXTEND_X_MAX)
                    c->extension_x += SIGNUM(player->vel.vz) * CAMERA_STEP;
                else c->extension_x = SIGNUM(c->extension_x) * CAMERA_EXTEND_X_MAX;
            } else if(abs(c->extension_x) > 0) {
                c->extension_x -= SIGNUM(c->extension_x) * CAMERA_STEP;
            }
        }

        // Crouch down / Look up
        if(player->action == ACTION_LOOKUP || player->action == ACTION_CROUCHDOWN) {
            if(c->delay < CAMERA_MOVE_DELAY) c->delay++;
            else {
                switch(player->action) {
                default: break;
                case ACTION_LOOKUP:
                    if(c->extension_y > -CAMERA_EXTEND_Y_UP) {
                        c->extension_y -= CAMERA_STEP;
                    } else c->extension_y = -CAMERA_EXTEND_Y_UP;
                    break;
                case ACTION_CROUCHDOWN:
                    if(c->extension_y < CAMERA_EXTEND_Y_DOWN) {
                        c->extension_y += CAMERA_STEP;
                    } else c->extension_y = CAMERA_EXTEND_Y_DOWN;
                    break;
                }
            }
        } else c->delay = 0;

        // Return camera to normal
        if(c->delay < CAMERA_MOVE_DELAY) {
            if(abs(c->extension_y) > 0) {
                c->extension_y -= SIGNUM(c->extension_y) * CAMERA_STEP;
            }
        }
    }

    c->pos.vx = c->realpos.vx + c->extension_x;
    c->pos.vy = c->realpos.vy + c->extension_y;

    if(c->pos.vx < CENTERX_FIXP) c->pos.vx = CENTERX_FIXP;
    else if(c->pos.vx > c->max_x) c->pos.vx = c->max_x;

    if(c->pos.vy < CENTERY_FIXP) c->pos.vy = CENTERY_FIXP;
    else if(c->pos.vy > CAMERAY_MAX) c->pos.vy = CAMERAY_MAX;
}

void
camera_set(Camera *c, int32_t vx, int32_t vy)
{
    c->pos.vx = c->realpos.vx = vx;
    c->pos.vy = c->realpos.vy = vy;
}

void
camera_set_right_bound(Camera *c, int32_t vx)
{
    c->max_x = vx - (CENTERX_FIXP >> 1);
    printf("Set camera max position to %08x\n", c->max_x);
}
