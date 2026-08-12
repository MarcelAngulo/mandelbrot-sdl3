
#ifndef APP_H
#define APP_H

#include "mandelbrot.h"
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#define FONT_RELATIVE_PATH "../assets/GoogleSansCode-Regular.ttf"
#define DEFAULT_IMAGE_FILENAME "%Y%m%d%H%M%S.png"
#define DEFAULT_APPSTATE_FILENAME "%Y%m%d%H%M%S.txt"
#define streq(s1,s2) (strcmp(s1, s2) == 0)
#define COMMAND_BUFFER_SIZE 1000
#define MAX_ARGS 20

typedef enum {
    MODE_EXPLORE = 0,
    MODE_COMMAND = 1
} AppMode;

typedef struct {
    Uint32 set_color;
    Uint32 *escape_palette;
    int escape_palette_size;
} MandelbrotColors;


// 1. App State Structure
typedef struct {
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *texture_fractal;
    SDL_Texture *texture_help;
    SDL_Texture *texture_info;
    SDL_Texture *texture_command;
    SDL_Texture *texture_help_command;
    SDL_FRect rect_fractal;
    SDL_FRect rect_help;
    SDL_FRect rect_info;
    SDL_FRect rect_command;
    SDL_FRect rect_help_command;
    SDL_Color color_font;
    TTF_Font *font;
    int width;
    int height;
    MandelbrotConfig config;
    MandelbrotConfig config_backup;
    MandelbrotColors colors;
    AppMode mode;
    bool is_rendering;
    bool show_info;
    bool show_pointer;
    bool show_help;
    bool show_help_command;
    char *base_path;
    char *output_path;
    char *read_path;
    char command_buffer[COMMAND_BUFFER_SIZE];
    unsigned long int command_length;
} AppState;

extern char help_commands_str[];
#endif
