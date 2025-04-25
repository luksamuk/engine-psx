#ifndef RENDER_H
#define RENDER_H

#include <psxgpu.h>

#define SCREEN_XRES 320
#define SCREEN_YRES 240
#define SCREEN_Z    320
#define CENTERX     (SCREEN_XRES >> 1)
#define CENTERY     (SCREEN_YRES >> 1)

// Length of the ordering table, i.e. the range Z coordinates can have, 0-15 in
// this case. Larger values will allow for more granularity with depth (useful
// when drawing a complex 3D scene) at the expense of RAM usage and performance.
#define OT_LENGTH 2048

// Size of the buffer GPU commands and primitives are written to. If the program
// crashes due to too many primitives being drawn, increase this value.
//#define BUFFER_LENGTH 8192
#define BUFFER_LENGTH 24576
//#define BUFFER_LENGTH 40960
//#define BUFFER_LENGTH 65532

// Lerp color with respect to background color (0-128) and target color
// (useful for fade in and fade out)
#define LERPC(bg, c) ((c * bg) / 128)


/* LAYER INDICES (OTZ for 2D elements) */
#define OTZ_LAYER_TOPMOST        0
#define OTZ_LAYER_HUD            1
#define OTZ_LAYER_LEVEL_FG_FRONT 2
#define OTZ_LAYER_OBJECTS        3
#define OTZ_LAYER_PLAYER         4
#define OTZ_LAYER_UNDER_PLAYER   5
#define OTZ_LAYER_LEVEL_FG_BACK  (OT_LENGTH - 3)
#define OTZ_LAYER_LEVEL_BG       (OT_LENGTH - 2)


/* Framebuffer/display list class */

typedef struct {
    DISPENV disp_env;
    DRAWENV draw_env;
    uint32_t ot[OT_LENGTH];
    uint8_t  buffer[BUFFER_LENGTH];
} RenderBuffer;

typedef struct {
    RenderBuffer buffers[2];
    uint8_t      *next_packet;
    int          active_buffer;
} RenderContext;

void     setup_context();
void     set_clear_color(uint8_t r, uint8_t g, uint8_t b);
void     force_clear();
void     swap_buffers();
void     *get_next_prim();
uint32_t *get_ot_at(uint32_t otz);
void     increment_prim(uint32_t size);
void     sort_prim(void *prim, uint32_t otz);
void     draw_quad(int16_t vx, int16_t vy,
                   int16_t w, int16_t h,
                   uint8_t r, uint8_t g, uint8_t b,
                   uint8_t semitrans,
                   uint16_t otz);

RECT *render_get_buffer_clip(void);

#endif
