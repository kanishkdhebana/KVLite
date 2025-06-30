#include "log.h"

#include <cstdio>
#include <ctime>
#include <cstdlib>
#include <cstdarg>

FILE* g_logFile = nullptr;

void initLog() {
    g_logFile = fopen("server.log", "a");
    if (!g_logFile) {
        perror("fopen log");
        exit(1);
    }
}

void closeLog() {
    if (g_logFile) fclose(g_logFile);
}

void logMessage(const char* fmt, ...) {
    if (!g_logFile) return;

    // Optional: prefix with timestamp
    char timebuf[64];
    time_t now = time(nullptr);
    struct tm tm_now;
    localtime_r(&now, &tm_now);
    strftime(timebuf, sizeof(timebuf), "[%Y-%m-%d %H:%M:%S] ", &tm_now);

    fprintf(g_logFile, "%s", timebuf);

    va_list args;
    va_start(args, fmt);
    vfprintf(g_logFile, fmt, args);
    va_end(args);

    fprintf(g_logFile, "\n");
    fflush(g_logFile);
}
