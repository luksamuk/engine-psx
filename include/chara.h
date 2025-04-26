#ifndef CHARA_H
#define CHARA_H

#include <psxgpu.h>

typedef struct {
    uint8_t x;
    uint8_t y;
    uint8_t cols;
    uint8_t rows;
    uint16_t width;
    uint16_t height;
    uint16_t *tiles;
} CharaFrame;

typedef struct {
    char name[16];
    uint8_t start;
    uint8_t end;
    uint32_t hname;
} CharaAnim;

typedef struct {
    uint16_t width;
    uint16_t height;
    uint16_t numframes;
    uint16_t numanims;
    CharaFrame *frames;
    CharaAnim *anims;

    uint16_t crectx, crecty;
    uint16_t prectx, precty;
} Chara;

void load_chara(Chara *chara, const char *filename, TIM_IMAGE *tim);
void free_chara(Chara *chara);
void chara_draw_gte(Chara *chara, int16_t framenum,
                    int16_t vx, int16_t vy,
                    uint8_t flipx, int32_t angle);

void chara_draw_prepare(RECT *render_area, int otz);
void chara_draw_offscreen(Chara *chara, int16_t framenum, int flipx, int otz);
void chara_draw_blit(RECT *render_area,
                     int16_t vx, int16_t vy,
                     uint8_t flipx, int32_t angle);
/* void chara_draw_fb(Chara *chara, int16_t framenum, */
/*                    RECT *render_area, */
/*                    int16_t vx, int16_t vy, */
/*                    uint8_t flipx, int32_t angle); */
void chara_draw_end(int otz);

#endif
