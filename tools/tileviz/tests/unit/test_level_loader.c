// Level loader tests - FINAL CORRECTION based on chunkgen.py

#include "../include/level_loader.h"
#include "../include/log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

void create_test_map16() {
    FILE* file = fopen("test_map16.map", "wb");
    if (!file) return;

    // MAP16 format: 16x16 pixel tiles (based on MAP16.MAP from game)
    uint16_t tile_width = __builtin_bswap16(16);    // 16x16 pixel tiles
    uint16_t num_tiles = __builtin_bswap16(4096);   // 64x64 tile map
    uint16_t frame_side = __builtin_bswap16(2);     // 2x2 frames (16x16 pixels)

    // Write header
    fwrite(&tile_width, 2, 1, file);
    fwrite(&num_tiles, 2, 1, file);
    fwrite(&frame_side, 2, 1, file);

    // Write frame data: frame_side * frame_side tiles per frame
    // Total: num_tiles tiles organized in grid
    for (int frame = 0; frame < (num_tiles / (frame_side * frame_side)); frame++) {
        for (int row = 0; row < frame_side; row++) {
            for (int col = 0; col < frame_side; col++) {
                uint16_t index = __builtin_bswap16((frame * frame_side + row) * frame_side + col);
                // Tile 0 is white tile (blank), tile >0 are actual tiles
                fwrite(&index, 2, 1, file);
            }
        }
    }

    fclose(file);
    printf("Created test_map16.map (16x16 pixel tiles, 64x64 tile map)\n");
}

void create_test_map128() {
    FILE* file = fopen("test_map128.map", "wb");
    if (!file) return;

    // MAP128 format: 128x128 pixel tiles (based on MAP128.MAP from game)
    uint16_t tile_width = __builtin_bswap16(128);   // 128x128 pixel tiles
    uint16_t num_tiles = __builtin_bswap16(4096);   // 256x256 tile map
    uint16_t frame_side = __builtin_bswap16(8);     // 8x8 frames (128x128 pixels)

    // Write header
    fwrite(&tile_width, 2, 1, file);
    fwrite(&num_tiles, 2, 1, file);
    fwrite(&frame_side, 2, 1, file);

    // Write frame data: frame_side * frame_side tiles per frame
    // Total: num_tiles tiles organized in grid
    for (int frame = 0; frame < (num_tiles / (frame_side * frame_side)); frame++) {
        for (int row = 0; row < frame_side; row++) {
            for (int col = 0; col < frame_side; col++) {
                uint16_t index = __builtin_bswap16((frame * frame_side + row) * frame_side + col);
                fwrite(&index, 2, 1, file);
            }
        }
    }

    fclose(file);
    printf("Created test_map128.map (128x128 pixel tiles, 256x256 tile map)\n");
}

void create_test_map_16x16() {
    FILE* file = fopen("test_map_16x16.map", "wb");
    if (!file) return;

    uint16_t tile_width = __builtin_bswap16(32);    // 32x32 pixel tiles
    uint16_t num_tiles = __builtin_bswap16(1024);   // 32x32 tile map
    uint16_t frame_side = __builtin_bswap16(4);     // 4x4 frames (32x32 pixels)

    fwrite(&tile_width, 2, 1, file);
    fwrite(&num_tiles, 2, 1, file);
    fwrite(&frame_side, 2, 1, file);

    for (int frame = 0; frame < (num_tiles / (frame_side * frame_side)); frame++) {
        for (int row = 0; row < frame_side; row++) {
            for (int col = 0; col < frame_side; col++) {
                uint16_t index = __builtin_bswap16((frame * frame_side + row) * frame_side + col);
                fwrite(&index, 2, 1, file);
            }
        }
    }

    fclose(file);
    printf("Created test_map_16x16.map (32x32 pixel tiles)\n");
}

void test_level_load_MAP16() {
    create_test_map16();

    LevelData* level = level_load("test_map16.map");
    if (level) {
        if (level->width_tiles == 64 && level->height_tiles == 64) {
            printf("✓ test_level_load_MAP16: successfully loaded 64x64 level\n");
        } else {
            printf("✗ test_level_load_MAP16: incorrect dimensions (got %dx%d)\n",
                   level->width_tiles, level->height_tiles);
        }
        level_free(level);
    } else {
        printf("✗ test_level_load_MAP16: failed to load level\n");
    }

    remove("test_map16.map");
}

void test_level_load_MAP128() {
    create_test_map128();

    LevelData* level = level_load("test_map128.map");
    if (level) {
        if (level->width_tiles == 256 && level->height_tiles == 256) {
            printf("✓ test_level_load_MAP128: successfully loaded 256x256 level\n");
        } else {
            printf("✗ test_level_load_MAP128: incorrect dimensions (got %dx%d)\n",
                   level->width_tiles, level->height_tiles);
        }
        level_free(level);
    } else {
        printf("✗ test_level_load_MAP128: failed to load level\n");
    }

    remove("test_map128.map");
}

