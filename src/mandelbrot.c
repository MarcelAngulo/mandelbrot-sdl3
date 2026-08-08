#include "mandelbrot.h"

int calculateEscape(COMPLEX_TYPE c, const MandelbrotConfig *config) {
    COMPLEX_TYPE z = 0;
    // Run the z = z^2+c loop up to config-> max_iterations
    int pow = config->power;
    if (pow == 2) {
        for (int i = 0; i < config->maxIterations; i++) {
            z = z*z + c;
            // Check if sequence diverge
            double x = creal(z);
            double y = cimag(z);
            if (x*x + y*y > 4.0)
                return i;
        }
    } else {
        for (int i = 0; i < config->maxIterations; i++) {
            COMPLEX_TYPE zp = 1;
            for (int j = 0; j < config->power; j++)
                zp *= z;
            z = zp + c;
            // Check if sequence diverge
            double x = creal(z);
            double y = cimag(z);
            if (x*x + y*y > 4.0)
                return i;
        }
    }
    // Return the interation count.
    return config->maxIterations;
}
