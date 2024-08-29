#include "screens/levelselect.h"
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "input.h"
#include "screen.h"
#include "render.h"
#include "screens/level.h"
#include "screens/fmv.h"

#define CHOICE_SONICT 4
#define CHOICE_INTRO  5
#define MAX_LEVELS   (CHOICE_INTRO + 1)

static uint8_t menu_choice = 0;

extern int debug_mode;

void screen_levelselect_load()
{
    menu_choice = 0;
}

void screen_levelselect_unload() {}

void screen_levelselect_update()
{
    if((pad_pressing(PAD_L1) && pad_pressed(PAD_R1)) ||
       (pad_pressed(PAD_L1) && pad_pressing(PAD_R1))) {
        debug_mode = (debug_mode + 1) % 3;
    }

    if(pad_pressed(PAD_DOWN))
            menu_choice++;
        else if(pad_pressed(PAD_UP)) {
            if(menu_choice == 0) menu_choice = MAX_LEVELS - 1;
            else menu_choice--;
        }

        menu_choice = (menu_choice % MAX_LEVELS);

        if(pad_pressed(PAD_START) || pad_pressed(PAD_CROSS)) {
            if(menu_choice == CHOICE_INTRO) {
                screen_fmv_set_next(SCREEN_LEVELSELECT);
                screen_fmv_set_path("\\INTRO.STR;1");
                scene_change(SCREEN_FMV);
            } else if(menu_choice == CHOICE_SONICT) {
                screen_fmv_set_next(SCREEN_LEVELSELECT);
                screen_fmv_set_path("\\SONICT.STR;1");
                scene_change(SCREEN_FMV);
            } else {
                screen_level_setlevel(menu_choice);
                scene_change(SCREEN_LEVEL);
            }
        }
}

void screen_levelselect_draw()
{
    char buffer[255] = { 0 };
    
    int16_t x;
    const char *title = "*** engine-psx ***";
    x = CENTERX - (strlen(title) * 4);
    draw_text(x, 12, 0, title);

    const char *subtitle = "https://luksamuk.codes/";
    x = CENTERX - (strlen(subtitle) * 4);
    draw_text(x, 24, 0, subtitle);

    snprintf(buffer, 255, "%s %s", __DATE__, __TIME__);
    x = SCREEN_XRES - (strlen(buffer) * 8) - 8;
    draw_text(x, SCREEN_YRES - 24, 0, buffer);

    snprintf(
        buffer, 255,
        "%c Round 0 Zone 1\n"
        "%c Round 0 Zone 2\n"
        "%c Round 1 Zone 1\n"
        "%c Round 1 Zone 2\n"
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
        "%c SonicT\n"
        "%c Intro",
        (menu_choice == 0) ? '>' : ' ',
        (menu_choice == 1) ? '>' : ' ',
        (menu_choice == 2) ? '>' : ' ',
        (menu_choice == 3) ? '>' : ' ',
        (menu_choice == CHOICE_SONICT) ? '>' : ' ',
        (menu_choice == CHOICE_INTRO) ? '>' : ' ');
    draw_text(8, 60, 0, buffer);
}

