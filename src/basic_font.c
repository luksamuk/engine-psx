#include <stdio.h>
#include <stdlib.h>
#include "basic_font.h"
#include "render.h"
#include "util.h"

static uint8_t font_mode;
static uint8_t font_color[] = {128, 128, 128};

static uint8_t glyph_info_big[] = {
    // u0, v0, w, h
    // a-z
    0, 0, 7, 11,
    10, 0, 7, 11,
    20, 0, 7, 11,
    30, 0, 7, 11,
    40, 0, 7, 11,
    50, 0, 7, 11,
    60, 0, 7, 11,
    70, 0, 7, 11,
    81, 0, 3, 11,
    90, 0, 7, 11,
    100, 0, 8, 11,
    110, 0, 7, 11,
    120, 0, 10, 11,
    130, 0, 9, 11,
    140, 0, 7, 11,
    150, 0, 7, 11,
    160, 0, 8, 11,
    170, 0, 7, 11,
    180, 0, 7, 11,
    190, 0, 7, 11,
    200, 0, 7, 11,
    210, 0, 7, 11,
    220, 0, 10, 11,
    230, 0, 8, 11,
    240, 0, 7, 11,
    0, 11, 8, 11,

    // 0-9
    10, 11, 7, 11,
    21, 11, 7, 11,
    30, 11, 7, 11,
    40, 11, 7, 11,
    50, 11, 7, 11,
    60, 11, 7, 11,
    70, 11, 7, 11,
    80, 11, 7, 11,
    90, 11, 7, 11,
    100, 11, 7, 11,

    0xff, 0, 0, 0, // * (no char)
    247, 0, 3, 11, // .
    108, 11, 7, 11, // :
    0xff, 0, 0, 0, // - (no char)
    0xff, 0, 0, 0, // = (no char)
    0xff, 0, 0, 0, // ! (no char)
    0xff, 0, 0, 0, // ? (no char)
};

static uint8_t glyph_info_sm[] = {
    // u0, v0, w, h
    // a-z
    0, 22, 8, 7,
    8, 22, 8, 7,
    16, 22, 8, 7,
    24, 22, 8, 7,
    32, 22, 8, 7,
    40, 22, 8, 7,
    48, 22, 8, 7,
    56, 22, 8, 7,
    64, 22, 8, 7,
    72, 22, 8, 7,
    80, 22, 8, 7,
    88, 22, 8, 7,
    96, 22, 8, 7,
    104, 22, 8, 7,
    112, 22, 8, 7,
    120, 22, 8, 7,
    128, 22, 8, 7,
    136, 22, 8, 7,
    144, 22, 8, 7,
    152, 22, 8, 7,
    160, 22, 8, 7,
    168, 22, 8, 7,
    176, 22, 8, 7,
    184, 22, 8, 7,
    192, 22, 8, 7,
    200, 22, 8, 7,

    // 0-9
    0, 29, 8, 7,
    8, 29, 8, 7,
    16, 29, 8, 7,
    24, 29, 8, 7,
    32, 29, 8, 7,
    40, 29, 8, 7,
    48, 29, 8, 7,
    56, 29, 8, 7,
    64, 29, 8, 7,
    72, 29, 8, 7,

    80, 29, 8, 7,  // *
    88, 29, 3, 7,  // .
    92, 29, 3, 7,  // :
    96, 29, 6, 7,  // -
    103, 29, 6, 7, // =
    110, 29, 4, 7, // !
    115, 29, 7, 7, // ?
};

