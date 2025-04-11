#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <psxgte.h>
#include <psxcd.h>
#include <inline_c.h>
#include <hwregs_c.h>
#include <string.h>

#include "render.h"
#include "util.h"
#include "chara.h"
#include "sound.h"
#include "input.h"
#include "player.h"
#include "level.h"
#include "timer.h"
#include "camera.h"
#include "memalloc.h"
#include "screen.h"
#include "basic_font.h"

/*
  Locations of common textures on frame buffer:
  ================================================
  Title screen:   320x0      No CLUT
  Player 1:       320x0;     CLUT: 0x480
  Common objects: 576x0;     CLUT: 0x481 (8-bit only)
  Level tiles:    448x0;     CLUT: 0x482 (4 or 8-bit CLUT)
  Level BG0:      448x256;   CLUT: 0x483 (4-bit only)
  Level BG1:      512x256;   CLUT: 0x484 (4-bit only)

  System fonts:   960x0      (CLUT on same TPAGE; reserved!)
  Basic fonts:    960x256;   CLUT: 0x490 (4-bit always)
 */

/*
  General depth for elements in ordering table
  (Ordering table depths act like sprite planes)
  ================================================
        0       | Highest plane (debug information, etc)
        1       | Heads-up display and text layer
        2       | Level tile (SPRT_8 + DR_TPAGE) layer (front) -- UNUSED
        3       | Object sprite layer (upper objects such as rings, and hitboxes)
        4       | Player sprite layer (most objects, player is atop)
       ...      | ...
  OT_LENGTH - 3 | Level tile (SPRT_8 + DR_TPAGE) layer (back)
  OT_LENGTH - 2 | Level background (parallax)
 */

int debug_mode = 0;

int
main(void)
{
    // Engine initialization
    setup_context();
    CdInit();
    sound_init();
    pad_init();
    timer_init();
    fastalloc_init();
    level_init();
    font_init();
    scene_init();

    // Initial loads from disc
    render_loading_logo();
    sound_sfx_init();
    sound_bgm_init();

    // Set first scene
    scene_change(SCREEN_DISCLAIMER);

    while(1) {
        // Update systems
        sound_update();
        pad_update();
        scene_update();
        timer_update();

        // Draw scene
        scene_draw();
        font_flush();
        swap_buffers();
    }

    return 0;
}
