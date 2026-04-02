/*
 * toml.h - Minimal TOML parser interface
 */

#ifndef TOML_H
#define TOML_H

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Entry types for internal use */
typedef enum entry_type {
    ENTRY_TABLE,
    ENTRY_ARRAY,
    ENTRY_STRING,
    ENTRY_INT,
    ENTRY_DOUBLE,
    ENTRY_BOOL
} entry_type;

/* Forward declarations */
typedef struct toml_table toml_table_t;
typedef struct toml_array toml_array_t;

/* Entry structure */
typedef struct entry {
    struct entry *next;
    char *key;
    entry_type type;
    union {
        toml_table_t *tab;
        toml_array_t *arr;
        char *s;
        int64_t i;
        double d;
        int b;
    } u;
} entry;

/* Table structure */
struct toml_table {
    char *name;
    entry *first;
    entry *last;
    toml_table_t *next;
};

/* Array structure (minimal) */
struct toml_array {
    int nelem;
    void *data;
};

/* Datum for return values */
typedef struct {
    int ok;
    union {
        int64_t  i;
        double   d;
        int      b;
        char    *s;
    } u;
} toml_datum_t;

/* Parse functions */
toml_table_t *toml_parse(char *toml, char *errbuf, int errbufsz);
toml_table_t *toml_parse_file(FILE *fp, char *errbuf, int errbufsz);

/* Free function */
void toml_free(toml_table_t *tab);

/* Key operations */
int toml_key_exists(toml_table_t *tab, const char *key);
const char *toml_key_in(toml_table_t *tab, int keyidx);

/* Access by key */
toml_table_t *toml_table_in(toml_table_t *tab, const char *key);
toml_array_t *toml_array_in(toml_table_t *tab, const char *key);
toml_datum_t toml_string_in(toml_table_t *tab, const char *key);
toml_datum_t toml_int_in(toml_table_t *tab, const char *key);
toml_datum_t toml_double_in(toml_table_t *tab, const char *key);
toml_datum_t toml_bool_in(toml_table_t *tab, const char *key);

/* Array access (minimal implementation) */
toml_datum_t toml_int_at(toml_array_t *arr, int idx);
toml_datum_t toml_double_at(toml_array_t *arr, int idx);

#ifdef __cplusplus
}
#endif

#endif /* TOML_H */
