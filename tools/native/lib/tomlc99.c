/*
 * tomlc99.c - Minimal TOML implementation for buildprl
 * Supports: tables, key=value (int, float, bool), comments
 */

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include "toml.h"

typedef struct {
    char *data;
    char *p;
} toml_parser;

static void skip_whitespace_and_comments(toml_parser *p) {
    while (1) {
        // Skip whitespace
        while (*p->p && isspace((unsigned char)*p->p)) p->p++;
        
        // Skip comments
        if (*p->p == '#') {
            while (*p->p && *p->p != '\n') p->p++;
            continue;
        }
        break;
    }
}

static char *parse_key(toml_parser *p) {
    skip_whitespace_and_comments(p);
    
    char *start = p->p;
    while (*p->p && (isalnum((unsigned char)*p->p) || *p->p == '_' || *p->p == '-')) {
        p->p++;
    }
    
    int len = p->p - start;
    if (len == 0) return NULL;
    
    char *key = malloc(len + 1);
    memcpy(key, start, len);
    key[len] = '\0';
    return key;
}

// Parse value - returns type in *type
static int parse_value(toml_parser *p, toml_datum_t *out, entry_type *type) {
    skip_whitespace_and_comments(p);
    
    if (!*p->p) return -1;
    
    // Try bool first (check before numbers)
    if (strncmp(p->p, "true", 4) == 0 && !isalnum((unsigned char)p->p[4])) {
        out->ok = 1;
        out->u.b = 1;
        *type = ENTRY_BOOL;
        p->p += 4;
        return 0;
    }
    if (strncmp(p->p, "false", 5) == 0 && !isalnum((unsigned char)p->p[5])) {
        out->ok = 1;
        out->u.b = 0;
        *type = ENTRY_BOOL;
        p->p += 5;
        return 0;
    }
    
    // Try number
    char *end;
    
    // Check if looks like integer (digits only, no dot, no e)
    char *check = p->p;
    if (*check == '-') check++;
    
    // Check if all digits
    int all_digits = 1;
    char *digit_start = check;
    while (isdigit((unsigned char)*check)) check++;
    if (check == digit_start) all_digits = 0;  // No digits at all
    
    // If followed by dot or e, it's a float
    if (all_digits && (*check == '.' || *check == 'e' || *check == 'E')) {
        // Parse as float
        double dval = strtod(p->p, &end);
        if (end != p->p) {
            out->ok = 1;
            out->u.d = dval;
            *type = ENTRY_DOUBLE;
            p->p = end;
            return 0;
        }
    }
    
    // Parse as integer
    if (all_digits) {
        long long ival = strtoll(p->p, &end, 10);
        if (end != p->p) {
            out->ok = 1;
            out->u.i = ival;
            *type = ENTRY_INT;
            p->p = end;
            return 0;
        }
    }
    
    // Try float starting with . (like .5)
    if (*p->p == '.') {
        double dval = strtod(p->p, &end);
        if (end != p->p) {
            out->ok = 1;
            out->u.d = dval;
            *type = ENTRY_DOUBLE;
            p->p = end;
            return 0;
        }
    }
    
    return -1;
}

static toml_table_t *new_table(void) {
    return calloc(1, sizeof(toml_table_t));
}

toml_table_t *toml_parse(char *toml, char *errbuf, int errbufsz) {
    (void)errbuf;
    (void)errbufsz;
    
    toml_parser p = { .data = toml, .p = toml };
    toml_table_t *root = new_table();
    toml_table_t *current = root;
    
    while (1) {
        skip_whitespace_and_comments(&p);
        if (!*p.p) break;
        
        // Check for table header [section]
        if (*p.p == '[') {
            p.p++; // skip [
            char *section = parse_key(&p);
            if (!section) break;
            
            // Skip to ]
            while (*p.p && *p.p != ']' && *p.p != '\n') p.p++;
            if (*p.p == ']') p.p++;
            
            // Create new table
            toml_table_t *new_tab = new_table();
            new_tab->name = section;
            
            // Add to root's chain
            toml_table_t **pp = &root->next;
            while (*pp) pp = &(*pp)->next;
            *pp = new_tab;
            current = new_tab;
            continue;
        }
        
        // Parse key=value
        char *key = parse_key(&p);
        if (!key) break;
        
        skip_whitespace_and_comments(&p);
        if (*p.p != '=') {
            free(key);
            break;
        }
        p.p++; // skip =
        
        toml_datum_t val;
        entry_type type;
        if (parse_value(&p, &val, &type) < 0) {
            free(key);
            continue;
        }
        
        // Store in current table
        entry *e = calloc(1, sizeof(entry));
        e->key = key;
        e->type = type;
        
        if (type == ENTRY_INT) e->u.i = val.u.i;
        else if (type == ENTRY_DOUBLE) e->u.d = val.u.d;
        else if (type == ENTRY_BOOL) e->u.b = val.u.b;
        
        entry **ep = &current->first;
        while (*ep) ep = &(*ep)->next;
        *ep = e;
    }
    
    return root;
}

