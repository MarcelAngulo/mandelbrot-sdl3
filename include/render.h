
#ifndef RENDER_H
#define RENDER_H

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <stdio.h>
#include "mandelbrot.h"

// COLOR_EXTRAPOLATE: calculate the rest of the colors using cos funtion
// COLOR_ARRAY: use only colors in array
// COLOR_MONO: use only one color which is as brighter as the escape
// number.
typedef struct {
    Uint32 setColor;
    Uint32 *noSetColors;
    int noSetSize;
} MandelbrotColors;
// Lops through every single pixel on the screen, calls the math
// module for the value, maps that value to a color, and updates
// the SDL texture or window surface.
SDL_Texture *renderText(SDL_Renderer *renderer, TTF_Font *font, char *text, SDL_Color color, float *w, float *h);

#endif
