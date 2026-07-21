
#ifndef RENDER_H
#define RENDER_H

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <stdio.h>
#include "mandelbrot.h"

// Lops through every single pixel on the screen, calls the math
// module for the value, maps that value to a color, and updates
// the SDL texture or window surface.
void draw_mandelbrot(SDL_Texture *texture, const MandelbrotConfig *config);
SDL_Texture *render_text(SDL_Renderer *renderer, TTF_Font *font,
        char *text, SDL_Color color, float *w, float *h);

#endif
