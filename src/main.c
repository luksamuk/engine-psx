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

/*
  Locations of common textures on frame buffer:
  ================================================
  Player 1:       320x0;   CLUT: 0x480
  Common objects: 576x0;   CLUT: 0x481
  Level tiles:    448x0;   CLUT: 0x482 (if exists)
  Level BG0:      448x256; CLUT: 0x483 (4-bit only)
  Level BG1:      512x256; CLUT: 0x484 (4-bit only)
 */

int debug_mode = 0;

int
main(void)
{
    // Engine initialization
    setup_context();
    sound_init();
    CdInit();
    pad_init();
    timer_init();
    fastalloc_init();
    level_init();

    // Set first scene
    scene_init();
    scene_change(SCREEN_DISCLAIMER);

    while(1) {
        // Update systems
        sound_update();
        pad_update();
        scene_update();
        timer_update();

        // Draw scene
        scene_draw();        
        swap_buffers();
    }

    return 0;
}
