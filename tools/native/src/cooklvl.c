/*
 * cooklvl.c - Convert level JSON to binary PSX format
 * Replaces cooklvl.py
 * 
 * Binary layout:
 * - num_layers (u8)
 * - _unused (u8)
 * Per layer:
 *   - width (u8)
 *   - height (u8)
 *   - tiles[] (u16 BE)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cJSON.h"
#include "psxtypes.h"

#define MAX_LAYERS 3
#define MAX_WIDTH 256
#define MAX_HEIGHT 256

typedef struct {
    u8 width;
    u8 height;
    u16 *tiles;
} LayerData;

typedef struct {
    u8 num_layers;
    LayerData layers[MAX_LAYERS];
} LevelData;

void print_usage(const char *prog) {
    fprintf(stderr, "Usage: %s <input.json> <output.LVL>\n", prog);
}

int parse_level(const char *filename, LevelData *level) {
    FILE *f = fopen(filename, "r");
    if (!f) {
        fprintf(stderr, "Error: cannot open %s\n", filename);
        return -1;
    }
    
    // Read entire file
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buf = malloc(size + 1);
    fread(buf, 1, size, f);
    buf[size] = '\0';
    fclose(f);
    
    cJSON *root = cJSON_Parse(buf);
    free(buf);
    
    if (!root) {
        fprintf(stderr, "Error: JSON parse failed: %s\n", cJSON_GetErrorPtr());
        return -1;
    }
    
    // Get num_layers
    cJSON *num_layers = cJSON_GetObjectItem(root, "num_layers");
    if (!cJSON_IsNumber(num_layers)) {
        fprintf(stderr, "Error: missing or invalid num_layers\n");
        cJSON_Delete(root);
        return -1;
    }
    level->num_layers = (u8)num_layers->valueint;
    printf("Number of level layers: %d\n", level->num_layers);
    
    if (level->num_layers > MAX_LAYERS) {
        fprintf(stderr, "Error: too many layers (max %d)\n", MAX_LAYERS);
        cJSON_Delete(root);
        return -1;
    }
    
    // Parse layer_data
    cJSON *layer_data = cJSON_GetObjectItem(root, "layer_data");
    if (!cJSON_IsArray(layer_data)) {
        fprintf(stderr, "Error: missing layer_data array\n");
        cJSON_Delete(root);
        return -1;
    }
    
    int layer_count = cJSON_GetArraySize(layer_data);
    for (int i = 0; i < layer_count && i < MAX_LAYERS; i++) {
        cJSON *layer = cJSON_GetArrayItem(layer_data, i);
        cJSON *width = cJSON_GetObjectItem(layer, "width");
        cJSON *height = cJSON_GetObjectItem(layer, "height");
        cJSON *tiles = cJSON_GetObjectItem(layer, "tiles");
        
        if (!cJSON_IsNumber(width) || !cJSON_IsNumber(height) || !cJSON_IsArray(tiles)) {
            fprintf(stderr, "Error: invalid layer %d format\n", i);
            continue;
        }
        
        level->layers[i].width = (u8)width->valueint;
        level->layers[i].height = (u8)height->valueint;
        
        if (level->layers[i].width == 0 || level->layers[i].width >= MAX_WIDTH ||
            level->layers[i].height == 0 || level->layers[i].height >= MAX_HEIGHT) {
            fprintf(stderr, "Error: layer %d exceeds max dimensions\n", i);
            cJSON_Delete(root);
            return -1;
        }
        
        int tile_count = cJSON_GetArraySize(tiles);
        level->layers[i].tiles = malloc(tile_count * sizeof(u16));
        
        for (int j = 0; j < tile_count; j++) {
            cJSON *tile = cJSON_GetArrayItem(tiles, j);
            level->layers[i].tiles[j] = (u16)tile->valueint;
        }
    }
    
    cJSON_Delete(root);
    return 0;
}

int write_level(const char *filename, const LevelData *level) {
    FILE *f = fopen(filename, "wb");
    if (!f) {
        fprintf(stderr, "Error: cannot create %s\n", filename);
        return -1;
    }
    
    // Write header
    write_u8(f, level->num_layers);
    write_u8(f, 0); // _unused
    
    // Write each layer
    for (int i = 0; i < level->num_layers; i++) {
        write_u8(f, level->layers[i].width);
        write_u8(f, level->layers[i].height);
        
        int tile_count = level->layers[i].width * level->layers[i].height;
        for (int j = 0; j < tile_count; j++) {
            write_u16_be(f, level->layers[i].tiles[j]);
        }
        
        printf("Layer %d: %dx%d = %d tiles\n", i, 
               level->layers[i].width, level->layers[i].height, tile_count);
    }
    
    fclose(f);
    return 0;
}

void free_level(LevelData *level) {
    for (int i = 0; i < MAX_LAYERS; i++) {
        free(level->layers[i].tiles);
        level->layers[i].tiles = NULL;
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        print_usage(argv[0]);
        return 1;
    }
    
    const char *input = argv[1];
    const char *output = argv[2];
    
    LevelData level = {0};
    
    if (parse_level(input, &level) != 0) {
        return 1;
    }
    
    if (write_level(output, &level) != 0) {
        free_level(&level);
        return 1;
    }
    
    free_level(&level);
    printf("Successfully wrote %s\n", output);
    
    return 0;
}
