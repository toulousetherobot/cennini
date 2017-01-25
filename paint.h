#ifndef CENNINI_PAINT_H
#define CENNINI_PAINT_H

struct _CacheView
{
  Image *image;
};

const char *get_filename_ext(const char *filename);

static inline MagickBooleanType SetImageProgress(const Image *image, const char *tag,const MagickOffsetType offset,const MagickSizeType extent);

MagickExport Image *BlankCanvasFromImage(const Image *image, const Quantum quantum, ExceptionInfo *exception);

void paint_spline_stroke(Image *canvas, CacheView *reference, CacheView *sobel_x, CacheView *sobel_y, ssize_t x, ssize_t y, int brush_size, double stroke_threshold, float curvature_filter, int max_stroke_length, int min_stroke_length, ExceptionInfo *exception);

void paint_stroke(Image *canvas, Image *reference, ssize_t x, ssize_t y, int brush_size, ExceptionInfo *exception);

void paint_layer(Image *canvas, Image *reference, double stroke_threshold, float curvature_filter, int max_stroke_length, int min_stroke_length, double gaussian_multiplier, int brush_size, ExceptionInfo *exception);

void paint(Image *image, ImageInfo *image_info, double stroke_threshold, float curvature_filter, int max_stroke_length, int min_stroke_length, double gaussian_multiplier, int brushes[], int brushes_size, ExceptionInfo *exception);

#endif