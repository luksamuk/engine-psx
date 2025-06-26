#include "demo.h"
#include <stdio.h>

static DemoState _demo;

void
demo_init()
{
    _demo = (DemoState){ 0 };
}

void
demo_record()
{
    InputState s;
    input_get_state(&s);

    if(s.current != _demo.state.current) {
        printf("    {.state = 0x%04x, .num_frames = %d},\n",
               _demo.state.current, _demo.timer + 1);
        _demo.state.current = s.current;
        _demo.timer = 0;
    } else _demo.timer++;
}

DemoPlaybackSample *_get_playback_data(int level);

void
demo_update_playback(int level, InputState *s)
{
    s->old = s->current;
    DemoPlaybackSample *_data = _get_playback_data(level);
    if(_demo.timer == 0) {
        _demo.sample++;

        DemoPlaybackSample *_sample = &_data[_demo.sample-1];
        // End of playback is marked by a sample with 0 frames
        if(_sample->num_frames == 0) {
            s->current = 0x0000;
            _demo.timer = 0;
            _demo.sample--;
            return;
        }

        s->current = _sample->state;
        _demo.timer = _sample->num_frames - 1;
        return;
    }

    _demo.timer--;
}

static DemoPlaybackSample _dummydemo[] = {
    {.state = 0x0000, .num_frames = 0},
};

static DemoPlaybackSample _pz1demo[] = {
    {.state = 0x0000, .num_frames = 59},
    {.state = 0x0020, .num_frames = 278},
    {.state = 0x0060, .num_frames = 15},
    {.state = 0x0000, .num_frames = 46},
    {.state = 0x0080, .num_frames = 13},
    {.state = 0x0000, .num_frames = 43},
    {.state = 0x0080, .num_frames = 13},
    {.state = 0x0000, .num_frames = 58},
    {.state = 0x0080, .num_frames = 125},
    {.state = 0x0000, .num_frames = 7},
    {.state = 0x0020, .num_frames = 49},
    {.state = 0x4020, .num_frames = 35},
    {.state = 0x0020, .num_frames = 182},
    {.state = 0x0000, .num_frames = 6},
    {.state = 0x0040, .num_frames = 14},
    {.state = 0x0000, .num_frames = 117},
    {.state = 0x0040, .num_frames = 66},
    {.state = 0x0000, .num_frames = 13},
    {.state = 0x0020, .num_frames = 12},
    {.state = 0x4020, .num_frames = 16},
    {.state = 0x0020, .num_frames = 4},
    {.state = 0x4020, .num_frames = 12},
    {.state = 0x4000, .num_frames = 13},
    {.state = 0x4020, .num_frames = 22},
    {.state = 0x0020, .num_frames = 10},
    {.state = 0x0000, .num_frames = 264},
    {.state = 0x0080, .num_frames = 25},
    {.state = 0x4080, .num_frames = 14},
    {.state = 0x0080, .num_frames = 25},
    {.state = 0x0000, .num_frames = 17},
    {.state = 0x4000, .num_frames = 2},
    {.state = 0x4080, .num_frames = 8},
    {.state = 0x0000, .num_frames = 17},
    {.state = 0x0020, .num_frames = 4},
    {.state = 0x0000, .num_frames = 15},
    {.state = 0x0020, .num_frames = 12},
    {.state = 0x0000, .num_frames = 34},
    {.state = 0x0020, .num_frames = 19},
    {.state = 0x0000, .num_frames = 16},
    {.state = 0x4000, .num_frames = 14},
    {.state = 0x0000, .num_frames = 37},
    {.state = 0x0020, .num_frames = 38},
    {.state = 0x0000, .num_frames = 8},
    {.state = 0x0080, .num_frames = 90},
    {.state = 0x0000, .num_frames = 99},
    {.state = 0x0080, .num_frames = 37},
    {.state = 0x4080, .num_frames = 40},
    {.state = 0x0080, .num_frames = 186},
    {.state = 0x0000, .num_frames = 43},
    {.state = 0x0020, .num_frames = 468},
    {.state = 0x0000, .num_frames = 7},
    {.state = 0x0080, .num_frames = 14},
    {.state = 0x0000, .num_frames = 9},
    {.state = 0x0020, .num_frames = 6},
    {.state = 0x0000, .num_frames = 0},
};

