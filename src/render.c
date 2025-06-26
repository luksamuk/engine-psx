#include "render.h"
#include <assert.h>
#include <psxgte.h>
#include <inline_c.h>

RenderContext ctx;

void
setup_context()
{
    // Initialize the GPU
    ResetGraph(0);

    // Place the two framebuffers vertically in VRAM.
    SetDefDrawEnv(&ctx.buffers[0].draw_env, 0, 0,           SCREEN_XRES, SCREEN_YRES);
    SetDefDispEnv(&ctx.buffers[0].disp_env, 0, 0,           SCREEN_XRES, SCREEN_YRES);
    SetDefDrawEnv(&ctx.buffers[1].draw_env, 0, SCREEN_YRES, SCREEN_XRES, SCREEN_YRES);
    SetDefDispEnv(&ctx.buffers[1].disp_env, 0, SCREEN_YRES, SCREEN_XRES, SCREEN_YRES);

    // Set the default background color and enable auto-clearing.
    set_clear_color(0, 0, 0);
    ctx.buffers[0].draw_env.isbg = 1;
    ctx.buffers[1].draw_env.isbg = 1;
    // Allow dithering
    ctx.buffers[0].draw_env.dtd = 1;
    ctx.buffers[1].draw_env.dtd = 1;
    // Allow drawing to display area
    ctx.buffers[0].draw_env.dfe = 1;
    ctx.buffers[1].draw_env.dfe = 1;

    // Initialize the first buffer and clear its OT so that it can be used for
    // drawing.
    ctx.active_buffer = 0;
    ctx.next_packet   = ctx.buffers[0].buffer;
    ClearOTagR(ctx.buffers[0].ot, OT_LENGTH);

    // Initialize and setup the GTE geometry offsets
    InitGeom();
    gte_SetGeomOffset(CENTERX, CENTERY);
    gte_SetGeomScreen(SCREEN_Z);

    // Turn on the video output.
    SetDispMask(1);

    // Clear the screen with a black rectangle so we don't see the PSX logo!
    force_clear();
}

void
force_clear()
{
    for(int i = 0; i < 2; i++) {
        POLY_F4 *poly = (POLY_F4 *)get_next_prim();
        setPolyF4(poly);
        setRGB0(poly, 0, 0, 0);
        setXYWH(poly, 0, 0, SCREEN_XRES, SCREEN_YRES);
        sort_prim(poly, OTZ_LAYER_TOPMOST);
        increment_prim(sizeof(POLY_F4));

        /* DrawOTagEnv(&ctx.buffers[ctx.active_buffer].ot[OT_LENGTH - 1], */
        /*             &ctx.buffers[ctx.active_buffer].draw_env); */
        DrawOTag(&ctx.buffers[ctx.active_buffer].ot[OT_LENGTH - 1]);
        PutDrawEnv(&ctx.buffers[ctx.active_buffer].draw_env);
        PutDispEnv(&ctx.buffers[ctx.active_buffer].disp_env);

        DrawSync(0);
        VSync(0);

        ctx.active_buffer ^= 1;
        ctx.next_packet    = ctx.buffers[ctx.active_buffer].buffer;
        ClearOTagR(ctx.buffers[ctx.active_buffer].ot, OT_LENGTH);
        ClearOTagR(ctx.buffers[ctx.active_buffer].sub_ot, SUB_OT_LENGTH);
    }
}

void
set_clear_color(uint8_t r, uint8_t g, uint8_t b)
{
    setRGB0(&(ctx.buffers[0].draw_env), r, g, b);
    setRGB0(&(ctx.buffers[1].draw_env), r, g, b);
}

void
swap_buffers()
{
    // Wait for the GPU to finish drawing, then wait for vblank in order to
    // prevent screen tearing.
    DrawSync(0);

    VSync(0);

    RenderBuffer *draw_buffer = &ctx.buffers[ctx.active_buffer];
    RenderBuffer *disp_buffer = &ctx.buffers[ctx.active_buffer ^ 1];

    // Display the framebuffer the GPU has just finished drawing and start
    // rendering the display list that was filled up in the main loop.
    PutDrawEnv(&draw_buffer->draw_env);
    PutDispEnv(&disp_buffer->disp_env);
    /* DrawOTagEnv(&draw_buffer->ot[OT_LENGTH - 1], &draw_buffer->draw_env); */
    sort_sub_ot();
    DrawOTag(&draw_buffer->ot[OT_LENGTH - 1]);

    SetDispMask(1);

    // Switch over to the next buffer, clear it and reset the packet allocation
    // pointer.
    ctx.active_buffer ^= 1;
    ctx.next_packet    = disp_buffer->buffer;

    ClearOTagR(disp_buffer->ot, OT_LENGTH);
    ClearOTagR(disp_buffer->sub_ot, SUB_OT_LENGTH);
}

void *
get_next_prim()
{
    return (void *) ctx.next_packet;
}

void
increment_prim(uint32_t size)
{
    ctx.next_packet += size;
}

void
sort_prim(void *prim, uint32_t otz)
{
    // Place the primitive after all previously allocated primitives, then
    // insert it into the OT and bump the allocation pointer.
    AddPrim(get_ot_at(otz), (uint8_t *) prim);

    // Make sure we haven't yet run out of space for future primitives.
    assert(ctx.next_packet <= &ctx.buffers[ctx.active_buffer].buffer[BUFFER_LENGTH]);
}

void
sort_sub_prim(void *prim, uint32_t otz)
{
    AddPrim(get_sub_ot_at(otz), (uint8_t *) prim);
    assert(ctx.next_packet <= &ctx.buffers[ctx.active_buffer].buffer[BUFFER_LENGTH]);
}

void
sort_sub_ot()
{
    RenderBuffer *buffer = &ctx.buffers[ctx.active_buffer];
    addPrims(&buffer->ot, &buffer->sub_ot[SUB_OT_LENGTH-1], &buffer->sub_ot);
}

uint32_t *
get_ot_at(uint32_t otz)
{
    RenderBuffer *buffer = &ctx.buffers[ctx.active_buffer];
    return &buffer->ot[otz];
}

uint32_t *
get_sub_ot_at(uint32_t otz)
{
    RenderBuffer *buffer = &ctx.buffers[ctx.active_buffer];
    return &buffer->sub_ot[otz];
}

RECT *
render_get_buffer_clip(void)
{
    return &ctx.buffers[ctx.active_buffer].draw_env.clip;
}


void
draw_quad(int16_t vx, int16_t vy,
          int16_t w, int16_t h,
          uint8_t r, uint8_t g, uint8_t b,
          uint8_t semitrans,
          uint16_t otz)
{
    TILE *tile = get_next_prim();
    setTile(tile);
    increment_prim(sizeof(TILE));
    setXY0(tile, vx, vy);
    setWH(tile, w, h);
    setRGB0(tile, r, g, b);
    setSemiTrans(tile, semitrans);
    sort_prim(tile, otz);
}
