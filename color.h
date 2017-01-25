#ifndef CENNINI_COLOR_H
#define CENNINI_COLOR_H

Quantum PixelLuminance (Image *image, const Quantum *p);

double ColorDifference(Image *canvas, Image *reference, const Quantum *magick_restrict p, const Quantum *magick_restrict q);

double region_error(Image *canvas, Image *reference, ssize_t x, ssize_t y, size_t columns, size_t rows, ssize_t *max_x, ssize_t *max_y, ExceptionInfo *exception);

#endif