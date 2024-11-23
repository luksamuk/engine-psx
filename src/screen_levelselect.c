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
#include "basic_font.h"

#define CHOICE_SOUNDTEST 19
#define CHOICE_MODELTEST 20
#define CHOICE_TITLE     21
#define CHOICE_SONICT    22
#define CHOICE_SOON      23
#define CHOICE_CREDITS   24
#define MAX_LEVELS   (CHOICE_CREDITS + 1)
#define MAX_COLUMN_CHOICES 15

extern uint32_t level_score_count;

typedef struct {
    uint8_t menu_choice;
    char buffer[255];
    int32_t  bg_prect_x;
    int32_t  bg_prect_y;
    uint8_t  bg_mode;
    uint8_t  bg_frame;
    uint8_t  bg_state;
    uint16_t bg_timer;
    uint8_t  music_selected;
    uint8_t  soundtest_selection;
} screen_levelselect_data;

extern int debug_mode;

static const char *menutext[] = {
    "PLAYGROUND    1",
    "              2",
    "              3",
    "              4",
    "GREEN HILL    1",
    "              2",
    "SURELY WOOD   1",
    "              2",
    "DAWN CANYON   1",
    "              2",
    "AMAZING OCEAN 1",
    "              2",
    "R6            1",
    "              2",
    "R7            1",
    "              2",
    "EGGMANLAND    1",
    "              2",
    "              3",
    "\n",
    "SOUND TEST  *??*",
    "\n",
    "\n",
    "MODELTEST",
    "TITLESCREEN",
    "SONICTEAM",
    "COMINGSOON",
    "CREDITS",
    NULL,
};

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

    data->music_selected = 0;
    data->soundtest_selection = 0x00;

    // Regardless of the level, reset score.
    // You're already cheating, I'm not going to allow you
    // to have any points. :)
    level_score_count = 0;

    sound_bgm_play(BGM_LEVELSELECT);
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

    // Sound test movement
    if(data->menu_choice == CHOICE_SOUNDTEST) {
        if(pad_pressed(PAD_LEFT)) {
            if(data->soundtest_selection == 0)
                data->soundtest_selection = BGM_NUM_SONGS - 1;
            else data->soundtest_selection--;
        } else if(pad_pressed(PAD_RIGHT)) {
            if(data->soundtest_selection == BGM_NUM_SONGS - 1)
                data->soundtest_selection = 0;
            else data->soundtest_selection++;
        }
    }

    if(pad_pressed(PAD_DOWN))
            data->menu_choice++;
    else if(pad_pressed(PAD_UP)) {
        if(data->menu_choice == 0) data->menu_choice = MAX_LEVELS - 1;
        else data->menu_choice--;
    } else if(
        (data->menu_choice != CHOICE_SOUNDTEST)
        && (pad_pressed(PAD_LEFT) || pad_pressed(PAD_RIGHT))) {
        if(data->menu_choice < MAX_COLUMN_CHOICES - 1) {
            data->menu_choice += MAX_COLUMN_CHOICES - 1;
            if(data->menu_choice >= MAX_LEVELS)
                data->menu_choice = MAX_LEVELS - 1;
        } else data->menu_choice -= MAX_COLUMN_CHOICES - 1;
    }

    data->menu_choice = (data->menu_choice % MAX_LEVELS);
        
    if(pad_pressed(PAD_START) || pad_pressed(PAD_CROSS)) {
        if(data->menu_choice == CHOICE_TITLE) {
            scene_change(SCREEN_TITLE);
        } else if(data->menu_choice == CHOICE_SONICT) {
            screen_fmv_set_next(SCREEN_LEVELSELECT);
            screen_fmv_enqueue("\\SONICT.STR;1");
            scene_change(SCREEN_FMV);
        } else if(data->menu_choice == CHOICE_MODELTEST) {
            scene_change(SCREEN_MODELTEST);
        } else if(data->menu_choice == CHOICE_SOON) {
            scene_change(SCREEN_COMINGSOON);
        } else if(data->menu_choice == CHOICE_CREDITS) {
            scene_change(SCREEN_CREDITS);
        } else if(data->menu_choice == CHOICE_SOUNDTEST) {
            sound_bgm_play(data->soundtest_selection);
            data->music_selected = data->soundtest_selection;
        } else {
            screen_level_setlevel(data->menu_choice);
            screen_level_setmode(LEVEL_MODE_NORMAL);
            // Start auto-demo
            if(pad_pressing(PAD_TRIANGLE)) {
                screen_level_setmode(LEVEL_MODE_DEMO);
            } else if(pad_pressing(PAD_CIRCLE)) {
                screen_level_setmode(LEVEL_MODE_RECORD);
            }
            scene_change(SCREEN_LEVEL);
        }
    }

    sound_bgm_check_stop(data->music_selected);
}

void
screen_levelselect_draw(void *d)
{
    screen_levelselect_data *data = (screen_levelselect_data *)d;

    if(debug_mode) {
        uint32_t elapsed_sectors;
        sound_xa_get_elapsed_sectors(&elapsed_sectors);
        FntPrint(-1, "%-29s %4s %3d\n",
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
            sort_prim(poly, OTZ_LAYER_LEVEL_BG);
        }
    }
    
    // Draw text
    font_reset_color();
    int16_t x;
    const char *title = "Level Select";
    x = CENTERX - (strlen(title) * 4);
    font_draw_big(title, x, 12);

    const char *subtitle = "*luksamuk.codes*";
    x = SCREEN_XRES - (strlen(subtitle) * 8) - 8;
    font_draw_sm(subtitle, x, SCREEN_YRES - 24);

    // Draw text
    uint8_t txtidx = 0;
    uint8_t cursel = 0;
    const char **txt = menutext;
    int16_t vx = 8;
    int16_t vy = 40;
    while(*txt != NULL) {
        if((*txt)[0] == '\n') goto end_line;
        if(data->menu_choice == cursel) font_set_color(128, 128, 0);

        if(cursel == CHOICE_SOUNDTEST) {
            char buffer[80];
            snprintf(buffer, 80, "SOUND TEST  *%02X*",
                     data->soundtest_selection);
            font_draw_sm(buffer, vx, vy);
        } else font_draw_sm(*txt, vx, vy);
        
        if(data->menu_choice == cursel) font_reset_color();
        cursel++;

        if((cursel + 1) % MAX_COLUMN_CHOICES == 0) {
            vx = CENTERX + 16;
            vy = 40 - (GLYPH_SML_WHITE_HEIGHT + GLYPH_SML_GAP);
        }

    end_line:
        vy += GLYPH_SML_WHITE_HEIGHT + GLYPH_SML_GAP;
        txt = &menutext[++txtidx];
    }
}

