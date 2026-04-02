/*
 * framepacker.c - Pack Aseprite JSON frames to binary CHARA format
 * Replaces framepacker.py
 * 
 * Binary layout:
 * - sprite_width (u16 BE)
 * - sprite_height (u16 BE)
 * - num_frames (u16 BE)
 * - num_animations (u16 BE)
 * Per frame:
 *   - x (u8), y (u8)
 *   - cols (u8), rows (u8)
 *   - width (u16 BE), height (u16 BE)
 *   - tiles[] (u16 BE * cols * rows)
 * Per animation:
 *   - name[16] (padded with 0s)
 *   - start (u8), end (u8)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include "cJSON.h"
#include "psxtypes.h"

#define MAX_NAME_LEN 16
#define MAX_TILES 2048

typedef struct {
    u8 x, y;
    u8 cols, rows;
    u16 width, height;
    u16 *tiles;
    int num_tiles;
} FrameData;

typedef struct {
    char name[MAX_NAME_LEN + 1];
    u8 start;
    u8 end;
} AnimationData;

typedef struct {
    u16 sprite_width;
    u16 sprite_height;
    int num_frames;
    FrameData frames[MAX_TILES];
    int num_animations;
    AnimationData animations[256];
} SpriteData;

void print_usage(const char *prog) {
    fprintf(stderr, "Usage: %s [--tilemap] <input.json> <output.CHAR>\n", prog);
    fprintf(stderr, "  --tilemap  Output level tilemap format instead of sprite\n");
}

// Parse name: uppercase, replace spaces with nothing, pad to 16 chars
void format_name(const char *input, char *output) {
    int j = 0;
    for (int i = 0; input[i] && j < MAX_NAME_LEN; i++) {
        if (input[i] != ' ') {
            output[j++] = (char)toupper((unsigned char)input[i]);
        }
    }
    while (j <= MAX_NAME_LEN) output[j++] = '\0';
}

int compare_frames(const void *a, const void *b) {
    cJSON *fa = *(cJSON **)a;
    cJSON *fb = *(cJSON **)b;
    cJSON *frame_a = cJSON_GetObjectItem(fa, "frame");
    cJSON *frame_b = cJSON_GetObjectItem(fb, "frame");
    int fa_i = frame_a ? frame_a->valueint : 0;
    int fb_i = frame_b ? frame_b->valueint : 0;
    return fa_i - fb_i;
}

int parse_sprite(const char *filename, SpriteData *sprite) {
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
        fprintf(stderr, "Error: JSON parse failed: %s\n", cJSON_GetErrorPtr());
        return -1;
    }
    
    // Parse basic info
    cJSON *width = cJSON_GetObjectItem(root, "width");
    cJSON *height = cJSON_GetObjectItem(root, "height");
    sprite->sprite_width = width ? (u16)width->valueint : 0;
    sprite->sprite_height = height ? (u16)height->valueint : 0;
    
    // Parse layers -> cels -> frames
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
    
    // Sort cels by frame number
    int num_cels = cJSON_GetArraySize(cels);
    cJSON **cel_array = malloc(num_cels * sizeof(cJSON *));
    for (int i = 0; i < num_cels; i++) {
        cel_array[i] = cJSON_GetArrayItem(cels, i);
    }
    qsort(cel_array, num_cels, sizeof(cJSON *), compare_frames);
    
    // Parse frames
    sprite->num_frames = num_cels;
    for (int i = 0; i < num_cels; i++) {
        cJSON *cel = cel_array[i];
        cJSON *tilemap = cJSON_GetObjectItem(cel, "tilemap");
        cJSON *bounds = cJSON_GetObjectItem(cel, "bounds");
        
        FrameData *frame = &sprite->frames[i];
        frame->x = (u8)cJSON_GetObjectItem(bounds, "x")->valueint;
        frame->y = (u8)cJSON_GetObjectItem(bounds, "y")->valueint;
        frame->cols = (u8)cJSON_GetObjectItem(tilemap, "width")->valueint;
        frame->rows = (u8)cJSON_GetObjectItem(tilemap, "height")->valueint;
        frame->width = (u16)cJSON_GetObjectItem(bounds, "width")->valueint;
        frame->height = (u16)cJSON_GetObjectItem(bounds, "height")->valueint;
        
        cJSON *tiles = cJSON_GetObjectItem(tilemap, "tiles");
        frame->num_tiles = cJSON_GetArraySize(tiles);
        frame->tiles = malloc(frame->num_tiles * sizeof(u16));
        
        for (int j = 0; j < frame->num_tiles; j++) {
            cJSON *t = cJSON_GetArrayItem(tiles, j);
            frame->tiles[j] = (u16)t->valueint;
        }
    }
    free(cel_array);
    
    // Parse animation tags
    cJSON *tags = cJSON_GetObjectItem(root, "tags");
    if (cJSON_IsArray(tags)) {
        int num_tags = cJSON_GetArraySize(tags);
        sprite->num_animations = num_tags;
        
        for (int i = 0; i < num_tags; i++) {
            cJSON *tag = cJSON_GetArrayItem(tags, i);
            cJSON *name = cJSON_GetObjectItem(tag, "name");
            cJSON *from = cJSON_GetObjectItem(tag, "from");
            cJSON *to = cJSON_GetObjectItem(tag, "to");
            
            format_name(name->valuestring, sprite->animations[i].name);
            sprite->animations[i].start = (u8)from->valueint;
            sprite->animations[i].end = (u8)to->valueint;
        }
    }
    
    cJSON_Delete(root);
    return 0;
}

void free_sprite(SpriteData *sprite) {
    for (int i = 0; i < sprite->num_frames; i++) {
        free(sprite->frames[i].tiles);
    }
}

int write_binary_sprite(const char *filename, const SpriteData *sprite) {
    FILE *f = fopen(filename, "wb");
    if (!f) {
        fprintf(stderr, "Error: cannot create %s\n", filename);
        return -1;
    }
    
    // Header
    write_u16_be(f, sprite->sprite_width);
    write_u16_be(f, sprite->sprite_height);
    write_u16_be(f, (u16)sprite->num_frames);
    write_u16_be(f, (u16)sprite->num_animations);
    
    // Frames
    for (int i = 0; i < sprite->num_frames; i++) {
        const FrameData *frame = &sprite->frames[i];
        write_u8(f, frame->x);
        write_u8(f, frame->y);
        write_u8(f, frame->cols);
        write_u8(f, frame->rows);
        write_u16_be(f, frame->width);
        write_u16_be(f, frame->height);
        
        for (int j = 0; j < frame->num_tiles; j++) {
            write_u16_be(f, frame->tiles[j]);
        }
    }
    
    // Animations
    for (int i = 0; i < sprite->num_animations; i++) {
        const AnimationData *anim = &sprite->animations[i];
        for (int j = 0; j < MAX_NAME_LEN; j++) {
            u8 c = (u8)((j < (int)strlen(anim->name) && anim->name[j]) ? anim->name[j] : 0);
            write_u8(f, c);
        }
        write_u8(f, anim->start);
        write_u8(f, anim->end);
    }
    
    fclose(f);
    return 0;
}

int write_binary_level(const char *filename, const SpriteData *sprite) {
    FILE *f = fopen(filename, "wb");
    if (!f) {
        fprintf(stderr, "Error: cannot create %s\n", filename);
        return -1;
    }
    
    // Simpler format for levels
    write_u16_be(f, sprite->sprite_width);
    write_u16_be(f, (u16)sprite->num_frames);
    
    if (sprite->num_frames > 0) {
        write_u16_be(f, sprite->frames[0].cols);
        
        // Write all tiles from all frames
        for (int i = 0; i < sprite->num_frames; i++) {
            const FrameData *frame = &sprite->frames[i];
            for (int j = 0; j < frame->num_tiles; j++) {
                write_u16_be(f, frame->tiles[j]);
            }
        }
    } else {
        write_u16_be(f, 0);
    }
    
    fclose(f);
    return 0;
}

int main(int argc, char *argv[]) {
    const char *input = NULL;
    const char *output = NULL;
    int is_level = 0;
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--tilemap") == 0) {
            is_level = 1;
        } else if (!input) {
            input = argv[i];
        } else if (!output) {
            output = argv[i];
        }
    }
    
    if (!input || !output) {
        print_usage(argv[0]);
        return 1;
    }
    
    SpriteData sprite = {0};
    
    if (parse_sprite(input, &sprite) != 0) {
        return 1;
    }
    
    int ret;
    if (is_level) {
        ret = write_binary_level(output, &sprite);
    } else {
        ret = write_binary_sprite(output, &sprite);
    }
    
    free_sprite(&sprite);
    
    if (ret != 0) {
        return 1;
    }
    
    printf("Successfully created %s\n", output);
    return 0;
}
