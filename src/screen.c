#include "screen.h"
#include <stdint.h>
#include <stdio.h>

#include "render.h"

#include "screens/disclaimer.h"
#include "screens/levelselect.h"
#include "screens/level.h"
#include "screens/fmv.h"

int8_t current_scene = -1;

void
scene_change(ScreenIndex scr)
{
    if((int8_t)scr != current_scene) {
        printf("Change scene: %d -> %d\n", current_scene, scr);
        set_clear_color(0, 0, 0);
        render_loading_text();

        if(current_scene >= 0)
            scene_unload();
        current_scene = scr;
        scene_load();
    }
}

void
scene_load()
{
    switch(current_scene) {
    case SCREEN_DISCLAIMER:  screen_disclaimer_load();  break;
    case SCREEN_LEVELSELECT: screen_levelselect_load(); break;
    case SCREEN_LEVEL:       screen_level_load();       break;
    case SCREEN_FMV:         screen_fmv_load();         break;
    default: break; // Unknown scene???
    }
}

void
scene_unload()
{
    switch(current_scene) {
    case SCREEN_DISCLAIMER:  screen_disclaimer_unload();  break;
    case SCREEN_LEVELSELECT: screen_levelselect_unload(); break;
    case SCREEN_LEVEL:       screen_level_unload();       break;
    case SCREEN_FMV:         screen_fmv_unload();         break;
    default: break; // Unknown scene???
    }
}

void
scene_update()
{
    switch(current_scene) {
    case SCREEN_DISCLAIMER:  screen_disclaimer_update();  break;
    case SCREEN_LEVELSELECT: screen_levelselect_update(); break;
    case SCREEN_LEVEL:       screen_level_update();       break;
    case SCREEN_FMV:         screen_fmv_update();         break;
    default: break; // Unknown scene???
    }
}


void
scene_draw()
{
    switch(current_scene) {
    case SCREEN_DISCLAIMER:  screen_disclaimer_draw();  break;
    case SCREEN_LEVELSELECT: screen_levelselect_draw(); break;
    case SCREEN_LEVEL:       screen_level_draw();       break;
    case SCREEN_FMV:         screen_fmv_draw();         break;
    default: break; // Unknown scene???
    }
}
