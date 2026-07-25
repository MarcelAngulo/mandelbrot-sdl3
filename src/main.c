
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <SDL3_image/SDL_image.h>
#include <stdbool.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <time.h>
#include "mandelbrot.h"
#include "render.h"

#define streq(s1,s2) (strcmp(s1, s2) == 0)
/* TODO LIST:

 * 8) Implement command line (to set commands)
 * 9) Implement to read and save states
 *
 * 10) Write README.md, license
 */

char helpStr[] =
"--------------| HELP |---------------\n"
"z  F1      . help\n"
"x          . return to last update\n"
"a          . zoom out\n"
"s          . zoom in\n"
"q  ESCAPE  . quit app\n"
"o  -       . decrease max iterations\n"
"p  +       . increase max iterations\n"
"g          . save screenshot\n"
"h  LEFT    . move left\n"
"l  RIGHT   . move right\n"
"j  DOWN    . move down\n"
"k  UP      . move up\n"
"i          . toogle info view\n"
"c          . toogle pointer view\n"
"f          . fullscreen\n"
"------------------------------------";

// 1. App State Structure
typedef struct {
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *fractalTexture;
    SDL_Texture *helpTexture;
    SDL_FRect fractalRect;
    SDL_FRect helpRect;
    TTF_Font *uiFont;
    int width;
    int height;
    MandelbrotConfig config;
    MandelbrotConfig configCopy;
    MandelbrotColors colors;
    bool running;
    bool updateFractal;
    bool showInfo;
    bool showPointer;
    bool showHelp;
    char *file;
    char *outDirectory;
} AppState;

// 2. Helper Function Declarations
bool initApp(AppState *app);
bool parseArguments(AppState *app, int argc, char *argv[]);
void handleEvents(AppState *app);
void renderAll(AppState *app);
void restartFractalRect(AppState *app);
void saveScreenshot(AppState *app);
void cleanupApp(AppState *app);
void calculateColors(int size, Uint32 *base, int range, Uint32 *dest);
bool parseDouble(const char *str, double *value);
bool parseLong(const char *str, long int *value, long int base);
bool parseColor(const char *str, Uint32 *value, int base);

// 3. The main function
int main(int argc, char *argv[]) {
    AppState app = {0};
    // Initialize systems
    if(initApp(&app))
        goto exitError;

    // Ovveride defaults if the user passed command-line args
    if (parseArguments(&app, argc, argv))
        goto exitError;

    // The main loop
    while(app.running){
        handleEvents(&app);
        renderAll(&app);
        SDL_Delay(32);
    }

    cleanupApp(&app);
    return EXIT_SUCCESS;
exitError:
    cleanupApp(&app);
    return EXIT_FAILURE;
}

void calculateColors(int size, Uint32 *base, int range, Uint32 *dest) {
    int delta = (int) range / (size-1);
    for (int x = 0; x < range; x++) {
        int i = (int) x / delta;
        double p = (SDL_cos((double) (x%delta) / delta * SDL_PI_D ) + 1.0) /2.0;
        int r1 = (base[i] >> 24) & 0xff;
        int g1 = (base[i] >> 16) & 0xff;
        int b1 = (base[i] >>  8) & 0xff;
        int r2 = (base[i+1] >> 24) & 0xff;
        int g2 = (base[i+1] >> 16) & 0xff;
        int b2 = (base[i+1] >>  8) & 0xff;
        int r = r2 - (r2-r1)*p;
        int g = g2 - (g2-g1)*p;
        int b = b2 - (b2-b1)*p;
        dest[x] = (r << 24) | (g << 16) | (b << 8) | 0xFF;
    }
}


