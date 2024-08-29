#include "render.h"
#include <assert.h>
#include <psxgte.h>
#include <inline_c.h>

RenderContext ctx;
static int using_mdec = 0;

void
setup_context()
{
    // Initialize the GPU and load the default font texture provided by
    // PSn00bSDK at (960, 0) in VRAM.
    ResetGraph(0);
    FntLoad(960, 0);

    // Place the two framebuffers vertically in VRAM.
    SetDefDrawEnv(&ctx.buffers[0].draw_env, 0, 0,           SCREEN_XRES, SCREEN_YRES);
    SetDefDispEnv(&ctx.buffers[0].disp_env, 0, 0,           SCREEN_XRES, SCREEN_YRES);
    SetDefDrawEnv(&ctx.buffers[1].draw_env, 0, SCREEN_YRES, SCREEN_XRES, SCREEN_YRES);
    SetDefDispEnv(&ctx.buffers[1].disp_env, 0, SCREEN_YRES, SCREEN_XRES, SCREEN_YRES);

    // Set the default background color and enable auto-clearing.
    using_mdec = 0;
    set_clear_color(0, 0, 0);
    ctx.buffers[0].draw_env.isbg = 1;
    ctx.buffers[1].draw_env.isbg = 1;
    ctx.buffers[0].draw_env.dtd = 0;
    ctx.buffers[1].draw_env.dtd = 0;

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

    // Don't use vsync here on MDEC since MDEC playback handles it
    // accordingly
    if(!using_mdec) VSync(0);

    RenderBuffer *draw_buffer = &ctx.buffers[ctx.active_buffer];
    RenderBuffer *disp_buffer = &ctx.buffers[ctx.active_buffer ^ 1];

    // Display the framebuffer the GPU has just finished drawing and start
    // rendering the display list that was filled up in the main loop.
    PutDispEnv(&disp_buffer->disp_env);
    if(!using_mdec)
        DrawOTagEnv(&draw_buffer->ot[OT_LENGTH - 1], &draw_buffer->draw_env);

    // Switch over to the next buffer, clear it and reset the packet allocation
    // pointer.
    ctx.active_buffer ^= 1;
    ctx.next_packet    = disp_buffer->buffer;

    // No need if not using MDEC. Save those CPU cycles. :)
    if(!using_mdec)
        ClearOTagR(disp_buffer->ot, OT_LENGTH);
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
    addPrim(get_ot_at(otz), (uint8_t *) prim);

    // Make sure we haven't yet run out of space for future primitives.
    assert(ctx.next_packet <= &ctx.buffers[ctx.active_buffer].buffer[BUFFER_LENGTH]);
}

// A simple helper for drawing text using PSn00bSDK's debug font API. Note that
// FntSort() requires the debug font texture to be uploaded to VRAM beforehand
// by calling FntLoad().
void
draw_text(int x, int y, int z, const char *text)
{
    ctx.next_packet = FntSort(get_ot_at(z), get_next_prim(), x, y, text);
    assert(ctx.next_packet <= &ctx.buffers[ctx.active_buffer].buffer[BUFFER_LENGTH]);
}

uint32_t *
get_ot_at(uint32_t otz)
{
    RenderBuffer *buffer = &ctx.buffers[ctx.active_buffer];
    return &buffer->ot[otz];
}

void
render_loading_text()
{
    swap_buffers();
    draw_text(CENTERX - 52, CENTERY - 4, 0, "Now Loading...");
    swap_buffers();
    swap_buffers();
}

RECT *
render_get_buffer_clip(void)
{
    return &ctx.buffers[ctx.active_buffer].draw_env.clip;
}


void
render_mdec_prepare(void)
{
    using_mdec = 1;

    // Enable dithering processing
    ctx.buffers[0].draw_env.dtd  = 1;
    ctx.buffers[1].draw_env.dtd  = 1;

    // Disable buffer clearing to prevent flickering
    ctx.buffers[0].draw_env.isbg = 0;
    ctx.buffers[1].draw_env.isbg = 0;

    // Set color mode to 24bpp
    //ctx.buffers[0].disp_env.isrgb24 = 1;
    //ctx.buffers[1].disp_env.isrgb24 = 1;
}

void
render_mdec_dispose(void)
{
    using_mdec = 0;

    // Disable dithering processing
    ctx.buffers[0].draw_env.dtd  = 0;
    ctx.buffers[1].draw_env.dtd  = 0;

    // Enable buffer clearing
    ctx.buffers[0].draw_env.isbg = 1;
    ctx.buffers[1].draw_env.isbg = 1;

    // Set color mode to 16bpp
    //ctx.buffers[0].disp_env.isrgb24 = 0;
    //ctx.buffers[1].disp_env.isrgb24 = 0;
}
