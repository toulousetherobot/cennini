#include <stdio.h>
#include <stdlib.h>
#include <MagickCore/MagickCore.h>
#include <string.h>

#include "sobel.h"
#include "color.h"
#include "paint.h"

/*
  https://www.tutorialspoint.com/c_standard_library/c_function_qsort.htm
*/
int compare( const void* a, const void* b)
{
   return ( *(int*)b - *(int*)a );
}

int main(int argc,char **argv)
{
  if (argc != 2)
  {
    fprintf(stdout,"Usage: %s image\n",argv[0]);
    exit(0);
  }

  ExceptionInfo *exception;

  Image *image;

  ImageInfo *image_info;

  /*
    Initialize the image info structure and read an image.
  */
    MagickCoreGenesis(*argv, MagickTrue);
    exception = AcquireExceptionInfo();
    image_info = CloneImageInfo((ImageInfo *) NULL);
    strcpy(image_info->filename, argv[1]);
    image = ReadImage(image_info, exception);
    if (exception->severity != UndefinedException)
      CatchException(exception);
    if (image == (Image *) NULL)
      exit(1);

  /*
    Start the Painting Subroutine.
  */
    int brushes[] = {4, 8, 30};
    qsort(brushes, sizeof(brushes)/sizeof(brushes[0]), sizeof(int), compare);

    PaintInfo paint_info = {
      .stroke_threshold = 4000.0,
      .min_stroke_length = 2,
      .max_stroke_length = 15,
      .gaussian_multiplier = 1.0,
      .curvature_filter = 1.0
    };

    paint(image, image_info, brushes, sizeof(brushes)/sizeof(brushes[0]), &paint_info, exception);    
    if (exception->severity != UndefinedException)
      CatchException(exception);

  /*
    Destroy the image and exit.
  */
    DestroyImageInfo(image_info);
    DestroyExceptionInfo(exception);
    MagickCoreTerminus();
    return(0); 

}