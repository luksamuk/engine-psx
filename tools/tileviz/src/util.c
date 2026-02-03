// Utility functions implementation

#include "util.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

const char* util_get_extension(const char* filepath) {
    const char* last_slash = filepath;
    const char* last_dot = NULL;

    for (const char* p = filepath; *p; p++) {
        if (*p == '/' || *p == '\\') {
            last_slash = p + 1;
        }
        if (*p == '.') {
            last_dot = p;
        }
    }

    if (last_dot && last_dot > last_slash) {
        return last_dot + 1;
    }

    return "";
}

bool util_file_exists(const char* filepath) {
    return access(filepath, F_OK) == 0;
}

void util_format_size(uint64_t size, char* buffer, size_t buffer_size) {
    if (size < 1024) {
        snprintf(buffer, buffer_size, "%lu B", (unsigned long)size);
    } else if (size < 1024 * 1024) {
        snprintf(buffer, buffer_size, "%.2f KB", (double)size / 1024.0);
    } else if (size < 1024 * 1024 * 1024) {
        snprintf(buffer, buffer_size, "%.2f MB", (double)size / (1024.0 * 1024.0));
    } else {
        snprintf(buffer, buffer_size, "%.2f GB", (double)size / (1024.0 * 1024.0 * 1024.0));
    }
}

bool util_str_copy(char* dest, const char* src, size_t dest_size) {
    if (!dest || !src || dest_size == 0) {
        return false;
    }

    size_t src_len = strlen(src);
    if (src_len >= dest_size) {
        return false;
    }

    strcpy(dest, src);
    return true;
}

bool util_has_path_separator(const char* str) {
    for (const char* p = str; *p; p++) {
        if (*p == '/' || *p == '\\') {
            return true;
        }
    }
    return false;
}