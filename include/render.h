
#ifndef RENDER_H
#define RENDER_H

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include "app.h"

// Lops through every single pixel on the screen, calls the math
// module for the value, maps that value to a color, and updates
// the SDL texture or window surface.
SDL_Texture *renderText(SDL_Renderer *renderer, TTF_Font *font, char *text, SDL_Color color, float *w, float *h);

void render_all(AppState *app);

void update_texture_info(AppState *app);
void update_texture_command(AppState *app);
void update_texture_fractal(AppState *app);

void view_displace(AppState *app, double dxp, double dyp);
void view_zoom(AppState *app, double factor);
void view_resize(AppState *app, double xratio, double yratio);


#endif
