#ifndef DEBUG_H
#define DEBUG_H

#include <stdbool.h>
#include <stdio.h>

typedef enum {
    DEBUG_LEVEL_ERROR = 0,
    DEBUG_LEVEL_WARN,
    DEBUG_LEVEL_INFO,
    DEBUG_LEVEL_DEBUG,
    DEBUG_LEVEL_TRACE
} DebugLevel;

void set_debug_level(DebugLevel level);
DebugLevel get_debug_level(void);

void debug_log(DebugLevel level, const char* file, int line, const char* func, const char* fmt, ...);

// Convenience macros for contextual logging
#define LOG_ERROR(fmt, ...) debug_log(DEBUG_LEVEL_ERROR, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...)  debug_log(DEBUG_LEVEL_WARN,  __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...)  debug_log(DEBUG_LEVEL_INFO,  __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)
#define LOG_DEBUG(fmt, ...) debug_log(DEBUG_LEVEL_DEBUG, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)
#define LOG_TRACE(fmt, ...) debug_log(DEBUG_LEVEL_TRACE, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)

#endif // DEBUG_H