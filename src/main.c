
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <stdbool.h>
#include <stdlib.h>
#include "mandelbrot.h"
#include "render.h"

// 1. App State Structure
typedef struct {
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *fractalTexture;
    SDL_FRect fractalRect;
    TTF_Font *uiFont;
    int width;
    int height;
    MandelbrotConfig config;
    MandelbrotColors colors;
    bool running;
    bool updateFractal;
    bool show_info;
} AppState;

// 2. Helper Function Declarations
bool initApp(AppState *app);
void handleEvents(AppState *app);
void renderAll(AppState *app);
void restartFractalRect(AppState *app);
void parseArguments(AppState *app, int argc, char *argv[]);
void saveScreenshot(SDL_Renderer *renderer);
void cleanupApp(AppState *app);

// 3. The main function
int main(int argc, char *argv[]) {
    AppState app = {0};
    // Initialize systems
    if(!initApp(&app))
        return 1; //Exit on erro

    // Ovveride defaults if the user passed command-line args
    parseArguments(&app, argc, argv);

    // The main loop
    while(app.running){
        handleEvents(&app);
        renderAll(&app);
        SDL_Delay(32);
    }

    cleanupApp(&app);
    return 0;
}

// 4. function implementations
void parseArguments(AppState *app, int argc, char *argv[]){
    // TODO: Loop through argv to check for "-position" or "-colors"
    // set position, colors, scale
    // Use standard string comparion strcmp()
}

bool initApp(AppState *app) {
    // Init mandelbrot config
    app->config = (MandelbrotConfig){
        .center = -0.75,
        .range = 3.0-3.0*I,
        .maxIterations = 40
    };

    app->colors.mode = COLOR_EXTRAPOLATE;
    app->colors.setColor = 0x000000FF; // Solid black for mandelbrot set
    app->colors.noSetSize = 40;
    app->colors.noSetColors = (Uint32 *) malloc(sizeof(Uint32) * app->colors.noSetSize);
    if(!app->colors.noSetColors) {
        SDL_Log("Failed to allocate memory for app->colors.noSetColors");
    }
    int defaultPalette[][3] = {
        {0xdb, 0x07, 0x3d},
        {0xdb, 0xa5, 0x07},
        {0x8e, 0xc7, 0xd2},
        {0x0d, 0x69, 0x86},
        {0x07, 0x48, 0x5b},
        {0xdb, 0x07, 0x3d}
    };
    int defsz = sizeof(defaultPalette) / sizeof(defaultPalette[0]);
    int section_width = (int) app->colors.noSetSize / (defsz-1);
    for (int x = 0; x < app->colors.noSetSize; x++) {
        int i = (int) x / section_width;
        double p = (SDL_cos((double) (x%section_width) / section_width * SDL_PI_D ) + 1.0) /2.0;
        SDL_Log("-> %i % 2.3f\n", i, p);
        int r1 = defaultPalette[i][0];
        int g1 = defaultPalette[i][1];
        int b1 = defaultPalette[i][2];
        int r2 = defaultPalette[i+1][0];
        int g2 = defaultPalette[i+1][1];
        int b2 = defaultPalette[i+1][2];
        int r = r2 - (r2-r1)*p;
        int g = g2 - (g2-g1)*p;
        int b = b2 - (b2-b1)*p;
        SDL_Log(" %+ 3i %+ 3i %+ 3i\n", r, g, b);
        app->colors.noSetColors[x] = (r << 24) | (g << 16) | (b << 8) | 0xFF;
    }

    // Init SDL Library
    if(!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL_Init Error: %s", SDL_GetError());
        return false;
    }

    // Init TTF for rendering fonts
    if(!TTF_Init()) {
        SDL_Log("TTF_Init Error: %s", SDL_GetError());
        SDL_Quit();
        return false;
    }

    // Create window
    if (!SDL_CreateWindowAndRenderer("Mandelbrot Explorer",
                800, 800, SDL_WINDOW_RESIZABLE,
                &app->window, &app->renderer)) {
        SDL_Log("Window/Renderer Error: %s", SDL_GetError());
        SDL_Quit();
        return false;
    }
    SDL_GetWindowSize(app->window, 
            &app->width, &app->height);
    // Create texture where fractal is drawn
    app->fractalTexture = SDL_CreateTexture(
            app->renderer,
            SDL_PIXELFORMAT_RGBA8888,
            SDL_TEXTUREACCESS_STREAMING,
            app->width, app->height
    );
    if(!app->fractalTexture) {
        SDL_Log("Texture Error: %s", SDL_GetError());
        SDL_Quit();
        return false;
    }

    // Open font used in the program
    app->uiFont = TTF_OpenFont(
            "assets/GoogleSansCode-Regular.ttf", 20.0f);
    if (!app->uiFont) {
        SDL_Log("Failed to load font: %s", SDL_GetError());
    }

    // Initialize rect of app->fractalTexture
    restartFractalRect(app);
    drawMandelbrot(app->fractalTexture, &app->config, &app->colors);

    app->updateFractal = false;
    app->running = true;
    app->show_info = true;
    return true;
}

