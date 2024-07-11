#include "render.h"
#include <assert.h>
#include <psxgte.h>
#include <inline_c.h>

static RenderContext ctx;

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
    set_clear_color(0, 0, 0);
    ctx.buffers[0].draw_env.isbg = 1;
    ctx.buffers[1].draw_env.isbg = 1;

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
    VSync(0);

    RenderBuffer *draw_buffer = &ctx.buffers[ctx.active_buffer];
    RenderBuffer *disp_buffer = &ctx.buffers[ctx.active_buffer ^ 1];

    // Display the framebuffer the GPU has just finished drawing and start
    // rendering the display list that was filled up in the main loop.
    PutDispEnv(&disp_buffer->disp_env);
    DrawOTagEnv(&draw_buffer->ot[OT_LENGTH - 1], &draw_buffer->draw_env);

    // Switch over to the next buffer, clear it and reset the packet allocation
    // pointer.
    ctx.active_buffer ^= 1;
    ctx.next_packet    = disp_buffer->buffer;
    ClearOTagR(disp_buffer->ot, OT_LENGTH);
}

void *
get_next_prim()
{
    return (void *) ctx.next_packet;
}

void
increment_prim(size_t size)
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
