#include "log.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <sys/time.h>

static LogLevel current_log_level = LOG_LEVEL_INFO;

void log_set_level(LogLevel level) {
    current_log_level = level;
}

const char* log_level_to_string(LogLevel level) {
    switch (level) {
        case LOG_LEVEL_TRACE: return "TRACE";
        case LOG_LEVEL_DEBUG: return "DEBUG";
        case LOG_LEVEL_INFO:  return "INFO";
        case LOG_LEVEL_WARNING: return "WARNING";
        case LOG_LEVEL_ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

void log_message(LogLevel level, const char* file, int line, const char* format, ...) {
    if (level < current_log_level) return;

    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    char time_buffer[64];
    strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%d %H:%M:%S", tm_info);

    va_list args;
    va_start(args, format);

    const char* prefix = "";
    switch (level) {
        case LOG_LEVEL_TRACE: prefix = "[TRACE] "; break;
        case LOG_LEVEL_DEBUG: prefix = "[DEBUG] "; break;
        case LOG_LEVEL_INFO:  prefix = "[INFO]  "; break;
        case LOG_LEVEL_WARNING: prefix = "[WARN]  "; break;
        case LOG_LEVEL_ERROR: prefix = "[ERROR] "; break;
    }

    fprintf(stderr, "%s %s %s:%d ", time_buffer, prefix, file, line);
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");

    va_end(args);
}

void log_debug(const char* file, int line, const char* format, ...) {
    log_message(LOG_LEVEL_DEBUG, file, line, format);
}

void log_info(const char* file, int line, const char* format, ...) {
    log_message(LOG_LEVEL_INFO, file, line, format);
}

void log_warning(const char* file, int line, const char* format, ...) {
    log_message(LOG_LEVEL_WARNING, file, line, format);
}

void log_error(const char* file, int line, const char* format, ...) {
    log_message(LOG_LEVEL_ERROR, file, line, format);
}