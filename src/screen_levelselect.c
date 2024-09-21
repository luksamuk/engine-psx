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

#define CHOICE_MODELTEST 5
#define CHOICE_TITLE     6
#define CHOICE_SONICT    7
#define CHOICE_INTRO     8
#define MAX_LEVELS   (CHOICE_INTRO + 1)

typedef struct {
    uint8_t menu_choice;
    char buffer[255];
} screen_levelselect_data;

extern int debug_mode;

void
screen_levelselect_load()
{
    screen_levelselect_data *data = screen_alloc(sizeof(screen_levelselect_data));
    data->menu_choice = 0;
    bzero(data->buffer, 255);
    sound_stop_xa();
    sound_play_xa("\\BGM\\MNU001.XA;1", 0, 0, 0);
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
            } else {
                screen_level_setlevel(data->menu_choice);
                scene_change(SCREEN_LEVEL);
            }
        }
}

void
screen_levelselect_draw(void *d)
{
    screen_levelselect_data *data = (screen_levelselect_data *)d;
    
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
        "\n"
        "%c MODEL TEST\n"
        "%c TITLE\n"
        "%c FMV:SONICT\n"
        "%c FMV:INTRO",
        (data->menu_choice == 0) ? '>' : ' ',
        (data->menu_choice == 1) ? '>' : ' ',
        (data->menu_choice == 2) ? '>' : ' ',
        (data->menu_choice == 3) ? '>' : ' ',
        (data->menu_choice == 4) ? '>' : ' ',
        (data->menu_choice == CHOICE_MODELTEST) ? '>' : ' ',
        (data->menu_choice == CHOICE_TITLE) ? '>' : ' ',
        (data->menu_choice == CHOICE_SONICT) ? '>' : ' ',
        (data->menu_choice == CHOICE_INTRO) ? '>' : ' ');
    draw_text(8, 36, 0, data->buffer);
}

