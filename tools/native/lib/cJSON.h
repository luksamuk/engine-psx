/*
 * cJSON.h - Minimal version of cJSON (header only subset)
 * For full version, see: https://github.com/DaveGamble/cJSON
 */

#ifndef CJSON_H
#define CJSON_H

#include <stddef.h>

/* cJSON Types */
typedef struct cJSON {
    struct cJSON *next;
    struct cJSON *prev;
    struct cJSON *child;
    int type;
    char *valuestring;
    int valueint;
    double valuedouble;
    char *string;
} cJSON;

#define cJSON_Invalid  (0)
#define cJSON_False    (1 << 0)
#define cJSON_True     (1 << 1)
#define cJSON_NULL     (1 << 2)
#define cJSON_Number   (1 << 3)
#define cJSON_String   (1 << 4)
#define cJSON_Array    (1 << 5)
#define cJSON_Object   (1 << 6)
#define cJSON_Raw      (1 << 7)

#define cJSON_IsFalse(x) ((x) && (x)->type == cJSON_False)
#define cJSON_IsTrue(x) ((x) && (x)->type == cJSON_True)
#define cJSON_IsBool(x) ((x) && ((x)->type == cJSON_True || (x)->type == cJSON_False))
#define cJSON_IsNull(x) ((x) && (x)->type == cJSON_NULL)
#define cJSON_IsNumber(x) ((x) && ((x)->type & cJSON_Number) != 0)
#define cJSON_IsString(x) ((x) && (x)->type == cJSON_String)
#define cJSON_IsArray(x) ((x) && (x)->type == cJSON_Array)
#define cJSON_IsObject(x) ((x) && (x)->type == cJSON_Object)

/* Public API */
cJSON *cJSON_Parse(const char *value);
char *cJSON_Print(const cJSON *item);
void cJSON_Delete(cJSON *item);
const char *cJSON_GetErrorPtr(void);

/* Object/Array helpers */
int cJSON_GetArraySize(const cJSON *array);
cJSON *cJSON_GetArrayItem(const cJSON *array, int index);
cJSON *cJSON_GetObjectItem(const cJSON *object, const char *string);
cJSON *cJSON_GetObjectItemCaseSensitive(const cJSON *object, const char *string);

#endif /* CJSON_H */