static uint8_t glyph_info_hg[] = {
    // u0, v0, w, h
    // a-z
    0, 36, 17, 24,
    18, 36, 16, 24,
    33, 36, 15, 24,
    48, 36, 15, 24,
    63, 36, 17, 24,
    80, 36, 16, 24,
    96, 36, 16, 24,
    112, 36, 16, 24,
    128, 36, 9, 24,
    137, 36, 9, 24,
    146, 36, 16, 24,
    162, 36, 8, 24,
    170, 36, 24, 24,
    194, 36, 15, 24,
    209, 36, 24, 24,
    233, 36, 16, 24,
    0, 60, 24, 24,
    24, 60, 16, 24,
    40, 60, 16, 24,
    56, 60, 16, 24,
    72, 60, 16, 24,
    88, 60, 15, 24,
    103, 60, 23, 24,
    126, 60, 16, 24,
    142, 60, 15, 24,
    157, 60, 16, 24,

    // 0-9 (Numbers 5-9 not added)
    0xff, 0, 0, 0,
    23, 84, 9, 32,
    32, 84, 17, 32,
    49, 84, 17, 32,
    66, 84, 17, 32,
    0xff, 0, 0, 0,
    0xff, 0, 0, 0,
    0xff, 0, 0, 0,
    0xff, 0, 0, 0,
    0xff, 0, 0, 0,

    0, 84, 23, 32, // * (ACT text)
    // Next symbols do not exist
    0xff, 0, 0, 0, // .
    0xff, 0, 0, 0, // :
    0xff, 0, 0, 0, // -
    0xff, 0, 0, 0, // =
    0xff, 0, 0, 0, // !
    0xff, 0, 0, 0, // ?
};

void
font_init()
{
    // Upload font to VRAM
    TIM_IMAGE tim;
    uint32_t length;
    printf("Loading basic font...\n");
    uint8_t *file = file_read("\\SPRITES\\BASEFONT.TIM;1", &length);
    if(file) {
        load_texture(file, &tim);
        font_mode = tim.mode;
        free(file);
    } else printf("ERROR: FONT LOADING FAILED!\n");
}

void
font_flush()
{
    DR_TPAGE *tpage = get_next_prim();
    increment_prim(sizeof(DR_TPAGE));
    setDrawTPage(tpage, 0, 1,
                 getTPage(font_mode & 0x3, 1, 960, 256));
    sort_prim(tpage, OTZ_LAYER_HUD);
}

void
_draw_glyph(
    int16_t vx, int16_t vy,
    uint8_t u0, uint8_t v0,
    uint8_t w, uint8_t h)
{
    SPRT *sprt = get_next_prim();
    setSprt(sprt);
    setRGB0(sprt, font_color[0], font_color[1], font_color[2]);
    increment_prim(sizeof(SPRT));
    setXY0(sprt, vx, vy);
    sprt->w = w;
    sprt->h = h;
    setUV0(sprt, u0, v0);
    sprt->clut = getClut(0, 490);
    sort_prim(sprt, OTZ_LAYER_HUD);
}

uint16_t
_font_measurew_generic(const char *text,
                       const uint8_t ws_w,
                       const uint8_t gap,
                       uint8_t *ginfo)
{
    uint16_t w = 0;
    uint16_t vx = 0;
    while(*text != '\0') {
        switch(*text) {
        case ' ':
            vx += ws_w + gap;
            text++;
            continue;
        case '\n':
            vx = 0;
            text++;
            continue;
        case '\t':
            vx += (ws_w << 2) + (gap << 2);
            text++;
            continue;
        }

        uint8_t offset = 0xff;

        if((*text >= 'a') && (*text <= 'z'))
            offset = (uint8_t)((unsigned)(*text) - (unsigned)('a'));
        else if((*text >= 'A') && (*text <= 'Z'))
            offset = (uint8_t)((unsigned)(*text) - (unsigned)('A'));
        else if((*text >= '0') && (*text <= '9')) {
            offset = 26 + (uint8_t)((unsigned)(*text) - (unsigned)('0'));
        } else {
            switch(*text) {
            case '*': offset = 36; break;
            case '.': offset = 37; break;
            case ':': offset = 38; break;
            case '-': offset = 39; break;
            case '=': offset = 40; break;
            case '!': offset = 41; break;
            case '?': offset = 42; break;
            default:  offset = 0xff; break;
            }
        }

        uint8_t gw = ws_w;
        if(offset != 0xff) {
            uint8_t *info = &ginfo[offset * 4];
            if(info[0] == 0xff) {
                // Glyph doesn't exist, so don't draw
                goto jump_ws;
            }
            gw = info[2];
        }

    jump_ws:
        vx += gw + gap;
        text++;

        if(vx > w) w = vx;
    }
    return w;
}