// 4. function implementations
bool parseArguments(AppState *app, int argc, char *argv[]){
    // ### CONFIG ###
    // --center [double] (double)
    // --range [double] 
    // --power [double] (double)
    // --window [int > 0] [int > 0]
    // --maxIter [N]
    // ### COLORS ###
    // --colors [Uint32 set] [long int range] [long int N] [Uint32 noSet[N]]
    // --setColor [N] [0x------]
    // ### STRING ###
    // --dir [str(/)]
    // --open [dir/filename]
    // ### BOOLEAN ###
    // --showPointer
    // --hidePointer
    // --showInfo
    // --hideInfo
    // --fullScreen
    // --help | -h
    for (int i = 1; i < argc; i++) {
        char *arg = argv[i];
        SDL_Log("Parsing %i %s\n", i, argv[i]);
        if (streq(arg, "--center") || streq(arg, "-c")) {
            double re, im;
            if ( i + 2 < argc &&
                parseDouble(argv[++i], &re) &&
                parseDouble(argv[++i], &im)) {
                app->config.center = app->configCopy.center = re + im*I;
            } else {
                SDL_Log("Invalid \"--center | -c\" argument.\n"
                        "Use: (--center | -r) [double re] [double im]\n");
            }
        }
        else if (streq(arg, "--range") || streq(arg, "-r")) {
            double range;
            if ( i + 1 < argc && parseDouble(argv[++i], &range)) {
                app->config.range = (1 - I*app->height/app->width)*range;
            } else {
                SDL_Log("Invalid \"--range | -r\" argument.\n"
                        "Use: (--range | -r) [double re]\n");
            }
        }
        else if (streq(arg, "--window") || streq(arg, "-w")) {
            long int width, height;
            if ( i + 2 < argc &&
                parseLong(argv[++i], &width, 10) && width > 0 &&
                parseLong(argv[++i], &height, 10) && height > 0) {
                app->config.range = (1 - I*height/width)*creal(app->config.range);
                app->width = width;
                app->height = height;
                SDL_SetWindowSize(app->window, width, height);
            } else {
                SDL_Log("Invalid \"--window | -w\" argument.\n"
                        "Use: (--window | -w) [long int width > 0] [long int height > 0]\n");
            }
        }
        else if (streq(arg, "--maxIter") || streq(arg, "-i")) {
            long int maxIter;
            if ( i + 1 < argc && parseLong(argv[++i], &maxIter, 10) && maxIter > 0) {
                app->config.maxIterations = app->configCopy.maxIterations = maxIter;
            } else {
                SDL_Log("Invalid \"--maxIter | -i\" argument.\n"
                        "Use: (--maxIter | -i)) [long int iters > 0]\n");
            }
        }
        else if (streq(arg, "--power") || streq(arg, "-p")) {
            double re, im;
            if ( i + 2 < argc &&
                parseDouble(argv[++i], &re) &&
                parseDouble(argv[++i], &im)) {
                app->config.power = app->configCopy.power = re + im*I;
            } else {
                SDL_Log("Invalid \"--power | -p\" argument.\n"
                        "Use: (--powep | -r) [double re] [double im]\n");
            }
        }
        else if (streq(arg, "--colors") || streq(arg, "-s")) {
            Uint32 setColor;
            long int size, rng;
            // Parse first 3 arguments
            if ( i + 3 < argc &&
                parseColor(argv[++i], &setColor, 16) &&
                parseLong(argv[++i], &rng, 10) && rng > 0 &&
                parseLong(argv[++i], &size, 10) && size > 0 && i + size <  argc) {
                app->colors.setColor = setColor;
            } else {
                SDL_Log("Invalid \"--colors | -s\" argument.\n"
                        "Use: (--colors | -s) [Uint32 setColor] [int range > 0] [int size > 0] [Uint32 noSetcolors[size]]\n");
            }
            int end = ++i + size;
            // Now will parse the colors to store in app->colorS.noSetColors
            Uint32 *palette = (Uint32 *) malloc(sizeof(Uint32) *size);
            if (!palette) {
                SDL_Log("Could not allocate memory for *palette in parseArguments: "
                        "%s\n", strerror(errno));
            }
            // Save colors in a temporary array
            for (int k = 0; i < end; i++, k++) {
                parseColor(argv[i], &palette[k], 16);
            }
            i--;

            // Ask for new memory
            app->colors.noSetColors = (Uint32 *) malloc(sizeof(Uint32) * rng);
            if(!app->colors.noSetColors) {
                SDL_Log("Could not allocate memory for app->colors.noSetColors in parseArguments: "
                        "%s\n", strerror(errno));
                free(palette);
                return EXIT_FAILURE;
            }

            // Stores new colors
            calculateColors(size, palette,
                    rng, app->colors.noSetColors);

            app->colors.noSetSize = rng;
            // Free temporary array
            free(palette);
            // Uint32 *noSetColor = (Uint32) malloc(sizeof(Uint32) * rng);
        }
        else if (streq(arg, "--showPointer")) {
            app->showPointer = true;
        }
        else if (streq(arg, "--showInfo")) {
            app->showInfo = true;
        }
        else if (streq(arg, "--hidePointer")) {
            app->showPointer = false;
        }
        else if (streq(arg, "--hideInfo")) {
            app->showInfo = false;
        }
        else if (streq(arg, "--fullscreen")) {
            SDL_SetWindowFullscreen(app->window, true);
        }
    }

    return EXIT_SUCCESS;
}
// Parse double, return true on success and false on failure.
bool parseDouble(const char *str, double *value) {
    if (str == NULL || *str == '\0') return false;

    char *endptr;
    errno = 0; // Reset thread-global error flag
    
    double val = strtod(str, &endptr);

    // Check if (1) if a number was found
    // (2) if there are non numeric numbers after
    // (3)check for underflow, overflow errors.
    if (endptr == str || *endptr != '\0' || errno != 0) {
        return false; // String format is invalid.
    }
    *value = val;
    return true;
}
// Parse Long, return true on success and false on failure.
bool parseLong(const char *str, long int *value, long int base) {
    if (str == NULL || *str == '\0') return false;

    char *endptr;
    errno = 0; // Reset thread-global error flag
    
    long int val = strtol(str, &endptr, base);

    // Check if (1) if a number was found
    // (2) if there are non numeric numbers after
    // (3)check for underflow, overflow errors.
    if (endptr == str || *endptr != '\0' || errno != 0) {
        return false; // String format is invalid.
    }
    *value = val;
    return true;
}
// Parse unsigned Long, return true on success and false on failure.
bool parseColor(const char *str, Uint32 *value, int base) {
    if (str == NULL || *str == '\0') return false;

    char *endptr;
    errno = 0; // Reset thread-global error flag
    
    unsigned long long val = strtoull(str, &endptr, base);

    // Check if (1) if a number was found
    // (2) if there are non numeric numbers after
    // (3)check for underflow, overflow errors.
    if (endptr == str || *endptr != '\0' || errno != 0) {
        return false; // String format is invalid.
    }
    // automatically add alpha opace
    *value = (Uint32) (val << 8) | 0xFF ; 
    return true;
}

