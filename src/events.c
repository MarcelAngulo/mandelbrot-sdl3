
#include "events.h"
#include "render.h"
#include "file_io.h"
#include "commands.h"
#include "utils.h"

SDL_AppResult handle_explore_keys(AppState *app, SDL_Event *event) {
    // Displacement when pressing UP, DOWN, LEFT, RIGHT, h,j,k,l keys.
    switch (event->key.key) {
        // Close app
        case SDLK_Q:
            {
                SDL_Event quit_event;
                quit_event.type = SDL_EVENT_QUIT;
                SDL_PushEvent(&quit_event);
            }
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
            view_displace(app, 0, app->height*app->config.displacement);
            update_texture_info(app);
            break;

        // Move downwards
        case SDLK_DOWN:
        case SDLK_J:
            view_displace(app, 0, -app->height*app->config.displacement);
            update_texture_info(app);
            break;

        // Move to the left 
        case SDLK_LEFT:
        case SDLK_H:
            view_displace(app, -app->width*app->config.displacement, 0);
            update_texture_info(app);
            break;

        // Move to the Right
        case SDLK_RIGHT:
        case SDLK_L:
            view_displace(app, app->width*app->config.displacement, 0);
            update_texture_info(app);
            break;

        // Zoom in view
        case SDLK_PLUS:
        case SDLK_S:
            view_zoom(app, 1/app->config.zoom_ratio);
            update_texture_info(app);
            break;

        // Zoom out view
        case SDLK_MINUS:
        case SDLK_A:
            view_zoom(app, app->config.zoom_ratio);
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

    return SDL_APP_CONTINUE;
}

SDL_AppResult handle_command_keys(AppState *app, SDL_Event *event) {
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

    return SDL_APP_CONTINUE;
}
SDL_AppResult handle_text_input(AppState *app, SDL_Event *event) {
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
    return SDL_APP_CONTINUE;
}
SDL_AppResult handle_window_resize(AppState *app, SDL_Event *event){
    // Update app state with new dimensions

    view_resize(app, (double) event->window.data1/app->width,
        (double) event->window.data2/app->height);

    app->width = event->window.data1;
    app->height = event->window.data2;
    // Updates info texture
    update_texture_info(app);
    return SDL_APP_CONTINUE;
}
SDL_AppResult handle_mouse_motion(AppState *app, SDL_Event *event) {
    // change app->config.center position if user move
    // mouse whith left button pressed.
    if(event->motion.state & SDL_BUTTON_LMASK) {
        view_displace(app, -event->motion.xrel, event->motion.yrel);
        update_texture_info(app);
    }
    return SDL_APP_CONTINUE;
}

SDL_AppResult handle_mouse_wheel(AppState *app, SDL_Event *event) {
    // This part zoom in and zoom out when scrolling
    // with mouse wheeel
    float y = event->wheel.y;
    if ( y > 0 ) {
        // zoom in
        view_zoom(app, 1/app->config.zoom_ratio);
    } else if ( y < 0 ) {
        // zoom out
        view_zoom(app, app->config.zoom_ratio);
    }
    update_texture_info(app);
    return SDL_APP_CONTINUE;
}
