#ifndef CENNINI_SOBEL_H
#define CENNINI_SOBEL_H

enum SobelFilter {
  SobelXFilter,
  SobelYFilter
};

MagickExport Image *SobelOperator(const Image *image, enum SobelFilter filter, ExceptionInfo *exception);

#endif