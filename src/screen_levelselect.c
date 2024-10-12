#include "screens/levelselect.h"
#include <stdint.h>
#include <strings.h>
#include <stdio.h>
#include <stdlib.h>

#include "input.h"
#include "screen.h"
#include "render.h"
#include "screens/level.h"
#include "screens/fmv.h"
#include "sound.h"
#include "util.h"
#include "timer.h"

#define CHOICE_MODELTEST 5
#define CHOICE_TITLE     6
#define CHOICE_SONICT    7
#define CHOICE_INTRO     8
#define CHOICE_SOON      9
#define MAX_LEVELS   (CHOICE_SOON + 1)

typedef struct {
    uint8_t menu_choice;
    char buffer[255];
    int32_t  bg_prect_x;
    int32_t  bg_prect_y;
    uint8_t  bg_mode;
    uint8_t  bg_frame;
    uint8_t  bg_state;
    uint16_t bg_timer;
} screen_levelselect_data;

extern int debug_mode;


#define BG_PAUSE  90
#define BG_FPS    4
#define BG_FRAMES 4

void
screen_levelselect_load()
{
    screen_levelselect_data *data = screen_alloc(sizeof(screen_levelselect_data));
    data->menu_choice = 0;
    bzero(data->buffer, 255);

    uint32_t length;
    TIM_IMAGE bg;
    uint8_t *img = file_read("\\MISC\\LVLSEL.TIM;1", &length);
    load_texture(img, &bg);
    data->bg_mode = bg.mode;
    data->bg_prect_x = bg.prect->x;
    data->bg_prect_y = bg.prect->y;
    free(img);

    data->bg_frame = 0;
    data->bg_state = 0;
    data->bg_timer = BG_PAUSE;
    
    sound_play_xa("\\BGM\\MNU001.XA;1", 0, 0, 0); // 2600
}

void
screen_levelselect_unload(void *)
{
    sound_stop_xa();
    screen_free();
}

void
screen_levelselect_update(void *d)
{
    if((pad_pressing(PAD_L1) && pad_pressed(PAD_R1)) ||
       (pad_pressed(PAD_L1) && pad_pressing(PAD_R1))) {
        debug_mode = (debug_mode + 1) % 3;
    }

    screen_levelselect_data *data = (screen_levelselect_data *)d;


    if((data->bg_state == 0) || (data->bg_state == 2)) {
        data->bg_timer--;
        if(data->bg_timer == 0) {
            data->bg_state++;
            data->bg_timer = BG_FPS;
        }
    } else if(data->bg_state == 1) {
        data->bg_timer--;
        if(data->bg_timer == 0) {
            data->bg_frame++;
            data->bg_timer = BG_FPS;
            if(data->bg_frame == BG_FRAMES - 1) {
                data->bg_state++;
                data->bg_timer = BG_PAUSE;
            }
        }
    } else if(data->bg_state == 3) {
        data->bg_timer--;
        if(data->bg_timer == 0) {
            data->bg_frame--;
            data->bg_timer = BG_FPS;
            if(data->bg_frame == 0) {
                data->bg_state = 0;
                data->bg_timer = BG_PAUSE;
            }
        }
    }
    

    if(pad_pressed(PAD_DOWN))
            data->menu_choice++;
    else if(pad_pressed(PAD_UP)) {
        if(data->menu_choice == 0) data->menu_choice = MAX_LEVELS - 1;
        else data->menu_choice--;
    }

    data->menu_choice = (data->menu_choice % MAX_LEVELS);
        
    if(pad_pressed(PAD_START) || pad_pressed(PAD_CROSS)) {
        if(data->menu_choice == CHOICE_TITLE) {
            scene_change(SCREEN_TITLE);
        } else if(data->menu_choice == CHOICE_INTRO) {
            screen_fmv_set_next(SCREEN_LEVELSELECT);
            screen_fmv_enqueue("\\INTRO.STR;1");
            scene_change(SCREEN_FMV);
        } else if(data->menu_choice == CHOICE_SONICT) {
            screen_fmv_set_next(SCREEN_LEVELSELECT);
            screen_fmv_enqueue("\\SONICT.STR;1");
            scene_change(SCREEN_FMV);
        } else if(data->menu_choice == CHOICE_MODELTEST) {
            scene_change(SCREEN_MODELTEST);
        } else if(data->menu_choice == CHOICE_SOON) {
            scene_change(SCREEN_COMINGSOON);
        } else {
            screen_level_setlevel(data->menu_choice);
            scene_change(SCREEN_LEVEL);
        }
    }

    uint32_t elapsed_sectors;
    sound_xa_get_elapsed_sectors(&elapsed_sectors);
    if(elapsed_sectors >= 2600) sound_stop_xa();
}