void handleEvents(AppState *app) {
    SDL_Event event;
    // Displacement when pressing UP, DOWN, LEFT, RIGHT, h,j,k,l keys.
    float displacement = 0.05;
    float zoom = 1.05;
    // Draw fractal for the first time.

    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT) {
            app->running = false;
        }
        else if (event.type == SDL_EVENT_KEY_DOWN) {
            switch (event.key.key) {
                // Close app
                case SDLK_Q:
                case SDLK_ESCAPE:
                    app->running = false;
                    break;

                // Update view
                case SDLK_SPACE:
                case SDLK_RETURN:
                    restartFractalRect(app);
                    drawMandelbrot(app->fractalTexture, &app->config, &app->colors);
                    break;

                // Move upwards
                case SDLK_UP:
                case SDLK_K:
                    app->fractalRect.y +=
                    app->height * displacement;
                    app->config.center += 
                        -cimag(app->config.range)*displacement*I;
                    break;

                // Move downwards
                case SDLK_DOWN:
                case SDLK_J:
                    app->fractalRect.y += 
                        -app->height * displacement;
                    app->config.center += 
                        cimag(app->config.range)*displacement*I;
                    break;

                // Move to the left 
                case SDLK_LEFT:
                case SDLK_H:
                    app->fractalRect.x += 
                        app->width*displacement;
                    app->config.center +=
                        -creal(app->config.range)*displacement;
                    break;

                // Move to the Right
                case SDLK_RIGHT:
                case SDLK_L:
                    app->fractalRect.x +=
                        -app->width*displacement;
                    app->config.center +=
                        creal(app->config.range)*displacement;
                    break;

                // Zoom in view
                case SDLK_PLUS:
                case SDLK_S:
                    app->config.range *= 1/zoom; 
                    app->fractalRect.x = app->width/2 -
                        (app->width/2 - app->fractalRect.x)*zoom;
                    app->fractalRect.y = app->height/2 -
                       (app->height/2 - app->fractalRect.y)*zoom;
                    app->fractalRect.w *= zoom;
                    app->fractalRect.h *= zoom;
                    break;

                // Zoom out view
                case SDLK_MINUS:
                case SDLK_A:
                    app->config.range *= zoom;
                    app->fractalRect.x = app->width/2 -
                        (app->width/2 - app->fractalRect.x)/zoom;
                    app->fractalRect.y = app->height/2 -
                       (app->height/2 - app->fractalRect.y)/zoom;
                    app->fractalRect.w *= 1/zoom;
                    app->fractalRect.h *= 1/zoom;
                    break;

                // Increase the number of max iterations per point
                case SDLK_P:
                    app->config.maxIterations++;
                    break;

                // Decrase number of max iterations per point.
                case SDLK_O:
                    app->config.maxIterations--;
                    if(app->config.maxIterations < 0)
                        app->config.maxIterations = 0;
                    break;

                // Toggle visibility of info window.
                case SDLK_I:
                    app->show_info = !app->show_info;
                    break;

                // When 'G' is pressed, save screenshot.
                case SDLK_G:
                    saveScreenshot(app->renderer);
                    break;
                // TODO: Add key for saving image // to update config
            }
        } else if (event.type == SDL_EVENT_WINDOW_RESIZED) {
            // Update app state with new dimensions
            app->width = event.window.data1;
            app->height = event.window.data2;
            app->config.range = (1 - I*app->height/app->width)*
                    creal(app->config.range);

            if (app->fractalTexture) {
                SDL_DestroyTexture(app->fractalTexture);
            }

            app->fractalTexture = SDL_CreateTexture(
                    app->renderer,
                    SDL_PIXELFORMAT_RGBA8888,
                    SDL_TEXTUREACCESS_STREAMING,
                    app->width, app->height
            );

            restartFractalRect(app);
            drawMandelbrot(app->fractalTexture, &app->config, &app->colors);
        }
    }
}

void renderAll(AppState *app) {
    // clear screen
    SDL_SetRenderDrawColor(app->renderer, 40, 40, 40, 255);
    SDL_RenderClear(app->renderer);
    // Render drawn fractal on window
    SDL_RenderTexture(app->renderer, app->fractalTexture,
            NULL, &app->fractalRect);
    // Render info text
    if (app->uiFont && app->show_info) {
        SDL_Color info_color = {255,255,255,255};
        char info_text[256];
        snprintf(info_text, sizeof(info_text),
                "Center: %.4f%+.4fi\nRange: %.4f%+.4fi"
                "\nMax Iter: %d\nStatus: %s",
                creal(app->config.center), cimag(app->config.center),
                creal(app->config.range), cimag(app->config.range),
                app->config.maxIterations,
                app->updateFractal? "Drawing..." : "Idle");
        float tw,th;
        SDL_Texture *info_texture = renderText(app->renderer, 
                app->uiFont, info_text, info_color, &tw, &th);
        if (!info_texture) {
            SDL_Log("Couldn't render text: %s", SDL_GetError());
        }
        SDL_FRect dest_rect = {
            0.0f, 0.0f, tw, th
        };
        SDL_SetRenderDrawColor(app->renderer, 0, 0, 0, 255);
        SDL_RenderFillRect(app->renderer, &dest_rect);
        SDL_RenderTexture(app->renderer, info_texture, NULL,&dest_rect);
        SDL_DestroyTexture(info_texture);
    }

    SDL_RenderPresent(app->renderer);
    // Makes ~30 FPS
}
void restartFractalRect(AppState *app) {
    app->fractalRect = (SDL_FRect) {
        .x = 0, .y = 0,
        .w = app->width,
        .h = app->height
    };
}

void saveScreenshot(SDL_Renderer *renderer) {
    // TODO: Use SDL3 surface functions to read pixels from
    // renderer ad save out a .png image file
}

void cleanupApp(AppState *app) {
    if(!app->colors.noSetColors) free(app->colors.noSetColors);
    if (app->uiFont) TTF_CloseFont(app->uiFont);
    if (app->renderer) SDL_DestroyRenderer(app->renderer);
    if (app->window) SDL_DestroyWindow(app->window);
    if (app->fractalTexture) SDL_DestroyTexture(app->fractalTexture);

    TTF_Quit();
    SDL_Quit();
}


