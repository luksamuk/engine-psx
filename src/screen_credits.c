#include <stdlib.h>
#include <stdint.h>

#include "screen.h"
#include "screens/credits.h"
#include "render.h"
#include "basic_font.h"
#include "input.h"

static const char *creditstxt[] = {
    "SONIC XA CREDITS",
    "\r",


    /* Programming credits */
    "Lead Developer",
    "luksamuk",
    "\r",

    "Level Design",
    "luksamuk",
    "\r",


    /* BGM */
    "BGM Track Listing",
    "\r",

    "Title Screen Theme",
    "Amen Break Remixed Loop 01 160 BPM",
    "By u_ul6ysm8501",
    "\r",
    
    "Playgrond Zone 1",
    "Rusty Ruin Act 1",
    "By Richard Jacques",
    "\r",

    "Playground Zone 2",
    "Rusty Ruin Act 2",
    "By Richard Jacques",
    "\r",

    "Playground Zone 3",
    "Let Mom Sleep",
    "By Hideki Naganuma",
    "\r",

    "Playground Zone 4",
    "Let Mom Sleep: No Sleep Remix",
    "By Hideki Naganuma",
    "Remixed by Richard Jacques",
    "\r",

    "Green Hill Zone",
    "Palmtree Panic P Mix",
    "By Sonic Team",
    "\r",

    "Surely Wood Zone",
    "El Gato Battle 2 Vortex Remake",
    "By pkVortex",
    "\r",
    
    "You Can Do Anything",
    "By Sonic Team",
    "\r",


    /* Graphics credits */
    "Character Sprites Ripping",
    "Triangly",
    "Paraemon",
    "\r",

    "Level and Menu Graphics Ripping",
    "Paraemon",
    "WarCR",
    "\r",

    "Original Level Graphics",
    "NiteShadow",
    "Techokami",
    "\r",

    "Fonts Ripping",
    "Flare",
    "Storice",
    "\r",

    "Original Fonts",
    "Mr. Alien",
    "\r",

    "Objects Ripping",
    "Paraemon",
    "\r",

    "Original Sprites and Tiles",
    "Sonic Team",
    "\r",


    /* Ending */
    "sonic the hedgehog owned by",
    "sonic team",
    "sega inc.",
    "\r",

    "special thanks",
    "PSXDEV Network",
    "The Spriters Resource",
    "Schnappy",
    "Lameguy64",
    "\r",

    "Thanks for Playing",
    "\r",

    NULL,
};

#define SLIDE_FRAMES 128
#define FADE_FRAMES   64

typedef struct {
    uint8_t entry;
    uint8_t fade;
    int8_t  countdown;
    uint8_t state;
} screen_credits_data;

void
screen_credits_load()
{
    screen_credits_data *data = screen_alloc(sizeof(screen_credits_data));
    set_clear_color(0, 0, 0);

    data->entry = 0;
    data->fade = 0;
    data->countdown = FADE_FRAMES;
    data->state = 0;
}

void
screen_credits_unload(void *)
{
    screen_free();
}

void
screen_credits_update(void *d)
{
    screen_credits_data *data = (screen_credits_data *)d;
    if(creditstxt[data->entry] != NULL
       && (!pad_pressed(PAD_START) && !pad_pressed(PAD_CROSS))) {
        switch(data->state) {
        case 0: // Pre-fade-in
            data->countdown = FADE_FRAMES;
            data->fade = 0;
            data->state = 1;
            break;
        case 1: // Fade-in
            data->countdown--;
            data->fade += 2;
            if(data->countdown == 0) {
                data->state = 2;
                data->countdown = SLIDE_FRAMES;
            }
            break;
        case 2: // Displaying
            data->countdown--;
            if(data->countdown == 0) {
                data->countdown = FADE_FRAMES;
                data->state = 3;
            }
            break;
        case 3: // Fade-out
            data->countdown--;
            data->fade -= 2;
            if(data->countdown == 0) {
                while(creditstxt[data->entry] != NULL
                      && creditstxt[data->entry][0] != '\r')
                    data->entry++;
                if(creditstxt[data->entry] != NULL &&
                   creditstxt[data->entry][0] == '\r') {
                    data->entry++;
                    data->state = 0;
                }
            }
            break;
        default: break;
        }
    } else {
        // Go to title screen
        scene_change(SCREEN_TITLE);
    }
}

void
screen_credits_draw(void *d)
{
    screen_credits_data *data = (screen_credits_data *)d;

    // Get current text
    const char **window = &creditstxt[data->entry];
    if(*window != NULL) {
        // Calculate height and vy
        const char **cursor = window;
        uint16_t height = 0;
        while((*cursor)[0] != '\r') {
            cursor++;
            height += GLYPH_WHITE_HEIGHT + GLYPH_GAP;
        }

        int16_t vy = CENTERY - (height >> 1);

        // Render text line by line
        cursor = window;
        uint8_t is_first = 1;
        while((*cursor)[0] != '\r') {
            uint16_t width = font_measurew_big(*cursor);
            int16_t vx = CENTERX - (width >> 1);
            if(is_first) {
                font_set_color(
                    LERPC(data->fade, 128),
                    LERPC(data->fade, 128),
                    LERPC(data->fade, 0));
            } else {
                font_set_color(
                    LERPC(data->fade, 128),
                    LERPC(data->fade, 128),
                    LERPC(data->fade, 128));
            }
            font_draw_big(*cursor, vx, vy);

            if(is_first) is_first = 0;
            cursor++;
            vy += GLYPH_WHITE_HEIGHT + GLYPH_GAP;
        }
    }
}
