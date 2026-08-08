#include "mandelbrot.h"

int calculateEscape(COMPLEX_TYPE c, const MandelbrotConfig *config) {
    COMPLEX_TYPE z = 0;
    // Run the z = z^2+c loop up to config-> max_iterations
    for (int i = 0; i < config->maxIterations; i++) {
        COMPLEX_TYPE zp = 1;
        for (int j = 0; j < config->power; j++)
            zp *= z;
        z = zp + c;
        // Check if sequence diverge
        if (creal(z * conj(z)) > 4.0)
            return i;
    }
    // Return the interation count.
    return config->maxIterations;
}
