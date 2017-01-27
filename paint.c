#include <assert.h>
#include <MagickCore/MagickCore.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>

#include "paint.h"
#include "sobel.h"
#include "color.h"

/*
  http://stackoverflow.com/questions/5309471/getting-file-extension-in-c
*/
const char *get_filename_ext(const char *filename)
{
    const char *dot = strrchr(filename, '.');
    if(!dot || dot == filename) return "";
    return dot + 1;
}

static inline MagickBooleanType SetImageProgress(const Image *image, const char *tag,const MagickOffsetType offset,const MagickSizeType extent)
{
  char
    message[MagickPathExtent];

  if (image->progress_monitor == (MagickProgressMonitor) NULL)
    return(MagickTrue);
  (void) FormatLocaleString(message,MagickPathExtent,"%s/%s",tag,
    image->filename);
  return(image->progress_monitor(message,offset,extent,image->client_data));
}


MagickExport Image *BlankCanvasFromImage(const Image *image, const Quantum quantum, ExceptionInfo *exception)
{
  #define BlankCanvasTag  "BlankCanvas/Image"

  CacheView *image_view, *separate_view;

  Image *separate_image;

  MagickBooleanType status;

  MagickOffsetType progress;

  // Check if Image is Properly Defined
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);

  // Check if Exception is Properly Defined
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);

  // Clone Image
  separate_image=CloneImage(image,image->columns,image->rows,MagickTrue, exception);
  if (separate_image == (Image *) NULL)
    return((Image *) NULL);

  // TODO: DEFINE
  if (SetImageStorageClass(separate_image, DirectClass, exception) == MagickFalse)
  {
    separate_image=DestroyImage(separate_image);
    return((Image *) NULL);
  }

  // Set No Alpha Channel
  separate_image->alpha_trait=UndefinedPixelTrait;

  /*
    Separate image.
  */
  status = MagickTrue;
  progress = 0;

  // Create Cache Required
  image_view = AcquireVirtualCacheView(image,exception);
  separate_view = AcquireAuthenticCacheView(separate_image,exception);

  // Traverse Each Row
  ssize_t y;
  for (y=0; y < (ssize_t) image->rows; y++)
  {

    if (status == MagickFalse)
      break;

    // Request Each Row of Pixles
    register const Quantum *magick_restrict p;
    p=GetCacheViewVirtualPixels(image_view, 0, y, image->columns, 1, exception);

    register Quantum *magick_restrict q;
    q=QueueCacheViewAuthenticPixels(separate_view, 0, y, separate_image->columns, 1, exception);

    // If either row is empty we're done. 
    if ((p == (const Quantum *) NULL) || (q == (Quantum *) NULL))
    {
      status = MagickFalse;
      break;
    }

    // Traverse Each Column
    register ssize_t x;
    for (x=0; x < (ssize_t) image->columns; x++)
    {

      if (GetPixelWriteMask(image,p) == 0)
      {
        p+=GetPixelChannels(image);
        q+=GetPixelChannels(separate_image);
        continue;
      }

      // Traverse Each Pixel Channel
      register ssize_t i;
      for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
      {
        PixelChannel channel = GetPixelChannelChannel(image,i);

        // Skip Undefined Traits
        if (GetPixelChannelTraits(image, channel) == UndefinedPixelTrait)
          continue;

        SetPixelChannel(separate_image, channel, quantum, q);

      }

      p+=GetPixelChannels(image);
      q+=GetPixelChannels(separate_image);
    }

    if (SyncCacheViewAuthenticPixels(separate_view,exception) == MagickFalse)
      status = MagickFalse;

    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
        MagickBooleanType proceed;
        proceed=SetImageProgress(image,BlankCanvasTag,progress++,image->rows);
        if (proceed == MagickFalse)
          status=MagickFalse;
      }
  }

  // Clean Up
  separate_view=DestroyCacheView(separate_view);
  image_view=DestroyCacheView(image_view);

  if (status == MagickFalse)
    separate_image=DestroyImage(separate_image);

  return(separate_image);
}

