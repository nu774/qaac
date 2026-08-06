/*
 * Builds fft4g.c (which is written in terms of `double`) as single
 * precision, by textually substituting float for double before inclusion.
 */
#include <math.h>
#define double float
#define sin(x) sinf(x)
#define cos(x) cosf(x)
#define atan(x) atanf(x)
#include "fft4g.c"
#undef double
