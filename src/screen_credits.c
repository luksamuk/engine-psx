#include <stdlib.h>
#include <stdint.h>

#include "screen.h"
#include "screens/credits.h"
#include "render.h"
#include "basic_font.h"
#include "input.h"
#include "sound.h"
#include "util.h"
#include "timer.h"

extern int debug_mode;

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

    /* SFX */
    "Sound Effects Ripping",
    "luksamuk",
    "Mr. Lange",
    "\r",


    /* Graphics credits */
    "Original Character Sprites",
    "CartoonsAnimate22",
    "\r",

    "Level and Menu Graphics Ripping",
    "Paraemon",
    "WarCR",
    "PicsAndPixels",
    "\r",

    "Original Level Graphics",
    "NiteShadow",
    "Techokami",
    "Blue Frenzy",
    "DerZocker",
    "Enrico Cartmanuel",
    "\r",

    "Fonts Ripping",
    "Flare",
    "Storice",
    "1001",
    "GameMaster12",
    "SupaChao",
    "Gato",
    "\r",

    "Original Fonts",
    "Mr. Alien",
    "GameMaster12",
    "Fred Bronze",
    "\r",

    "Objects Ripping",
    "Paraemon",
    "Triangly",
    "Dolphman",
    "MFF",
    "\r",

    "Original Sprites and Tiles",
    "Sonic Team",
    "Devy",
    "\r",


    /* BGM */
    "BGM Track Listing",
    "\r",

    "Title Screen Theme",
    "Amen Break Remixed Loop 01 160 BPM",
    "By u_ul6ysm8501",
    "\r",
    
    "Test Level Zone 1",
    "Rusty Ruin Act 1",
    "By Richard Jacques",
    "\r",

    "Test Level Zone 2",
    "Rusty Ruin Act 2",
    "By Richard Jacques",
    "\r",

    "Test Level Zone 3",
    "Let Mom Sleep",
    "By Hideki Naganuma",
    "\r",

    "Test Level Zone 4",
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

    /* "Dawn Canyon Zone", */
    /* "Chaos Island Act 2", */
    /* "From Sonic x Shadow Generations", */
    /* "By Sonic Team", */
    /* "\r", */

    "Amazing Ocean Zone",
    "Aquarium Park -Dragon Dance Mashup-",
    "By DarkHyperSonic7",
    "\r",

    /* "Eggmanland Zone", */
    /* "Eggmanland: 16 Bit Remix", */
    /* "By DRocca", */
    /* "\r", */

    "Level Select Theme",
    "Sonic 3D Blast Main Title:",
    "Youre My Hero",
    "By Richard Jacques",
    "\r",

    "Credits Theme",
    "Sonic Generations Challenge Mission 5:",
    "Youre My Hero Remix",
    "By Richard Jacques",
    "\r",

    /* "Windmill Isle", */
    /* "Windmill Isle Day: Silent Dreams Remix", */
    /* "By Silent Dreams", */
    /* "\r", */

    "Boss Music",
    "Sonic 3D Blast Boss Music",
    "By Richard Jacques",
    "\r",


    /* Ending */
    "Sonic the Hedgehog owned by",
    "SONIC TEAM",
    "SEGA Inc.",
    "\r",

    "Special Thanks",
    "Pikuma.com",
    "PSXDEV Network",
    "The Spriters Resource",
    "Schnappy",
    "Lameguy64",
    "spicyjpeg",
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
    int16_t countdown;
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

    sound_bgm_play(BGM_CREDITS);
}

void
screen_credits_unload(void *data)
{
    (void)(data);
    sound_cdda_stop();
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
