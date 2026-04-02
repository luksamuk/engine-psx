/*
 * minilib.c - Implementation
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "minilib.h"

/* String Builder */
void sb_init(StringBuilder *sb) {
    sb->data = NULL;
    sb->len = 0;
    sb->cap = 0;
}

void sb_free(StringBuilder *sb) {
    free(sb->data);
    sb->data = NULL;
    sb->len = sb->cap = 0;
}

void sb_ensure_cap(StringBuilder *sb, size_t needed) {
    if (needed > sb->cap) {
        size_t new_cap = sb->cap ? sb->cap * 2 : 64;
        while (new_cap < needed) new_cap *= 2;
        sb->data = realloc(sb->data, new_cap);
        sb->cap = new_cap;
    }
}

void sb_append(StringBuilder *sb, const char *str) {
    size_t len = strlen(str);
    sb_ensure_cap(sb, sb->len + len + 1);
    memcpy(sb->data + sb->len, str, len);
    sb->len += len;
    sb->data[sb->len] = '\0';
}

void sb_append_char(StringBuilder *sb, char c) {
    sb_ensure_cap(sb, sb->len + 2);
    sb->data[sb->len] = c;
    sb->len++;
    sb->data[sb->len] = '\0';
}

char* sb_get(StringBuilder *sb) {
    return sb->data ? sb->data : "";
}

/* File utilities */
char* read_file_text(const char *filename) {
    FILE *f = fopen(filename, "rb");
    if (!f) return NULL;
    
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    char *buf = malloc(size + 1);
    if (!buf) {
        fclose(f);
        return NULL;
    }
    
    fread(buf, 1, size, f);
    buf[size] = '\0';
    fclose(f);
    return buf;
}

unsigned char* read_file_binary(const char *filename, size_t *size) {
    FILE *f = fopen(filename, "rb");
    if (!f) return NULL;
    
    fseek(f, 0, SEEK_END);
    *size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    unsigned char *buf = malloc(*size);
    if (!buf) {
        fclose(f);
        return NULL;
    }
    
    fread(buf, 1, *size, f);
    fclose(f);
    return buf;
}

/* CSV parsing - simple version */
IntMatrix* csv_parse_int_matrix(const char *filename) {
    char *text = read_file_text(filename);
    if (!text) return NULL;
    
    IntMatrix *mat = calloc(1, sizeof(IntMatrix));
    
    // First pass: count rows and columns
    int cols = 0, max_cols = 0;
    int rows = 1;  // At least one row if file not empty
    bool in_value = false;
    
    for (char *p = text; *p; p++) {
        if (*p == ',') {
            if (in_value) cols++;
            in_value = false;
        } else if (*p == '\n') {
            if (in_value) cols++;
            if (cols > max_cols) max_cols = cols;
            cols = 0;
            in_value = false;
            rows++;
        } else if (*p != '\r' && *p != ' ' && *p != '\t') {
            in_value = true;
        }
    }
    if (in_value) cols++;
    if (cols > max_cols) max_cols = cols;
    
    // Adjust for trailing newline
    int len = strlen(text);
    if (len > 0 && (text[len-1] == '\n' || text[len-1] == '\r')) rows--;
    
    mat->rows = rows;
    mat->cols = max_cols;
    mat->data = malloc(rows * max_cols * sizeof(int));
    memset(mat->data, -1, rows * max_cols * sizeof(int));
    
    // Second pass: parse values
    int row = 0, col = 0;
    char *p = text;
    while (*p && row < rows) {
        // Skip whitespace
        while (*p == ' ' || *p == '\t') p++;
        
        if (*p == '\n' || *p == '\r') {
            row++;
            col = 0;
            p++;
            if (*p == '\n' || *p == '\r') p++;
            continue;
        }
        
        if (*p == ',') {
            col++;
            p++;
            continue;
        }
        
        // Parse number
        char *end;
        int val = strtol(p, &end, 10);
        if (end != p) {
            if (row < rows && col < max_cols) {
                mat->data[row * max_cols + col] = val;
            }
            p = end;
            col++;
        } else {
            p++;
        }
    }
    
    free(text);
    return mat;
}

void csv_free_matrix(IntMatrix *mat) {
    if (mat) {
        free(mat->data);
        free(mat);
    }
}

/* Normalize vector (float) */
void normalize_vector(float *v, int len) {
    float sum = 0;
    for (int i = 0; i < len; i++) sum += v[i] * v[i];
    float norm = sqrt(sum);
    if (norm > 0) {
        for (int i = 0; i < len; i++) v[i] /= norm;
    }
}

/* cJSON path helper - currently not used
cJSON* cJSON_GetObjectPath(cJSON *root, const char *path) {
    cJSON *current = root;
    char *buf = strdup(path);
    char *token = strtok(buf, ".");
    
    while (token && current) {
        current = cJSON_GetObjectItemCaseSensitive(current, token);
        token = strtok(NULL, ".");
    }
    
    free(buf);
    return current;
}*/