void
screen_levelselect_draw(void *d)
{
    screen_levelselect_data *data = (screen_levelselect_data *)d;

    if(debug_mode) {
        uint32_t elapsed_sectors;
        sound_xa_get_elapsed_sectors(&elapsed_sectors);
        FntPrint(-1, "BRANCH: %-21s %4s %3d\n",
                 GIT_COMMIT,
                 GetVideoMode() == MODE_PAL ? "PAL" : "NTSC",
                 get_frame_rate());
        FntPrint(-1, "                              %08u\n", elapsed_sectors);
        FntFlush(-1);
    }

    // Draw background
    for(uint16_t y = 0; y < SCREEN_YRES; y += 32) {
        for(uint16_t x = 0; x < SCREEN_XRES; x += 48) {
            POLY_FT4 *poly = (POLY_FT4 *)get_next_prim();
            increment_prim(sizeof(POLY_FT4));
            setPolyFT4(poly);
            setRGB0(poly, 128, 128, 128);
            poly->tpage = getTPage(data->bg_mode & 0x3,
                                   0,
                                   data->bg_prect_x,
                                   data->bg_prect_y);
            poly->clut = 0;
            setXYWH(poly, x, y, 48, 32);
            setUVWH(poly, 0, 32 * data->bg_frame, 48, 32);
            sort_prim(poly, 3);
        }
    }
    
    // Draw text
    int16_t x;
    const char *title = "Level Select";
    x = CENTERX - (strlen(title) * 4);
    draw_text(x, 12, 0, title);

    const char *subtitle = "https://luksamuk.codes/";
    x = SCREEN_XRES - (strlen(subtitle) * 8) - 8;
    draw_text(x, SCREEN_YRES - 24, 0, subtitle);

    snprintf(
        data->buffer, 255,
        "%c ROUND 0    ZONE 1\n"
        "%c            ZONE 2\n"
        "%c ROUND 1    ZONE 1\n"
        "%c            ZONE 2\n"
        "%c GREEN HILL ZONE 1\n"
        "\n"
        "\n"
        "\n"
        "\n"
        "\n"
        "\n"
        "\n"
        "\n"
        "\n"
        "\n"
        "\n"
        "%c MODEL TEST\n"
        "%c TITLE\n"
        "%c SONICTEAM\n"
        "%c INTRO\n"
        "%c COMING SOON",
        (data->menu_choice == 0) ? '>' : ' ',
        (data->menu_choice == 1) ? '>' : ' ',
        (data->menu_choice == 2) ? '>' : ' ',
        (data->menu_choice == 3) ? '>' : ' ',
        (data->menu_choice == 4) ? '>' : ' ',
        (data->menu_choice == CHOICE_MODELTEST) ? '>' : ' ',
        (data->menu_choice == CHOICE_TITLE) ? '>' : ' ',
        (data->menu_choice == CHOICE_SONICT) ? '>' : ' ',
        (data->menu_choice == CHOICE_INTRO) ? '>' : ' ',
        (data->menu_choice == CHOICE_SOON) ? '>' : ' ');
    draw_text(8, 40, 0, data->buffer);
}

