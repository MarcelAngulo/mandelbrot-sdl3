
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "commands.h"
#include "utils.h"
#include "file_io.h"
#include "logger.h"

SDL_AppResult execute_command(AppState *app, int argc, char *argv[]) {
    for(int i = 0; i < argc; i++){
        const char *cmd = strip_dashes(argv[i]);
        if (streq(cmd, "center") || streq(cmd, "c")) {
            double re, im;
            if ( i + 2 < argc &&
                parse_double(argv[++i], &re) &&
                parse_double(argv[++i], &im)) {
                app->config.center = app->config_backup.center = re + im*I;
            } else {
                LOG_WARN("Invalid \"center | c\" command.\n"
                        "Use: (center | c) [double re] [double im]");
            }
        }
        else if (streq(cmd, "range") || streq(cmd, "r")) {
            double range;
            if ( i + 1 < argc && parse_double(argv[++i], &range)) {
                app->config.range = (1 - I*app->height/app->width)*range;
            } else {
                LOG_WARN("Invalid \"range | r\" command.\n"
                        "Use: (range | r) [double re]");
            }
        }
        else if (streq(cmd, "window") || streq(cmd, "w")) {
            long int width, height;
            if ( i + 2 < argc &&
                parse_long(argv[++i], &width, 10) && width > 0 &&
                parse_long(argv[++i], &height, 10) && height > 0) {
                app->config.range = (1 - I*height/width)*creal(app->config.range);
                app->width = width;
                app->height = height;
                if (app->window)
                    SDL_SetWindowSize(app->window, width, height);
            } else {
                LOG_WARN("Invalid \"window | w\" command.\n"
                        "Use: (window | w) [long int width > 0] [long int height > 0]");
            }
        }
        else if (streq(cmd, "maxiter") || streq(cmd, "i")) {
            long int maxIter;
            if ( i + 1 < argc && parse_long(argv[++i], &maxIter, 10) && maxIter > 0) {
                app->config.maxIterations = app->config_backup.maxIterations = maxIter;
            } else {
                LOG_WARN("Invalid \"maxiter | i\" argument.\n"
                        "Use: (maxIter | i) [long int iters > 0]");
            }
        }
        else if (streq(cmd, "power") || streq(cmd, "p")) {
            long power;
            if ( i + 1 < argc &&
                parse_long(argv[++i], &power, 10)) {
                app->config.power = app->config_backup.power = power;
            } else {
                LOG_WARN("Invalid \"power | p\" argument.\n"
                        "Use: (power | p) [double re] [double im]");
            }
        }
        else if (streq(cmd, "outputpath") || streq(cmd, "o")) {
            long int l;
            if (i + 1 < argc && (l = strlen(argv[++i])) > 0 ) {
                // Free previous allocated memory during initialization
                free(app->output_path);
                app->output_path = NULL;
                if (argv[i][l-1] != '/') {
                    app->output_path = join_strings(argv[i], "/");
                } else {
                    app->output_path = copy_string(argv[i]);
                }
                if (!app->output_path) {
                    LOG_ERROR("Failed to create app->output_path.");
                    return SDL_APP_FAILURE;
                } else {
                    LOG_INFO("Parsed --outputPath: \"%s\"", app->output_path);
                }
            } else {
                LOG_WARN("Invalid \"outputpath | o\" argument.\n"
                        "Use: (outputpath | o) [string path not empty]");
            }
        }
        else if (streq(cmd, "colors") || streq(cmd, "s")) {
            Uint32 set_color;
            long int size, rng;
            // Parse first 3 arguments
            if ( i + 3 < argc &&
                parse_color(argv[++i], &set_color, 16) &&
                parse_long(argv[++i], &rng, 10) && rng > 0 &&
                parse_long(argv[++i], &size, 10) && size > 0 && i + size <  argc) {
                // Parse colors
                app->colors.set_color = set_color;
                int end = ++i + size;
                // Now will parse the colors to store in app->colorS.escape_palette
                Uint32 *palette = (Uint32 *) malloc(sizeof(Uint32) *size);
                if (!palette) {
                    LOG_ERROR("Could not allocate memory for *palette in parse_arguments: %s", strerror(errno));
                    return SDL_APP_FAILURE;
                }
                // Save colors in a temporary array
                for (int k = 0; i < end; i++, k++) {
                    parse_color(argv[i], &palette[k], 16);
                }
                i--;

                // Free previous escape_palette
                free(app->colors.escape_palette);
                // Ask for new memory
                app->colors.escape_palette = (Uint32 *) malloc(sizeof(Uint32) * rng);
                if(!app->colors.escape_palette) {
                    LOG_ERROR("Could not allocate memory for app->colors.escape_palette in parse_arguments: %s", strerror(errno));
                    free(palette);
                    return SDL_APP_FAILURE;
                }

                // Stores new colors
                calculate_colors(size, palette, rng, app->colors.escape_palette);

                app->colors.escape_palette_size = rng;
                // Free temporary array
                free(palette);
            } else {
                LOG_WARN("Invalid \"colors | s\" argument.\n"
                        "Use: (colors | s) [Uint32 set_color] [int range > 0] [int 20 > size > 0] [Uint32 noSetcolors[size]]");
            }
        }
        else if (streq(cmd, "showpointer")) {
            if ( i + 1 < argc && streq(argv[i+1], "true") ) {
                app->show_pointer = true;
                i++;
            } else if ( i + 1 < argc && streq(argv[i+1], "false") ) {
                app->show_pointer = false;
                i++;
            } else {
                LOG_WARN("Invalid \"showpointer\" argument.\n"
                        "Use: showpointer [true | false]");
            }
        }
        else if (streq(cmd, "showinfo")) {
            if ( i + 1 < argc && streq(argv[i+1], "true") ) {
                app->show_info = true;
                i++;
            } else if ( i + 1 < argc && streq(argv[i+1], "false") ) {
                app->show_info = false;
                i++;
            } else {
                LOG_WARN("Invalid \"showinfo\" argument.\n"
                        "Use: (showinfo) [true | false]");
            }
        }
        else if (streq(cmd, "screenshot")) {
            if (i+1 < argc && !streq(argv[++i], "--default")) {
                save_image(app, argv[i]);
            } else {
                save_image(app, DEFAULT_IMAGE_FILENAME);
            }
        }
        else if (streq(cmd, "read")) {
            if (i+1 < argc && !streq(argv[++i], "--default")) {
                // Free previous allocated path
                free(app->read_path);
                app->read_path = copy_string(argv[i]);
                if(!app->read_path) {
                    LOG_WARN("Failed to allocate app->read_path:%s", argv[i]);
                    continue;
                } 
            } 
            read_status(app, app->read_path);
        }
        else if (streq(cmd, "write")) {
            if (i+1 < argc && !streq(argv[++i], "--default")) {
                save_status(app, argv[i]);
            } else {
                save_status(app, DEFAULT_APPSTATE_FILENAME);
            }
        }
        else if (streq(cmd, "exit")) {
            SDL_Event quit_event;
            quit_event.type = SDL_EVENT_QUIT;
            SDL_PushEvent(&quit_event);
            return SDL_APP_SUCCESS;
        }
        else if (cmd[0] == '#') {
            return SDL_APP_CONTINUE;
        }
        else {
            LOG_WARN("Unkown command \"%s\". Ignoring it.", cmd);
        }
    }
    return SDL_APP_CONTINUE;
}