bool initApp(AppState *app) {
    // Init mandelbrot config
    app->config = (MandelbrotConfig){
        .center = -0.75,
        .range = 3.0-3.0*I,
        .power = 2.0,
        .maxIterations = 20
    };
    app->configCopy = app->config;

    // Solid black for mandelbrot set
    app->colors.setColor = 0x000000FF; 
    // Colors of the points that don't belong to mandelbrot set.
    app->colors.noSetSize = 50;
    app->colors.noSetColors = (Uint32 *) malloc(sizeof(Uint32) * app->colors.noSetSize);
    if(!app->colors.noSetColors) {
        SDL_Log("Failed to allocate memory for app->colors.noSetColors: %s", strerror(errno));
        return EXIT_FAILURE;
    }
    Uint32 defaultPalette[] = {
        0xdb073dff, 0xdba507ff, 0x8ec7d2ff, 0x0d6986ff, 0x07485bff, 0xdb073dff
    };
    int defsz = sizeof(defaultPalette) / sizeof(Uint32);
    calculateColors(defsz, defaultPalette,
            app->colors.noSetSize, app->colors.noSetColors);

    // Init SDL Library
    if(!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL_Init Error: %s", SDL_GetError());
        return EXIT_FAILURE;
    }

    // Init TTF for rendering fonts
    if(!TTF_Init()) {
        SDL_Log("TTF_Init Error: %s", SDL_GetError());
        return EXIT_FAILURE;
    }

    // Create window
    if (!SDL_CreateWindowAndRenderer("Mandelbrot Explorer",
                800, 800, SDL_WINDOW_RESIZABLE,
                &app->window, &app->renderer)) {
        SDL_Log("Window/Renderer Error: %s", SDL_GetError());
        return EXIT_FAILURE;
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
        return EXIT_FAILURE;
    }

    // Open font used in the program
    app->uiFont = TTF_OpenFont(
            "assets/GoogleSansCode-Regular.ttf", 20.0f);
    if (!app->uiFont) {
        SDL_Log("Failed to load font: %s", SDL_GetError());
    }

    // Render help texture
    SDL_Color white = {0xff, 0xff, 0xff, 0xff};
    app->helpTexture = renderText(app->renderer,
            app->uiFont, helpStr, white,
            &app->helpRect.w, &app->helpRect.h);
    app->helpRect.x = (app->width - app->helpRect.w)/2;
    app->helpRect.y = (app->height - app->helpRect.h)/2;

    // Creates default output path
    char *base = SDL_GetBasePath();
    if (!base) {
        SDL_Log("Failed to allocate memory: %s", SDL_GetError());
    }
    char out[] = "../output/";
    app->outDirectory = (char *) malloc(sizeof(char) * (strlen(out) + strlen(base) + 1));
    if (!app->outDirectory) {
        SDL_Log("Failed to allocate memory: %s", strerror(errno));
        return EXIT_FAILURE;
    }
    strcpy(app->outDirectory, base);
    strcat(app->outDirectory, out);

    // Initialize rect of app->fractalTexture
    //restartFractalRect(app);
    //drawMandelbrot(app->fractalTexture, &app->config, &app->colors);

    app->updateFractal = false;
    app->running = true;
    app->showInfo = true;
    app->showPointer = true;
    app->showHelp = false;
    return EXIT_SUCCESS;
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

                case SDLK_Z:
                case SDLK_F1:
                    app->showHelp = !app->showHelp;
                    break;

                // Update view
                case SDLK_SPACE:
                case SDLK_RETURN:
                    app->configCopy = app->config;
                    app->updateFractal = true;
                    renderAll(app);
                    restartFractalRect(app);
                    drawMandelbrot(app->fractalTexture, &app->config, &app->colors);
                    app->updateFractal = false;
                    renderAll(app);
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
                    app->showInfo = !app->showInfo;
                    break;

                // Toggle visibility of cross at the middle.
                case SDLK_C:
                    app->showPointer = !app->showPointer;
                    break;

                // Change to state of last updating.
                case SDLK_X:
                    app->config = app->configCopy;
                    restartFractalRect(app);
                    break;

                case SDLK_F:
                    SDL_SetWindowFullscreen(app->window,
                        !(SDL_GetWindowFlags(app->window) & SDL_WINDOW_FULLSCREEN));
                    break;

                // When 'G' is pressed, save screenshot.
                case SDLK_G:
                    saveScreenshot(app);
                    break;
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

            app->configCopy = app->config;
            app->updateFractal = true;
            app->helpRect.x = (app->width - app->helpRect.w)/2;
            app->helpRect.y = (app->height - app->helpRect.h)/2;
            renderAll(app);
            restartFractalRect(app);
            drawMandelbrot(app->fractalTexture, &app->config, &app->colors);
            app->updateFractal = false;
            renderAll(app);
        } else if (event.type == SDL_EVENT_MOUSE_MOTION) {
            if(event.motion.state & SDL_BUTTON_LMASK) {
                float xrel =  event.motion.xrel;
                float yrel =  event.motion.yrel;
                    app->fractalRect.x += xrel;
                    app->config.center += xrel*-creal(app->config.range)/app->width;
                    app->fractalRect.y += yrel;
                    app->config.center += yrel * -cimag(app->config.range)*I/app->height;
            }
        } else if (event.type == SDL_EVENT_MOUSE_WHEEL) {
            float y = event.wheel.y;
            if ( y > 0 ) {
                app->config.range *= 1/zoom; 
                app->fractalRect.x = app->width/2 -
                    (app->width/2 - app->fractalRect.x)*zoom;
                app->fractalRect.y = app->height/2 -
                   (app->height/2 - app->fractalRect.y)*zoom;
                app->fractalRect.w *= zoom;
                app->fractalRect.h *= zoom;
            } else if ( y < 0 ) {
                app->config.range *= zoom;
                app->fractalRect.x = app->width/2 -
                    (app->width/2 - app->fractalRect.x)/zoom;
                app->fractalRect.y = app->height/2 -
                   (app->height/2 - app->fractalRect.y)/zoom;
                app->fractalRect.w *= 1/zoom;
                app->fractalRect.h *= 1/zoom;
            }
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
    if (app->uiFont && app->showInfo) {
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
        SDL_FRect destRect = { 0.0f, 0.0f, tw, th };
        SDL_SetRenderDrawColor(app->renderer, 0, 0, 0, 255);
        SDL_RenderFillRect(app->renderer, &destRect);
        SDL_RenderTexture(app->renderer, info_texture, NULL,&destRect);
        SDL_DestroyTexture(info_texture);
    }

    // Render cross
    if(app->showPointer) {
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

    if (app->showHelp) {
        float pad = 10;
        SDL_FRect r = app->helpRect;
        r.x -= pad;
        r.y -= pad;
        r.w += 2*pad;
        r.h += 2*pad;
        SDL_SetRenderDrawColor(app->renderer,
                0, 0, 0, 240);
        SDL_RenderFillRect(app->renderer, &r);

        SDL_SetRenderDrawColor(app->renderer,
                0xff, 0xff, 0xff, 240);

        SDL_RenderTexture(app->renderer, app->helpTexture,
                NULL, &app->helpRect);
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

void saveScreenshot(AppState *app) {
    // Generating Filename based on date and time
    time_t rawTime;
    struct tm *info;
    char buffer[80];
    time(&rawTime);
    info = localtime(&rawTime);
    strftime(buffer, sizeof(buffer), "%Y%m%d%H%M%S.png", info);
    SDL_Surface *surface = NULL;
    char *outFilename = (char *) malloc(
            sizeof(char) * (strlen(app->outDirectory) + strlen(buffer) + 1));
    if (!outFilename) {
        SDL_Log("Error allocating memory for outFilename: %s", strerror(errno));
        goto end;
    }
    strcpy(outFilename, app->outDirectory);
    strcat(outFilename, buffer);

    // Reading image from app->renderer
    surface = SDL_RenderReadPixels(app->renderer, NULL);
    if (!surface) {
        SDL_Log("Error to read pixels: %s", SDL_GetError());
        goto end;
    }

    if (!IMG_SavePNG(surface, outFilename)) {
        SDL_Log("Error saving \"%s\": %s", outFilename, SDL_GetError());
    } else {
        SDL_Log("Saved \"%s\" image successfully.", outFilename);
    }

end:
    free(outFilename);
    SDL_DestroySurface(surface);
}

void cleanupApp(AppState *app) {
    if (app->colors.noSetColors) free(app->colors.noSetColors);
    if (app->outDirectory) free(app->outDirectory);
    if (app->uiFont) TTF_CloseFont(app->uiFont);
    if (app->renderer) SDL_DestroyRenderer(app->renderer);
    if (app->window) SDL_DestroyWindow(app->window);
    if (app->fractalTexture) SDL_DestroyTexture(app->fractalTexture);
    if (app->helpTexture) SDL_DestroyTexture(app->helpTexture);

    TTF_Quit();
    SDL_Quit();
}


