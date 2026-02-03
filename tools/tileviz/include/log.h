// Logging system for debugging

#ifndef LOG_H
#define LOG_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>

typedef enum {
    LOG_LEVEL_TRACE,
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARNING,
    LOG_LEVEL_ERROR
} LogLevel;

#ifdef DEBUG
    #define LOG_LEVEL_DEFAULT LOG_LEVEL_DEBUG
#else
    #define LOG_LEVEL_DEFAULT LOG_LEVEL_INFO
#endif

void log_set_level(LogLevel level);
const char* log_level_to_string(LogLevel level);
void log_message(LogLevel level, const char* file, int line, const char* format, ...);

// Convenience macros
#define LOG_TRACE(file, line, format, ...) log_message(LOG_LEVEL_TRACE, file, line, format, ##__VA_ARGS__)
#define LOG_DEBUG(file, line, format, ...) log_message(LOG_LEVEL_DEBUG, file, line, format, ##__VA_ARGS__)
#define LOG_INFO(file, line, format, ...) log_message(LOG_LEVEL_INFO, file, line, format, ##__VA_ARGS__)
#define LOG_WARNING(file, line, format, ...) log_message(LOG_LEVEL_WARNING, file, line, format, ##__VA_ARGS__)
#define LOG_ERROR(file, line, format, ...) log_message(LOG_LEVEL_ERROR, file, line, format, ##__VA_ARGS__)

#endif // LOG_H