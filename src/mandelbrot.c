#include "mandelbrot.h"

int calculateEscape(COMPLEX_TYPE c, const MandelbrotConfig *config) {
    COMPLEX_TYPE z = 0;
    // Run the z = z^2+c loop up to config-> max_iterations
    for (int i = 0; i < config->maxIterations; i++) {
        z = cpow(z,config->power) + c;
        // Check if sequence diverge
        if (creal(z * conj(z)) > 4.0)
            return i;
    }
    // Return the interation count.
    return config->maxIterations;
}
