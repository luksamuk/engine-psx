#include "screens/levelselect.h"
#include <stdint.h>
#include <strings.h>
#include <stdio.h>
#include <stdlib.h>

#include "input.h"
#include "screen.h"
#include "render.h"
#include "screens/level.h"
#include "screens/slide.h"
#include "sound.h"
#include "util.h"
#include "timer.h"
#include "basic_font.h"
#include "player.h"
#include "screens/sprite_test.h"

/* // FULL LEVEL SELECT */
/* #define CHOICE_SOUNDTEST  20 */
/* #define CHOICE_SLIDE      21 */
/* #define CHOICE_CHARACTER  22 */
/* #define CHOICE_OPTIONS    23 */
/* #define CHOICE_SPRITETEST 24 */
/* #define CHOICE_MODELTEST  25 */
/* #define CHOICE_TITLE      26 */
/* #define CHOICE_CREDITS    27 */
/* #define MAX_COLUMN_CHOICES 17 */

// Demo level select (SAGE 2025 build)
#define CHOICE_SOUNDTEST  12
#define CHOICE_SLIDE      13
#define CHOICE_CHARACTER  14
#define CHOICE_OPTIONS    15
#define CHOICE_SPRITETEST 16
#define CHOICE_MODELTEST  17
#define CHOICE_TITLE      18
#define CHOICE_CREDITS    19
#define MAX_COLUMN_CHOICES 16

#define MAX_LEVELS   (CHOICE_CREDITS + 1)

extern uint32_t level_score_count;
extern int debug_mode;
extern ScreenIndex slide_screen_override;

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
    uint8_t  slidetest_selection;
    uint8_t  character_selection;
} screen_levelselect_data;


/* static const char *menutext[] = { */
/*     "TEST LEVEL    1", */
/*     "              2", */
/*     "              3A", */
/*     "              3B", */
/*     "GREEN HILL    1", */
/*     "              2", */
/*     "SURELY WOOD   1", */
/*     "              2", */
/*     "DAWN CANYON   1", */
/*     "              2", */
/*     "AMAZING OCEAN 1", */
/*     "              2", */
/*     "R6            1", */
/*     "              2", */
/*     "R7            1", */
/*     "              2", */
/*     "EGGMANLAND    1", */
/*     "              2", */
/*     "              3", */
/*     "WINDMILL ISLE 1", */
/*     "\n", */
/*     "SOUND TEST  *??*", */
/*     "SPLASH SCR  *??*", */
/*     "PLAYER  ????????", */
/*     "\n", */
/*     "\n", */
/*     "\n", */
/*     "OPTIONS", */
/*     "SPRITE TEST", */
/*     "MODEL TEST", */
/*     "TITLE SCREEN", */
/*     "CREDITS", */
/*     NULL, */
/* }; */

static const char *menutext[] = {
    "TEST LEVEL    1",
    "              2",
    "              3A",
    "              3B",
    "GREEN HILL    1",
    "              2",
    "SURELY WOOD   1",
    "              2",
    "DAWN CANYON   1",
    "              2",
    "AMAZING OCEAN 1",
    "              2",
    "\n",
    "SOUND TEST  *??*",
    "SPLASH SCR  *??*",
    "PLAYER  ????????",
    "\n",
    "\n",
    "\n",
    "OPTIONS",
    "SPRITE TEST",
    "MODEL TEST",
    "TITLE SCREEN",
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

    // Load icon images at 384x0 (16-bit colors because yes)
    img = file_read("\\MISC\\STICONS.TIM;1", &length);
    load_texture(img, &bg);
    free(img);

    data->bg_frame = 0;
    data->bg_state = 0;
    data->bg_timer = BG_PAUSE;

    data->music_selected = 0;
    data->soundtest_selection = 0x00;
    data->slidetest_selection = 0x00;
    data->character_selection = screen_level_getcharacter();

    // Regardless of the level, reset score.
    // You're already cheating, I'm not going to allow you
    // to have any points. :)
    level_score_count = 0;

    sound_bgm_play(BGM_LEVELSELECT);
}

