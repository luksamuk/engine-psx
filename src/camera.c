#include "camera.h"
#include "render.h"
#include "level.h"

#define CENTERX_FIXP          (CENTERX << 12)
#define CENTERY_FIXP          (CENTERY << 12)
#define SCREENX_BORDER_RADIUS (8 << 12)
#define SCREENY_BORDER_RADIUS (32 << 12)
#define SPEEDX_CAP            (16 << 12)
#define SPEEDY_CAP            (24 << 12)
#define CAMERAX_MAX           ((LEVEL_MAX_X_CHUNKS << 19) - CENTERX_FIXP)
#define CAMERAY_MAX           ((LEVEL_MAX_Y_CHUNKS << 19) - CENTERY_FIXP)

void
camera_init(Camera *c)
{
    camera_set(c, CENTERX_FIXP, CENTERY_FIXP);
    c->pos.vz = c->realpos.vz = 0;
    c->extension_x = c->extension_y = 0;
}

void
camera_update(Camera *c, Player *player)
{   
    if(player) {
        VECTOR *center = &player->pos;

        // X movement
        int32_t left_border  = c->realpos.vx - SCREENX_BORDER_RADIUS;
        int32_t right_border = c->realpos.vx + SCREENX_BORDER_RADIUS;
        int32_t deltax = 0;

        if(right_border < center->vx) {
            deltax = center->vx - right_border;
            deltax = deltax > SPEEDX_CAP ? SPEEDX_CAP : deltax;
        } else if(left_border > center->vx) {
            deltax = center->vx - left_border;
            deltax = deltax < -SPEEDX_CAP ? -SPEEDX_CAP : deltax;
        }

        // Y movement
        int32_t deltay = 0;
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

        c->realpos.vx += deltax;
        c->realpos.vy += deltay;
    }

    if(c->realpos.vx < CENTERX_FIXP) c->realpos.vx = CENTERX_FIXP;
    else if(c->realpos.vx > CAMERAX_MAX) c->realpos.vx = CAMERAX_MAX;

    if(c->realpos.vy < CENTERY_FIXP) c->realpos.vy = CENTERY_FIXP;
    else if(c->realpos.vy > CAMERAY_MAX) c->realpos.vy = CAMERAY_MAX;

    c->pos.vx = c->realpos.vx + c->extension_x;
    c->pos.vy = c->realpos.vy + c->extension_y;
}

void
camera_set(Camera *c, int32_t vx, int32_t vy)
{
    c->pos.vx = c->realpos.vx = vx;
    c->pos.vy = c->realpos.vy = vy;
}