toml_table_t *toml_parse_file(FILE *fp, char *errbuf, int errbufsz) {
    fseek(fp, 0, SEEK_END);
    long len = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    char *buf = malloc(len + 1);
    if (!buf) return NULL;
    fread(buf, 1, len, fp);
    buf[len] = '\0';
    
    toml_table_t *t = toml_parse(buf, errbuf, errbufsz);
    // Note: buf is not freed; attached to parser
    return t;
}

void toml_free(toml_table_t *tab) {
    while (tab) {
        toml_table_t *next = tab->next;
        entry *e = tab->first;
        while (e) {
            entry *en = e->next;
            free(e->key);
            free(e);
            e = en;
        }
        free(tab->name);
        free(tab);
        tab = next;
    }
}

int toml_key_exists(toml_table_t *tab, const char *key) {
    for (entry *e = tab->first; e; e = e->next) {
        if (e->key && strcmp(e->key, key) == 0) return 1;
    }
    return 0;
}

const char *toml_key_in(toml_table_t *tab, int idx) {
    entry *e = tab->first;
    while (idx-- > 0 && e) e = e->next;
    return e ? e->key : NULL;
}

entry *find_entry(toml_table_t *tab, const char *key) {
    for (entry *e = tab->first; e; e = e->next) {
        if (e->key && strcmp(e->key, key) == 0) return e;
    }
    return NULL;
}

toml_table_t *toml_table_in(toml_table_t *tab, const char *key) {
    entry *e = find_entry(tab, key);
    if (e && e->type == ENTRY_TABLE) {
        return e->u.tab;
    }
    return NULL;
}

toml_array_t *toml_array_in(toml_table_t *tab, const char *key) {
    (void)tab;
    (void)key;
    return NULL;
}

toml_datum_t toml_string_in(toml_table_t *tab, const char *key) {
    toml_datum_t d = {0};
    entry *e = find_entry(tab, key);
    if (e && e->type == ENTRY_STRING) {
        d.ok = 1;
        d.u.s = e->u.s;
    }
    return d;
}

toml_datum_t toml_int_in(toml_table_t *tab, const char *key) {
    toml_datum_t d = {0};
    entry *e = find_entry(tab, key);
    if (e) {
        if (e->type == ENTRY_INT) {
            d.ok = 1;
            d.u.i = e->u.i;
        } else if (e->type == ENTRY_DOUBLE) {
            d.ok = 1;
            d.u.i = (int64_t)e->u.d;
        }
    }
    return d;
}

toml_datum_t toml_double_in(toml_table_t *tab, const char *key) {
    toml_datum_t d = {0};
    entry *e = find_entry(tab, key);
    if (e) {
        if (e->type == ENTRY_DOUBLE) {
            d.ok = 1;
            d.u.d = e->u.d;
        } else if (e->type == ENTRY_INT) {
            d.ok = 1;
            d.u.d = (double)e->u.i;
        }
    }
    return d;
}

toml_datum_t toml_bool_in(toml_table_t *tab, const char *key) {
    toml_datum_t d = {0};
    entry *e = find_entry(tab, key);
    if (e && e->type == ENTRY_BOOL) {
        d.ok = 1;
        d.u.b = e->u.b;
    }
    return d;
}

toml_datum_t toml_int_at(toml_array_t *arr, int idx) {
    (void)arr;
    (void)idx;
    toml_datum_t d = {0};
    return d;
}

toml_datum_t toml_double_at(toml_array_t *arr, int idx) {
    (void)arr;
    (void)idx;
    toml_datum_t d = {0};
    return d;
}
