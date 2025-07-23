#include "screen.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <strings.h>

#include "render.h"
#include "memalloc.h"
#include "util.h"

#include "screens/disclaimer.h"
#include "screens/levelselect.h"
#include "screens/level.h"
#include "screens/title.h"
#include "screens/modeltest.h"
#include "screens/slide.h"
#include "screens/credits.h"
#include "screens/sprite_test.h"
#include "screens/charselect.h"
#include "screens/options.h"

// Start with 64k until we make the actual level scene
// an object as well
//#define SCREEN_BUFFER_LEN 65536
#define SCREEN_BUFFER_LEN 122880

static int8_t current_scene = -1;
static uint8_t *scene_data[SCREEN_BUFFER_LEN] = { 0 };
static ArenaAllocator screen_arena;
static uint8_t loading_logo[9050] = { 0 };// Image has 9034B, should be enough

void
scene_init()
{
    uint32_t length;
    alloc_arena_init(&screen_arena, scene_data, SCREEN_BUFFER_LEN);
    uint8_t *file = file_read("\\MISC\\LOAD.TIM;1", &length);
    assert(length < 9050);
    memcpy(loading_logo, file, length);
    free(file);
}

void
render_loading_logo()
{
    TIM_IMAGE tim;
    GetTimInfo((const uint32_t *)loading_logo, &tim);
    RECT r2 = *tim.prect;
    r2.y += 240;
    set_clear_color(0, 0, 0);
    swap_buffers();
    force_clear();
    LoadImage(tim.prect, tim.paddr);
    LoadImage(&r2, tim.paddr);
    swap_buffers();
}

void
scene_change(ScreenIndex scr)
{
    printf("Change scene: %d -> %d\n", current_scene, scr);

    if(!(scr == SCREEN_SLIDE && current_scene == SCREEN_SLIDE))
        render_loading_logo();

    if(current_scene >= 0)
        scene_unload();
    current_scene = scr;
    scene_load();
}

void
scene_load()
{
    switch(current_scene) {
    case SCREEN_DISCLAIMER:  screen_disclaimer_load();  break;
    case SCREEN_LEVELSELECT: screen_levelselect_load(); break;
    case SCREEN_LEVEL:       screen_level_load();       break;
    case SCREEN_TITLE:       screen_title_load();       break;
    case SCREEN_MODELTEST:   screen_modeltest_load();   break;
    case SCREEN_SLIDE:       screen_slide_load();       break;
    case SCREEN_CREDITS:     screen_credits_load();     break;
    case SCREEN_SPRITETEST:  screen_sprite_test_load(); break;
    case SCREEN_CHARSELECT:  screen_charselect_load();  break;
    case SCREEN_OPTIONS:     screen_options_load();  break;
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
    case SCREEN_TITLE:       screen_title_unload(scene_data);       break;
    case SCREEN_MODELTEST:   screen_modeltest_unload(scene_data);   break;
    case SCREEN_SLIDE:       screen_slide_unload(scene_data);       break;
    case SCREEN_CREDITS:     screen_credits_unload(scene_data);     break;
    case SCREEN_SPRITETEST:  screen_sprite_test_unload(scene_data); break;
    case SCREEN_CHARSELECT:  screen_charselect_unload(scene_data);  break;
    case SCREEN_OPTIONS:     screen_options_unload(scene_data);     break;
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
    case SCREEN_TITLE:       screen_title_update(scene_data);       break;
    case SCREEN_MODELTEST:   screen_modeltest_update(scene_data);   break;
    case SCREEN_SLIDE:       screen_slide_update(scene_data);       break;
    case SCREEN_CREDITS:     screen_credits_update(scene_data);     break;
    case SCREEN_SPRITETEST:  screen_sprite_test_update(scene_data); break;
    case SCREEN_CHARSELECT:  screen_charselect_update(scene_data);  break;
    case SCREEN_OPTIONS:     screen_options_update(scene_data);     break;
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
    case SCREEN_TITLE:       screen_title_draw(scene_data);       break;
    case SCREEN_MODELTEST:   screen_modeltest_draw(scene_data);   break;
    case SCREEN_SLIDE:       screen_slide_draw(scene_data);       break;
    case SCREEN_CREDITS:     screen_credits_draw(scene_data);     break;
    case SCREEN_SPRITETEST:  screen_sprite_test_draw(scene_data); break;
    case SCREEN_CHARSELECT:  screen_charselect_draw(scene_data);  break;
    case SCREEN_OPTIONS:     screen_options_draw(scene_data);     break;
    default: break; // Unknown scene???
    }
}


void *
screen_alloc(uint32_t size)
{
    void *ptr = alloc_arena_malloc(&screen_arena, size);
    bzero(ptr, size);
    return ptr;
}

void
screen_free()
{
    printf("Scene: Disposing of %u / %u bytes\n",
           alloc_arena_bytes_used(&screen_arena),
           alloc_arena_bytes_free(&screen_arena));
    alloc_arena_free(&screen_arena);
}

void *
screen_get_data()
{
    return scene_data;
}
