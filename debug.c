#include "debug.h"
#include <stdarg.h>
#include <time.h>
#include <string.h>
#include <stdint.h>
#ifdef __APPLE__
#include <pthread.h>
#endif

static DebugLevel current_level = DEBUG_LEVEL_ERROR;

void set_debug_level(DebugLevel level) {
    current_level = level;
}

DebugLevel get_debug_level(void) {
    return current_level;
}

static const char* debug_level_name(DebugLevel level) {
    switch (level) {
        case DEBUG_LEVEL_ERROR: return "ERROR";
        case DEBUG_LEVEL_WARN:  return "WARN";
        case DEBUG_LEVEL_INFO:  return "INFO";
        case DEBUG_LEVEL_DEBUG: return "DEBUG";
        case DEBUG_LEVEL_TRACE: return "TRACE";
        default:                return "UNKNOWN";
    }
}

void debug_log(DebugLevel level, const char* file, int line, const char* func, const char* fmt, ...) {
    if (level > current_level) return;

    // Timestamp
    time_t now = time(NULL);
    struct tm tm_now;
    localtime_r(&now, &tm_now);
    char timebuf[20];
    strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", &tm_now);

    // Thread id (optional, only on POSIX)
    #ifdef __APPLE__
    uint64_t tid;
    pthread_threadid_np(NULL, &tid);
    #endif

    // Print log header
    fprintf(stderr, "[%s] [%s]", timebuf, debug_level_name(level));
    #ifdef __APPLE__
    fprintf(stderr, " [TID:%llu]", (unsigned long long)tid);
    #endif
    fprintf(stderr, " [%s:%d:%s] ", file, line, func);

    // Print log message
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);

    fprintf(stderr, "\n");
    fflush(stderr);
}