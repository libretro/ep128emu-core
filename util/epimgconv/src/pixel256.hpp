
// epimgconv: Enterprise 128 image converter utility
// Copyright (C) 2008 Istvan Varga <istvanv@users.sourceforge.net>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// The Enterprise 128 program files generated by this utility are not covered
// by the GNU General Public License, and can be used, modified, and
// distributed without any restrictions.

#ifndef EPIMGCONV_PIXEL256_HPP
#define EPIMGCONV_PIXEL256_HPP

#include "epimgconv.hpp"
#include "imageconv.hpp"

namespace Ep128ImgConv {

  class ImageConv_Pixel256 : public ImageConverter {
   protected:
    int           width;
    int           height;
    YUVImage      inputImage;
    YUVImage      ditherErrorImage;
    IndexedImage  convertedImage;
    int           borderColor;
    int           ditherType;
    float         ditherDiffusion;
    float         paletteY[256];
    float         paletteU[256];
    float         paletteV[256];
    // --------
    void initializePalettes();
    static void pixelStoreCallback(void *userData, int xc, int yc,
                                   float y, float u, float v);
    static void pixelStoreCallbackI(void *userData, int xc, int yc,
                                    float y, float u, float v);
   public:
    ImageConv_Pixel256();
    virtual ~ImageConv_Pixel256();
    // the return value is false if the processing has been stopped
    virtual bool processImage(ImageData& imgData, const char *infileName,
                              YUVImageConverter& imgConv,
                              const ImageConvConfig& config);
  };

}       // namespace Ep128ImgConv

#endif  // EPIMGCONV_PIXEL256_HPP

