
#ifndef LOGGER_H
#define LOGGER_H

#include <SDL3/SDL.h>

#define USE_COLORS 

#ifdef USE_COLORS
    #define COLOR_RESET   "\x1b[0m"
    #define COLOR_INFO    "\x1b[32m" // Green
    #define COLOR_WARN    "\x1b[33m" // Yellow
    #define COLOR_ERROR   "\x1b[31m" // Red
#else
    #define COLOR_RESET   ""
    #define COLOR_INFO    ""
    #define COLOR_WARN    ""
    #define COLOR_ERROR   ""
#endif

// Uses SDL_LogInfo (routes to stdout)
#define LOG_INFO(fmt, ...) \
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, COLOR_INFO "[INFO] %s:%d: " fmt COLOR_RESET, __FILE__, __LINE__, ##__VA_ARGS__)

// Uses SDL_LogWarn (routes to stdout)
#define LOG_WARN(fmt, ...) \
    SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, COLOR_WARN "%s:%d: " fmt COLOR_RESET, __FILE__, __LINE__, ##__VA_ARGS__)

// Uses SDL_LogError (routes to stderr automatically)
#define LOG_ERROR(fmt, ...) \
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, COLOR_ERROR "[ERROR] %s:%d: " fmt COLOR_RESET, __FILE__, __LINE__, ##__VA_ARGS__)

#endif