static DemoPlaybackSample _ghz1demo[] = {
    {.state = 0x0000, .num_frames = 127},
    {.state = 0x0020, .num_frames = 122},
    {.state = 0x0060, .num_frames = 10},
    {.state = 0x0040, .num_frames = 6},
    {.state = 0x0000, .num_frames = 25},
    {.state = 0x0020, .num_frames = 17},
    {.state = 0x4020, .num_frames = 12},
    {.state = 0x0020, .num_frames = 77},
    {.state = 0x0060, .num_frames = 3},
    {.state = 0x0040, .num_frames = 10},
    {.state = 0x0000, .num_frames = 47},
    {.state = 0x0020, .num_frames = 25},
    {.state = 0x0000, .num_frames = 2},
    {.state = 0x4000, .num_frames = 4},
    {.state = 0x0000, .num_frames = 35},
    {.state = 0x0020, .num_frames = 103},
    {.state = 0x4020, .num_frames = 1},
    {.state = 0x4000, .num_frames = 8},
    {.state = 0x0000, .num_frames = 35},
    {.state = 0x0020, .num_frames = 14},
    {.state = 0x0000, .num_frames = 10},
    {.state = 0x4000, .num_frames = 7},
    {.state = 0x0000, .num_frames = 2},
    {.state = 0x0080, .num_frames = 37},
    {.state = 0x0000, .num_frames = 10},
    {.state = 0x0020, .num_frames = 243},
    {.state = 0x4020, .num_frames = 25},
    {.state = 0x0020, .num_frames = 18},
    {.state = 0x4020, .num_frames = 55},
    {.state = 0x4000, .num_frames = 1},
    {.state = 0x0000, .num_frames = 29},
    {.state = 0x0020, .num_frames = 1},
    {.state = 0x4020, .num_frames = 14},
    {.state = 0x0020, .num_frames = 7},
    {.state = 0x0000, .num_frames = 21},
    {.state = 0x0080, .num_frames = 10},
    {.state = 0x0000, .num_frames = 34},
    {.state = 0x0080, .num_frames = 12},
    {.state = 0x0000, .num_frames = 11},
    {.state = 0x0020, .num_frames = 38},
    {.state = 0x0000, .num_frames = 22},
    {.state = 0x0080, .num_frames = 35},
    {.state = 0x0000, .num_frames = 10},
    {.state = 0x0020, .num_frames = 33},
    {.state = 0x4000, .num_frames = 7},
    {.state = 0x0000, .num_frames = 39},
    {.state = 0x0020, .num_frames = 9},
    {.state = 0x0000, .num_frames = 14},
    {.state = 0x0080, .num_frames = 55},
    {.state = 0x0000, .num_frames = 3},
    {.state = 0x0040, .num_frames = 14},
    {.state = 0x0000, .num_frames = 38},
    {.state = 0x0020, .num_frames = 23},
    {.state = 0x0000, .num_frames = 67},
    {.state = 0x0020, .num_frames = 41},
    {.state = 0x4020, .num_frames = 12},
    {.state = 0x0020, .num_frames = 9},
    {.state = 0x0000, .num_frames = 21},
    {.state = 0x0080, .num_frames = 49},
    {.state = 0x0000, .num_frames = 10},
    {.state = 0x0020, .num_frames = 81},
    {.state = 0x0000, .num_frames = 12},
    {.state = 0x0020, .num_frames = 15},
    {.state = 0x0000, .num_frames = 2},
    {.state = 0x0020, .num_frames = 32},
    {.state = 0x4020, .num_frames = 21},
    {.state = 0x0020, .num_frames = 72},
    {.state = 0x0000, .num_frames = 0},
};

