
#include <time.h>
#include <stdio.h>
#include <SDL3_image/SDL_image.h>

#include "file_io.h"
#include "utils.h"
#include "commands.h"
#include "logger.h"

void save_image(AppState *app, char *fn) {
    // Generating Filename based on date and time
    time_t rawTime;
    struct tm *info;
    char buffer[80];
    time(&rawTime);
    info = localtime(&rawTime);
    strftime(buffer, sizeof(buffer), fn, info);
    SDL_Surface *surface = NULL;
    char *output_filepath = join_strings(app->output_path, buffer);
    if (!output_filepath) {
        LOG_ERROR("Failed to allocate memory for output_filepath: %s", strerror(errno));
        goto end;
    }

    // Reading image from app->renderer
    surface = SDL_RenderReadPixels(app->renderer, NULL);
    if (!surface) {
        LOG_ERROR("Failed to read pixels: %s", SDL_GetError());
        goto end;
    }

    if (!IMG_SavePNG(surface, output_filepath)) {
        LOG_ERROR("Failed to save \"%s\": %s", output_filepath, SDL_GetError());
    } else {
        LOG_INFO("Image saved successfully into \"%s\"", output_filepath);
    }

end:
    free(output_filepath);
    SDL_DestroySurface(surface);
}
void read_status(AppState *app, char *fn) {

    // 2. Open file to read
    FILE *loadFile = fopen(fn, "r");
    if (loadFile == NULL) {
        LOG_WARN("Failed to open \"%s\" for reading: %s\n", fn, strerror(errno));
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
        strncpy(app->command_buffer, lineBuffer, sizeof(app->command_buffer) - 1);
        
        // Ensure it is properly null-terminated just in case
        app->command_buffer[sizeof(app->command_buffer) - 1] = '\0';

        // 6. Execute the command exactly as if the user typed it!
        char *argv[MAX_ARGS];
        int argc = parse_command(app->command_buffer, argv, MAX_ARGS);
        execute_command(app, argc, argv);
    }

    // 7. Clean up memory and close the file
    fclose(loadFile);

    printf("Successfully loaded fractal state from %s\n", fn);
}
void save_status(AppState *app, char *fn) {
    time_t rawTime;
    struct tm *info;
    char buffer[80];
    time(&rawTime);
    info = localtime(&rawTime);
    strftime(buffer, sizeof(buffer), fn, info);
    char *output_filepath = join_strings(app->output_path, buffer);
    if (!output_filepath) {
        LOG_WARN("Error allocating memory for output_filepath: %s", strerror(errno));
        goto end;
    }

    // Open file to write
    FILE *file_to_save = fopen(output_filepath, "w");
    if (file_to_save == NULL) {
        LOG_WARN("Failed to write \"%s\": %s\n", output_filepath, strerror(errno));
        goto end;
    }
    // Save file
    fprintf(file_to_save, "center %.4f %.4f\n",
            creal(app->config.center), cimag(app->config.center));
    fprintf(file_to_save, "range %.4f\n", creal(app->config.range));
    fprintf(file_to_save, "power %i\n", app->config.power);
    fprintf(file_to_save, "maxiter %i\n", app->config.maxIterations);

    fclose(file_to_save);
end:
    free(output_filepath);
}
