#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <MagickCore/MagickCore.h>

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

/*
  https://www.tutorialspoint.com/c_standard_library/c_function_qsort.htm
*/
int compare( const void* a, const void* b)
{
   return ( *(int*)b - *(int*)a );
}

/*
  http://stackoverflow.com/questions/5309471/getting-file-extension-in-c
*/
const char *get_filename_ext(const char *filename) {
    const char *dot = strrchr(filename, '.');
    if(!dot || dot == filename) return "";
    return dot + 1;
}

void paint(Image *image, ImageInfo *image_info, double gaussian_multiplier, int brushes[], int brushes_size, ExceptionInfo *exception)
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
      double gaussian_blur_sigma = gaussian_multiplier * brushes[i];
      reference = GaussianBlurImage(image, 0, gaussian_blur_sigma, exception);
      if (reference == (Image *) NULL)
        MagickError(exception->severity, exception->reason, exception->description);

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
    paint(image, image_info, 1.0, brushes, sizeof(brushes)/sizeof(brushes[0]), exception);
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