void
screen_levelselect_unload(void *d)
{
    (void)(d);
    sound_cdda_stop();
    screen_free();
}

void
screen_levelselect_update(void *d)
{
#ifdef ALLOW_DEBUG
    if((pad_pressing(PAD_L1) && pad_pressed(PAD_R1)) ||
       (pad_pressed(PAD_L1) && pad_pressing(PAD_R1))) {
        debug_mode = (debug_mode + 1) % 3;
    }
#endif

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
                data->soundtest_selection = BGM_NUM_SONGS;
            else data->soundtest_selection--;
        } else if(pad_pressed(PAD_RIGHT)) {
            if(data->soundtest_selection == BGM_NUM_SONGS)
                data->soundtest_selection = 0;
            else data->soundtest_selection++;
        }
    } else if(data->menu_choice == CHOICE_SLIDE) {
        if(pad_pressed(PAD_LEFT)) {
            if(data->slidetest_selection == 0)
                data->slidetest_selection = SLIDE_NUM_SLIDES;
            else data->slidetest_selection--;
        } else if(pad_pressed(PAD_RIGHT)) {
            if(data->slidetest_selection == SLIDE_NUM_SLIDES)
                data->slidetest_selection = 0;
            else data->slidetest_selection++;
        }
    } else if(data->menu_choice == CHOICE_CHARACTER) {
        if(pad_pressed(PAD_LEFT)) {
            if(data->character_selection == 0)
                data->character_selection = CHARA_MAX;
            else data->character_selection--;
        } else if(pad_pressed(PAD_RIGHT)) {
            if(data->character_selection == CHARA_MAX)
                data->character_selection = 0;
            else data->character_selection++;
        }
    }

    if(pad_pressed(PAD_DOWN))
            data->menu_choice++;
    else if(pad_pressed(PAD_UP)) {
        if(data->menu_choice == 0) data->menu_choice = MAX_LEVELS - 1;
        else data->menu_choice--;
    } else if(
        (data->menu_choice != CHOICE_SOUNDTEST)
        && (data->menu_choice != CHOICE_SLIDE)
        && (data->menu_choice != CHOICE_CHARACTER)
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
        } else if(data->menu_choice == CHOICE_MODELTEST) {
            scene_change(SCREEN_MODELTEST);
        } else if(data->menu_choice == CHOICE_SLIDE) {
            if(data->slidetest_selection > 0) {
                screen_slide_set_next(data->slidetest_selection - 1);
                slide_screen_override = SCREEN_LEVELSELECT;
                scene_change(SCREEN_SLIDE);
            }
        } else if(data->menu_choice == CHOICE_CREDITS) {
            scene_change(SCREEN_CREDITS);
        } else if(data->menu_choice == CHOICE_SOUNDTEST) {
            if(data->soundtest_selection > 0) {
                sound_bgm_play(data->soundtest_selection - 1);
                data->music_selected = data->soundtest_selection - 1;
            }
        } else if(data->menu_choice == CHOICE_OPTIONS) {
            scene_change(SCREEN_OPTIONS);
        } else if(data->menu_choice == CHOICE_SPRITETEST) {
            screen_level_setcharacter(data->character_selection);
            screen_sprite_test_setcharacter(data->character_selection);
            scene_change(SCREEN_SPRITETEST);
        } else if(data->menu_choice == CHOICE_CHARACTER) {
            // Do nothing
        } else {
            // Select a level

            // Check for blacklisted levels.
            // (Important for SAGE 2025 demo)
#ifndef ALLOW_DEBUG
            if((data->menu_choice == 7) // SWZ2
               || (data->menu_choice == 9) // DCZ2
               || (data->menu_choice == 11) // AOZ2
            ) {
                return;
            }
#endif

            // When selecting Test Level A or B, check character
            if(data->menu_choice == 2 && data->character_selection == CHARA_KNUCKLES) {
                data->character_selection = CHARA_SONIC;
            } else if(data->menu_choice == 3) {
                data->character_selection = CHARA_KNUCKLES;
            }

            screen_level_setlevel(data->menu_choice);
            screen_level_setmode(LEVEL_MODE_NORMAL);
            screen_level_setcharacter(data->character_selection);
            // Start auto-demo
            if(pad_pressing(PAD_TRIANGLE)) {
                screen_level_setmode(LEVEL_MODE_DEMO);
            } else if(pad_pressing(PAD_CIRCLE)) {
                screen_level_setmode(LEVEL_MODE_RECORD);
            }
            scene_change(SCREEN_LEVEL);
        }
    }
}