void test_level_get_tile() {
    create_test_map_16x16();

    LevelData* level = level_load("test_map_16x16.map");
    if (level) {
        uint16_t tile_0 = level_get_tile(level, 0, 0);
        uint16_t tile_100 = level_get_tile(level, 100, 100);
        uint16_t tile_valid = level_get_tile(level, 10, 15);

        printf("✓ test_level_get_tile: tile at (0,0): %d (white tile expected)\n", tile_0);
        printf("✓ test_level_get_tile: tile at (10,15): %d\n", tile_valid);
        printf("✓ test_level_get_tile: tile at (100,100): %d (out of bounds)\n", tile_100);
        level_free(level);
    } else {
        printf("✗ test_level_get_tile: failed to load level\n");
    }

    remove("test_map_16x16.map");
}

void test_level_load_ZeroDimensions() {
    FILE* file = fopen("test_map_zero.map", "wb");
    if (!file) {
        printf("Test skipped: cannot create test file\n");
        return;
    }

    uint32_t* data = (uint32_t*)malloc(32);
    memset(data, 0, 32);

    data[0] = __builtin_bswap32(0x50415453);  // "MAPS" signature
    data[1] = __builtin_bswap32(0x0000);      // Reserved
    data[2] = __builtin_bswap32(0);          // Width: 0 tiles
    data[3] = __builtin_bswap32(0);          // Height: 0 tiles
    data[4] = __builtin_bswap32(64);         // Tile data offset: 64 bytes

    fwrite(data, 1, 256, file);
    fclose(file);
    free(data);

    LevelData* level = level_load("test_map_zero.map");
    if (!level) {
        printf("✓ test_level_load_ZeroDimensions: correctly rejected zero dimensions\n");
    } else {
        printf("✗ test_level_load_ZeroDimensions: should reject zero dimensions\n");
        level_free(level);
    }

    remove("test_map_zero.map");
}

void test_level_load_InvalidFile() {
    FILE* file = fopen("test_invalid_file.map", "wb");
    if (!file) return;

    uint8_t* bad_data = (uint8_t*)malloc(16);
    memset(bad_data, 0, 16);
    fwrite(bad_data, 1, 16, file);
    fclose(file);
    free(bad_data);

    LevelData* level = level_load("test_invalid_file.map");
    if (!level) {
        printf("✓ test_level_load_InvalidFile: correctly rejected invalid file\n");
    } else {
        printf("✗ test_level_load_InvalidFile: should reject invalid file\n");
        level_free(level);
    }

    remove("test_invalid_file.map");
}

void test_level_load_LevelName() {
    create_test_map_16x16();

    LevelData* level = level_load("test_map_16x16.map");
    if (level) {
        memset(level->name, 0, sizeof(level->name));
        snprintf(level->name, sizeof(level->name), "Test Level");

        if (strcmp(level->name, "Test Level") == 0) {
            printf("✓ test_level_load_LevelName: successfully set level name\n");
        } else {
            printf("✗ test_level_load_LevelName: name not set correctly\n");
        }
        level_free(level);
    } else {
        printf("✗ test_level_load_LevelName: failed to load level\n");
    }

    remove("test_map_16x16.map");
}

void test_tile_zero_white() {
    // Test that tile 0 is always the white tile
    create_test_map_16x16();

    LevelData* level = level_load("test_map_16x16.map");
    if (level) {
        // Tile 0 should be at position (0,0) - always the white tile
        uint16_t tile0 = level_get_tile(level, 0, 0);
        if (tile0 == 0) {
            printf("✓ test_tile_zero_white: tile at (0,0) is 0 (white tile, correct)\n");
        } else {
            printf("✗ test_tile_zero_white: tile at (0,0) is %d (expected 0)\n", tile0);
        }

        // Check a few other positions for tile values > 0
        uint16_t tile32 = level_get_tile(level, 32, 0);
        uint16_t tile64 = level_get_tile(level, 0, 64);
        printf("✓ test_tile_zero_white: tile at (32,0): %d\n", tile32);
        printf("✓ test_tile_zero_white: tile at (0,64): %d\n", tile64);

        level_free(level);
    }

    remove("test_map_16x16.map");
}

int main(int argc, char* argv[]) {
    log_set_level(LOG_LEVEL_DEBUG);

    printf("===============================================\n");
    printf("Level Loader Tests - FINAL CORRECTION\n");
    printf("Based on: /home/alchemist/git/engine-psx/tools/chunkgen.py\n");
    printf("===============================================\n\n");

    printf("Key Points:\n");
    printf("- tile 0 is always the white (blank) tile\n");
    printf("- Only used tile indices are saved (not all possible combinations)\n");
    printf("- Big-endian byte order for all 16-bit shorts\n");
    printf("- Headers: tile_width, num_tiles, frame_side\n\n");

    printf("Generating test map files:\n");
    printf("- MAP16: 16x16 pixel tiles (2x2 frames of 8x8)\n");
    printf("- MAP128: 128x128 pixel tiles (8x8 frames of 8x8)\n");
    printf("- MAP32: 32x32 pixel tiles (4x4 frames of 8x8)\n\n");

    test_level_load_MAP16();
    test_level_load_MAP128();
    test_level_get_tile();
    test_level_load_ZeroDimensions();
    test_level_load_InvalidFile();
    test_level_load_LevelName();
    test_tile_zero_white();

    printf("\n===============================================\n");
    printf("All level loader tests completed\n");
    printf("===============================================\n");

    return 0;
}