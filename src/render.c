
#include "render.h"
#include "mandelbrot.h"

void draw_mandelbrot(SDL_Texture *texture, const MandelbrotConfig *config) {
    void *pixels;
    int pitch;
    float width,height;
    // Get size in pixels of texture
    SDL_GetTextureSize(texture, &width, &height);

    // 1. Lock the texture to gain direct access
    // to its raw pixel buffer memory
    if (!SDL_LockTexture(texture, NULL, &pixels, &pitch) != 0) {
        SDL_Log("Failed to lock texture: %s", SDL_GetError());
        return;
    }

    // A pitch is the number of bytes in one row of pixels. 
    // Since we use RGBA8888, each pixel is 4 bytes.
    Uint32 *pixel_buffer = (Uint32 *)pixels;
    COMPLEX_TYPE top_left = config->center - config->range/2;


    // 2. Loop through and write directly to the raw memory array
    for(int y = 0; y < height; y++) {
        for(int x = 0; x < width; x++) {
            double complex c = top_left +
                x*creal(config->range)/width +
                y*cimag(config->range)/height*I;

            // Calculate how many iterations are needed to c to escape
            // If r==config->max_iterations, then c possibly belongs to
            // Mandelbrot Set, if not, doesn't belong
            int r = calculate_escape(c, config);

            Uint8 red, green, blue;

            // Determine pixel color based on the escape number.
            if(r == config->max_iterations) {
                red = 0; green = 0; blue = 0; // Inside set = Black
            } else {
                red = 255; green = 255; blue = 255;
            }

            // Pack the channels into a single 32-bit integer
            // (RGBA8888 layout) pitch / 4 handles the exact
            // alignment memory offset per row
            int pixel_index = y * (pitch / 4) + x;
            pixel_buffer[pixel_index] =
                (red << 24) | (green << 16) | (blue << 8) | 0xFF;
        }
    }

    // 3. Unlock the texture to upload the finished image to the GPU
    SDL_UnlockTexture(texture);
}

// Creates a texrture with text
SDL_Texture *render_text(SDL_Renderer *renderer, TTF_Font *font,
        char *text, SDL_Color color, float *w, float *h) {
    SDL_Surface *text_surface = TTF_RenderText_Blended_Wrapped(
            font, text, 0, color, 0);
    SDL_Texture *text_texture = NULL;

    if (text_surface) {
        text_texture = SDL_CreateTextureFromSurface(
                renderer, text_surface);
        *w = (float)text_surface->w;
        *h = (float)text_surface->h;
        SDL_DestroySurface(text_surface);
    }
    return text_texture;
}