void paint_spline_stroke(Image *canvas, CacheView *reference, CacheView *sobel_x, CacheView *sobel_y, ssize_t x, ssize_t y, int brush_size, PaintInfo *paint_info, ExceptionInfo *exception)
{

  DrawInfo *clone_info;
  clone_info=CloneDrawInfo((ImageInfo *) NULL, (DrawInfo*) NULL);

  const Quantum *r;
  r = GetCacheViewVirtualPixels(reference, x, y, 1, 1, exception);

  clone_info->fill.alpha  = 0;

  QueryColorCompliance("green", AllCompliance, &clone_info->stroke, exception);
  clone_info->stroke.red   = GetPixelRed(reference->image, r);
  clone_info->stroke.green = GetPixelGreen(reference->image, r);
  clone_info->stroke.blue  = GetPixelBlue(reference->image, r);

  clone_info->stroke_width = brush_size;

  char buffer[MaxTextExtent];
  int n = snprintf(buffer, MaxTextExtent, "bezier %zd,%zd ", x,y);

  float dx, dy, prev_dx, prev_dy;
  dx = 0;  dy = 0;  prev_dx = 0;  prev_dy = 0;

  int i;
  for (i = 1; i < paint_info->max_stroke_length; i++)
  {
    const Quantum *p, *q;
    p = GetCacheViewVirtualPixels(sobel_x, x, y, 1, 1, exception);
    q = GetCacheViewVirtualPixels(sobel_y, x, y, 1, 1, exception);

    // If the color of the stroke deviates from the color under a control point of the curve by more than a specified threshold, the stroke ends at that control point.
    if ((i > paint_info->min_stroke_length) && (ColorDifference(canvas, reference->image, p, q) > paint_info->stroke_threshold))
      break;      

    Quantum lum_x =PixelLuminance(sobel_x->image, p);
    Quantum lum_y =PixelLuminance(sobel_y->image, q);

    // Detect if the Gradient is Vanishing
    if (sqrt(pow(lum_x,2) + pow(lum_y,2)) == 0)
      return;      

    // Get Unit Vector of Gradient
    float theta = atan(lum_y/lum_x);
    dx = -sin(theta);
    dy = cos(theta);

    // If Necessary, Reverse Direction
    if (prev_dx * dx + prev_dy * dy < 0)
    {
      dx = -dx;
      dy = -dy;
    }

    // Filter the stroke direction
    dx = paint_info->curvature_filter * dx + (1-paint_info->curvature_filter) * prev_dx;
    dy = paint_info->curvature_filter * dy + (1-paint_info->curvature_filter) * prev_dy;

    float magnitude = sqrt(pow(dx,2) + pow(dy,2));

    dx = dx/magnitude;
    dy = dy/magnitude;

    x = x + brush_size * dx;
    y = y + brush_size * dy;

    prev_dx = dx;
    prev_dy = dy;

    // Add Control Point to DrawInfo
    n = n + snprintf(buffer+n, MaxTextExtent-n, "%zd,%zd ", x,y);
  }

  // Add Stroke to Canvas
  CloneString(&clone_info->primitive, buffer);
  DrawImage(canvas, clone_info, exception);

  // Clean Up
  clone_info = DestroyDrawInfo(clone_info);

  return;
}

void paint_stroke(Image *canvas, Image *reference, ssize_t x, ssize_t y, int brush_size, ExceptionInfo *exception)
{

  DrawInfo *clone_info;
  clone_info=CloneDrawInfo((ImageInfo *) NULL, (DrawInfo*) NULL);
  char buffer[MaxTextExtent];
  snprintf(buffer, MaxTextExtent, "circle %zd,%zd %zd,%zd", x,y, x, y+brush_size/2);
  CloneString(&clone_info->primitive, buffer);

  CacheView *reference_view;
  reference_view = AcquireVirtualCacheView(reference, exception);

  const Quantum *p;
  p = GetCacheViewVirtualPixels(reference_view, x, y, 1, 1, exception);

  clone_info->fill.red   = GetPixelRed(reference, p);
  clone_info->fill.green = GetPixelGreen(reference, p);
  clone_info->fill.blue  = GetPixelBlue(reference, p);

  reference_view = DestroyCacheView(reference_view);

  DrawImage(canvas, clone_info, exception);

  clone_info = DestroyDrawInfo(clone_info);

  return;
}

void paint_layer(Image *canvas, Image *reference, int brush_size, PaintInfo *paint_info, ExceptionInfo *exception)
{
  // Check if Exception is Properly Defined
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);

  ssize_t grid = brush_size * paint_info->gaussian_multiplier;

  Image *sobel_x_filter, *sobel_y_filter;
  sobel_x_filter = SobelOperator(reference, SobelXFilter, exception);
  sobel_y_filter = SobelOperator(reference, SobelYFilter, exception);

  // Pull Stroke Colour
  CacheView *reference_view, *sobel_x_view, *sobel_y_view;
  reference_view = AcquireVirtualCacheView(reference, exception);
  sobel_x_view = AcquireVirtualCacheView(sobel_x_filter, exception);
  sobel_y_view = AcquireVirtualCacheView(sobel_y_filter, exception);

  ssize_t num_points = ceil((double)reference->rows/grid)*ceil((double)reference->columns/grid);

  PointInfo* points = malloc(num_points*sizeof(PointInfo));
  size_t i = 0;

  // Traverse Each Row
  ssize_t y;
  for (y=0; y < (ssize_t) reference->rows; y+=grid)
  {
    // Traverse Each Column
    ssize_t x;
    for (x=0; x < (ssize_t) reference->columns; x+=grid)
    {
      double area_error = 0;
      ssize_t max_x, max_y; 
      max_y = 0;
      max_x = 0;

      // Fetch Pixles in Ref & Canvas near (x,y)
      area_error = region_error(canvas, reference, x, y, grid, grid, &max_x, &max_y, exception);
      area_error = area_error/pow(grid, 2);

      if (area_error > paint_info->stroke_threshold)
      {
        points[i].x = max_x;
        points[i].y = max_y;
        i++;
      }
    }
  }

  struct timeval tv;
  gettimeofday(&tv, NULL);
  int usec = tv.tv_usec;
  srand48(usec);

  if (num_points > 1) {
      for (i = num_points - 1; i > 0; i--) {
          size_t j = (unsigned int) (drand48()*(i+1));
          PointInfo t = points[j];
          points[j] = points[i];
          points[i] = t;
      }
  }

  for (i = 0; i < num_points; i++){
    paint_spline_stroke(canvas, reference_view, sobel_x_view, sobel_y_view, points[i].x, points[i].y, brush_size, paint_info, exception);
  }

  free(points);

  reference_view = DestroyCacheView(reference_view);
  sobel_x_view = DestroyCacheView(sobel_x_view);
  sobel_y_view = DestroyCacheView(sobel_y_view);

  sobel_x_filter = DestroyImage(sobel_x_filter);
  sobel_y_filter = DestroyImage(sobel_y_filter);

  return;
}