static DemoPlaybackSample _swz1demo[] = {
        {.state = 0x0000, .num_frames = 84},
    {.state = 0x0020, .num_frames = 211},
    {.state = 0x0000, .num_frames = 7},
    {.state = 0x0080, .num_frames = 35},
    {.state = 0x0000, .num_frames = 6},
    {.state = 0x0080, .num_frames = 26},
    {.state = 0x4080, .num_frames = 32},
    {.state = 0x0000, .num_frames = 9},
    {.state = 0x0020, .num_frames = 253},
    {.state = 0x0000, .num_frames = 10},
    {.state = 0x0020, .num_frames = 47},
    {.state = 0x4000, .num_frames = 9},
    {.state = 0x0000, .num_frames = 10},
    {.state = 0x0020, .num_frames = 17},
    {.state = 0x4020, .num_frames = 50},
    {.state = 0x0020, .num_frames = 24},
    {.state = 0x0000, .num_frames = 6},
    {.state = 0x0080, .num_frames = 28},
    {.state = 0x0000, .num_frames = 6},
    {.state = 0x4000, .num_frames = 11},
    {.state = 0x0000, .num_frames = 2},
    {.state = 0x0080, .num_frames = 8},
    {.state = 0x0000, .num_frames = 17},
    {.state = 0x0080, .num_frames = 11},
    {.state = 0x0000, .num_frames = 6},
    {.state = 0x4000, .num_frames = 24},
    {.state = 0x4080, .num_frames = 19},
    {.state = 0x0080, .num_frames = 3},
    {.state = 0x0000, .num_frames = 15},
    {.state = 0x0020, .num_frames = 32},
    {.state = 0x4020, .num_frames = 6},
    {.state = 0x4000, .num_frames = 1},
    {.state = 0x0000, .num_frames = 13},
    {.state = 0x0080, .num_frames = 9},
    {.state = 0x0000, .num_frames = 16},
    {.state = 0x4020, .num_frames = 9},
    {.state = 0x0020, .num_frames = 1},
    {.state = 0x0000, .num_frames = 36},
    {.state = 0x0080, .num_frames = 20},
    {.state = 0x4080, .num_frames = 23},
    {.state = 0x4000, .num_frames = 3},
    {.state = 0x0000, .num_frames = 10},
    {.state = 0x0020, .num_frames = 41},
    {.state = 0x0000, .num_frames = 21},
    {.state = 0x0020, .num_frames = 77},
    {.state = 0x4020, .num_frames = 8},
    {.state = 0x0020, .num_frames = 99},
    {.state = 0x4020, .num_frames = 41},
    {.state = 0x0020, .num_frames = 78},
    {.state = 0x4020, .num_frames = 9},
    {.state = 0x4000, .num_frames = 1},
    {.state = 0x0000, .num_frames = 8},
    {.state = 0x0080, .num_frames = 15},
    {.state = 0x0000, .num_frames = 17},
    {.state = 0x0020, .num_frames = 50},
    {.state = 0x4020, .num_frames = 8},
    {.state = 0x4000, .num_frames = 17},
    {.state = 0x0000, .num_frames = 4},
    {.state = 0x0080, .num_frames = 23},
    {.state = 0x0000, .num_frames = 12},
    {.state = 0x0020, .num_frames = 2},
    {.state = 0x4020, .num_frames = 13},
    {.state = 0x0020, .num_frames = 11},
    {.state = 0x0000, .num_frames = 42},
    {.state = 0x4000, .num_frames = 7},
    {.state = 0x4020, .num_frames = 4},
    {.state = 0x0020, .num_frames = 6},
    {.state = 0x0000, .num_frames = 33},
    {.state = 0x0020, .num_frames = 82},
    {.state = 0x0000, .num_frames = 0},
};

static DemoPlaybackSample _aoz1demo[] = {
    {.state = 0x0000, .num_frames = 92},
    {.state = 0x4080, .num_frames = 36},
    {.state = 0x0080, .num_frames = 2},
    {.state = 0x0000, .num_frames = 14},
    {.state = 0x0020, .num_frames = 54},
    {.state = 0x4020, .num_frames = 11},
    {.state = 0x0020, .num_frames = 15},
    {.state = 0x0000, .num_frames = 24},
    {.state = 0x0020, .num_frames = 16},
    {.state = 0x0000, .num_frames = 41},
    {.state = 0x4000, .num_frames = 19},
    {.state = 0x0000, .num_frames = 8},
    {.state = 0x0020, .num_frames = 5},
    {.state = 0x0000, .num_frames = 8},
    {.state = 0x0080, .num_frames = 20},
    {.state = 0x0000, .num_frames = 13},
    {.state = 0x0020, .num_frames = 179},
    {.state = 0x4020, .num_frames = 24},
    {.state = 0x0020, .num_frames = 23},
    {.state = 0x4020, .num_frames = 9},
    {.state = 0x0020, .num_frames = 85},
    {.state = 0x4020, .num_frames = 27},
    {.state = 0x0020, .num_frames = 22},
    {.state = 0x4020, .num_frames = 9},
    {.state = 0x0020, .num_frames = 113},
    {.state = 0x0000, .num_frames = 15},
    {.state = 0x0080, .num_frames = 19},
    {.state = 0x0000, .num_frames = 13},
    {.state = 0x0020, .num_frames = 22},
    {.state = 0x4020, .num_frames = 11},
    {.state = 0x0020, .num_frames = 5},
    {.state = 0x4020, .num_frames = 8},
    {.state = 0x0020, .num_frames = 19},
    {.state = 0x0000, .num_frames = 18},
    {.state = 0x0020, .num_frames = 46},
    {.state = 0x4020, .num_frames = 4},
    {.state = 0x0020, .num_frames = 88},
    {.state = 0x4020, .num_frames = 7},
    {.state = 0x0020, .num_frames = 6},
    {.state = 0x4020, .num_frames = 3},
    {.state = 0x0020, .num_frames = 59},
    {.state = 0x4020, .num_frames = 4},
    {.state = 0x0020, .num_frames = 18},
    {.state = 0x0000, .num_frames = 76},
    {.state = 0x0020, .num_frames = 56},
    {.state = 0x4020, .num_frames = 8},
    {.state = 0x4000, .num_frames = 1},
    {.state = 0x0000, .num_frames = 51},
    {.state = 0x0020, .num_frames = 5},
    {.state = 0x0000, .num_frames = 14},
    {.state = 0x0080, .num_frames = 20},
    {.state = 0x0000, .num_frames = 17},
    {.state = 0x0040, .num_frames = 9},
    {.state = 0x4040, .num_frames = 9},
    {.state = 0x0040, .num_frames = 21},
    {.state = 0x0000, .num_frames = 88},
    {.state = 0x0020, .num_frames = 54},
    {.state = 0x4020, .num_frames = 9},
    {.state = 0x0020, .num_frames = 262},
    {.state = 0x4020, .num_frames = 28},
    {.state = 0x0020, .num_frames = 1},
    {.state = 0x0000, .num_frames = 33},
    {.state = 0x4000, .num_frames = 2},
    {.state = 0x4020, .num_frames = 32},
    {.state = 0x0000, .num_frames = 35},
    {.state = 0x4000, .num_frames = 1},
    {.state = 0x4020, .num_frames = 9},
    {.state = 0x0000, .num_frames = 27},
    {.state = 0x0020, .num_frames = 49},
    {.state = 0x0000, .num_frames = 14},
    {.state = 0x0040, .num_frames = 4},
    {.state = 0x4040, .num_frames = 6},
    {.state = 0x0040, .num_frames = 7},
    {.state = 0x0000, .num_frames = 0},
};

