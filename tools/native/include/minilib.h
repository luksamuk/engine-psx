/*
 * minilib.h - Minimal utility library for engine-psx tools
 * CSV, file I/O, string utilities
 */

#ifndef MINILIB_H
#define MINILIB_H

#include <stddef.h>
#include <stdbool.h>
#include "cJSON.h"

/* Dynamic array (vector) helper - simplified version */
typedef struct {
    char *data;
    size_t len;
    size_t cap;
} StringBuilder;

void sb_init(StringBuilder *sb);
void sb_free(StringBuilder *sb);
void sb_append(StringBuilder *sb, const char *str);
void sb_append_char(StringBuilder *sb, char c);
char* sb_get(StringBuilder *sb);

/* File utilities */
char* read_file_text(const char *filename);
unsigned char* read_file_binary(const char *filename, size_t *size);

/* CSV parsing - simple version */
typedef struct {
    int *data;
    int rows;
    int cols;
} IntMatrix;

IntMatrix* csv_parse_int_matrix(const char *filename);
void csv_free_matrix(IntMatrix *mat);

/* Simple JSON-like path parsing for cJSON
cJSON* cJSON_GetObjectPath(cJSON *root, const char *path);*/

/* Math utilities */
#define MIN(a,b) ((a)<(b)?(a):(b))
#define MAX(a,b) ((a)>(b)?(a):(b))
#define CLAMP(x,lo,hi) (MAX(lo,MIN(hi,x)))

/* Normalized vector */
void normalize_vector(float *v, int len);

#endif /* MINILIB_H */
