// PS1 type definitions and constants

#ifndef PS1_TYPES_H
#define PS1_TYPES_H

#include <stdint.h>
#include <stdbool.h>

// PS1 memory configuration
#define PS1_VRAM_WIDTH 1024
#define PS1_VRAM_HEIGHT 512
#define PS1_VRAM_SIZE (PS1_VRAM_WIDTH * PS1_VRAM_HEIGHT)

// Tile size (64x64 pixels)
#define TILE_WIDTH 64
#define TILE_HEIGHT 64

// Level dimensions
#define MAX_LEVEL_WIDTH 256
#define MAX_LEVEL_HEIGHT 256

// Color formats
#define COLOR_RGB15 15
#define COLOR_RGB16 16
#define COLOR_R5G5B5A1 1
#define COLOR_YUV422 422

// Palette formats
#define PALETTE_DIRECT 0x00
#define PALETTE_INDIRECT 0x10
#define PALETTE_COMPRESSED 0x20

// TIM compression types
typedef enum {
    TIM_COMPRESSION_NONE = 0,
    TIM_COMPRESSION_VQ
} TIMCompression;

// PS1 RGB color (15-bit or 16-bit)
typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} PS1Color;

// PS1 Color Look-Up Table (CLUT)
typedef struct {
    uint16_t entry_count;
    PS1Color* colors;
} PS1CLUT;

// TIM file header
typedef struct {
    uint16_t signature;
    uint16_t image_format;
    uint8_t palette_format;
    uint8_t clut_entries;
    uint32_t clut_offset;
    uint32_t image_offset;
} TIMFileHeader;

#endif // PS1_TYPES_H