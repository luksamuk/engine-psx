// TIM format parser implementation

#include "tim_types.h"
#include "log.h"
#include <string.h>
#include <stdlib.h>
#include "raylib.h"

bool TIM_ParseHeader(FILE* file, TIMFileHeader* header) {
    if (fread(header, sizeof(TIMFileHeader), 1, file) != 1) {
        LOG_ERROR("tim_parser.c", __LINE__, "Failed to read TIM header");
        return false;
    }

    // PS1 TIM files are little-endian, no byte swap needed on LE hosts
    // Note: If running on big-endian system, would need bswap here
    
    LOG_DEBUG("tim_parser.c", __LINE__, "TIM header: sig=0x%04X fmt=%d palette=%d clut=%d offset_clut=%d offset_img=%d",
             header->signature, header->image_format, header->palette_format,
             header->clut_entries, header->clut_offset, header->image_offset);

    return true;
}

bool TIM_LoadCLUT(FILE* file, uint32_t offset, uint8_t palette_format, uint8_t clut_entries, PS1CLUT** out_clut) {
    if (fseek(file, offset, SEEK_SET) != 0) {
        LOG_ERROR("tim_parser.c", __LINE__, "Failed to seek to CLUT offset");
        return false;
    }

    PS1CLUT* clut = (PS1CLUT*)malloc(sizeof(PS1CLUT));
    if (!clut) {
        LOG_ERROR("tim_parser.c", __LINE__, "Failed to allocate CLUT memory");
        return false;
    }

    clut->entry_count = clut_entries;
    clut->colors = (PS1Color*)malloc(clut_entries * sizeof(PS1Color));
    if (!clut->colors) {
        LOG_ERROR("tim_parser.c", __LINE__, "Failed to allocate CLUT colors");
        free(clut);
        return false;
    }

    for (uint32_t i = 0; i < clut_entries; i++) {
        if (palette_format == TIM_PALETTE_DIRECT) {
            uint32_t color_value;
            if (fread(&color_value, sizeof(uint32_t), 1, file) != 1) {
                LOG_ERROR("tim_parser.c", __LINE__, "Failed to read CLUT color %d", i);
                free(clut->colors);
                free(clut);
                return false;
            }
            // PS1 TIM is little-endian, no swap needed on LE hosts

            clut->colors[i].r = (color_value >> 10) & 0x1F;
            clut->colors[i].g = (color_value >> 5) & 0x1F;
            clut->colors[i].b = color_value & 0x1F;
        } else if (palette_format == TIM_PALETTE_INDIRECT) {
            uint16_t color_value;
            if (fread(&color_value, sizeof(uint16_t), 1, file) != 1) {
                LOG_ERROR("tim_parser.c", __LINE__, "Failed to read CLUT color %d", i);
                free(clut->colors);
                free(clut);
                return false;
            }
            // PS1 TIM is little-endian, no swap needed on LE hosts

            clut->colors[i].r = (color_value >> 10) & 0x1F;
            clut->colors[i].g = (color_value >> 5) & 0x1F;
            clut->colors[i].b = color_value & 0x1F;
        } else {
            LOG_ERROR("tim_parser.c", __LINE__, "Unsupported palette format: 0x%02X", palette_format);
            free(clut->colors);
            free(clut);
            return false;
        }

        LOG_TRACE("tim_parser.c", __LINE__, "CLUT[%d]: RGB(%d,%d,%d)", i, clut->colors[i].r, clut->colors[i].g, clut->colors[i].b);
    }

    *out_clut = clut;
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

    TIMFile* tim = (TIMFile*)malloc(sizeof(TIMFile));
    if (!tim) {
        LOG_ERROR("tim_parser.c", __LINE__, "Failed to allocate TIM file structure");
        free(file_data);
        return NULL;
    }

    // Parse header
    TIMFileHeader* header = &tim->header;
    memcpy(header, file_data, sizeof(TIMFileHeader));

    // PS1 TIM files are little-endian, no byte swap needed on LE hosts
    // Note: If running on big-endian system, would need bswap here

    tim->file_size = file_size;
    tim->file_data = file_data;
    tim->palette = NULL;
    tim->image_data = NULL;
    tim->width = 0;
    tim->height = 0;

    // Detect format and set dimensions
    uint16_t img_format = header->image_format;
    uint8_t clut_count = header->clut_entries;

    switch (img_format) {
        case TIM_FORMAT_CLUT_RAW_4BIT:
            tim->width = 64;  // Will be calculated from actual data
            tim->height = 64;
            break;
        case TIM_FORMAT_CLUT_RAW_8BIT:
            tim->width = 64;
            tim->height = 64;
            break;
        case TIM_FORMAT_CLUT_RAW_16BIT:
            tim->width = 64;
            tim->height = 64;
            break;
        default:
            LOG_ERROR("tim_parser.c", __LINE__, "Unsupported image format: 0x%04X", img_format);
            free(tim);
            free(file_data);
            return NULL;
    }

    LOG_INFO("tim_parser.c", __LINE__, "TIM file loaded: %s (%dx%d) format=0x%04X", filepath, tim->width, tim->height, img_format);

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