static uint8_t
get_level_icon(screen_levelselect_data *data)
{
    if(data->menu_choice <= 3)
        // Test level
        return 4;

    if(data->menu_choice <= 5)
        // Green Hill
        return 5;

    if(data->menu_choice <= 7)
        // Surely Wood
        return 6;

    if(data->menu_choice <= 9)
        // Dawn Canyon (no icon)
        return 3;

    if(data->menu_choice <= 11)
        return 7;

    // Player icons
    if(data->menu_choice >= CHOICE_SOUNDTEST)
        return data->character_selection;

    // Interrogation
    return 3;
}

static void
draw_level_icons(int16_t vx, int16_t vy, uint8_t icon)
{
    POLY_FT4 *poly;

    // Draw background
    poly = (POLY_FT4 *)get_next_prim();
    increment_prim(sizeof(POLY_FT4));
    setPolyFT4(poly);
    setRGB0(poly, 128, 128, 128);
    setTPage(poly, 2, 0, 384, 0);
    setUVWH(poly, 32, 0, 80, 50);
    setXYWH(poly, vx, vy, 80, 50);
    sort_prim(poly, OTZ_LAYER_LEVEL_FG_FRONT);

    // Icons are 32x24 and placed one below another
    uint8_t v0 = icon * 24;
    poly = (POLY_FT4 *)get_next_prim();
    increment_prim(sizeof(POLY_FT4));
    setPolyFT4(poly);
    setRGB0(poly, 128, 128, 128);
    setTPage(poly, 2, 0, 384, 0);
    setUVWH(poly, 0, v0, 32, 24);
    setXYWH(poly, vx + 24, vy + 12, 32, 24);
    sort_prim(poly, OTZ_LAYER_LEVEL_FG_FRONT);
}

void
screen_levelselect_draw(void *d)
{
    screen_levelselect_data *data = (screen_levelselect_data *)d;

#ifdef ALLOW_DEBUG
    if(debug_mode) {
        char buffer[80];
        snprintf(buffer, 120,
                 "DBG %1d"
                 "%29s  %3d Hz\n"
                 "%31s %8s",
                 debug_mode,
                 GetVideoMode() == MODE_PAL ? "PAL" : "NTSC",
                 get_frame_rate(),
                 __DATE__, __TIME__);
        font_set_color(0x00, 0x7f, 0x00);
        font_draw_sm(buffer, 8, 12);
        font_reset_color();
        draw_quad(0, 0, SCREEN_XRES, 35,
                  0, 0, 0, 1,
                  OTZ_LAYER_LEVEL_FG_FRONT);
    }
#endif

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

    // Level icons
    draw_level_icons(SCREEN_XRES - 128,
                     SCREEN_YRES - 86,
                     get_level_icon(data));
    
    // Draw text
    font_reset_color();

    if(!debug_mode) {
        int16_t x;
        const char *title = "Level Select";
        x = CENTERX - (strlen(title) * 4);
        font_draw_big(title, x, 12);
    }

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
        } else if(cursel == CHOICE_SLIDE) {
            char buffer[80];
            snprintf(buffer, 80, "SPLASH SCR  *%02X*",
                     data->slidetest_selection);
            font_draw_sm(buffer, vx, vy);
        } else if(cursel == CHOICE_CHARACTER) {
            char buffer[80];
            const char *charaname = "";
            switch(data->character_selection) {
            case CHARA_SONIC:    charaname = "SONIC";    break;
            case CHARA_MILES:    charaname = "MILES";    break;
            case CHARA_KNUCKLES: charaname = "KNUCKLES"; break;
            default:          charaname = "UNKNOWN"; break;
            }
            snprintf(buffer, 80, "PLAYER  %8s",
                     charaname);
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

