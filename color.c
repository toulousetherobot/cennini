#include <assert.h>
#include <MagickCore/MagickCore.h>
#include "color.h"

#define MagickMax(x,y)  (((x) > (y)) ? (x) : (y))
#define MagickMin(x,y)  (((x) < (y)) ? (x) : (y))

Quantum PixelLuminance (Image *image, const Quantum *p)
{
  return 0.30 * GetPixelRed(image, p) + 0.59 * GetPixelGreen(image, p) + 0.11 * GetPixelBlue(image, p);
}

double ColorDifference(Image *canvas, Image *reference, const Quantum *magick_restrict p, const Quantum *magick_restrict q)
{
  double error = 0;

  // Traverse Each Pixel Channel
  register ssize_t i;
  for (i=0; i < (ssize_t) GetPixelChannels(canvas); i++)
  {
    PixelChannel channel=GetPixelChannelChannel(canvas,i);

    PixelTrait traits=GetPixelChannelTraits(canvas, channel);
    PixelTrait reconstruct_traits=GetPixelChannelTraits(reference, channel);

    if ((traits == UndefinedPixelTrait) || (reconstruct_traits == UndefinedPixelTrait) || ((reconstruct_traits & UpdatePixelTrait) == 0))
      continue;

    error += pow(p[i]-(double) GetPixelChannel(reference, channel, q), 2);

  }

  return sqrt(error);
} 


double region_error(Image *canvas, Image *reference, ssize_t x, ssize_t y, size_t columns, size_t rows, ssize_t *max_x, ssize_t *max_y, ExceptionInfo *exception)
{

  CacheView *canvas_view, *reference_view;

  // Check if Image is Properly Defined
  assert(canvas != (Image *) NULL);
  assert(canvas->signature == MagickCoreSignature);

  // Check if Reference Image is Properly Defined
  assert(reference != (const Image *) NULL);
  assert(reference->signature == MagickCoreSignature);

  // Use canvas with maximum rows / columns
  rows = MagickMin(y + rows , MagickMax(canvas->rows, reference->rows));
  columns = MagickMin(x + columns , MagickMax(canvas->columns, reference->columns));

  // Construct the Image Views
  canvas_view=AcquireVirtualCacheView(canvas, exception);
  reference_view=AcquireVirtualCacheView(reference, exception);

  double total_error = 0;
  double max_error = 0;
  ssize_t starting_x = x;

  // Traverse Each Row
  for (; y < (ssize_t) rows; y++)
  {
    register const Quantum
      *magick_restrict p,
      *magick_restrict q;

    // Request Each Row of Pixles
    p=GetCacheViewVirtualPixels(canvas_view, 0, y, columns, 1, exception);
    q=GetCacheViewVirtualPixels(reference_view, 0, y, columns, 1, exception);

    // If either row is empty we're done. 
    if ((p == (const Quantum *) NULL) || (q == (Quantum *) NULL))
      break;      

    // Traverse Each Column
    for (x=starting_x; x < (ssize_t) columns; x++)
    {

      if (GetPixelWriteMask(canvas, p) == 0)
      {
        p+=GetPixelChannels(canvas);
        q+=GetPixelChannels(reference);
        continue;
      }

      double error = 0;

      // Traverse Each Pixel Channel
      error = ColorDifference(canvas, reference, p, q);

      if (error > max_error)
      {
        max_error = error;
        *max_x = x;
        *max_y = y;
      }

      total_error += error;

      p+=GetPixelChannels(canvas);
      q+=GetPixelChannels(reference);

    }

  }

  // Clean Up
  reference_view=DestroyCacheView(reference_view);
  canvas_view=DestroyCacheView(canvas_view);

  return total_error;
}
