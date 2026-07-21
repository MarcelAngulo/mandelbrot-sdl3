
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <stdbool.h>
#include "mandelbrot.h"
#include "render.h"

// 1. App State Structure
typedef struct {
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *fractal_texture;
    SDL_FRect fractal_rect;
    TTF_Font *ui_font;
    int w_width;
    int w_height;
    MandelbrotConfig config;
    bool running;
    bool update_fractal;
    bool show_info;
} AppState;

// 2. Helper Function Declarations
bool init_app(AppState *app, int width, int height);
void handle_events(AppState *app);
void render_all(AppState *app);
void parse_arguments(int argc, char *argv[], MandelbrotConfig *config);
void save_screenshot(SDL_Renderer *renderer);
void cleanup_app(AppState *app);

// 3. The main function
int main(int argc, char *argv[]) {
    AppState app = {0};

    // Set default config values first
    app.config = (MandelbrotConfig){
        .center = -0.75,
        .range = 3.0-3.0*I,
        .max_iterations = 40
    };

    // Ovveride defaults if the user passed command-line args
    parse_arguments(argc, argv, &app.config);

    // Initialize systems
    if(!init_app(&app, 800, 800)) {
        return 1;
    }


    // The main loop
    while(app.running){
        handle_events(&app);
        render_all(&app);
        SDL_Delay(32);
    }

    cleanup_app(&app);
    return 0;
}

// 4. function implementations

void parse_arguments(int argc, char *argv[], MandelbrotConfig *config){
    // TODO: Loop through argv to check for "-position" or "-colors"
    // set position, colors, scale
    // Use standard string comparion strcmp()
}

bool init_app(AppState *app, int width, int height) {
    if(!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL_Init Error: %s", SDL_GetError());
        return false;
    }

    if(!TTF_Init()) {
        SDL_Log("TTF_Init Error: %s", SDL_GetError());
        SDL_Quit();
        return false;
    }

    // Create window
    if (!SDL_CreateWindowAndRenderer("Mandelbrot Explorer",
                width, height, SDL_WINDOW_RESIZABLE,
                &app->window, &app->renderer)) {
        SDL_Log("Window/Renderer Error: %s", SDL_GetError());
        SDL_Quit();
        return false;
    }
    SDL_GetWindowSize(app->window, 
            &app->w_width, &app->w_height);
    // Create texture where fractal is drawn
    app->fractal_texture = SDL_CreateTexture(
            app->renderer,
            SDL_PIXELFORMAT_RGBA8888,
            SDL_TEXTUREACCESS_STREAMING,
            app->w_width, app->w_height
    );
    if(!app->fractal_texture) {
        SDL_Log("Texture Error: %s", SDL_GetError());
        SDL_Quit();
        return false;
    }

    app->ui_font = TTF_OpenFont(
            "assets/GoogleSansCode-Regular.ttf", 20.0f);
    if (!app->ui_font) {
        SDL_Log("Failed to load font: %s", SDL_GetError());
    }

    // Initialize rect of app->fractal_texture
    app->fractal_rect = (SDL_FRect) {
        .x = 0, .y= 0,
        .w = app->w_width, .h = app->w_height
    };
    draw_mandelbrot(app->fractal_texture, &app->config);

    app->update_fractal = false;
    app->running = true;
    app->show_info = true;
    return true;
}