void paint(Image *image, ImageInfo *image_info, int brushes[], int brushes_size, PaintInfo *paint_info, ExceptionInfo *exception)
{
  // Check if Exception is Properly Defined
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);

  Image *reference;
  DrawInfo* draw_info, *canvas_draw_info;

  /*
    Extract Filename and Extension
  */
    const char *image_extension;
    image_extension = get_filename_ext(image_info->filename);

    char image_basename[MaxTextExtent];
    unsigned long basename_length = (image_extension - image_info->filename)/sizeof(image_info->filename[0]);
    strncpy(image_basename, image_info->filename, basename_length);
    image_basename[basename_length-1] = '\0';

  /*
    Create an Empy Canvas
  */
    Image *canvas;
    canvas = BlankCanvasFromImage(image, 65535.0, exception);

  int i;
  for (i = 0; i < brushes_size; ++i) {

    /*
      Apply a Gaussian Blur to the Image
    */
      double gaussian_blur_sigma = paint_info->gaussian_multiplier * brushes[i];
      reference = GaussianBlurImage(image, 0, gaussian_blur_sigma, exception);
      if (reference == (Image *) NULL)
        MagickError(exception->severity, exception->reason, exception->description);
  
    /*
      Paint the Layer
    */
      paint_layer(canvas, reference, brushes[i], paint_info, exception);
      if (exception->severity != UndefinedException)
        CatchException(exception);

    /*
      Annotate ReferenceImage for Diagonistic Purposes
    */
      draw_info = CloneDrawInfo(image_info, (DrawInfo*) NULL);

      // TODO: @aramael Refactor to Remove the Buffer & Use Native ImageMagick Annotate Tools
      char buffer[MaxTextExtent];
      snprintf(buffer, MaxTextExtent, "cennini v%s\nFile: %s\nW: %zu H: %zu\nGaussian Blur: %lf", "0.0.0", image_info->filename, image->columns, image->rows, gaussian_blur_sigma);
      CloneString(&draw_info->text, buffer);

      draw_info->pointsize = 12;

      // Position in Upper Right (North East)
      draw_info->gravity = NorthEastGravity;
      CloneString(&draw_info->geometry, "+25+25");

      // Font Color
      draw_info->fill.red   = 0;
      draw_info->fill.green = 0;
      draw_info->fill.blue  = 0;

      AnnotateImage(reference, draw_info, exception);
      if (exception->severity != UndefinedException)
        CatchException(exception);

    /*
      Annotate Current Canvas Image for Diagonistic Purposes
    */
      canvas_draw_info = CloneDrawInfo(image_info, draw_info);

      // TODO: @aramael Refactor to Remove the Buffer & Use Native ImageMagick Annotate Tools
      snprintf(buffer, MaxTextExtent, "cennini v%s\nFile: %s\nW: %zu H: %zu\nCanvas w/ Gaussian Reference: %lf", "0.0.0", image_info->filename, image->columns, image->rows, gaussian_blur_sigma);
      CloneString(&canvas_draw_info->text, buffer);

      AnnotateImage(canvas, canvas_draw_info, exception);
      if (exception->severity != UndefinedException)
        CatchException(exception);

    /*
      Write the image.
    */
      snprintf(reference->filename, MaxTextExtent, "%s%s%0.2lf.%s", image_basename, "_gaussian_blur_", gaussian_blur_sigma, image_extension);
      WriteImage(image_info, reference, exception);
      if (exception->severity != UndefinedException)
        CatchException(exception);

      snprintf(canvas->filename, MaxTextExtent, "%s%s%0.2lf.%s", image_basename, "_canvas_after_", gaussian_blur_sigma, image_extension);
      WriteImage(image_info, canvas, exception);
      if (exception->severity != UndefinedException)
        CatchException(exception);

    DestroyImage(reference);
  }

  DestroyImage(image);
}