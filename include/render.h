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
//#define BUFFER_LENGTH 24576
//#define BUFFER_LENGTH 40960
#define BUFFER_LENGTH 65532

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
void     swap_buffers();
void     *get_next_prim();
uint32_t *get_ot_at(uint32_t otz);
void     increment_prim(uint32_t size);
void     sort_prim(void *prim, uint32_t otz);

void draw_text(int x, int y, int z, const char *text);

void render_loading_text();

#endif
