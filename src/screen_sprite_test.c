#include <stdlib.h>
#include <stdio.h>
#include "render.h"
#include "screen.h"
#include "chara.h"
#include "util.h"
#include "input.h"
#include "timer.h"
#include "basic_font.h"
#include "screens/sprite_test.h"

extern uint8_t level_fade;
extern uint8_t frame_debug;

typedef struct {
    Chara chara;
    VECTOR pos;
    int16_t frame;
    uint8_t play_frames;
    uint8_t flipx;
    int32_t angle;
} screen_sprite_test_data;

void
screen_sprite_test_load()
{
    screen_sprite_test_data *data = screen_alloc(sizeof(screen_sprite_test_data));

    uint32_t filelength;
    TIM_IMAGE tim;
    uint8_t *timfile = file_read("\\SPRITES\\SONIC.TIM;1", &filelength);
    if(timfile) {
        load_texture(timfile, &tim);
        free(timfile);
    }
    
    load_chara(&data->chara, "\\SPRITES\\SONIC.CHARA;1", &tim);

    data->pos = (VECTOR){ .vx = CENTERX, .vy = CENTERY };
    data->frame = 0;
    data->play_frames = 0;
    data->angle = 0;
    data->flipx = 0;
    level_fade = 0x7f;
    set_clear_color(36, 0, 180);
}

void
screen_sprite_test_unload(void *d)
{
    screen_sprite_test_data *data = (screen_sprite_test_data *) d;
    free_chara(&data->chara);
    screen_free();
    frame_debug = 0;
}

void
screen_sprite_test_update(void *d)
{
    screen_sprite_test_data *data = (screen_sprite_test_data *) d;

    const int32_t spd = 5;
    const int32_t rotspd = 5;

    if(pad_pressing(PAD_UP)) {
        data->pos.vy -= spd;
    }

    if(pad_pressing(PAD_DOWN)) {
        data->pos.vy += spd;
    }

    if(pad_pressing(PAD_RIGHT)) {
        data->pos.vx += spd;
    }

    if(pad_pressing(PAD_LEFT)) {
        data->pos.vx -= spd;
    }

    if(pad_pressed(PAD_START)) {
        data->play_frames ^= 1;
    }

    if(pad_pressed(PAD_TRIANGLE)) {
        data->flipx ^= 1;
    }

    if(pad_pressed(PAD_CROSS)) {
        frame_debug ^= 1;
    }

    if(pad_pressed(PAD_SELECT)) {
        scene_change(SCREEN_LEVELSELECT);
    }

    if(data->play_frames) {
        if(!(get_global_frames() % 30))
            data->frame++;
    } else {
        if(pad_pressed(PAD_CIRCLE)) data->frame++;
        else if(pad_pressed(PAD_SQUARE)) data->frame--;
    }

    if(pad_pressing(PAD_R1)) data->angle -= rotspd;
    else if(pad_pressing(PAD_L1)) data->angle += rotspd;

    if(data->frame < 0) data->frame = data->chara.numframes - 1;
    else if(data->frame >= data->chara.numframes) data->frame = 0;

    if(data->angle < 0) data->angle = ONE - spd;
    else if(data->angle >= ONE) data->angle = 0;
}

void
screen_sprite_test_draw(void *d)
{
    screen_sprite_test_data *data = (screen_sprite_test_data *) d;

    char buffer[80];
    sprintf(buffer,
            "POS   %04x %04x\n"
            "FRAME %02d/%02d\n"
            "ANGLE %04x",
            data->pos.vx,
            data->pos.vy,
            data->frame + 1,
            data->chara.numframes,
            data->angle);
    
    font_set_color(128, 128, 128);
    font_draw_sm(buffer, 10, 10);
    
    /* chara_render_frame(&data->chara, */
    /*                    data->frame, */
    /*                    (int16_t)(data->pos.vx), */
    /*                    (int16_t)(data->pos.vy), */
    /*                    data->flipx); */
    chara_draw_gte(&data->chara,
                   data->frame,
                   (int16_t)(data->pos.vx),
                   (int16_t)(data->pos.vy),
                   data->flipx,
                   data->angle);

    font_set_color(0xc8, 0xc8, 0xc8);

    // Video debug
    snprintf(buffer, 255,
             "%4s %3d",
             GetVideoMode() == MODE_PAL ? "PAL" : "NTSC", get_frame_rate());
    font_draw_sm(buffer, 248, 12);
}
