#ifndef MANDELBROT_H
#define MANDELBROT_H

#include <complex.h>

#define COMPLEX_TYPE double complex

// A clean way to pass complex view configurations around
typedef struct {
    COMPLEX_TYPE center; // e.g., -0.5 + 0.0 * I
    COMPLEX_TYPE range; // width and height span in the complex plane
    int max_iterations;
} MandelbrotConfig;

// Takes a specific complex number c
// and return how many iterations it took to escape.
int calculate_escape(COMPLEX_TYPE c,
        const MandelbrotConfig *config);

#endif
