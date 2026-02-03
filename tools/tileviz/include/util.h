// Utility functions

#ifndef UTIL_H
#define UTIL_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>

// Find extension in filepath
const char* util_get_extension(const char* filepath);

// Check if file exists
bool util_file_exists(const char* filepath);

// Format file size to human readable string
void util_format_size(uint64_t size, char* buffer, size_t buffer_size);

// Safe string copy with length check
bool util_str_copy(char* dest, const char* src, size_t dest_size);

// Check for path separators
bool util_has_path_separator(const char* str);

#endif // UTIL_H