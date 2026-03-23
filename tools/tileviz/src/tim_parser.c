// TIM format parser implementation

#include "tim_types.h"
#include "log.h"
#include <string.h>
#include <stdlib.h>
#include "raylib.h"

// Tim format:
// - uint32 magic (always 0x10)
// - uint32 flags (bits 0-1 = bpp, bit 3 = has_clut)
// - if has_clut:
//     - uint32 clut_len (size in bytes, including header)
//     - uint16 clut_x, clut_y (vram position)
//     - uint16 clut_w (colors per row), clut_h (rows)
//     - uint16 colors[clut_w * clut_h] (RGB555)
// - uint32 img_len (size in bytes, including header)
// - uint16 img_x, img_y (vram position)
// - uint16 img_w (words per row), img_h (rows)
// - uint16 pixels[...] (indices or RGB555)

bool TIM_ParseHeader(FILE* file, TIMFileHeader* header) {
    if (fread(header, sizeof(TIMFileHeader), 1, file) != 1) {
        LOG_ERROR("tim_parser.c", __LINE__, "Failed to read TIM header");
        return false;
    }
    
    // Verify magic number
    if (header->magic != TIM_MAGIC) {
        LOG_ERROR("tim_parser.c", __LINE__, "Invalid TIM magic: 0x%08X (expected 0x%08X)", 
                  header->magic, TIM_MAGIC);
        return false;
    }
    
    LOG_DEBUG("tim_parser.c", __LINE__, "TIM header: magic=0x%08X flags=0x%08X bpp=%d has_clut=%d",
              header->magic, header->flags, TIM_GET_BPP(header->flags), TIM_HAS_CLUT(header->flags));
    
    return true;
}

// Convert PS1 RGB555 (16-bit) to RGB888
static PS1Color RGB555ToRGB888(uint16_t color555) {
    PS1Color color;
    // PS1 RGB555: x-b4-b3-b2-b1-b0-g5-g4-g3-g2-g1-g0-r4-r3-r2-r1-r0
    // Bit 15 is semi-transparent flag (ignored for color extraction)
    color.r = (color555 >> 0) & 0x1F;   // 5 bits red
    color.g = (color555 >> 5) & 0x1F;   // 5 bits green
    color.b = (color555 >> 10) & 0x1F;  // 5 bits blue
    
    // Scale to 8-bit (multiply by ~8)
    color.r = (color.r << 3) | (color.r >> 2);
    color.g = (color.g << 3) | (color.g >> 2);
    color.b = (color.b << 3) | (color.b >> 2);
    
    return color;
}

// Parse CLUT block from file data
static bool TIM_ParseCLUT(TIMFile* tim, const uint8_t* data, uint32_t* offset) {
    // Read CLUT block length
    uint32_t clut_len;
    memcpy(&clut_len, data + *offset, sizeof(uint32_t));
    *offset += sizeof(uint32_t);
    
    // Read CLUT rectangle header
    memcpy(&tim->clut_rect, data + *offset, sizeof(TIMBlockHeader));
    *offset += sizeof(TIMBlockHeader);
    
    uint16_t clut_w = tim->clut_rect.width;
    uint16_t clut_h = tim->clut_rect.height;
    uint32_t num_colors = clut_w * clut_h;
    
    LOG_DEBUG("tim_parser.c", __LINE__, "CLUT: %d x %d = %d colors, vram(%d,%d)",
              clut_w, clut_h, num_colors, tim->clut_rect.vram_x, tim->clut_rect.vram_y);
    
    // Allocate palette
    tim->palette = (PS1CLUT*)malloc(sizeof(PS1CLUT));
    if (!tim->palette) {
        LOG_ERROR("tim_parser.c", __LINE__, "Failed to allocate CLUT");
        return false;
    }
    
    tim->palette->colors = (PS1Color*)malloc(num_colors * sizeof(PS1Color));
    if (!tim->palette->colors) {
        LOG_ERROR("tim_parser.c", __LINE__, "Failed to allocate CLUT colors");
        free(tim->palette);
        tim->palette = NULL;
        return false;
    }
    
    tim->palette->entry_count = (uint16_t)num_colors;
    
    // Read and convert colors from RGB555
    for (uint32_t i = 0; i < num_colors; i++) {
        uint16_t color555;
        memcpy(&color555, data + *offset, sizeof(uint16_t));
        *offset += sizeof(uint16_t);
        
        tim->palette->colors[i] = RGB555ToRGB888(color555);
        
        if (i < 4 || (i % 16 == 0 && i < 32)) {
            LOG_TRACE("tim_parser.c", __LINE__, "CLUT[%d]: RGB(%d,%d,%d)", i,
                      tim->palette->colors[i].r, tim->palette->colors[i].g, tim->palette->colors[i].b);
        }
    }
    
    return true;
}

