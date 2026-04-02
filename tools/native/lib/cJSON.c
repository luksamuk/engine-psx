/*
 * cJSON.c - Minimal implementation of cJSON
 * Sufficient for engine-psx tools
 */

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdbool.h>
#include "cJSON.h"

static const char *ep; // error pointer

const char *cJSON_GetErrorPtr(void) {
    return ep;
}

static const char* skip_ws(const char *c) {
    while (isspace((unsigned char)*c)) c++;
    return c;
}

static const char *parse_string(cJSON *item, const char *str) {
    const char *ptr = str + 1;
    char *ptr2;
    char *out;
    int len = 0;
    
    if (*str != '"') { ep = str; return NULL; }
    
    while (*ptr != '"' && *ptr) {
        if (*ptr++ == '\\') ptr++;
        len++;
    }
    
    out = (char*)malloc(len + 1);
    if (!out) return NULL;
    
    ptr = str + 1;
    ptr2 = out;
    while (*ptr != '"' && *ptr) {
        if (*ptr != '\\') *ptr2++ = *ptr++;
        else {
            ptr++;
            switch (*ptr++) {
                case 'b': *ptr2++ = '\b'; break;
                case 'f': *ptr2++ = '\f'; break;
                case 'n': *ptr2++ = '\n'; break;
                case 'r': *ptr2++ = '\r'; break;
                case 't': *ptr2++ = '\t'; break;
                default: *ptr2++ = *(ptr-1); break;
            }
        }
    }
    *ptr2 = '\0';
    
    if (*ptr == '"') ptr++;
    item->valuestring = out;
    item->type = cJSON_String;
    return ptr;
}

static const char *parse_number(cJSON *item, const char *num) {
    double n = 0;
    int sign = 1;
    
    if (*num == '-') sign = -1, num++;
    if (*num == '0') num++;
    if (*num >= '1' && *num <= '9') {
        do n = (n * 10.0) + (*num++ - '0'); while (*num >= '0' && *num <= '9');
    }
    if (*num == '.' && num[1] >= '0' && num[1] <= '9') {
        double frac = 0;
        int div = 1;
        num++;
        while (*num >= '0' && *num <= '9') {
            frac = (frac * 10.0) + (*num++ - '0');
            div *= 10;
        }
        n += frac / div;
    }
    if (*num == 'e' || *num == 'E') {
        int e = 0, esign = 1;
        num++;
        if (*num == '+') num++;
        else if (*num == '-') esign = -1, num++;
        while (*num >= '0' && *num <= '9') e = (e * 10) + (*num++ - '0');
        if (esign < 0) for (int i = 0; i < e; i++) n /= 10.0;
        else for (int i = 0; i < e; i++) n *= 10.0;
    }
    
    item->valuedouble = n * sign;
    item->valueint = (int)(n * sign);
    item->type = cJSON_Number;
    return num;
}

static const char *parse_value(cJSON *item, const char *value);

static const char *parse_array(cJSON *item, const char *value) {
    cJSON *child;
    if (*value != '[') { ep = value; return NULL; }
    
    item->type = cJSON_Array;
    value = skip_ws(value + 1);
    
    if (*value == ']') return value + 1;
    
    item->child = child = (cJSON*)malloc(sizeof(cJSON));
    if (!item->child) return NULL;
    memset(item->child, 0, sizeof(cJSON));
    
    value = parse_value(child, skip_ws(value));
    if (!value) return NULL;
    
    while (*value == ',') {
        cJSON *new_item = (cJSON*)malloc(sizeof(cJSON));
        if (!new_item) return NULL;
        memset(new_item, 0, sizeof(cJSON));
        child->next = new_item;
        new_item->prev = child;
        child = new_item;
        value = skip_ws(value + 1);
        value = parse_value(child, value);
        if (!value) return NULL;
    }
    
    if (*value == ']') return value + 1;
    ep = value;
    return NULL;
}

static const char *parse_object(cJSON *item, const char *value) {
    cJSON *child;
    if (*value != '{') { ep = value; return NULL; }
    
    item->type = cJSON_Object;
    value = skip_ws(value + 1);
    
    if (*value == '}') return value + 1;
    
    item->child = child = (cJSON*)malloc(sizeof(cJSON));
    if (!item->child) return NULL;
    memset(item->child, 0, sizeof(cJSON));
    
    const char *name = parse_string(child, skip_ws(value));
    if (!name) return NULL;
    child->string = child->valuestring;
    child->valuestring = NULL;
    
    value = skip_ws(name);
    if (*value != ':') { ep = value; return NULL; }
    value = skip_ws(value + 1);
    value = parse_value(child, value);
    if (!value) return NULL;
    
    while (*value == ',') {
        cJSON *new_item = (cJSON*)malloc(sizeof(cJSON));
        if (!new_item) return NULL;
        memset(new_item, 0, sizeof(cJSON));
        child->next = new_item;
        new_item->prev = child;
        child = new_item;
        
        value = skip_ws(value + 1);
        name = parse_string(child, value);
        if (!name) return NULL;
        child->string = child->valuestring;
        child->valuestring = NULL;
        
        value = skip_ws(name);
        if (*value != ':') { ep = value; return NULL; }
        value = skip_ws(value + 1);
        value = parse_value(child, value);
        if (!value) return NULL;
    }
    
    if (*value == '}') return value + 1;
    ep = value;
    return NULL;
}

static const char *parse_value(cJSON *item, const char *value) {
    if (!value) { ep = value; return NULL; }
    
    if (!strncmp(value, "null", 4)) { item->type = cJSON_NULL; return value + 4; }
    if (!strncmp(value, "false", 5)) { item->type = cJSON_False; return value + 5; }
    if (!strncmp(value, "true", 4)) { item->type = cJSON_True; return value + 4; }
    if (*value == '"') return parse_string(item, value);
    if (*value == '-' || (*value >= '0' && *value <= '9')) return parse_number(item, value);
    if (*value == '[') return parse_array(item, value);
    if (*value == '{') return parse_object(item, value);
    
    ep = value;
    return NULL;
}

cJSON *cJSON_Parse(const char *value) {
    cJSON *c = (cJSON*)malloc(sizeof(cJSON));
    if (!c) return NULL;
    memset(c, 0, sizeof(cJSON));
    
    ep = NULL;
    const char *end = parse_value(c, skip_ws(value));
    
    if (!end || *skip_ws(end)) {
        cJSON_Delete(c);
        return NULL;
    }
    return c;
}

void cJSON_Delete(cJSON *c) {
    cJSON *next;
    while (c) {
        next = c->next;
        if (c->child) cJSON_Delete(c->child);
        if (c->valuestring) free(c->valuestring);
        if (c->string) free(c->string);
        free(c);
        c = next;
    }
}

int cJSON_GetArraySize(const cJSON *array) {
    int size = 0;
    cJSON *c = array ? array->child : NULL;
    while (c) { size++; c = c->next; }
    return size;
}

cJSON *cJSON_GetArrayItem(const cJSON *array, int item) {
    cJSON *c = array ? array->child : NULL;
    while (c && item > 0) { item--; c = c->next; }
    return c;
}

cJSON *cJSON_GetObjectItem(const cJSON *object, const char *string) {
    cJSON *c = object ? object->child : NULL;
    while (c && strcmp(c->string, string)) c = c->next;
    return c;
}

cJSON *cJSON_GetObjectItemCaseSensitive(const cJSON *object, const char *string) {
    return cJSON_GetObjectItem(object, string);
}

char *cJSON_Print(const cJSON *item) {
    (void)item;
    return NULL;
}
