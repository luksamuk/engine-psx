#include "screen.h"
#include <stdint.h>
#include <stdio.h>

#include "render.h"
#include "memalloc.h"

#include "screens/disclaimer.h"
#include "screens/levelselect.h"
#include "screens/level.h"
#include "screens/fmv.h"
#include "screens/title.h"
#include "screens/modeltest.h"

// Start with 64k until we make the actual level scene
// an object as well
//#define SCREEN_BUFFER_LEN 65536
#define SCREEN_BUFFER_LEN 122880

static int8_t current_scene = -1;
static uint8_t *scene_data[SCREEN_BUFFER_LEN] = { 0 };
static ArenaAllocator screen_arena;

void
scene_init()
{
    alloc_arena_init(&screen_arena, scene_data, SCREEN_BUFFER_LEN);
}

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
    case SCREEN_TITLE:       screen_title_load();       break;
    case SCREEN_MODELTEST:   screen_modeltest_load();   break;
    default: break; // Unknown scene???
    }
}

void
scene_unload()
{
    switch(current_scene) {
    case SCREEN_DISCLAIMER:  screen_disclaimer_unload(scene_data);  break;
    case SCREEN_LEVELSELECT: screen_levelselect_unload(scene_data); break;
    case SCREEN_LEVEL:       screen_level_unload(scene_data);       break;
    case SCREEN_FMV:         screen_fmv_unload(scene_data);         break;
    case SCREEN_TITLE:       screen_title_unload(scene_data);       break;
    case SCREEN_MODELTEST:   screen_modeltest_unload(scene_data);   break;
    default: break; // Unknown scene???
    }
}

void
scene_update()
{
    switch(current_scene) {
    case SCREEN_DISCLAIMER:  screen_disclaimer_update(scene_data);  break;
    case SCREEN_LEVELSELECT: screen_levelselect_update(scene_data); break;
    case SCREEN_LEVEL:       screen_level_update(scene_data);       break;
    case SCREEN_FMV:         screen_fmv_update(scene_data);         break;
    case SCREEN_TITLE:       screen_title_update(scene_data);       break;
    case SCREEN_MODELTEST:   screen_modeltest_update(scene_data);   break;
    default: break; // Unknown scene???
    }
}


void
scene_draw()
{
    switch(current_scene) {
    case SCREEN_DISCLAIMER:  screen_disclaimer_draw(scene_data);  break;
    case SCREEN_LEVELSELECT: screen_levelselect_draw(scene_data); break;
    case SCREEN_LEVEL:       screen_level_draw(scene_data);       break;
    case SCREEN_FMV:         screen_fmv_draw(scene_data);         break;
    case SCREEN_TITLE:       screen_title_draw(scene_data);       break;
    case SCREEN_MODELTEST:   screen_modeltest_draw(scene_data);   break;
    default: break; // Unknown scene???
    }
}


void *
screen_alloc(uint32_t size)
{
    return alloc_arena_malloc(&screen_arena, size);
}

void
screen_free()
{
    printf("Scene: Disposing of %d / %d bytes\n",
           alloc_arena_bytes_used(&screen_arena),
           alloc_arena_bytes_free(&screen_arena));
    alloc_arena_free(&screen_arena);
}
