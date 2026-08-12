
#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL_main.h>
#include <stdbool.h>
#include <stdlib.h>
#include <errno.h>
#include "app.h"
#include "render.h"
#include "utils.h"
#include "commands.h"
#include "logger.h"
#include "events.h"

/* TODO LIST:
 *
 * POSSIBLE TODO LIST:
 * 6) escribir README usage and command line
 * 7) Revisar read_status (nunca la probe) para leer otros archivos
 * 8) annadir help for commands and help for arguments command lines
 */

char help_keys_str[] =
"--------------| HELP |---------------\n"
"z  F1      . help\n"
"x          . return to last update\n"
"a          . zoom out\n"
"s          . zoom in\n"
"q  ESCAPE  . quit app\n"
"o  -       . decrease max iterations\n"
"p  +       . increase max iterations\n"
"g          . save screenshot\n"
"h  LEFT    . move left\n"
"l  RIGHT   . move right\n"
"j  DOWN    . move down\n"
"k  UP      . move up\n"
"i          . toogle info view\n"
"c          . toogle pointer view\n"
"f          . fullscreen\n"
"------------------------------------";


SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[]) {
    AppState *app = (AppState *) calloc(1, sizeof(AppState));
    if (!app) {
        LOG_ERROR("Memory Allocation Failed");
        return SDL_APP_FAILURE;
    }
    *appstate = app;


    // In this section the app initialize variables that are not so
    // related with SDL
    // Init mandelbrot config
    app->config = (MandelbrotConfig){
        .center = -0.75,
        .range = 3.0-3.0*I,
        .power = 2,
        .maxIterations = 20,
        .zoom_ratio = 1.05,
        .displacement = 0.05
    };
    app->config_backup = app->config;
    
    // Default size of app
    app->width = 800;
    app->height = 800;
    app->mode = MODE_EXPLORE;
    app->is_rendering = false;
    app->show_info = true;
    app->show_pointer = true;
    app->show_help = false;

    // Generates Default colors
    // Solid black for mandelbrot set
    app->colors.set_color = 0x000000FF; 
    app->color_font = (SDL_Color) {0xFF, 0xFF, 0xFF, SDL_ALPHA_OPAQUE};
    // Colors of the points that don't belong to mandelbrot set.
    app->colors.escape_palette_size = 50;
    app->colors.escape_palette = (Uint32 *) malloc(sizeof(Uint32) * app->colors.escape_palette_size);
    if(!app->colors.escape_palette) {
        LOG_ERROR("Failed to allocate memory: %s", strerror(errno));
        return SDL_APP_FAILURE;
    }
    Uint32 palette[] = {
        0xdb073dff, 0xdba507ff, 0x8ec7d2ff, 0x0d6986ff, 0x07485bff, 0xdb073dff
    };
    calculate_colors(
        sizeof(palette)/sizeof(Uint32), palette,
        app->colors.escape_palette_size, app->colors.escape_palette);
    app->output_path = copy_string("./");
    if (!app->output_path) {
        LOG_ERROR("Failed to allocate memory: %s", strerror(errno));
        return SDL_APP_FAILURE;
    }
    LOG_INFO("app->output_path allocated: \"%s\"", app->output_path);

    LOG_INFO("Default variables initializated.");

    // Parse arguments from command line
    switch (execute_command(app, argc-1, argv+1)) {
        case SDL_APP_FAILURE:
            return SDL_APP_FAILURE;
        case SDL_APP_SUCCESS:
            return SDL_APP_SUCCESS;
        case SDL_APP_CONTINUE:
            LOG_INFO("Arguments parsed.");
            break;
    }

    // Init SDL Library
    if(!SDL_Init(SDL_INIT_VIDEO)) {
        LOG_ERROR("SDL_Init Error: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    } else {
        LOG_INFO("SDL3 Initialized.");
    }
    // Init TTF for rendering fonts
    if(!TTF_Init()) {
        LOG_ERROR("TTF_Init Error: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    } else {
        LOG_INFO("SDL3_ttf Initialized.");
    }

    // Create window
    if (!SDL_CreateWindowAndRenderer("Mandelbrot Explorer",
                app->width, app->height,
                SDL_WINDOW_HIDDEN | SDL_WINDOW_RESIZABLE,
                &app->window, &app->renderer)) {
        LOG_ERROR("Failed to create window and renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    } else {
        LOG_INFO("Window and renderer created.");
    }

    // Creates default output path
    const char* base_path = SDL_GetBasePath();
    if (!base_path) {
        LOG_ERROR("Failed to get base path: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    } else {
        LOG_INFO("Base path obtained.");
    }


    // Creates a string to open the font
    char *font_path = join_strings(base_path, FONT_RELATIVE_PATH);
    if (!font_path) {
        LOG_ERROR("Failed to create font path: %s", strerror(errno));
        return SDL_APP_FAILURE;
    } else {
        LOG_INFO("Relative font path created: \"%s\"", font_path);
    }

    // Open font used in the program
    app->font = TTF_OpenFont(font_path, 20.0f);

    // Frees string used to open font
    if (!app->font) {
        LOG_ERROR("Failed to load font \"%s\": %s", font_path, SDL_GetError());
        free(font_path);
        return SDL_APP_FAILURE;
    } else {
        LOG_INFO("Font \"%s\" loaded", font_path);
    }

    free(font_path);

    // Render help texture
    app->texture_help = renderText(app->renderer,
            app->font, help_keys_str, app->color_font,
            &app->rect_help.w, &app->rect_help.h);
    if(!app->texture_help) {
        LOG_ERROR("Failed to create texture_help: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    } else {
        LOG_INFO("app->texture_help created.");
    }

    app->rect_help.x = (app->width - app->rect_help.w)/2;
    app->rect_help.y = (app->height - app->rect_help.h)/2;


    SDL_SetHint(SDL_HINT_MAIN_CALLBACK_RATE, "waitevent");

    // Render texture_info for the first time
    update_texture_info(app);
    LOG_INFO("Info texture updated successfully.");
    // Draw fractal on app->texture_fractal for the first time
    update_texture_fractal(app);
    LOG_INFO("Fractal texture updated successfully.");

    LOG_INFO("Initialization completed!");
    SDL_ShowWindow(app->window);
    return SDL_APP_CONTINUE;
}



SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
    AppState *app = (AppState *)appstate;
    // Draw fractal for the first time.

    if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;
        LOG_INFO("closing app");
    }
    else if (event->type == SDL_EVENT_KEY_DOWN) {
        // Handle events from MODE_EXPLORE
        if (app->mode == MODE_EXPLORE) {
            return handle_explore_keys(app, event);
        // Handle events in MODE_COMMAND
        } else if (app->mode == MODE_COMMAND) {
            return handle_command_keys(app, event);
        }
    } else if (event->type == SDL_EVENT_TEXT_INPUT) {
        return handle_text_input(app, event);
    } else if (event->type == SDL_EVENT_WINDOW_RESIZED) {
        return handle_window_resize(app, event);
    } else if (event->type == SDL_EVENT_MOUSE_MOTION) {
        return handle_mouse_motion(app, event);
    } else if (event->type == SDL_EVENT_MOUSE_WHEEL) {
        return handle_mouse_wheel(app, event);
    }

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate) {
    AppState *app = appstate;
    render_all(app);
    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result) {
    if (appstate) {
        AppState *app = (AppState *)appstate;
        if (app->colors.escape_palette) free(app->colors.escape_palette);
        if (app->output_path) free(app->output_path);
        if (app->read_path) free(app->read_path);
        if (app->font) TTF_CloseFont(app->font);
        SDL_DestroyRenderer(app->renderer);
        SDL_DestroyWindow(app->window);
        SDL_DestroyTexture(app->texture_fractal);
        SDL_DestroyTexture(app->texture_help);
        SDL_DestroyTexture(app->texture_info);
        SDL_DestroyTexture(app->texture_command);

        free(app);
    }
}

