
#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL_main.h>
#include <stdbool.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include "app.h"
#include "render.h"
#include "utils.h"
#include "commands.h"
#include "file_io.h"
#include "logger.h"

/* TODO LIST:
 *
 * POSSIBLE TODO LIST:
 * 5) annadir command argument para guardar output
 *       directory(chequear / al final) y read file
 * 5) event_handling events
 * 7) Revisar read_status (nunca la probe) para leer otros archivos
 * 1) Create little message bottom screen with info
 * 6) escribir README usage and command line
 */

char help_str[] =
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
        .maxIterations = 20
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


    // Parse arguments from command line
    if(parse_arguments(app, argc, argv)) {
        return SDL_APP_FAILURE;
    }

    // Init SDL Library
    if(!SDL_Init(SDL_INIT_VIDEO)) {
        LOG_ERROR("SDL_Init Error: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    // Init TTF for rendering fonts
    if(!TTF_Init()) {
        LOG_ERROR("TTF_Init Error: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    // Create window
    if (!SDL_CreateWindowAndRenderer("Mandelbrot Explorer",
                app->width, app->height, SDL_WINDOW_RESIZABLE,
                &app->window, &app->renderer)) {
        LOG_ERROR("Failed to create window and renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    // Create texture where fractal is drawn
    app->texture_fractal = SDL_CreateTexture(
            app->renderer,
            SDL_PIXELFORMAT_RGBA8888,
            SDL_TEXTUREACCESS_STREAMING,
            app->width, app->height
    );
    if(!app->texture_fractal) {
        LOG_ERROR("Failed to create fractal texture: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    // Creates default output path
    const char* base_path = SDL_GetBasePath();
    if (!base_path) {
        LOG_ERROR("Failed to get base path: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    // Creates a string to open the font
    char *font_path = join_strings(base_path, FONT_RELATIVE_PATH);
    if (!font_path) {
        LOG_ERROR("Failed to create font path: %s", strerror(errno));
        return SDL_APP_FAILURE;
    }
    // Open font used in the program
    app->font = TTF_OpenFont(font_path, 20.0f);

    // Frees string used to open font
    if (!app->font) {
        LOG_ERROR("Failed to load font \"%s\": %s", font_path, SDL_GetError());
        free(font_path);
        return SDL_APP_FAILURE;
    }
    free(font_path);

    // Render help texture
    app->texture_help = renderText(app->renderer,
            app->font, help_str, app->color_font,
            &app->rect_help.w, &app->rect_help.h);
    if(!app->texture_help) {
        LOG_ERROR("Failed to create texture_help: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    app->rect_help.x = (app->width - app->rect_help.w)/2;
    app->rect_help.y = (app->height - app->rect_help.h)/2;

    char out[] = "./";
    app->output_path = (char *) malloc(strlen(out)+1);
    if (!app->output_path) {
        LOG_ERROR("Failed to allocate memory: %s", strerror(errno));
        return SDL_APP_FAILURE;
    }
    strcpy(app->output_path, out);

    SDL_SetHint(SDL_HINT_MAIN_CALLBACK_RATE, "waitevent");

    // Render texture_info for the first time
    update_texture_info(app);
    // Draw fractal on app->texture_fractal for the first time
    update_texture_fractal(app);

    LOG_INFO("Initialization completed!");
    return SDL_APP_CONTINUE;
}



SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
    AppState *app = (AppState *)appstate;
    // Displacement when pressing UP, DOWN, LEFT, RIGHT, h,j,k,l keys.
    const float displacement = 0.05;
    const float zoom = 1.05;
    // Draw fractal for the first time.

    if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;
    }
    else if (event->type == SDL_EVENT_KEY_DOWN) {

        // Handle events from MODE_EXPLORE
        if (app->mode == MODE_EXPLORE) {
            switch (event->key.key) {
                // Close app
                case SDLK_Q:
                    return SDL_APP_SUCCESS;
                    break;

                case SDLK_Z:
                case SDLK_F1:
                    app->show_help = !app->show_help;
                    break;

                // Update view
                case SDLK_SPACE:
                case SDLK_RETURN:
                    app->is_rendering = true;
                    update_texture_info(app);
                    render_all(app);
                    update_texture_fractal(app);
                    app->is_rendering = false;
                    update_texture_info(app);
                    render_all(app);
                    break;

                // Move upwards
                case SDLK_UP:
                case SDLK_K:
                    view_displace(app, 0, app->height*displacement);
                    update_texture_info(app);
                    break;

                // Move downwards
                case SDLK_DOWN:
                case SDLK_J:
                    view_displace(app, 0, -app->height*displacement);
                    update_texture_info(app);
                    break;

                // Move to the left 
                case SDLK_LEFT:
                case SDLK_H:
                    view_displace(app, -app->width*displacement, 0);
                    update_texture_info(app);
                    break;

                // Move to the Right
                case SDLK_RIGHT:
                case SDLK_L:
                    view_displace(app, app->width*displacement, 0);
                    update_texture_info(app);
                    break;

                // Zoom in view
                case SDLK_PLUS:
                case SDLK_S:
                    view_zoom(app, 1/zoom);
                    update_texture_info(app);
                    break;

                // Zoom out view
                case SDLK_MINUS:
                case SDLK_A:
                    view_zoom(app, zoom);
                    update_texture_info(app);
                    break;

                // Increase the number of max iterations per point
                case SDLK_P:
                    app->config.maxIterations++;
                    update_texture_info(app);
                    break;

                // Decrase number of max iterations per point.
                case SDLK_O:
                    app->config.maxIterations--;
                    if(app->config.maxIterations < 0)
                        app->config.maxIterations = 0;
                    update_texture_info(app);
                    break;

                // Toggle visibility of info window.
                case SDLK_I:
                    app->show_info = !app->show_info;
                    update_texture_info(app);
                    break;

                // Toggle visibility of cross at the middle.
                case SDLK_C:
                    app->show_pointer = !app->show_pointer;
                    break;

                // Change to state of last updating.
                case SDLK_X:
                    app->config = app->config_backup;
                    SDL_GetTextureSize(app->texture_fractal,
                            &app->rect_fractal.w, &app->rect_fractal.h);
                    app->rect_fractal.x = (app->width-app->rect_fractal.w)/2;
                    app->rect_fractal.y = (app->height-app->rect_fractal.h)/2;
                    update_texture_info(app);
                    break;

                case SDLK_F:
                    SDL_SetWindowFullscreen(app->window,
                        !(SDL_GetWindowFlags(app->window) & SDL_WINDOW_FULLSCREEN));
                    update_texture_info(app);
                    break;

                // When 'G' is pressed, save screenshot.
                case SDLK_G:
                    save_image(app, DEFAULT_IMAGE_FILENAME);
                    break;

                // Enter to command mode. Command will appear at
                // the bottom of the screen
                case SDLK_N:
                    SDL_StartTextInput(app->window);
                    app->mode = MODE_COMMAND;
                    app->command_length = 1;
                    strcpy(app->command_buffer, ":");
                    update_texture_command(app);
                    update_texture_info(app);
                    break;
            }

        // Handle events in MODE_COMMAND
        } else if (app->mode == MODE_COMMAND) {
            switch(event->key.key) {
                // Exit from command mode
                case SDLK_ESCAPE:
                    SDL_StopTextInput(app->window);
                    app->mode = MODE_EXPLORE;
                    update_texture_info(app);
                    break;

                // Exit from command mode
                case SDLK_C:
                    if ((event->key.mod & SDL_KMOD_CTRL) &&
                        (event->key.mod & SDL_KMOD_SHIFT)) {
                            SDL_StopTextInput(app->window);
                            app->mode = MODE_EXPLORE;
                    }
                    update_texture_info(app);
                    break;

                // Executes command and exit from command mode.
                case SDLK_RETURN:
                    {
                        char *argv[MAX_ARGS];
                        int argc = parse_command(app->command_buffer+1, argv, MAX_ARGS);
                        execute_command(app, argc, argv);
                    }
                    SDL_StopTextInput(app->window);
                    app->mode = MODE_EXPLORE;
                    update_texture_info(app);
                    break;

                // Deletes one character
                case SDLK_BACKSPACE:
                    if (app->command_length > 1) {
                        app->command_buffer[--app->command_length] = '\0';
                        update_texture_command(app);
                    } else {
                        SDL_StopTextInput(app->window);
                        app->mode = MODE_EXPLORE;
                        update_texture_info(app);
                    }
                    break;
            }
        }
    } else if (event->type == SDL_EVENT_TEXT_INPUT) {
        // This event will store text into app->command_buffer if there is
        // enough space
        long unsigned int nl = strlen(event->text.text);
        if (nl + app->command_length > sizeof(app->command_buffer)-1) {
            ; // Dont write anything. Buffer full
        } else {
            strcat(app->command_buffer, event->text.text);
            app->command_length += nl;
            update_texture_command(app);
        }
    } else if (event->type == SDL_EVENT_WINDOW_RESIZED) {
        // Update app state with new dimensions

        view_resize(app, (double) event->window.data1/app->width,
            (double) event->window.data2/app->height);

        app->width = event->window.data1;
        app->height = event->window.data2;
        // Updates info texture
        update_texture_info(app);

    // Process mouse motion events
    } else if (event->type == SDL_EVENT_MOUSE_MOTION) {
        // change app->config.center position if user move
        // mouse whith left button pressed.
        if(event->motion.state & SDL_BUTTON_LMASK) {
            view_displace(app, -event->motion.xrel, event->motion.yrel);
            update_texture_info(app);
        }
    } else if (event->type == SDL_EVENT_MOUSE_WHEEL) {
        // This part zoom in and zoom out when scrolling
        // with mouse wheeel
        float y = event->wheel.y;
        if ( y > 0 ) {
            // zoom in
            view_zoom(app, 1/zoom);
        } else if ( y < 0 ) {
            // zoom out
            view_zoom(app, zoom);
        }
        update_texture_info(app);
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

