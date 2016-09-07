
// epimgconv: Enterprise 128 image converter utility
// Copyright (C) 2008-2009 Istvan Varga <istvanv@users.sourceforge.net>
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

#include "epimgconv.hpp"
#include "imageconv.hpp"
#include "pixel2.hpp"

#include <vector>

namespace Ep128ImgConv {

  void ImageConv_Pixel2::initializePalettes()
  {
    for (int i = 0; i < 256; i++)
      convertEPColorToYUV(i, paletteY[i], paletteU[i], paletteV[i]);
  }

  void ImageConv_Pixel2::randomizePalette(int yc, int seedValue)
  {
    int     tmp = 0;
    setRandomSeed(tmp, uint32_t(seedValue));
    for (int i = 0; i < 2; i++) {
      unsigned char c = (unsigned char) (getRandomNumber(tmp) & 0xFF);
      if (!fixedColors[i])
        palette[yc][i] = c;
    }
  }

  double ImageConv_Pixel2::calculateLineError(int yc, double maxError)
  {
    float   tmpPaletteY[5];
    float   tmpPaletteU[5];
    float   tmpPaletteV[5];
    for (int i = 0; i < 5; i++) {
      float   f = float(i > 0 ? (i > 1 ? (i > 2 ? (i > 3 ? 3 : 1) : 2) : 4) : 0)
                  * 0.25f;
      int     c0 = palette[yc][0];
      int     c1 = palette[yc][1];
      tmpPaletteY[i] = (paletteY[c0] * (1.0f - f)) + (paletteY[c1] * f);
      tmpPaletteU[i] = (paletteU[c0] * (1.0f - f)) + (paletteU[c1] * f);
      tmpPaletteV[i] = (paletteV[c0] * (1.0f - f)) + (paletteV[c1] * f);
    }
    double  totalError = 0.0;
    for (int xc = 0; xc < width; xc += 8) {
      if (totalError > (maxError * 1.000001))
        break;
      float   inBufY[8];
      float   inBufU[8];
      float   inBufV[8];
      for (int i = 0; i < 8; i++) {
        inBufY[i] = inputImage.y(xc + i, yc) + ditherErrorImage.y(xc + i, yc);
        inBufU[i] = inputImage.u(xc + i, yc) + ditherErrorImage.u(xc + i, yc);
        inBufV[i] = inputImage.v(xc + i, yc) + ditherErrorImage.v(xc + i, yc);
        limitYUVColor(inBufY[i], inBufU[i], inBufV[i]);
      }
      // 8x1
      for (int i = 0; i < 8; i++) {
        double  minErr = 1000000000.0;
        float   y = inBufY[i];
        float   u = inBufU[i];
        float   v = inBufV[i];
        for (int j = 0; j < 2; j++) {
          double  err = Ep128ImgConv::calculateYUVErrorSqr(
                            tmpPaletteY[j], tmpPaletteU[j], tmpPaletteV[j],
                            y, u, v, colorErrorScale);
          if (err < minErr)
            minErr = err;
        }
        totalError += minErr;
      }
      if (ditherType == 0)
        continue;
      for (int i = 0; i < 4; i++) {
        // downsample to 4x1
        inBufY[i] = (inBufY[i << 1] + inBufY[(i << 1) + 1]) * 0.5f;
        inBufU[i] = (inBufU[i << 1] + inBufU[(i << 1) + 1]) * 0.5f;
        inBufV[i] = (inBufV[i << 1] + inBufV[(i << 1) + 1]) * 0.5f;
        double  minErr = 1000000000.0;
        float   y = inBufY[i];
        float   u = inBufU[i];
        float   v = inBufV[i];
        for (int j = 0; j < 3; j++) {
          double  err = Ep128ImgConv::calculateYUVErrorSqr(
                            tmpPaletteY[j], tmpPaletteU[j], tmpPaletteV[j],
                            y, u, v, colorErrorScale);
          if (err < minErr)
            minErr = err;
        }
        totalError += (minErr * 4.0);
      }
      for (int i = 0; i < 2; i++) {
        // downsample to 2x1
        double  minErr = 1000000000.0;
        float   y = (inBufY[i << 1] + inBufY[(i << 1) + 1]) * 0.5f;
        float   u = (inBufU[i << 1] + inBufU[(i << 1) + 1]) * 0.5f;
        float   v = (inBufV[i << 1] + inBufV[(i << 1) + 1]) * 0.5f;
        for (int j = 0; j < 5; j++) {
          double  err = Ep128ImgConv::calculateYUVErrorSqr(
                            tmpPaletteY[j], tmpPaletteU[j], tmpPaletteV[j],
                            y, u, v, colorErrorScale);
          if (err < minErr)
            minErr = err;
        }
        totalError += (minErr * 6.0);
      }
    }
    return totalError;
  }

  double ImageConv_Pixel2::calculateTotalError(double maxError)
  {
    double  totalError = 0.0;
    for (int yc = 0; yc < height; yc++) {
      totalError += calculateLineError(yc);
      if (totalError > (maxError * 1.000001))
        break;
    }
    return totalError;
  }

