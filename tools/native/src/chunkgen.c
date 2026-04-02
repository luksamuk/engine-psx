/*
 * chunkgen.c - Convert 128x128 chunks from CSV to MAP binary format
 * Replaces chunkgen.py
 * 
 * Binary layout:
 * - tile_width (u16 BE) - always 128
 * - num_tiles (u16 BE) - grid_width * grid_height
 * - frame_dim (u16 BE) - always 8
 * - For each chunk:
 *   - For each of 8x8 tiles:
 *     - tile_index (u16 BE)
 *     - props (u8) - 0=solid, 1=oneway, 2=nocol, 4=front
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "psxtypes.h"

#define CHUNK_SIZE 8
#define TILE_WIDTH 128

// Layer type enum
enum LayerType {
    LAYER_SOLID = 0,
    LAYER_ONEWAY = 1,
    LAYER_NOCOL = 2,
    LAYER_FRONT = 3,
    MAX_LAYERS = 4
};

// Simple CSV matrix
typedef struct {
    int *data;
    int rows;
    int cols;
} Matrix;

void matrix_free(Matrix *m) {
    if (m) {
        free(m->data);
        m->data = NULL;
        m->rows = m->cols = 0;
    }
}

// Round up dimension to multiple of 8
int round_up_8(int d) {
    if (d % 8 > 0) {
        return d + (8 - (d % 8));
    }
    return d;
}

// Get chunk at position
void get_chunk(const Matrix *m, int cx, int cy, int *out) {
    int startx = cx * CHUNK_SIZE;
    int starty = cy * CHUNK_SIZE;
    
    for (int y = 0; y < CHUNK_SIZE; y++) {
        for (int x = 0; x < CHUNK_SIZE; x++) {
            int row = starty + y;
            int col = startx + x;
            int idx = y * CHUNK_SIZE + x;
            
            if (row < m->rows && col < m->cols) {
                out[idx] = m->data[row * m->cols + col];
            } else {
                out[idx] = -1;  // Empty
            }
        }
    }
}

// Parse CSV file
Matrix* csv_load(const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) return NULL;
    
    Matrix *m = (Matrix*)calloc(1, sizeof(Matrix));
    
    // First pass: count rows and max columns
    char line[16384];
    int cols = 0, max_cols = 0;
    int rows = 0;
    
    while (fgets(line, sizeof(line), f)) {
        cols = 0;
        char *p = line;
        while (*p) {
            // Skip whitespace
            while (*p == ' ' || *p == '\t' || *p == '\r') p++;
            if (*p == '\n' || *p == '\0') break;
            
            // Parse number
            if (*p == '-') {
                p++; // Skip minus
                while (isdigit((unsigned char)*p)) p++;
                cols++;
            } else if (isdigit((unsigned char)*p)) {
                while (isdigit((unsigned char)*p)) p++;
                cols++;
            } else if (*p == ',') {
                p++;
            } else {
                p++;
            }
        }
        if (cols > 0) {
            rows++;
            if (cols > max_cols) max_cols = cols;
        }
    }
    
    // Round up dimensions
    int final_cols = round_up_8(max_cols);
    int final_rows = round_up_8(rows);
    
    // Allocate and init with -1
    m->data = (int*)malloc(final_rows * final_cols * sizeof(int));
    for (int i = 0; i < final_rows * final_cols; i++) {
        m->data[i] = -1;
    }
    m->rows = final_rows;
    m->cols = final_cols;
    
    // Second pass: fill data
    rewind(f);
    int row = 0;
    
    while (fgets(line, sizeof(line), f) && row < final_rows) {
        int col = 0;
        char *p = line;
        
        while (*p && col < final_cols) {
            // Skip whitespace
            while (*p == ' ' || *p == '\t' || *p == '\r') p++;
            if (*p == '\n' || *p == '\0') break;
            if (*p == ',') {
                p++;
                continue;
            }
            
            // Parse number
            char *end;
            long val = strtol(p, &end, 10);
            if (end != p) {
                m->data[row * final_cols + col] = (int)val;
                col++;
                p = end;
            } else {
                p++;
            }
        }
        row++;
    }
    
    fclose(f);
    return m;
}

// Get grid dimensions
void get_grid_size(const Matrix *m, int *grid_w, int *grid_h) {
    *grid_w = m->cols / CHUNK_SIZE;
    *grid_h = m->rows / CHUNK_SIZE;
}

// Export binary
void export_binary(FILE *f, Matrix *layers[MAX_LAYERS]) {
    Matrix *solid = layers[LAYER_SOLID];
    int grid_w, grid_h;
    get_grid_size(solid, &grid_w, &grid_h);
    int num_chunks = grid_w * grid_h;
    
    // Header
    write_u16_be(f, TILE_WIDTH);
    write_u16_be(f, (u16)num_chunks);
    write_u16_be(f, CHUNK_SIZE);
    
    printf("Grid: %dx%d = %d chunks\n", grid_w, grid_h, num_chunks);
    
    // For each chunk
    for (int cy = 0; cy < grid_h; cy++) {
        for (int cx = 0; cx < grid_w; cx++) {
            int solid_chunk[CHUNK_SIZE * CHUNK_SIZE];
            int oneway_chunk[CHUNK_SIZE * CHUNK_SIZE] = {0};
            int nocol_chunk[CHUNK_SIZE * CHUNK_SIZE] = {0};
            int front_chunk[CHUNK_SIZE * CHUNK_SIZE] = {0};
            
            get_chunk(solid, cx, cy, solid_chunk);
            
            if (layers[LAYER_ONEWAY]) {
                get_chunk(layers[LAYER_ONEWAY], cx, cy, oneway_chunk);
            }
            if (layers[LAYER_NOCOL]) {
                get_chunk(layers[LAYER_NOCOL], cx, cy, nocol_chunk);
            }
            if (layers[LAYER_FRONT]) {
                get_chunk(layers[LAYER_FRONT], cx, cy, front_chunk);
            }
            
            // For each tile in chunk (row-major)
            for (int py = 0; py < CHUNK_SIZE; py++) {
                for (int px = 0; px < CHUNK_SIZE; px++) {
                    int idx = py * CHUNK_SIZE + px;
                    int tile_index = solid_chunk[idx];
                    u8 props = 0;
                    
                    // Check alternative layers if solid is empty
                    if (tile_index <= 0 && layers[LAYER_ONEWAY] && oneway_chunk[idx] > 0) {
                        tile_index = oneway_chunk[idx];
                        props = 1;
                    }
                    if (tile_index <= 0 && layers[LAYER_NOCOL] && nocol_chunk[idx] > 0) {
                        tile_index = nocol_chunk[idx];
                        props = 2;
                    }
                    if (tile_index <= 0 && layers[LAYER_FRONT] && front_chunk[idx] > 0) {
                        tile_index = front_chunk[idx];
                        props = 4;
                    }
                    
                    write_u16_be(f, (u16)(tile_index > 0 ? tile_index : 0));
                    write_u8(f, props);
                }
            }
        }
    }
}

void print_usage(const char *prog) {
    fprintf(stderr, "Usage: %s <input> <output.MAP>\n", prog);
    fprintf(stderr, "  input: either a single .cnk file or base path\n");
    fprintf(stderr, "         (will look for _solid.cnk, _oneway.cnk, etc.)\n");
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        print_usage(argv[0]);
        return 1;
    }
    
    const char *input = argv[1];
    const char *output = argv[2];
    
    Matrix *layers[MAX_LAYERS] = {0};
    
    // Check if single file or base path
    if (strstr(input, ".cnk")) {
        // Single mode
        layers[LAYER_SOLID] = csv_load(input);
        if (!layers[LAYER_SOLID]) {
            fprintf(stderr, "Error: cannot load %s\n", input);
            return 1;
        }
        printf("Loaded %s: %dx%d\n", input, layers[LAYER_SOLID]->cols, layers[LAYER_SOLID]->rows);
    } else {
        // Multi-layer mode
        char path[1024];
        
        // Solid (required)
        snprintf(path, sizeof(path), "%s_solid.cnk", input);
        layers[LAYER_SOLID] = csv_load(path);
        if (!layers[LAYER_SOLID]) {
            fprintf(stderr, "Error: cannot load %s\n", path);
            return 1;
        }
        printf("Loaded %s: %dx%d\n", path, layers[LAYER_SOLID]->cols, layers[LAYER_SOLID]->rows);
        
        // Optional layers
        snprintf(path, sizeof(path), "%s_oneway.cnk", input);
        FILE *test = fopen(path, "r");
        if (test) {
            fclose(test);
            layers[LAYER_ONEWAY] = csv_load(path);
            if (layers[LAYER_ONEWAY]) {
                printf("Loaded %s: %dx%d\n", path, layers[LAYER_ONEWAY]->cols, layers[LAYER_ONEWAY]->rows);
            }
        }
        
        snprintf(path, sizeof(path), "%s_none.cnk", input);
        test = fopen(path, "r");
        if (test) {
            fclose(test);
            layers[LAYER_NOCOL] = csv_load(path);
            if (layers[LAYER_NOCOL]) {
                printf("Loaded %s: %dx%d\n", path, layers[LAYER_NOCOL]->cols, layers[LAYER_NOCOL]->rows);
            }
        }
        
        snprintf(path, sizeof(path), "%s_front.cnk", input);
        test = fopen(path, "r");
        if (test) {
            fclose(test);
            layers[LAYER_FRONT] = csv_load(path);
            if (layers[LAYER_FRONT]) {
                printf("Loaded %s: %dx%d\n", path, layers[LAYER_FRONT]->cols, layers[LAYER_FRONT]->rows);
            }
        }
    }
    
    FILE *f = fopen(output, "wb");
    if (!f) {
        fprintf(stderr, "Error: cannot create %s\n", output);
        goto cleanup;
    }
    
    export_binary(f, layers);
    fclose(f);
    
    printf("Successfully wrote %s\n", output);
    
cleanup:
    for (int i = 0; i < MAX_LAYERS; i++) {
        matrix_free(layers[i]);
    }
    
    return 0;
}