// Parse image data block from file data
static bool TIM_ParseImageData(TIMFile* tim, const uint8_t* data, uint32_t* offset) {
    // Read image block length
    uint32_t img_len;
    memcpy(&img_len, data + *offset, sizeof(uint32_t));
    *offset += sizeof(uint32_t);
    
    // Read image rectangle header
    memcpy(&tim->image_rect, data + *offset, sizeof(TIMBlockHeader));
    *offset += sizeof(TIMBlockHeader);
    
    uint16_t img_w = tim->image_rect.width;   // In 16-bit words
    uint16_t img_h = tim->image_rect.height;  // In rows
    
    LOG_DEBUG("tim_parser.c", __LINE__, "Image: %d words x %d rows, vram(%d,%d)",
              img_w, img_h, tim->image_rect.vram_x, tim->image_rect.vram_y);
    
    // Calculate actual pixel width based on BPP
    // For 4-bit: each 16-bit word holds 4 pixels
    // For 8-bit: each 16-bit word holds 2 pixels
    // For 16-bit: each 16-bit word is 1 pixel
    
    uint32_t pixel_width;
    switch (tim->bpp) {
        case TIM_BPP_4BIT:
            // width is number of 16-bit words, each word = 4 pixels
            pixel_width = (uint32_t)img_w * 4;
            break;
        case TIM_BPP_8BIT:
            // width is number of 16-bit words, each word = 2 pixels
            pixel_width = (uint32_t)img_w * 2;
            break;
        case TIM_BPP_16BIT:
            // width is number of pixels directly
            pixel_width = img_w;
            break;
        default:
            LOG_ERROR("tim_parser.c", __LINE__, "Unsupported BPP: %d", tim->bpp);
            return false;
    }
    
    tim->width = pixel_width;
    tim->height = img_h;
    
    // Calculate image data size (in bytes)
    uint32_t data_size = img_w * img_h * sizeof(uint16_t);  // Always stored as 16-bit words
    
    // Allocate and copy image data
    tim->image_data = (uint8_t*)malloc(data_size);
    if (!tim->image_data) {
        LOG_ERROR("tim_parser.c", __LINE__, "Failed to allocate image data");
        return false;
    }
    
    memcpy(tim->image_data, data + *offset, data_size);
    *offset += data_size;
    
    LOG_INFO("tim_parser.c", __LINE__, "Image parsed: %dx%d (%d bytes)", 
             tim->width, tim->height, data_size);
    
    return true;
}

TIMFile* TIM_LoadFile(const char* filepath) {
    FILE* file = fopen(filepath, "rb");
    if (!file) {
        LOG_ERROR("tim_parser.c", __LINE__, "Failed to open file: %s", filepath);
        return NULL;
    }

    // Get file size
    fseek(file, 0, SEEK_END);
    uint32_t file_size = ftell(file);
    rewind(file);

    LOG_DEBUG("tim_parser.c", __LINE__, "Loading TIM file: %s (%d bytes)", filepath, file_size);

    // Allocate memory for file data
    uint8_t* file_data = (uint8_t*)malloc(file_size);
    if (!file_data) {
        LOG_ERROR("tim_parser.c", __LINE__, "Failed to allocate file data memory");
        fclose(file);
        return NULL;
    }

    // Read entire file
    if (fread(file_data, 1, file_size, file) != file_size) {
        LOG_ERROR("tim_parser.c", __LINE__, "Failed to read file data");
        free(file_data);
        fclose(file);
        return NULL;
    }

    fclose(file);

    // Allocate TIM file structure
    TIMFile* tim = (TIMFile*)malloc(sizeof(TIMFile));
    if (!tim) {
        LOG_ERROR("tim_parser.c", __LINE__, "Failed to allocate TIM file structure");
        free(file_data);
        return NULL;
    }
    
    memset(tim, 0, sizeof(TIMFile));
    tim->file_size = file_size;
    tim->file_data = file_data;

    // Parse header (first 8 bytes)
    uint32_t offset = 0;
    memcpy(&tim->header, file_data + offset, sizeof(TIMFileHeader));
    offset += sizeof(TIMFileHeader);
    
    // Verify magic
    if (tim->header.magic != TIM_MAGIC) {
        LOG_ERROR("tim_parser.c", __LINE__, "Invalid TIM magic: 0x%08X (expected 0x%08X)", 
                  tim->header.magic, TIM_MAGIC);
        free(file_data);
        free(tim);
        return NULL;
    }
    
    // Extract flags
    tim->bpp = TIM_GET_BPP(tim->header.flags);
    tim->has_clut = TIM_HAS_CLUT(tim->header.flags);
    
    LOG_DEBUG("tim_parser.c", __LINE__, "TIM: bpp=%d has_clut=%d", tim->bpp, tim->has_clut);

    // Parse CLUT if present
    if (tim->has_clut) {
        if (!TIM_ParseCLUT(tim, file_data, &offset)) {
            LOG_ERROR("tim_parser.c", __LINE__, "Failed to parse CLUT");
            free(file_data);
            if (tim->palette) {
                free(tim->palette->colors);
                free(tim->palette);
            }
            free(tim);
            return NULL;
        }
    }

    // Parse image data
    if (!TIM_ParseImageData(tim, file_data, &offset)) {
        LOG_ERROR("tim_parser.c", __LINE__, "Failed to parse image data");
        free(file_data);
        if (tim->palette) {
            free(tim->palette->colors);
            free(tim->palette);
        }
        free(tim);
        return NULL;
    }

    LOG_INFO("tim_parser.c", __LINE__, "TIM file loaded: %s (%dx%d, %dbpp%s)",
             filepath, tim->width, tim->height, 
             tim->bpp == TIM_BPP_4BIT ? 4 : (tim->bpp == TIM_BPP_8BIT ? 8 : 16),
             tim->has_clut ? ", CLUT" : "");

    return tim;
}

void TIM_FreeFile(TIMFile* tim) {
    if (!tim) return;

    if (tim->palette) {
        if (tim->palette->colors) {
            free(tim->palette->colors);
        }
        free(tim->palette);
    }

    if (tim->image_data) {
        free(tim->image_data);
    }

    if (tim->file_data) {
        free(tim->file_data);
    }

    free(tim);
}