uint16_t
font_measurew_big(const char *text)
{
    return _font_measurew_generic(
        text, GLYPH_WHITE_WIDTH, GLYPH_GAP, glyph_info_big);
}

uint16_t
font_measurew_sm(const char *text)
{
   return _font_measurew_generic(
       text, GLYPH_SML_WHITE_WIDTH, GLYPH_SML_GAP, glyph_info_sm);
}

uint16_t
font_measurew_hg(const char *text)
{
   return _font_measurew_generic(
       text, GLYPH_HG_WHITE_WIDTH, GLYPH_HG_GAP, glyph_info_hg);
}

void
_font_draw_generic(const char *text, int16_t vx, int16_t vy,
                   const uint8_t ws_w, const uint8_t ws_h,
                   const uint8_t gap,
                   uint8_t *ginfo)
{
    int16_t start_vx = vx;
    while(*text != '\0') {
        switch(*text) {
        case ' ':
            vx += ws_w + gap;
            text++;
            continue;
        case '\n':
            vy += ws_h + gap;
            vx = start_vx;
            text++;
            continue;
        case '\t':
            vx += (ws_w << 2) + (gap << 2);
            text++;
            continue;
        }

        uint8_t offset = 0xff;

        if((*text >= 'a') && (*text <= 'z'))
            offset = (uint8_t)((unsigned)(*text) - (unsigned)('a'));
        else if((*text >= 'A') && (*text <= 'Z'))
            offset = (uint8_t)((unsigned)(*text) - (unsigned)('A'));
        else if((*text >= '0') && (*text <= '9')) {
            offset = 26 + (uint8_t)((unsigned)(*text) - (unsigned)('0'));
        } else {
            switch(*text) {
            case '*': offset = 36; break;
            case '.': offset = 37; break;
            case ':': offset = 38; break;
            case '-': offset = 39; break;
            case '=': offset = 40; break;
            case '!': offset = 41; break;
            case '?': offset = 42; break;
            default:  offset = 0xff; break;
            }
        }

        uint8_t gw = ws_w;
        if(offset != 0xff) {
            uint8_t *info = &ginfo[offset * 4];
            if(info[0] == 0xff) {
                // Glyph doesn't exist, so don't draw
                goto jump_ws;
            }
            gw = info[2];
            _draw_glyph(vx, vy, info[0], info[1], info[2], info[3]);
        }

    jump_ws:
        vx += gw + gap;
        text++;
    }
}

void
font_draw_big(const char *text, int16_t vx, int16_t vy)
{
    _font_draw_generic(text, vx, vy,
                       GLYPH_WHITE_WIDTH, GLYPH_WHITE_HEIGHT, GLYPH_GAP,
                       glyph_info_big);
}

void
font_draw_sm(const char *text, int16_t vx, int16_t vy)
{
    _font_draw_generic(text, vx, vy,
                       GLYPH_SML_WHITE_WIDTH, GLYPH_SML_WHITE_HEIGHT, GLYPH_SML_GAP,
                       glyph_info_sm);
}

void
font_draw_hg(const char *text, int16_t vx, int16_t vy)
{
    _font_draw_generic(text, vx, vy,
                       GLYPH_HG_WHITE_WIDTH, GLYPH_HG_WHITE_HEIGHT, GLYPH_HG_GAP,
                       glyph_info_hg);
}

void
font_set_color(uint8_t r0, uint8_t g0, uint8_t b0)
{
    font_color[0] = r0; font_color[1] = g0; font_color[2] = b0;
}

void
font_reset_color()
{
    font_color[0] = font_color[1] = font_color[2] = 128;
}
