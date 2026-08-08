
#include "utils.h"

// Parse double, return true on success and false on failure.
bool parse_double(const char *str, double *value) {
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
bool parse_long(const char *str, long int *value, long int base) {
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
bool parse_color(const char *str, Uint32 *value, int base) {
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


// This function takes a string contained in buffer, and converts
// it to a list of args for parsing
int parse_command(char *buffer, char *argv[], int max_args) {
    int argc = 0;
    char *p = buffer;
    bool in_quotes = false;

    while (*p != '\0' && argc < max_args) {
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

// Creates a new string and join them there. The resulting string must be
// liberated
char *join_strings(const char *str1, const char *str2) {
    int l1 = strlen(str1);
    int l2 = strlen(str2);
    char *newstr = (char *) malloc(l1 + l2 + 1);
    if(!newstr)
        return NULL;
    strcpy(newstr, str1);
    strcat(newstr, str2);
    return newstr;
}

void calculate_colors(int size, Uint32 *base, int range, Uint32 *dest) {
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
