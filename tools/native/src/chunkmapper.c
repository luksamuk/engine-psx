/*
 * chunkmapper.c - Generate .tmx from Aseprite JSON tile mapping
 * Replaces chunkmapper.py
 * 
 * Input: JSON from Aseprite export_tilemap_psx
 * Output: tilemap128.tmx (in current directory)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cJSON.h"

#define CHUNK_SIZE 8
#define CHUNKS_PER_ROW 4

typedef struct {
    int *tiles;  // CHUNK_SIZE * CHUNK_SIZE tiles
    int width;
    int height;
} Chunk;

void print_usage(const char *prog) {
    fprintf(stderr, "Usage: %s <input.json>\n", prog);
    fprintf(stderr, "Output: tilemap128.tmx (in current directory)\n");
}

// Parse JSON and extract chunks
int parse_chunks(const char *filename, Chunk **chunks, int *num_chunks) {
    FILE *f = fopen(filename, "r");
    if (!f) {
        fprintf(stderr, "Error: cannot open %s\n", filename);
        return -1;
    }
    
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
        fprintf(stderr, "Error: JSON parse failed\n");
        return -1;
    }
    
    // Get layers[0].cels
    cJSON *layers = cJSON_GetObjectItem(root, "layers");
    if (!cJSON_IsArray(layers) || cJSON_GetArraySize(layers) == 0) {
        fprintf(stderr, "Error: no layers found\n");
        cJSON_Delete(root);
        return -1;
    }
    
    cJSON *layer0 = cJSON_GetArrayItem(layers, 0);
    cJSON *cels = cJSON_GetObjectItem(layer0, "cels");
    if (!cJSON_IsArray(cels)) {
        fprintf(stderr, "Error: no cels found\n");
        cJSON_Delete(root);
        return -1;
    }
    
    int count = cJSON_GetArraySize(cels);
    *chunks = calloc(count + 1, sizeof(Chunk));  // +1 for empty first chunk
    
    // First chunk is always empty
    (*chunks)[0].width = CHUNK_SIZE;
    (*chunks)[0].height = CHUNK_SIZE;
    (*chunks)[0].tiles = calloc(CHUNK_SIZE * CHUNK_SIZE, sizeof(int));
    memset((*chunks)[0].tiles, 0, CHUNK_SIZE * CHUNK_SIZE * sizeof(int));
    
    // Parse each cel as a chunk
    for (int i = 0; i < count; i++) {
        cJSON *cel = cJSON_GetArrayItem(cels, i);
        cJSON *tilemap = cJSON_GetObjectItem(cel, "tilemap");
        cJSON *tiles = cJSON_GetObjectItem(tilemap, "tiles");
        
        int cols = cJSON_GetObjectItem(tilemap, "width")->valueint;
        int rows = cJSON_GetObjectItem(tilemap, "height")->valueint;
        
        // Validate - must be at least 8x8
        if (cols < CHUNK_SIZE || rows < CHUNK_SIZE) {
            fprintf(stderr, "ERROR: chunk %d is smaller than %dx%d\n", i, CHUNK_SIZE, CHUNK_SIZE);
            cJSON_Delete(root);
            return -1;
        }
        
        Chunk *chunk = &(*chunks)[i + 1];
        chunk->width = cols;
        chunk->height = rows;
        chunk->tiles = malloc(cols * rows * sizeof(int));
        
        // Copy tiles (add 1 to each value, as per Python version)
        int tile_count = cJSON_GetArraySize(tiles);
        for (int j = 0; j < tile_count; j++) {
            cJSON *t = cJSON_GetArrayItem(tiles, j);
            chunk->tiles[j] = t->valueint + 1;
        }
    }
    
    *num_chunks = count + 1;
    cJSON_Delete(root);
    return 0;
}

// Add padding chunks to make total divisible by 4
int pad_chunks(Chunk **chunks, int *num_chunks) {
    int needed = CHUNKS_PER_ROW - (*num_chunks % CHUNKS_PER_ROW);
    if (needed == CHUNKS_PER_ROW) return 0;  // Already divisible
    
    int new_count = *num_chunks + needed;
    *chunks = realloc(*chunks, new_count * sizeof(Chunk));
    
    for (int i = *num_chunks; i < new_count; i++) {
        (*chunks)[i].width = CHUNK_SIZE;
        (*chunks)[i].height = CHUNK_SIZE;
        (*chunks)[i].tiles = calloc(CHUNK_SIZE * CHUNK_SIZE, sizeof(int));
        memset((*chunks)[i].tiles, 0, CHUNK_SIZE * CHUNK_SIZE * sizeof(int));
    }
    
    *num_chunks = new_count;
    return needed;
}

// Write TMX file
void write_tmx(const char *filename, Chunk *chunks, int num_chunks) {
    FILE *f = fopen(filename, "w");
    if (!f) {
        fprintf(stderr, "Error: cannot create %s\n", filename);
        return;
    }
    
    // Calculate dimensions
    int num_chunk_rows = num_chunks / CHUNKS_PER_ROW;
    int width_tiles = CHUNKS_PER_ROW * CHUNK_SIZE;  // 32
    int height_tiles = num_chunk_rows * CHUNK_SIZE;
    
    // Header
    fprintf(f, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
    fprintf(f, "<map version=\"1.10\" tiledversion=\"1.11.2\" "
               "orientation=\"orthogonal\" renderorder=\"right-down\" "
               "width=\"%d\" height=\"%d\" tilewidth=\"16\" tileheight=\"16\" "
               "infinite=\"0\" nextlayerid=\"2\" nextobjectid=\"1\">\n",
               width_tiles, height_tiles);
    fprintf(f, " <tileset firstgid=\"1\" source=\"tiles16.tsx\" />\n");
    fprintf(f, " <layer id=\"1\" name=\"solid\" width=\"%d\" height=\"%d\">\n",
               width_tiles, height_tiles);
    fprintf(f, "  <data encoding=\"csv\">\n");
    
    // Group chunks into rows of 4
    for (int row = 0; row < num_chunk_rows; row++) {
        Chunk *row_chunks[CHUNKS_PER_ROW];
        for (int c = 0; c < CHUNKS_PER_ROW; c++) {
            row_chunks[c] = &chunks[row * CHUNKS_PER_ROW + c];
        }
        
        // For each row within the chunks
        for (int y = 0; y < CHUNK_SIZE; y++) {
            int first = 1;
            // Concatenate 4 chunks horizontally
            for (int c = 0; c < CHUNKS_PER_ROW; c++) {
                Chunk *chunk = row_chunks[c];
                for (int x = 0; x < CHUNK_SIZE; x++) {
                    if (!first) fprintf(f, ",");
                    first = 0;
                    fprintf(f, "%d", chunk->tiles[y * CHUNK_SIZE + x]);
                }
            }
            fprintf(f, "\n");
        }
    }
    
    fprintf(f, "  </data>\n");
    fprintf(f, " </layer>\n");
    fprintf(f, "</map>\n");
    
    fclose(f);
    printf("Created %s (%dx%d tiles)\n", filename, width_tiles, height_tiles);
}

void free_chunks(Chunk *chunks, int num_chunks) {
    for (int i = 0; i < num_chunks; i++) {
        free(chunks[i].tiles);
    }
    free(chunks);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        print_usage(argv[0]);
        return 1;
    }
    
    const char *input = argv[1];
    
    Chunk *chunks = NULL;
    int num_chunks = 0;
    
    if (parse_chunks(input, &chunks, &num_chunks) != 0) {
        return 1;
    }
    
    printf("Parsed %d chunks\n", num_chunks);
    
    // Pad to make divisible by 4
    int padded = pad_chunks(&chunks, &num_chunks);
    if (padded > 0) {
        printf("Added %d padding chunks\n", padded);
    }
    
    // Generate TMX
    write_tmx("tilemap128.tmx", chunks, num_chunks);
    
    free_chunks(chunks, num_chunks);
    
    return 0;
}
