// Level loader implementation - PS1 MAP file parser

#include "level_loader.h"
#include "tim_parser.h"
#include "log.h"
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <arpa/inet.h>  // For ntohs() - big-endian conversion

// Helper: read big-endian uint16
static inline uint16_t read_be_u16(const uint8_t* data) {
    return (data[0] << 8) | data[1];
}

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

    // Parse file extension
    const char* ext = level_path;
    while (*ext) ext++;
    while (*ext != '.' && ext > level_path) ext--;
    if (*ext == '.') ext++;

    bool is_map = (strcasecmp(ext, "MAP") == 0);

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
    level->tile_size = 16;  // Default tile size
    level->name[0] = 0;
    level->description[0] = 0;

    if (is_map && file_size >= 6) {
        // MAP file format (BIG ENDIAN):
        // Header: 
        //   - uint16 tile_width (16 or 128)
        //   - uint16 num_tiles (or frame count)
        //   - uint16 frame_size (2 for MAP16, 8 for MAP128)
        // Data: array of (tile_index + props) or just tile_index
        
        uint16_t tile_width = read_be_u16(file_data);
        uint16_t num_tiles = read_be_u16(file_data + 2);
        uint16_t frame_size = read_be_u16(file_data + 4);
        
        LOG_INFO("level_loader.c", __LINE__, "MAP header: tile_width=%u, num_tiles=%u, frame_size=%u",
                 tile_width, num_tiles, frame_size);
        
        // Detect file type
        bool is_map16 = (tile_width == 16);
        bool is_map128 = (tile_width == 128);
        
        if (is_map16 || (tile_width < 1000 && frame_size == 2)) {
            // MAP16 format: 16x16 pixel tiles, animated frames
            level->tile_size = 16;
            
            // The MAP16.MAP contains frames of 2x2 tiles (4 tiles per frame)
            // Each frame is a mini-animation stored as tile indices
            // For visualization, we display all frames in a grid
            
            // Calculate grid dimensions for display
            // num_tiles = number of animation frames
            // Each frame is frame_size x frame_size tiles
            
            // Arrange frames in a reasonable grid
            uint32_t frames_per_row = 20;  // 20 frames per row
            if (num_tiles < 100) frames_per_row = 10;
            else if (num_tiles < 500) frames_per_row = 16;
            
            uint32_t rows = (num_tiles + frames_per_row - 1) / frames_per_row;
            if (rows < 1) rows = 1;
            
            // Total dimensions in tiles
            level->width_tiles = frames_per_row * frame_size;
            level->height_tiles = rows * frame_size;
            
            uint32_t tile_count = level->width_tiles * level->height_tiles;
            level->tile_data = (uint16_t*)malloc(tile_count * sizeof(uint16_t));
            if (!level->tile_data) {
                LOG_ERROR("level_loader.c", __LINE__, "Failed to allocate tile data");
                free(level);
                free(file_data);
                return NULL;
            }
            
            // Initialize all tiles to 0 (blank)
            memset(level->tile_data, 0, tile_count * sizeof(uint16_t));
            
            // Read frame data from offset 6
            // For MAP16: each frame has 4 tiles (2x2), stored as tile_index
            size_t data_offset = 6;
            
            // Read tile indices (big-endian uint16)
            uint32_t tiles_read = 0;
            for (uint32_t frame = 0; frame < num_tiles; frame++) {
                for (int row = 0; row < frame_size; row++) {
                    for (int col = 0; col < frame_size; col++) {
                        size_t src_idx = frame * (frame_size * frame_size) + row * frame_size + col;
                        size_t byte_offset = data_offset + src_idx * 2;
                        
                        if (byte_offset + 2 <= file_size) {
                            uint16_t tile_val = read_be_u16(file_data + byte_offset);
                            
                            // Calculate position in output grid
                            uint32_t frame_x = frame % frames_per_row;
                            uint32_t frame_y = frame / frames_per_row;
                            
                            uint32_t tile_x = frame_x * frame_size + col;
                            uint32_t tile_y = frame_y * frame_size + row;
                            
                            if (tile_x < level->width_tiles && tile_y < level->height_tiles) {
                                uint32_t out_idx = tile_y * level->width_tiles + tile_x;
                                level->tile_data[out_idx] = tile_val;
                                tiles_read++;
                            }
                        }
                    }
                }
            }
            
            LOG_INFO("level_loader.c", __LINE__, "MAP16: %u frames (%ux%u grid), %u tiles read, dims=%ux%u", 
                     num_tiles, frames_per_row, rows, tiles_read, level->width_tiles, level->height_tiles);
        } 
        else if (is_map128 || (tile_width >= 100 && frame_size == 8)) {
            // MAP128 format: 128x128 pixel tiles, level map (collision/proximity)
            level->tile_size = 128;
            
            // MAP128 stores chunks of 8x8 tiles
            // Each chunk represents a section of the level map
            
            // Calculate dimensions based on number of chunks
            uint32_t chunks_x = 32;  // Default: 32 chunks per row (for 256x256 tiles = 32*8 x 32*8)
            if (num_tiles <= 256) {
                chunks_x = 16;  // Smaller maps: 16 chunks per row
            } else if (num_tiles > 1024) {
                chunks_x = 64;  // Larger maps
            }
            
            // Calculate rows needed
            uint32_t chunks_y = (num_tiles + chunks_x - 1) / chunks_x;
            if (chunks_y < 1) chunks_y = 1;
            
            // Total dimensions in tiles (each chunk is frame_size x frame_size tiles)
            level->width_tiles = chunks_x * frame_size;
            level->height_tiles = chunks_y * frame_size;
            
            uint32_t tile_count = level->width_tiles * level->height_tiles;
            level->tile_data = (uint16_t*)malloc(tile_count * sizeof(uint16_t));
            if (!level->tile_data) {
                LOG_ERROR("level_loader.c", __LINE__, "Failed to allocate tile data");
                free(level);
                free(file_data);
                return NULL;
            }
            
            memset(level->tile_data, 0, tile_count * sizeof(uint16_t));
            
            // MAP128 format: each entry is uint16 tile_index + uint8 props (3 bytes)
            size_t data_offset = 6;
            uint32_t tiles_read = 0;
            
            for (uint32_t chunk = 0; chunk < num_tiles; chunk++) {
                for (int row = 0; row < 8; row++) {
                    for (int col = 0; col < 8; col++) {
                        // Index within the chunk data
                        size_t tile_in_chunk = row * 8 + col;
                        // 3 bytes per tile: 2 for index, 1 for props
                        size_t byte_offset = data_offset + (chunk * 64 + tile_in_chunk) * 3;
                        
                        if (byte_offset + 2 < file_size) {
                            uint16_t tile_val = read_be_u16(file_data + byte_offset);
                            
                            uint32_t chunk_x = chunk % chunks_x;
                            uint32_t chunk_y = chunk / chunks_x;
                            
                            uint32_t tile_x = chunk_x * 8 + col;
                            uint32_t tile_y = chunk_y * 8 + row;
                            
                            if (tile_x < level->width_tiles && tile_y < level->height_tiles) {
                                uint32_t out_idx = tile_y * level->width_tiles + tile_x;
                                level->tile_data[out_idx] = tile_val;
                                tiles_read++;
                            }
                        }
                    }
                }
            }
            
            LOG_INFO("level_loader.c", __LINE__, "MAP128: %u chunks (%ux%u), %u tiles read, dims=%ux%u", 
                     num_tiles, chunks_x, chunks_y, tiles_read, level->width_tiles, level->height_tiles);
        }
        else {
            // Unknown format - try raw tile data
            level->tile_size = tile_width > 0 ? tile_width : 16;
            
            // Calculate dimensions from file size
            uint32_t tile_count = (file_size - 6) / 2;  // Assume 16-bit tiles
            
            // Try to find reasonable dimensions
            level->width_tiles = 32;
            level->height_tiles = (tile_count + 31) / 32;
            
            uint32_t total = level->width_tiles * level->height_tiles;
            level->tile_data = (uint16_t*)malloc(total * sizeof(uint16_t));
            if (!level->tile_data) {
                LOG_ERROR("level_loader.c", __LINE__, "Failed to allocate tile data");
                free(level);
                free(file_data);
                return NULL;
            }
            
            memset(level->tile_data, 0, total * sizeof(uint16_t));
            
            for (uint32_t i = 0; i < tile_count && (6 + i * 2 + 2) <= file_size; i++) {
                level->tile_data[i] = read_be_u16(file_data + 6 + i * 2);
            }
            
            LOG_INFO("level_loader.c", __LINE__, "MAP (unknown): tile_size=%u, dims=%ux%u", 
                     level->tile_size, level->width_tiles, level->height_tiles);
        }
    } else {
        LOG_WARNING("level_loader.c", __LINE__, "Unknown file format or file too small: %s", ext);
    }

    free(file_data);
    strncpy(level->name, level_path, sizeof(level->name) - 1);
    level->name[sizeof(level->name) - 1] = 0;

    LOG_INFO("level_loader.c", __LINE__, "Level loaded: %s (%ux%u tiles, tile_size=%u)", 
             level->name, level->width_tiles, level->height_tiles, level->tile_size);
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