  double ImageConv_Pixel2::optimizeLinePalette(int yc, int optimizeLevel)
  {
    double  bestError = 1000000000.0;
    int     bestPalette[2];
    for (int l = 0; l < optimizeLevel; l++) {
      randomizePalette(yc, l + 1);
      double  minErr = calculateLineError(yc);
      bool    doneFlag = true;
      do {
        doneFlag = true;
        for (int i = 0; i < 2; i++) {
          int     bestColor = palette[yc][i];
          if (!fixedColors[i]) {
            for (int c = 0; c < 256; c++) {
              palette[yc][i] = (unsigned char) c;
              double  err = calculateLineError(yc, minErr);
              if (err < (minErr * 0.999999)) {
                bestColor = c;
                doneFlag = false;
                minErr = err;
              }
            }
          }
          palette[yc][i] = (unsigned char) bestColor;
        }
      } while (!doneFlag);
      if (minErr < bestError) {
        for (int i = 0; i < 2; i++)
          bestPalette[i] = palette[yc][i];
        bestError = minErr;
      }
    }
    for (int i = 0; i < 2; i++)
      palette[yc][i] = (unsigned char) bestPalette[i];
    sortLinePalette(yc);
    return bestError;
  }

  double ImageConv_Pixel2::optimizeImagePalette(int optimizeLevel)
  {
    double  bestError = 1000000000.0;
    int     bestPalette[2];
    int     progressCnt = 0;
    int     progressMax = optimizeLevel * 3 * 2 * 256;
    for (int l = 0; l < optimizeLevel; l++) {
      randomizePalette(0, l + 1);
      setFixedPalette();
      double  minErr = calculateTotalError();
      bool    doneFlag = true;
      int     loopCnt = 0;
      do {
        doneFlag = true;
        if (++loopCnt > 3)
          progressCnt -= (2 * 256);
        for (int i = 0; i < 2; i++) {
          int     bestColor = palette[0][i];
          if (!fixedColors[i]) {
            for (int c = 0; c < 256; c++) {
              if (!setProgressPercentage(progressCnt * 100 / progressMax))
                return -1.0;
              progressCnt++;
              palette[0][i] = (unsigned char) c;
              setFixedPalette();
              double  err = calculateTotalError(minErr);
              if (err < (minErr * 0.999999)) {
                bestColor = c;
                doneFlag = false;
                minErr = err;
              }
            }
          }
          else {
            if (!setProgressPercentage(progressCnt * 100 / progressMax))
              return -1.0;
            progressCnt += 256;
          }
          palette[0][i] = (unsigned char) bestColor;
        }
      } while (!doneFlag);
      if (loopCnt < 3)
        progressCnt += ((3 - loopCnt) * (2 * 256));
      if (minErr < bestError) {
        for (int i = 0; i < 2; i++)
          bestPalette[i] = palette[0][i];
        bestError = minErr;
      }
    }
    for (int i = 0; i < 2; i++)
      palette[0][i] = (unsigned char) bestPalette[i];
    sortLinePalette(0);
    setFixedPalette();
    return bestError;
  }

  void ImageConv_Pixel2::sortLinePalette(int yc)
  {
    // sort palette colors by bit-reversed color value
    for (int i = 0; i < 2; i++) {
      unsigned char tmp = palette[yc][i];
      tmp = ((tmp & 0x55) << 1) | ((tmp & 0xAA) >> 1);
      tmp = ((tmp & 0x33) << 2) | ((tmp & 0xCC) >> 2);
      tmp = ((tmp & 0x0F) << 4) | ((tmp & 0xF0) >> 4);
      palette[yc][i] = tmp;
    }
    if (palette[yc][0] > palette[yc][1] &&
        !(fixedColors[0] || fixedColors[1])) {
      unsigned char tmp = palette[yc][0];
      palette[yc][0] = palette[yc][1];
      palette[yc][1] = tmp;
    }
    for (int i = 0; i < 2; i++) {
      unsigned char tmp = palette[yc][i];
      tmp = ((tmp & 0x55) << 1) | ((tmp & 0xAA) >> 1);
      tmp = ((tmp & 0x33) << 2) | ((tmp & 0xCC) >> 2);
      tmp = ((tmp & 0x0F) << 4) | ((tmp & 0xF0) >> 4);
      palette[yc][i] = tmp;
    }
  }

  void ImageConv_Pixel2::setFixedPalette()
  {
    for (int yc = 1; yc < height; yc++) {
      for (int i = 0; i < 2; i++)
        palette[yc][i] = palette[0][i];
    }
  }

  void ImageConv_Pixel2::pixelStoreCallback(void *userData, int xc, int yc,
                                            float y, float u, float v)
  {
    ImageConv_Pixel2&  this_ =
        *(reinterpret_cast<ImageConv_Pixel2 *>(userData));
    yc = yc >> 1;
    if (xc < 0 || xc >= this_.width || yc < 0 || yc >= this_.height)
      return;
    limitYUVColor(y, u, v);
    this_.inputImage.y(xc, yc) += (y * 0.5f);
    this_.inputImage.u(xc, yc) += (u * 0.5f);
    this_.inputImage.v(xc, yc) += (v * 0.5f);
  }

