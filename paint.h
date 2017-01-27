#ifndef CENNINI_PAINT_H
#define CENNINI_PAINT_H

struct _CacheView
{
  Image *image;
};

typedef struct _PaintInfo
{
	double
		stroke_threshold,
		gaussian_multiplier;
	float
		curvature_filter;
	int
		min_stroke_length,
		max_stroke_length;
	size_t
		signature;
} PaintInfo;

const char *get_filename_ext(const char *filename);

MagickExport Image *BlankCanvasFromImage(const Image *image, const Quantum quantum, ExceptionInfo *exception);

void paint_spline_stroke(Image *canvas, CacheView *reference, CacheView *sobel_x, CacheView *sobel_y, ssize_t x, ssize_t y, int brush_size, PaintInfo *paint_info, ExceptionInfo *exception);

void paint_stroke(Image *canvas, Image *reference, ssize_t x, ssize_t y, int brush_size, ExceptionInfo *exception);

void paint_layer(Image *canvas, Image *reference, int brush_size, PaintInfo *paint_info, ExceptionInfo *exception);

void paint(Image *image, ImageInfo *image_info, int brushes[], int brushes_size, PaintInfo *paint_info, ExceptionInfo *exception);

#endif