void handle_events(AppState *app) {
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
                    app->fractal_rect = (SDL_FRect) {
                        .x = 0, .y = 0,
                        .w = app->w_width,
                        .h = app->w_height
                    };
                draw_mandelbrot(app->fractal_texture, &app->config);
                    break;

                // Move upwards
                case SDLK_UP:
                case SDLK_K:
                    app->fractal_rect.y +=
                    app->w_height * displacement;
                    app->config.center += 
                        -cimag(app->config.range)*displacement*I;
                    break;

                // Move downwards
                case SDLK_DOWN:
                case SDLK_J:
                    app->fractal_rect.y += 
                        -app->w_height * displacement;
                    app->config.center += 
                        cimag(app->config.range)*displacement*I;
                    break;

                // Move to the left 
                case SDLK_LEFT:
                case SDLK_H:
                    app->fractal_rect.x += 
                        app->w_width*displacement;
                    app->config.center +=
                        -creal(app->config.range)*displacement;
                    break;

                // Move to the Right
                case SDLK_RIGHT:
                case SDLK_L:
                    app->fractal_rect.x +=
                        -app->w_width*displacement;
                    app->config.center +=
                        creal(app->config.range)*displacement;
                    break;

                // Zoom in view
                case SDLK_PLUS:
                case SDLK_S:
                    app->config.range *= 1/zoom; 
                    app->fractal_rect.x = app->w_width/2 -
                        (app->w_width/2 - app->fractal_rect.x)*zoom;
                    app->fractal_rect.y = app->w_height/2 -
                       (app->w_height/2 - app->fractal_rect.y)*zoom;
                    app->fractal_rect.w *= zoom;
                    app->fractal_rect.h *= zoom;
                    break;

                // Zoom out view
                case SDLK_MINUS:
                case SDLK_A:
                    app->config.range *= zoom;
                    app->fractal_rect.x = app->w_width/2 -
                        (app->w_width/2 - app->fractal_rect.x)/zoom;
                    app->fractal_rect.y = app->w_height/2 -
                       (app->w_height/2 - app->fractal_rect.y)/zoom;
                    app->fractal_rect.w *= 1/zoom;
                    app->fractal_rect.h *= 1/zoom;
                    break;

                // Increase the number of max iterations per point
                case SDLK_P:
                    app->config.max_iterations++;
                    break;

                // Decrase number of max iterations per point.
                case SDLK_O:
                    app->config.max_iterations--;
                    if(app->config.max_iterations < 0)
                        app->config.max_iterations = 0;
                    break;

                // Toggle visibility of info window.
                case SDLK_I:
                    app->show_info = !app->show_info;
                    break;

                // When 'G' is pressed, save screenshot.
                case SDLK_G:
                    save_screenshot(app->renderer);
                    break;
                // TODO: Add key for saving image // to update config
            }
        } else if (event.type == SDL_EVENT_WINDOW_RESIZED) {
            // Update app state with new dimensions
            app->w_width = event.window.data1;
            app->w_height = event.window.data2;
            app->config.range = (1 - I*app->w_height/app->w_width)*
                    creal(app->config.range);

            if (app->fractal_texture) {
                SDL_DestroyTexture(app->fractal_texture);
            }

            app->fractal_texture = SDL_CreateTexture(
                    app->renderer,
                    SDL_PIXELFORMAT_RGBA8888,
                    SDL_TEXTUREACCESS_STREAMING,
                    app->w_width, app->w_height
            );

            app->fractal_rect = (SDL_FRect) {
                .x = 0, .y = 0,
                .w = app->w_width,
                .h = app->w_height
            };
            draw_mandelbrot(app->fractal_texture, &app->config);
        }
    }
}

void render_all(AppState *app) {
    SDL_SetRenderDrawColor(app->renderer, 40, 40, 40, 255);
    SDL_RenderClear(app->renderer);
    // Render drawn fractal on window
    SDL_RenderTexture(app->renderer, app->fractal_texture,
            NULL, &app->fractal_rect);
    // Render info text
    if (app->ui_font && app->show_info) {
        SDL_Color info_color = {255,255,255,255};
        char info_text[256];
        snprintf(info_text, sizeof(info_text),
                "Center: %.4f%+.4fi\nRange: %.4f%+.4fi"
                "\nMax Iter: %d\nStatus: %s",
                creal(app->config.center), cimag(app->config.center),
                creal(app->config.range), cimag(app->config.range),
                app->config.max_iterations,
                app->update_fractal? "Drawing..." : "Idle");
        float tw,th;
        SDL_Texture *info_texture = render_text(app->renderer, 
                app->ui_font, info_text, info_color, &tw, &th);
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

void save_screenshot(SDL_Renderer *renderer) {
    // TODO: Use SDL3 surface functions to read pixels from
    // renderer ad save out a .png image file
}

void cleanup_app(AppState *app) {
    if (app->ui_font) TTF_CloseFont(app->ui_font);
    if (app->renderer) SDL_DestroyRenderer(app->renderer);
    if (app->window) SDL_DestroyWindow(app->window);
    if (app->fractal_texture) SDL_DestroyTexture(app->fractal_texture);

    TTF_Quit();
    SDL_Quit();
}