  void ImageConv_Pixel2::pixelStoreCallbackI(void *userData, int xc, int yc,
                                             float y, float u, float v)
  {
    ImageConv_Pixel2&  this_ =
        *(reinterpret_cast<ImageConv_Pixel2 *>(userData));
    if (xc < 0 || xc >= this_.width || yc < 0 || yc >= this_.height)
      return;
    limitYUVColor(y, u, v);
    this_.inputImage.y(xc, yc) = y;
    this_.inputImage.u(xc, yc) = u;
    this_.inputImage.v(xc, yc) = v;
  }

  ImageConv_Pixel2::ImageConv_Pixel2()
    : ImageConverter(),
      width(1),
      height(1),
      colorErrorScale(0.5f),
      inputImage(1, 1),
      ditherErrorImage(1, 1),
      convertedImage(1, 1),
      palette(2, 1),
      conversionQuality(3),
      borderColor(0x00),
      ditherType(1),
      ditherDiffusion(0.95f)
  {
    for (int i = 0; i < 2; i++)
      fixedColors[i] = false;
    initializePalettes();
  }

  ImageConv_Pixel2::~ImageConv_Pixel2()
  {
  }

  bool ImageConv_Pixel2::processImage(
      ImageData& imgData, const char *infileName,
      YUVImageConverter& imgConv, const ImageConvConfig& config)
  {
    width = config.width << 4;
    height = ((imgData[5] & 0x80) == 0 ? config.height : (config.height << 1));
    colorErrorScale = config.colorErrorScale;
    limitValue(colorErrorScale, 0.05f, 1.0f);
    conversionQuality = config.conversionQuality;
    limitValue(conversionQuality, 1, 9);
    borderColor = config.borderColor & 0xFF;
    float   borderY = 0.0f;
    float   borderU = 0.0f;
    float   borderV = 0.0f;
    convertEPColorToYUV(borderColor, borderY, borderU, borderV);
    inputImage.setBorderColor(borderY, borderU, borderV);
    ditherType = config.ditherType;
    limitValue(ditherType, 0, 5);
    ditherDiffusion = config.ditherDiffusion;
    limitValue(ditherDiffusion, 0.0f, 1.0f);

    inputImage.resize(width, height);
    ditherErrorImage.resize(width, height);
    convertedImage.resize(width, height);
    palette.resize(2, height);
    inputImage.clear();
    ditherErrorImage.clear();
    convertedImage.clear();
    palette.clear();

    initializePalettes();
    for (int i = 0; i < 2; i++)
      fixedColors[i] = (config.paletteColors[i] >= 0);
    for (int yc = 0; yc < height; yc++) {
      randomizePalette(yc, yc + 1000);
      for (int i = 0; i < 2; i++) {
        if (fixedColors[i])
          palette[yc][i] = (unsigned char) (config.paletteColors[i] & 0xFF);
      }
    }

    if (!(imgData[5] & 0x80))
      imgConv.setPixelStoreCallback(&pixelStoreCallback, (void *) this);
    else
      imgConv.setPixelStoreCallback(&pixelStoreCallbackI, (void *) this);
    if (!imgConv.convertImageFile(infileName))
      return false;

    progressMessage("Converting image");
    setProgressPercentage(0);
    int     optimizeLevel = 1 + ((conversionQuality - 1) >> 1);
    ditherErrorImage.clear();
    if (config.paletteResolution != 0) {
      // generate optimal palette independently for each line
      int     progressCnt = 0;
      int     progressMax = height;
      for (int yc = 0; yc < height; yc++) {
        if (!setProgressPercentage((progressCnt * 100) / progressMax))
          return false;
        optimizeLinePalette(yc, optimizeLevel);
        ditherLine(convertedImage, inputImage, ditherErrorImage, yc,
                   ditherType, ditherDiffusion,
                   colorErrorScale, &(palette[yc][0]), 2,
                   paletteY, paletteU, paletteV);
        progressCnt++;
      }
    }
    else {
      // generate optimal palette for the whole image
      if (optimizeImagePalette(optimizeLevel) < -0.5)
        return false;
      for (int yc = 0; yc < height; yc++) {
        ditherLine(convertedImage, inputImage, ditherErrorImage, yc,
                   ditherType, ditherDiffusion,
                   colorErrorScale, &(palette[0][0]), 2,
                   paletteY, paletteU, paletteV);
      }
    }
    imgData.setBorderColor(borderColor);
    for (int yc = 0; yc < height; yc++) {
      for (int i = 0; i < 2; i++)
        imgData.setPaletteColor(yc, i, palette[yc][i]);
      for (int xc = 0; xc < width; xc++)
        imgData.setPixel(xc, yc, convertedImage[yc][xc]);
    }
    setProgressPercentage(100);
    if (config.paletteResolution != 0) {
      progressMessage("");
    }
    else {
      char    tmpBuf[32];
      std::sprintf(&(tmpBuf[0]), "Done; palette = %d %d",
                   int(palette[0][0]), int(palette[0][1]));
      progressMessage(&(tmpBuf[0]));
    }
    return true;
  }

}       // namespace Ep128ImgConv

