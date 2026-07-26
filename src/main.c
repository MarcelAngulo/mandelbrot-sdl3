
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
#define COMMAND_BUFFER_SIZE 1000
#define MAX_ARGS 20
/* TODO LIST:
 *
 * POSSIBLE TODO LIST:
 * 1) ADVANCED errors for errors, info, etc
 * 5) Reorganize code (clean large function, split files, etc)
 * 2) Autocompleting for command mode-tab
 * 3) Adding other forms of color
 * 4) Adding advanced message OUTPUT (with line, char, function, message, etc)
 * 6) README usage and command line
 * 7) Revisar readStatus (nunca la probe)
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
char defaultImageFilename[] = "%Y%m%d%H%M%S.png";
char defaultStatusFilename[] = "%Y%m%d%H%M%S.txt";

char modeStr[][10] = {
    "Explorer",
    "Command"
};

typedef enum {
    MODE_EXPLORE = 0,
    MODE_COMMAND = 1
} AppMode;

SDL_Color white = {0xff, 0xff, 0xff, SDL_ALPHA_OPAQUE};

// 1. App State Structure
typedef struct {
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *fractalTexture;
    SDL_Texture *helpTexture;
    SDL_Texture *infoTexture;
    SDL_Texture *commandTexture;
    SDL_FRect fractalRect;
    SDL_FRect helpRect;
    SDL_FRect infoRect;
    SDL_FRect commandRect;
    TTF_Font *uiFont;
    int width;
    int height;
    MandelbrotConfig config;
    MandelbrotConfig configCopy;
    MandelbrotColors colors;
    AppMode mode;
    bool running;
    bool updateFractal;
    bool showInfo;
    bool showPointer;
    bool showHelp;
    char *file;
    char *outDirectory;
    char commandBuffer[COMMAND_BUFFER_SIZE];
    unsigned long int commandLenght;
} AppState;

// 2. Helper Function Declarations
bool initApp(AppState *app);
bool parseArguments(AppState *app, int argc, char *argv[]);
int parseCommand(char *buffer, char *argv[], int maxArgs);
bool executeCommand(int argc, char *argv[], AppState *app);
void handleEvents(AppState *app);
void renderAll(AppState *app);
void updateInfoTexture(AppState *app);
void updateCommandTexture(AppState *app);
void restartFractalRect(AppState *app);
void saveScreenshot(AppState *app, char *fn);
void saveStatus(AppState *app, char *fn);
void readStatus(AppState *app, char *fn);
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
        // Run 60 FPS
        SDL_Delay(16);
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

int parseCommand(char *buffer, char *argv[], int maxArgs) {
    int argc = 0;
    char *p = buffer;
    bool in_quotes = false;

    while (*p != '\0' && argc < maxArgs) {
        // Skip leading spaces
        while (*p == ' ' && !in_quotes) {
            p++;
        }

        if (*p == '\0') {
            break;
        }

        // Check is argument is wrapped in quotes
        if (*p == '"') {
            in_quotes = true;
            p++; // Move to the pointer pass the opening quote
        }

        argv[argc++] = p;

        while (*p != '\0') {
            if (*p == '"' && in_quotes) {
                *p = '\0'; // Strip the closing quote and terminate string
                in_quotes = false;
                p++;
                break;
            } else if (*p == ' ' && !in_quotes) {
                *p = '\0'; // Replace space with null terminator
                p++;
                break;
            }
            p++;
        }
    }
    return argc; // Returns the total number of arguments found
}
bool executeCommand(int argc, char *argv[], AppState *app) {
    for(int i = 0; i < argc; i++){
        char *arg = argv[i];
        if (streq(arg, "center") || streq(arg, "c")) {
            double re, im;
            if ( i + 2 < argc &&
                parseDouble(argv[++i], &re) &&
                parseDouble(argv[++i], &im)) {
                app->config.center = app->configCopy.center = re + im*I;
            } else {
                SDL_Log("Invalid \"center | c\" command.\n"
                        "Use: (center | c) [double re] [double im]\n");
            }
        }
        else if (streq(arg, "range") || streq(arg, "r")) {
            double range;
            if ( i + 1 < argc && parseDouble(argv[++i], &range)) {
                app->config.range = (1 - I*app->height/app->width)*range;
            } else {
                SDL_Log("Invalid \"range | r\" command.\n"
                        "Use: (range | r) [double re]\n");
            }
        }
        else if (streq(arg, "window") || streq(arg, "w")) {
            long int width, height;
            if ( i + 2 < argc &&
                parseLong(argv[++i], &width, 10) && width > 0 &&
                parseLong(argv[++i], &height, 10) && height > 0) {
                app->config.range = (1 - I*height/width)*creal(app->config.range);
                app->width = width;
                app->height = height;
                SDL_SetWindowSize(app->window, width, height);
            } else {
                SDL_Log("Invalid \"window | w\" command.\n"
                        "Use: (window | w) [long int width > 0] [long int height > 0]\n");
            }
        }
        else if (streq(arg, "maxiter") || streq(arg, "i")) {
            long int maxIter;
            if ( i + 1 < argc && parseLong(argv[++i], &maxIter, 10) && maxIter > 0) {
                app->config.maxIterations = app->configCopy.maxIterations = maxIter;
            } else {
                SDL_Log("Invalid \"maxiter | i\" argument.\n"
                        "Use: (maxIter | i) [long int iters > 0]\n");
            }
        }
        else if (streq(arg, "power") || streq(arg, "p")) {
            double re, im;
            if ( i + 2 < argc &&
                parseDouble(argv[++i], &re) &&
                parseDouble(argv[++i], &im)) {
                app->config.power = app->configCopy.power = re + im*I;
            } else {
                SDL_Log("Invalid \"power | p\" argument.\n"
                        "Use: (power | p) [double re] [double im]\n");
            }
        }
        else if (streq(arg, "colors") || streq(arg, "s")) {
            Uint32 setColor;
            long int size, rng;
            // Parse first 3 arguments
            if ( i + 3 < argc &&
                parseColor(argv[++i], &setColor, 16) &&
                parseLong(argv[++i], &rng, 10) && rng > 0 &&
                parseLong(argv[++i], &size, 10) && size > 0 && i + size <  argc) {
                // Parse colors
                app->colors.setColor = setColor;
                int end = ++i + size;
                // Now will parse the colors to store in app->colorS.noSetColors
                Uint32 *palette = (Uint32 *) malloc(sizeof(Uint32) *size);
                if (!palette) {
                    SDL_Log("Could not allocate memory for *palette in parseArguments: "
                            "%s\n", strerror(errno));
                    return EXIT_FAILURE;
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
                calculateColors(size, palette, rng, app->colors.noSetColors);

                app->colors.noSetSize = rng;
                // Free temporary array
                free(palette);
            } else {
                SDL_Log("Invalid \"colors | s\" argument.\n"
                        "Use: (colors | s) [Uint32 setColor] [int range > 0] [int 20 > size > 0] [Uint32 noSetcolors[size]]\n");
            }
        }
        else if (streq(arg, "showpointer")) {
            if ( i + 1 < argc && streq(argv[i+1], "true") ) {
                app->showPointer = true;
                i++;
            } else if ( i + 1 < argc && streq(argv[i+1], "false") ) {
                app->showPointer = false;
                i++;
            } else {
                SDL_Log("Invalid \"showpointer\" argument.\n"
                        "Use: showpointer [true | false]\n");
            }
        }
        else if (streq(arg, "showinfo")) {
            if ( i + 1 < argc && streq(argv[i+1], "true") ) {
                app->showInfo = true;
                i++;
            } else if ( i + 1 < argc && streq(argv[i+1], "false") ) {
                app->showInfo = false;
                i++;
            } else {
                SDL_Log("Invalid \"showinfo\" argument.\n"
                        "Use: (showinfo) [true | false]\n");
            }
        }
        else if (streq(arg, "screenshot")) {
            if (i+1 < argc && !streq(argv[++i], "--default")) {
                saveScreenshot(app, argv[i]);
            } else {
                saveScreenshot(app, defaultImageFilename);
            }
        }
        else if (streq(arg, "read")) {
            ; // read
        }
        else if (streq(arg, "write")) {
            if (i+1 < argc && !streq(argv[++i], "--default")) {
                saveStatus(app, argv[i]);
            } else {
                saveStatus(app, defaultStatusFilename);
            }
        }
    }
    return true;
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
                        "Use: (--maxIter | -i) [long int iters > 0]\n");
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
                        "Use: (--power | -p) [double re] [double im]\n");
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
            } else {
                SDL_Log("Invalid \"--colors | -s\" argument.\n"
                        "Use: (--colors | -s) [Uint32 setColor] [int range > 0] [int size > 0] [Uint32 noSetcolors[size]]\n");
            }
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
        else if (streq(arg, "exit")) {
            cleanupApp(app);
            exit(EXIT_SUCCESS);
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

    // Generates Default colors
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
    app->uiFont = TTF_OpenFont("assets/GoogleSansCode-Regular.ttf", 20.0f);
    if (!app->uiFont) {
        SDL_Log("Failed to load font: %s", SDL_GetError());
        return EXIT_FAILURE;
    }

    // Render help texture
    app->helpTexture = renderText(app->renderer,
            app->uiFont, helpStr, white,
            &app->helpRect.w, &app->helpRect.h);
    if(!app->helpTexture) {
        SDL_Log("Cannot create helpTexture: %s", SDL_GetError());
    }
    app->helpRect.x = (app->width - app->helpRect.w)/2;
    app->helpRect.y = (app->height - app->helpRect.h)/2;

    // Creates default output path
    char *base = SDL_GetBasePath();
    if (!base) {
        SDL_Log("Failed to allocate memory: %s", SDL_GetError());
    }
    char out[] = ""; // None
    app->outDirectory = (char *) malloc(sizeof(char) * (strlen(out) + strlen(base) + 1));
    if (!app->outDirectory) {
        SDL_Log("Failed to allocate memory: %s", strerror(errno));
        return EXIT_FAILURE;
    }
    strcpy(app->outDirectory, base);
    strcat(app->outDirectory, out);

    // Render infoTexture for the first time
    app->infoTexture = NULL; // Always put NULL for the first time
    updateInfoTexture(app);

    // Initialize rect of app->fractalTexture
    //restartFractalRect(app);
    //drawMandelbrot(app->fractalTexture, &app->config, &app->colors);
    app->mode = MODE_EXPLORE;
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
            if (app->mode == MODE_EXPLORE) {
                switch (event.key.key) {
                    // Close app
                    case SDLK_Q:
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
                        // Updates info texture
                        if(app->showInfo)
                            updateInfoTexture(app);
                        break;

                    // Move downwards
                    case SDLK_DOWN:
                    case SDLK_J:
                        app->fractalRect.y += 
                            -app->height * displacement;
                        app->config.center += 
                            cimag(app->config.range)*displacement*I;
                        // Updates info texture
                        if(app->showInfo)
                            updateInfoTexture(app);
                        break;

                    // Move to the left 
                    case SDLK_LEFT:
                    case SDLK_H:
                        app->fractalRect.x += 
                            app->width*displacement;
                        app->config.center +=
                            -creal(app->config.range)*displacement;
                        // Updates info texture
                        if(app->showInfo)
                            updateInfoTexture(app);
                        break;

                    // Move to the Right
                    case SDLK_RIGHT:
                    case SDLK_L:
                        app->fractalRect.x +=
                            -app->width*displacement;
                        app->config.center +=
                            creal(app->config.range)*displacement;
                        // Updates info texture
                        if(app->showInfo)
                            updateInfoTexture(app);
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
                        // Updates info texture
                        if(app->showInfo)
                            updateInfoTexture(app);
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
                        // Updates info texture
                        if(app->showInfo)
                            updateInfoTexture(app);
                        break;

                    // Increase the number of max iterations per point
                    case SDLK_P:
                        app->config.maxIterations++;
                        // Updates info texture
                        if(app->showInfo)
                            updateInfoTexture(app);
                        break;

                    // Decrase number of max iterations per point.
                    case SDLK_O:
                        app->config.maxIterations--;
                        if(app->config.maxIterations < 0)
                            app->config.maxIterations = 0;
                        // Updates info texture
                        if(app->showInfo)
                            updateInfoTexture(app);
                        break;

                    // Toggle visibility of info window.
                    case SDLK_I:
                        app->showInfo = !app->showInfo;
                        // Updates info texture
                        if(app->showInfo)
                            updateInfoTexture(app);
                        break;

                    // Toggle visibility of cross at the middle.
                    case SDLK_C:
                        app->showPointer = !app->showPointer;
                        break;

                    // Change to state of last updating.
                    case SDLK_X:
                        app->config = app->configCopy;
                        restartFractalRect(app);
                        // Updates info texture
                        if(app->showInfo)
                            updateInfoTexture(app);
                        break;

                    case SDLK_F:
                        SDL_SetWindowFullscreen(app->window,
                            !(SDL_GetWindowFlags(app->window) & SDL_WINDOW_FULLSCREEN));
                        // Updates info texture
                        if(app->showInfo)
                            updateInfoTexture(app);
                        break;

                    // When 'G' is pressed, save screenshot.
                    case SDLK_G:
                        saveScreenshot(app, defaultImageFilename);
                        break;

                    // Enter to command mode. Command will appear at
                    // the bottom of the screen
                    case SDLK_N:
                        SDL_StartTextInput(app->window);
                        app->mode = MODE_COMMAND;
                        app->commandLenght = 0;
                        strcpy(app->commandBuffer, "");
                        updateCommandTexture(app);
                        if(app->showInfo)
                            updateInfoTexture(app);
                        break;
                }

            } else if (app->mode == MODE_COMMAND) {
                switch(event.key.key) {
                    // Exit from command mode
                    case SDLK_ESCAPE:
                        SDL_StopTextInput(app->window);
                        app->mode = MODE_EXPLORE;
                        if(app->showInfo)
                            updateInfoTexture(app);
                        break;

                    // Exit from command mode
                    case SDLK_C:
                        if ((event.key.mod & SDL_KMOD_CTRL) &&
                            (event.key.mod & SDL_KMOD_SHIFT)) {
                                SDL_StopTextInput(app->window);
                                app->mode = MODE_EXPLORE;
                        }
                        if(app->showInfo)
                            updateInfoTexture(app);
                        break;

                    // Executes command and exit from command mode.
                    case SDLK_RETURN:
                        {
                            char *argv[MAX_ARGS];
                            int argc = parseCommand(app->commandBuffer, argv, MAX_ARGS);
                            executeCommand(argc, argv, app);
                        }
                        SDL_StopTextInput(app->window);
                        app->mode = MODE_EXPLORE;
                        app->updateFractal = true;
                        renderAll(app);
                        restartFractalRect(app);
                        // Recalculates 
                        drawMandelbrot(app->fractalTexture, &app->config, &app->colors);
                        app->updateFractal = false;
                        renderAll(app);
                        if(app->showInfo)
                            updateInfoTexture(app);
                        break;

                    // Deletes one character
                    case SDLK_BACKSPACE:
                        if (app->commandLenght > 1) {
                            app->commandBuffer[--app->commandLenght] = '\0';
                            updateCommandTexture(app);
                        } else {
                            SDL_StopTextInput(app->window);
                            app->mode = MODE_EXPLORE;
                            if(app->showInfo)
                                updateInfoTexture(app);
                        }
                        break;
                }
            }
        } else if (event.type == SDL_EVENT_TEXT_INPUT) {
            // This event will store text into app->commandBuffer if there is
            // enough space
            long unsigned int nl = strlen(event.text.text);
            if (nl + app->commandLenght > sizeof(app->commandBuffer)-1) {
                ; // Dont write anything. Buffer full
            } else {
                strcat(app->commandBuffer, event.text.text);
                app->commandLenght += nl;
                updateCommandTexture(app);
            }
        } else if (event.type == SDL_EVENT_WINDOW_RESIZED) {
            // Update app state with new dimensions
            app->width = event.window.data1;
            app->height = event.window.data2;
            // Updates range so keep dimensions of image
            app->config.range = (1 - I*app->height/app->width)*
                    creal(app->config.range);

            SDL_DestroyTexture(app->fractalTexture);
            app->fractalTexture = SDL_CreateTexture(
                    app->renderer,
                    SDL_PIXELFORMAT_RGBA8888,
                    SDL_TEXTUREACCESS_STREAMING,
                    app->width, app->height
            );

            // Updates info texture
            if(app->showInfo)
                updateInfoTexture(app);

            // Reset configCopy
            app->configCopy = app->config;
            // Recalculate position of help 
            app->helpRect.x = (app->width - app->helpRect.w)/2;
            app->helpRect.y = (app->height - app->helpRect.h)/2;
            // Put true to show on rendering
            app->updateFractal = true;
            renderAll(app);
            restartFractalRect(app);
            // Recalculates 
            drawMandelbrot(app->fractalTexture, &app->config, &app->colors);
            app->updateFractal = false;
            renderAll(app);
        // Process mouse motion events
        } else if (event.type == SDL_EVENT_MOUSE_MOTION) {
            // change app->config.center position if user move
            // mouse whith left button pressed.
            if(event.motion.state & SDL_BUTTON_LMASK) {
                float xrel =  event.motion.xrel;
                float yrel =  event.motion.yrel;
                app->fractalRect.x += xrel;
                app->config.center += xrel*-creal(app->config.range)/app->width;
                app->fractalRect.y += yrel;
                app->config.center += yrel*-cimag(app->config.range)*I/app->height;
                // Updates info texture
                if(app->showInfo)
                    updateInfoTexture(app);
            }
        } else if (event.type == SDL_EVENT_MOUSE_WHEEL) {
            // This part zoom in and zoom out when scrolling
            // with mouse wheeel
            float y = event.wheel.y;
            if ( y > 0 ) {
                // zoom in
                app->config.range *= 1/zoom; 
                app->fractalRect.x = app->width/2 -
                    (app->width/2 - app->fractalRect.x)*zoom;
                app->fractalRect.y = app->height/2 -
                   (app->height/2 - app->fractalRect.y)*zoom;
                app->fractalRect.w *= zoom;
                app->fractalRect.h *= zoom;
                // Updates info texture
                if(app->showInfo)
                    updateInfoTexture(app);
            } else if ( y < 0 ) {
                // zoom out
                app->config.range *= zoom;
                app->fractalRect.x = app->width/2 -
                    (app->width/2 - app->fractalRect.x)/zoom;
                app->fractalRect.y = app->height/2 -
                   (app->height/2 - app->fractalRect.y)/zoom;
                app->fractalRect.w *= 1/zoom;
                app->fractalRect.h *= 1/zoom;
                // Updates info texture
                if(app->showInfo)
                    updateInfoTexture(app);
            }
        }
    }
}

void updateCommandTexture(AppState *app) {
    // Free previous texture
    SDL_DestroyTexture(app->commandTexture);
    // Render command text
    app->commandTexture = renderText(app->renderer,
            app->uiFont, app->commandBuffer, white,
            &app->commandRect.w, &app->commandRect.h);
    // Calculates new coordinates of command Rect text
    app->commandRect.x = (app->width < app->commandRect.w) ? app->width - app->commandRect.w : 0;
    app->commandRect.y = app->height - app->commandRect.h;
}
void updateInfoTexture(AppState *app) {
    // Destroy previously allocated texture if it exists
    SDL_DestroyTexture(app->infoTexture);
    // Format text into buffer
    char buffer[300];
    snprintf(buffer, sizeof(buffer),
            "Center: %.4f%+.4fi\n"
            "Range: %.4f%+.4fi\n"
            "Power: %.4f%+.4fi\n"
            "Max Iter: %d\n"
            "Status: %s\n"
            "Mode: %s",
            creal(app->config.center), cimag(app->config.center),
            creal(app->config.range), cimag(app->config.range),
            creal(app->config.power), cimag(app->config.power),
            app->config.maxIterations,
            app->updateFractal? "Drawing..." : "Idle",
            modeStr[app->mode]);
    // Created texture
    app->infoTexture = renderText(app->renderer, 
            app->uiFont, buffer, white,
            &app->infoRect.w, &app->infoRect.h);
    if (!app->infoTexture) {
        SDL_Log("Couldn't render text: %s", SDL_GetError());
    }
    app->infoRect.x = app->infoRect.y = 0.0f;
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
        SDL_SetRenderDrawColor(app->renderer, 0, 0, 0, 255);
        SDL_RenderFillRect(app->renderer, &app->infoRect);
        SDL_RenderTexture(app->renderer,
                app->infoTexture, NULL, &app->infoRect);
    }

    // Render Pointer at the center of screen
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

    // Render help window
    if (app->showHelp) {
        float pad = 10;
        // Creates an auxiliary rect to fill with padding
        SDL_FRect r = {
            .x = app->helpRect.x - pad,
            .y = app->helpRect.y - pad,
            .w = app->helpRect.w + 2*pad,
            .h = app->helpRect.h + 2*pad
        };
        SDL_SetRenderDrawColor(app->renderer,
                0, 0, 0, 255);
        SDL_RenderFillRect(app->renderer, &r);
        SDL_SetRenderDrawColor(app->renderer,
                0xff, 0xff, 0xff, 255);
        SDL_RenderTexture(app->renderer, app->helpTexture,
                NULL, &app->helpRect);
    }

    if (app->mode == MODE_COMMAND) {
        SDL_FRect rr = {
            .x = 0, .y = app->height - app->commandRect.h,
            .w = app->width, .h=app->commandRect.h
        };
        SDL_SetRenderDrawColor(app->renderer,
                0, 0, 0, 255);
        SDL_RenderFillRect(app->renderer, &rr);
        SDL_RenderTexture(app->renderer,
                app->commandTexture, NULL, &app->commandRect);
    }

    SDL_RenderPresent(app->renderer);
}

// Little function that reset fractal rect position
void restartFractalRect(AppState *app) {
    app->fractalRect = (SDL_FRect) {
        .x = 0, .y = 0,
        .w = app->width,
        .h = app->height
    };
}

void readStatus(AppState *app, char *fn) {

    // 2. Open file to read
    FILE *loadFile = fopen(fn, "r");
    if (loadFile == NULL) {
        fprintf(stderr, "Could not open \"%s\" for reading: %s\n", fn, strerror(errno));
        return;
    }

    // 3. Create a temporary array (buffer) to hold each line
    char lineBuffer[512]; 

    // 4. Read the file line by line until we reach the end (NULL)
    while (fgets(lineBuffer, sizeof(lineBuffer), loadFile) != NULL) {
        
        // Strip the trailing newline character (\n or \r\n) that fgets includes
        lineBuffer[strcspn(lineBuffer, "\r\n")] = '\0';
        
        // Skip completely empty lines to avoid parsing errors
        if (lineBuffer[0] == '\0') {
            continue; 
        }

        // 5. Copy the cleaned line into your app's command buffer
        // (Using strncpy is safer than strcpy to prevent buffer overflows)
        strncpy(app->commandBuffer, lineBuffer, sizeof(app->commandBuffer) - 1);
        
        // Ensure it is properly null-terminated just in case
        app->commandBuffer[sizeof(app->commandBuffer) - 1] = '\0';

        // 6. Execute the command exactly as if the user typed it!
        char *argv[MAX_ARGS];
        int argc = parseCommand(app->commandBuffer, argv, MAX_ARGS);
        executeCommand(argc, argv, app);
    }

    // 7. Clean up memory and close the file
    fclose(loadFile);

    printf("Successfully loaded fractal state from %s\n", fn);
}
void saveStatus(AppState *app, char *fn) {
    time_t rawTime;
    struct tm *info;
    char buffer[80];
    time(&rawTime);
    info = localtime(&rawTime);
    strftime(buffer, sizeof(buffer), fn, info);
    char *outFilename = (char *) malloc(
            sizeof(char) * (strlen(app->outDirectory) + strlen(buffer) + 1));
    if (!outFilename) {
        SDL_Log("Error allocating memory for outFilename: %s", strerror(errno));
        goto end;
    }
    strcpy(outFilename, app->outDirectory);
    strcat(outFilename, buffer);

    // Open file to write
    FILE *saveFile = fopen(outFilename, "w");
    if (saveFile == NULL) {
        SDL_Log("Could not write \"%s\": %s\n", outFilename, strerror(errno));
        goto end;
    }
    // Save file
    fprintf(saveFile, "center %.4f %.4f\n",
            creal(app->config.center), cimag(app->config.center));
    fprintf(saveFile, "range %.4f\n", creal(app->config.range));
    fprintf(saveFile, "power %.4f %.4f\n",
            creal(app->config.power), cimag(app->config.power));
    fprintf(saveFile, "maxiter %i\n", app->config.maxIterations);

    fclose(saveFile);
end:
    free(outFilename);
}

void saveScreenshot(AppState *app, char *fn) {
    // Generating Filename based on date and time
    time_t rawTime;
    struct tm *info;
    char buffer[80];
    time(&rawTime);
    info = localtime(&rawTime);
    strftime(buffer, sizeof(buffer), fn, info);
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
        SDL_Log("error saving \"%s\": %s", outFilename, SDL_GetError());
    } else {
        SDL_Log("saved \"%s\" image successfully.", outFilename);
    }

end:
    free(outFilename);
    SDL_DestroySurface(surface);
}

void cleanupApp(AppState *app) {
    if (app->colors.noSetColors) free(app->colors.noSetColors);
    if (app->outDirectory) free(app->outDirectory);
    if (app->uiFont) TTF_CloseFont(app->uiFont);
    SDL_DestroyRenderer(app->renderer);
    SDL_DestroyWindow(app->window);
    SDL_DestroyTexture(app->fractalTexture);
    SDL_DestroyTexture(app->helpTexture);
    SDL_DestroyTexture(app->infoTexture);
    SDL_DestroyTexture(app->commandTexture);

    TTF_Quit();
    SDL_Quit();
}


