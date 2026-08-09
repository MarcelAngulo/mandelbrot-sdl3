
#ifndef EVENTS_H
#define EVENTS_H

#include <SDL3/SDL.h>
#include "app.h"

SDL_AppResult handle_explore_keys(AppState *app, SDL_Event *event);
SDL_AppResult handle_command_keys(AppState *app, SDL_Event *event);
SDL_AppResult handle_text_input(AppState *app, SDL_Event *event);
SDL_AppResult handle_mouse_wheel(AppState *app, SDL_Event *event);
SDL_AppResult handle_mouse_motion(AppState *app, SDL_Event *event);
SDL_AppResult handle_window_resize(AppState *app, SDL_Event *event);

#endif
