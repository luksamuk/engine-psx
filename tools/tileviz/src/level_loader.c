// Level loader implementation

#include "level_loader.h"
#include "tim_parser.h"
#include "log.h"
#include <stdlib.h>
#include <string.h>

LevelData* level_load(const char* level_path) {
    FILE* file = fopen(level_path, "rb");
    if (!file) {
        LOG_ERROR("level_loader.c", __LINE__, "Failed to open level file: %s", level_path);
        return NULL;
    }

    // Get file size
    fseek(file, 0, SEEK_END);
    uint64_t file_size = ftell(file);
    rewind(file);

    LOG_INFO("level_loader.c", __LINE__, "Loading level file: %s (%.2f KB)", level_path, file_size / 1024.0);

    // Parse header
    uint8_t* file_data = (uint8_t*)malloc(file_size);
    if (!file_data) {
        LOG_ERROR("level_loader.c", __LINE__, "Failed to allocate file data");
        fclose(file);
        return NULL;
    }

    if (fread(file_data, 1, file_size, file) != file_size) {
        LOG_ERROR("level_loader.c", __LINE__, "Failed to read file data");
        free(file_data);
        fclose(file);
        return NULL;
    }

    fclose(file);

    LevelData* level = (LevelData*)malloc(sizeof(LevelData));
    if (!level) {
        LOG_ERROR("level_loader.c", __LINE__, "Failed to allocate level data");
        free(file_data);
        return NULL;
    }

    level->file_size = file_size;
    level->width_tiles = 0;
    level->height_tiles = 0;
    level->tile_data = NULL;
    level->name[0] = 0;
    level->description[0] = 0;

    // Parse MAP/PRG file based on extension
    const char* ext = level_path;
    while (*ext) ext++;
    while (*ext != '.' && ext > level_path) ext--;
    if (*ext == '.') ext++;

    bool is_map = false;
    bool is_prg = false;

    if (strcasecmp(ext, "MAP") == 0) {
        is_map = true;
    } else if (strcasecmp(ext, "PRG") == 0) {
        is_prg = true;
    }

    if (is_map) {
        if (file_size < sizeof(uint32_t) * 4) {
            LOG_ERROR("level_loader.c", __LINE__, "MAP file too small");
            free(level);
            free(file_data);
            return NULL;
        }

        uint32_t* header = (uint32_t*)file_data;

        level->width_tiles = __builtin_bswap32(header[2]);
        level->height_tiles = __builtin_bswap32(header[3]);

        if (level->width_tiles > MAX_LEVEL_WIDTH) {
            LOG_WARNING("level_loader.c", __LINE__, "Level width exceeds max: %u", level->width_tiles);
        }
        if (level->height_tiles > MAX_LEVEL_HEIGHT) {
            LOG_WARNING("level_loader.c", __LINE__, "Level height exceeds max: %u", level->height_tiles);
        }

        LOG_INFO("level_loader.c", __LINE__, "Level map: %dx%d tiles", level->width_tiles, level->height_tiles);

        // Allocate tile data
        level->tile_data = (uint16_t*)malloc(level->width_tiles * level->height_tiles * sizeof(uint16_t));
        if (!level->tile_data) {
            LOG_ERROR("level_loader.c", __LINE__, "Failed to allocate tile data");
            free(level);
            free(file_data);
            return NULL;
        }

        // Copy tile data from file
        uint32_t tile_offset = __builtin_bswap32(header[2]);
        if (tile_offset > 0 && tile_offset + level->width_tiles * level->height_tiles * 2 <= file_size) {
            for (uint32_t i = 0; i < level->width_tiles * level->height_tiles; i++) {
                level->tile_data[i] = __builtin_bswap16(((uint16_t*)(file_data + tile_offset))[i]);
                LOG_TRACE("level_loader.c", __LINE__, "Tile[%d] = %u", i, level->tile_data[i]);
            }
        }
    } else {
        LOG_WARNING("level_loader.c", __LINE__, "PRG file format support not yet implemented");
    }

    free(file_data);
    strncpy(level->name, level_path, sizeof(level->name) - 1);
    level->name[sizeof(level->name) - 1] = 0;

    LOG_INFO("level_loader.c", __LINE__, "Level loaded successfully: %s", level->name);
    return level;
}

void level_free(LevelData* level) {
    if (!level) return;

    if (level->tile_data) {
        free(level->tile_data);
    }

    free(level);
}

uint16_t level_get_tile(LevelData* level, int tile_x, int tile_y) {
    if (!level || !level->tile_data) {
        return 0;
    }

    int width = (int)level->width_tiles;
    int height = (int)level->height_tiles;

    if (tile_x < 0 || tile_y < 0 || tile_x >= width || tile_y >= height) {
        return 0;
    }

    return level->tile_data[tile_y * width + tile_x];
}