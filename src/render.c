
#include <stdio.h>
#include "app.h"
#include "render.h"
#include "mandelbrot.h"
#include "logger.h"

char modeStr[][10] = {
    "Explorer",
    "Command"
};
// Creates a texture with text
SDL_Texture *renderText(SDL_Renderer *renderer, TTF_Font *font, char *text, SDL_Color color, float *w, float *h) {
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

// Displace view in dxp, dyp pixels horizontally and vertically
void view_displace(AppState *app, double dxp, double dyp) {
    app->rect_fractal.y +=  dyp;
    app->config.center += -dyp*cimag(app->config.range)/app->height*I;
    app->rect_fractal.x += -dxp;
    app->config.center +=  dxp*creal(app->config.range)/app->width;
}
void view_zoom(AppState *app, double factor) {
    app->config.range *= factor;
    app->rect_fractal.x = app->width/2 -
        (app->width/2 - app->rect_fractal.x)/factor;
    app->rect_fractal.y = app->height/2 -
       (app->height/2 - app->rect_fractal.y)/factor;
    app->rect_fractal.w *= 1/factor;
    app->rect_fractal.h *= 1/factor;
}
void view_resize(AppState *app, double xratio, double yratio) {
    app->config_backup.range =app->config.range = creal(app->config.range)*xratio + cimag(app->config.range)*yratio*I;
    

    app->rect_fractal.x += app->width/2*(xratio-1);
    app->rect_fractal.y += app->height/2*(yratio-1);
}

void update_texture_fractal(AppState *app) {
    void *pixels;
    int pitch;
    float width,height;
    // Get size in pixels of texture
    SDL_GetTextureSize(app->texture_fractal, &width, &height);
    // Creates a new texture to render if old texture dimensions
    // are not equal to those of window
    if (width != app->width || height != app->height) {
        width = app->width;
        height = app->height;

        SDL_DestroyTexture(app->texture_fractal);
        app->texture_fractal = SDL_CreateTexture(
                app->renderer, SDL_PIXELFORMAT_RGBA8888,
                SDL_TEXTUREACCESS_STREAMING,
                app->width, app->height);
    }


    // Lock the texture to gain direct access to
    // its raw pixel buffer memory
    if (!SDL_LockTexture(app->texture_fractal, NULL, &pixels, &pitch) != 0) {
        LOG_WARN("Failed to lock texture: %s", SDL_GetError());
        return;
    }

    // A pitch is the number of bytes in one row of pixels. 
    // Since we use RGBA8888, each pixel is 4 bytes.
    Uint32 *pixel_buffer = (Uint32 *)pixels;
    COMPLEX_TYPE top_left = app->config.center - app->config.range/2;


    // 2. Loop through and write directly to the raw memory array
    for(int y = 0; y < height; y++) {
        for(int x = 0; x < width; x++) {
            double complex c = top_left +
                x*creal(app->config.range)/width +
                y*cimag(app->config.range)/height*I;

            // Calculate how many iterations are needed to c to escape
            // If r==config->max_iterations, then c possibly belongs to
            // Mandelbrot Set, if not, doesn't belong
            int r = calculateEscape(c, &app->config);

            int pixel_index = y * (pitch / 4) + x;
            if (r == app->config.maxIterations){
                pixel_buffer[pixel_index] = app->colors.set_color;
            } else {
                pixel_buffer[pixel_index] = app->colors.escape_palette[r % app->colors.escape_palette_size];
            }
        }
    }

    // 3. Unlock the texture to upload the finished image to the GPU
    SDL_UnlockTexture(app->texture_fractal);
    app->rect_fractal = (SDL_FRect) {
        .x = 0, .y = 0,
        .w = app->width,
        .h = app->height
    };
    app->config_backup = app->config;
}



void update_texture_command(AppState *app) {
    // Free previous texture
    SDL_DestroyTexture(app->texture_command);
    // Render command text
    app->texture_command = renderText(app->renderer,
            app->font, app->command_buffer, app->color_font,
            &app->rect_command.w, &app->rect_command.h);
    // Calculates new coordinates of command Rect text
    app->rect_command.x = (app->width < app->rect_command.w) ? app->width - app->rect_command.w : 0;
    app->rect_command.y = app->height - app->rect_command.h;
}
void update_texture_info(AppState *app) {
    if (app->show_info) {
        // Destroy previously allocated texture if it exists
        SDL_DestroyTexture(app->texture_info);
        // Format text into buffer
        char buffer[300];
        snprintf(buffer, sizeof(buffer),
                "Center: %.4f%+.4fi\n"
                "Range: %.4f%+.4fi\n"
                "Power: %d\n"
                "Max Iter: %d\n"
                "Status: %s\n"
                "Mode: %s",
                creal(app->config.center), cimag(app->config.center),
                creal(app->config.range), cimag(app->config.range),
                app->config.power,
                app->config.maxIterations,
                app->is_rendering? "Drawing..." : "Idle",
                modeStr[app->mode]);
        // Created texture
        app->texture_info = renderText(app->renderer, 
                app->font, buffer, app->color_font,
                &app->rect_info.w, &app->rect_info.h);
        if (!app->texture_info) {
            LOG_ERROR("Failed to render info text: %s", SDL_GetError());
        }
        app->rect_info.x = app->rect_info.y = 0.0f;
    }
}

void render_all(AppState *app) {
    // clear screen
    SDL_SetRenderDrawColor(app->renderer, 40, 40, 40, 255);
    SDL_RenderClear(app->renderer);
    // Render drawn fractal on window
    SDL_RenderTexture(app->renderer, app->texture_fractal,
            NULL, &app->rect_fractal);
    // Render info text
    if (app->font && app->show_info) {
        SDL_SetRenderDrawColor(app->renderer, 0, 0, 0, 255);
        SDL_RenderFillRect(app->renderer, &app->rect_info);
        SDL_RenderTexture(app->renderer,
                app->texture_info, NULL, &app->rect_info);
    }

    // Render Pointer at the center of screen
    if(app->show_pointer) {
        float crossSize = SDL_min(app->width, app->height)/20;
        float halfWidth = app->width/2;
        float halfHeight = app->height/2;
        float lineWidth = 4;
        SDL_SetRenderDrawColor(app->renderer,
                0, 0, 0, SDL_ALPHA_OPAQUE);
        for(int i = -lineWidth/2; i < 0; i++) {
            SDL_RenderLine(app->renderer,
                    halfWidth+i, halfHeight - crossSize,
                    halfWidth+i, halfHeight + crossSize);
            SDL_RenderLine(app->renderer,
                    halfWidth - crossSize, halfHeight+i,
                    halfWidth + crossSize, halfHeight+i);
        }
        SDL_SetRenderDrawColor(app->renderer,
                255, 255, 255, SDL_ALPHA_OPAQUE);
        for(int i = 0; i < lineWidth/2; i++) {
            SDL_RenderLine(app->renderer,
                    halfWidth+i, halfHeight - crossSize,
                    halfWidth+i, halfHeight + crossSize);
            SDL_RenderLine(app->renderer,
                    halfWidth - crossSize, halfHeight+i,
                    halfWidth + crossSize, halfHeight+i);
        }
    }

    // Render help window
    if (app->show_help) {
        float pad = 10;
        // Creates an auxiliary rect to fill with padding
        SDL_FRect r = {
            .x = app->rect_help.x - pad,
            .y = app->rect_help.y - pad,
            .w = app->rect_help.w + 2*pad,
            .h = app->rect_help.h + 2*pad
        };
        SDL_SetRenderDrawColor(app->renderer,
                0, 0, 0, 255);
        SDL_RenderFillRect(app->renderer, &r);
        SDL_SetRenderDrawColor(app->renderer,
                0xff, 0xff, 0xff, 255);
        SDL_RenderTexture(app->renderer, app->texture_help,
                NULL, &app->rect_help);
    }

    // Render command bar at the bottom of the window
    if (app->mode == MODE_COMMAND) {
        SDL_FRect rr = {
            .x = 0, .y = app->height - app->rect_command.h,
            .w = app->width, .h=app->rect_command.h
        };
        SDL_SetRenderDrawColor(app->renderer,
                0, 0, 0, 255);
        SDL_RenderFillRect(app->renderer, &rr);
        SDL_RenderTexture(app->renderer,
                app->texture_command, NULL, &app->rect_command);
    }

    SDL_RenderPresent(app->renderer);
}