static DemoPlaybackSample _ez1demo[] = {
    {.state = 0x0000, .num_frames = 59},
    {.state = 0x0020, .num_frames = 248},
    {.state = 0x0000, .num_frames = 5},
    {.state = 0x0080, .num_frames = 45},
    {.state = 0x4080, .num_frames = 8},
    {.state = 0x4000, .num_frames = 7},
    {.state = 0x0000, .num_frames = 3},
    {.state = 0x4020, .num_frames = 46},
    {.state = 0x0020, .num_frames = 208},
    {.state = 0x4020, .num_frames = 6},
    {.state = 0x0020, .num_frames = 4},
    {.state = 0x4020, .num_frames = 42},
    {.state = 0x0020, .num_frames = 80},
    {.state = 0x0000, .num_frames = 45},
    {.state = 0x0080, .num_frames = 254},
    {.state = 0x0000, .num_frames = 57},
    {.state = 0x0020, .num_frames = 47},
    {.state = 0x4020, .num_frames = 40},
    {.state = 0x0020, .num_frames = 64},
    {.state = 0x0060, .num_frames = 14},
    {.state = 0x0040, .num_frames = 3},
    {.state = 0x0000, .num_frames = 264},
    {.state = 0x0080, .num_frames = 126},
    {.state = 0x4080, .num_frames = 15},
    {.state = 0x0080, .num_frames = 1},
    {.state = 0x0000, .num_frames = 40},
    {.state = 0x0020, .num_frames = 5},
    {.state = 0x4020, .num_frames = 40},
    {.state = 0x0020, .num_frames = 7},
    {.state = 0x0000, .num_frames = 65},
    {.state = 0x0020, .num_frames = 163},
    {.state = 0x4020, .num_frames = 14},
    {.state = 0x0020, .num_frames = 137},
    {.state = 0x0000, .num_frames = 1},
    {.state = 0x0020, .num_frames = 1},
    {.state = 0x0000, .num_frames = 37},
    {.state = 0x4000, .num_frames = 4},
    {.state = 0x4020, .num_frames = 7},
    {.state = 0x4000, .num_frames = 2},
    {.state = 0x0000, .num_frames = 26},
    {.state = 0x0080, .num_frames = 18},
    {.state = 0x0000, .num_frames = 10},
    {.state = 0x0020, .num_frames = 3},
    {.state = 0x4020, .num_frames = 34},
    {.state = 0x0000, .num_frames = 27},
    {.state = 0x0020, .num_frames = 6},
    {.state = 0x0000, .num_frames = 0},
};

DemoPlaybackSample *
_get_playback_data(int level)
{
    switch(level) {
    case 0: return _pz1demo;   // (Unused)
    case 4: return _ghz1demo;  // Knuckles
    case 6: return _swz1demo;  // Sonic
    case 10: return _aoz1demo; // Miles
    case 16: return _ez1demo;  // (Unused)
    default: return _dummydemo;
    }
}
