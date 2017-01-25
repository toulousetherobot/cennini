#include <assert.h>
#include <MagickCore/MagickCore.h>
#include "sobel.h"

MagickExport Image *SobelOperator(const Image *image, enum SobelFilter filter, ExceptionInfo *exception)
{
  KernelInfo *kernel;
  Image *sobel_image;

  assert(image != (const Image *) NULL);
  assert(image->signature == MagickCoreSignature);

  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);

  // https://en.wikipedia.org/wiki/Sobel_operator
  if (filter == SobelXFilter)
    kernel = AcquireKernelInfo("3: -1,0,1  -2,0,2  -1,0,1", exception);
  else if (filter == SobelYFilter)
    kernel = AcquireKernelInfo("3: 1,2,1  0,0,0  -1,-2,-1", exception);

  sobel_image = MorphologyImage(image, ConvolveMorphology, 1, kernel, exception);

  kernel = DestroyKernelInfo(kernel);

  return(sobel_image);
}