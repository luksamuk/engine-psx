#ifndef CHARA_H
#define CHARA_H

#include <psxgpu.h>

typedef struct {
    unsigned char x;
    unsigned char y;
    unsigned char cols;
    unsigned char rows;
    unsigned short *tiles;
} CharaFrame;

typedef struct {
    char name[16];
    unsigned char start;
    unsigned char end;
} CharaAnim;

typedef struct {
    unsigned short width;
    unsigned short height;
    unsigned short numframes;
    unsigned short numanims;
    CharaFrame *frames;
    CharaAnim *anims;

    unsigned short crectx, crecty;
    unsigned short prectx, precty;
} Chara;

Chara *load_chara(const char *filename, TIM_IMAGE *tim);
void  free_chara(Chara *chara);

void chara_render_test(Chara *chara);

#endif
