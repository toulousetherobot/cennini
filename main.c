#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <MagickCore/MagickCore.h>

int compare( const void* a, const void* b)
{
   return ( *(int*)b - *(int*)a );
}

void paint(Image **image, ImageInfo **image_info, int brushes[], int brushes_size)
{
  ExceptionInfo *exception;
  exception = AcquireExceptionInfo();

  Image *original_image = *image;
  Image *temp_image;
  ImageInfo *original_image_info = *image_info;
  DrawInfo* draw_info;

  int i;
  for (i = 0; i < brushes_size; ++i) {
    /*
      Apply a Gaussian Blur to the Image
    */
      temp_image = GaussianBlurImage(original_image, 0, brushes[i], exception);
      if (temp_image == (Image *) NULL)
        MagickError(exception->severity, exception->reason, exception->description);

    /*
      Annotate Image for Diagonistic Purposes
    */
      draw_info = CloneDrawInfo(original_image_info, (DrawInfo*) NULL);

      // TODO: @aramael Refactor to Remove the Buffer & Use Native ImageMagick Annotate Tools
      char buffer[MaxTextExtent];
      snprintf(buffer, MaxTextExtent, "cennini v%s\nFile: %s\nW: %zu H: %zu\nGaussian Blur: %d", "0.0.0", original_image_info->filename, original_image->columns, original_image->rows, brushes[0]);
      CloneString(&draw_info->text, buffer);

      draw_info->pointsize = 12;

      // Position in Upper Right (North East)
      draw_info->gravity = NorthEastGravity;
      CloneString(&draw_info->geometry, "+25+25");

      // Font Color
      draw_info->fill.red   = 0;
      draw_info->fill.green = 0;
      draw_info->fill.blue  = 0;

      AnnotateImage(temp_image, draw_info, exception);
      if (exception->severity != UndefinedException)
        CatchException(exception);

    /*
      Write the image.
    */
      snprintf(temp_image->filename, MaxTextExtent, "%s%s%d%s", "test", "_gaussian_blur_", brushes[i], ".jpg");
      WriteImage(original_image_info, temp_image, exception);
      if (exception->severity != UndefinedException)
        CatchException(exception);

    DestroyImage(temp_image);
  }

  DestroyImage(original_image);

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
    int brushes[] = {10, 20, 30};
    qsort(brushes, sizeof(brushes)/sizeof(brushes[0]), sizeof(int), compare);
    paint(&image, &image_info, brushes, sizeof(brushes)/sizeof(brushes[0]));

  /*
    Destroy the image and exit.
  */
    DestroyImageInfo(image_info);
    DestroyExceptionInfo(exception);
    MagickCoreTerminus();
    return(0); 

}