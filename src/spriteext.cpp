
// ep128emu -- portable Enterprise 128 emulator
// Copyright (C) 2026 zoltanvb
// https://github.com/libretro/ep128emu-core/
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

// tvc256++
// Copyright TBA
// URL TBA

/*
   Backlog (sort of)
   General:
   - Document TVC memory map correctly -- what about segment 3? (does floppy work after gamecard changes?)
   - Align SDExt segment / memory map handling with the common scheme
   - Update SConstruct - introduce special wine+mingw cross-compile option for Devtool
   - Update SConstruct - simplify fltk, portaudio etc. version detection
   - Test devtool with 64-bit exe
   - Fix standalone version joystick handling in Linux
   - Fake gamecard rom for presenting the ID string
   - State save/load support

   TVC256++ gfx:
   - Write to port 0x00 to set sprite border as well
   - Sprite only gfx
   - fix resolution when overlay on top of graphics 2
   - Scroll
   - Sprite 2-color
   - Sprite 16-color
   - Sprite foreground / background
   - Ext graphics calculation delay from prev line
   - Sprite registers
   - Sprite interrupt
   - Screen height setting
   - Test bitmap with lot of transparency
   
   TVC256++ drives:
   - USB drive handling
   - Flash mem drive handling
   - PSRAM drive handling
   
   TVC256++ others:
   - Delay for slow RAM paging
   - Extend slow RAM to the real 8 MB instead of 2
   - SID emulation
   - Reset handling (cold, warm)
   - Function registers
   - Function implementation
   - File I/O disable from rom (use tvcfileio in emulator instead)
   - Extra file i/o functions (get_pwd .. seek_file)
   - File i/o functions for card (get_iobase, get_membase)

*/

#include "ep128emu.hpp"
#include "system.hpp"

#include <cstring>
#include <limits.h>
#include <unistd.h>
#include <cerrno>
#include <stdlib.h>

#include "spriteext.hpp"
#include "tvcmem.hpp"
#include "ide.hpp"

namespace Ep128 {

  // Port masks for the upper 128 named ports - lower half needs no mask
  static const uint8_t namedPortMasks[128] = {
      0,    0,    0,    0,   0,    0,    0,    0,   0,  0,  0,  0,   0,  0,  0,  0,
      0,    0,    0,    0,   0,    0,    0,    0,   0,  0,  0,  0,   0,  0,  3,  0,
   0x03, 0x3f, 0x3f, 0x1f,   1, 0x87, 0x87, 0x0f,   0,  0,  0,  0,   0,  0,  0,  0,
      0,    0,    0,    1,   0,    1,    0,    0,   0,  0,  0,  0,   0,  0,  0,  0,
      0,    0,    0,    0,   0,    0,    0,    0,   0,  0,  0,  0,   0,  0,  0,  0,
      0,    0,    0,    0,   0,    0,    0,    0,   0,  0,  0,  0,   0,  0,  0,  0,
      0,    0,    0,    0,   0,    0,    0,    0,   0,  0,  0,  0,   0,  0,  0,  0,
      0,    0,    0,    0,   0,    0,    0,    0,   0,  0,  0,  0,   0,  0,  0,  0
};


  SpriteExt::SpriteExt()
    : curLine(0),
      spriteExt_enabled(false),
      spriteExtSegment(0xFFFFFFFFU),
      spriteExtAddress(0xFFFFFFFFU)
  {
    for (int i = 0; i < 16; i++)
      io_port_values[i] = 0xFF;
    io_port_values[SPRITEEXT_REG_INCREMENT] = 0x01;
    io_port_values[SPRITEEXT_SEC_REG_INCREMENT] = 0x01;

    for (int i = 0; i < 256; i++)
      namedPortValues[i] = 0x00;
    namedPortValues[REG_SCREEN_BITMAP_BASE_ADDR] = REG_SCREEN_BITMAP_BASE_ADDR_DEFAULT;
    namedPortValues[REG_SCREEN_SCREEN_BASE_ADDR] = REG_SCREEN_SCREEN_BASE_ADDR_DEFAULT;
    namedPortValues[REG_SCREEN_SCREEN_COLOR_BASE_ADDR] = REG_SCREEN_SCREEN_COLOR_BASE_ADDR_DEFAULT;
    namedPortValues[REG_SCREEN_FONT_BASE_ADDR] = REG_SCREEN_FONT_BASE_ADDR_DEFAULT;
    namedPortValues[REG_MEMORY_P2] = REG_MEMORY_P2_DEFAULT;
    namedPortValues[REG_MEMORY_P3] = REG_MEMORY_P3_DEFAULT;
    namedPortValues[REG_MEMORY_MAP_8M_P2_LOW] = REG_MEMORY_MAP_8M_P2_LOW_DEFAULT;
    namedPortValues[REG_MEMORY_MAP_8M_P3_LOW] = REG_MEMORY_MAP_8M_P3_LOW_DEFAULT;
    namedPortValues[REG_USB_MOUSE_SPEED] = REG_USB_MOUSE_SPEED_DEFAULT;
    updateMouseSpeed(REG_USB_MOUSE_SPEED_DEFAULT);
    sd_ram_ext.resize(0x00001C00, 0xFF);
    sd_rom_ext.resize(0x00010000, 0xFF);
    this->reset(1);
  }

  SpriteExt::~SpriteExt()
  {
    openImage((char *) 0);
    try {
      openROMFile((char *) 0);
    }
    catch (...) {
      // FIXME: errors are ignored here
    }
  }

  uint8_t SpriteExt::i4ToTVCRGB(uint8_t val, uint8_t transparent_val)
  {
    // igrb with double bits - see TVCVideo::convertPixelToRGB
    // todo: replace with fixed palette conversion array
    if (val == SPRITEEXT_TRANSPARENT_COLOR)
      return transparent_val;
    uint8_t r = (val & 0x04) ? 0x0C : 0x00;
    uint8_t g = (val & 0x02) ? 0x30 : 0x00;
    uint8_t b = (val & 0x01) ? 0x03 : 0x00;
    uint8_t i = (val & 0x08) ? 0xC0 : 0x00;
    return (i |r | g | b);
  }

  void SpriteExt::setEnabled(bool isEnabled)
  {
    spriteExt_enabled = isEnabled;
    spriteExtSegment = 0x07U | (uint32_t(isEnabled) - 1U);
    spriteExtAddress = (0x07U << 14) | (uint32_t(isEnabled) - 1U);
  }

  void SpriteExt::temporaryDisable(bool isDisabled)
  {
    if (isDisabled) {
      spriteExtSegment = 0xFFFFFFFFU;
      spriteExtAddress = 0xFFFFFFFFU;
    }
    else {
      setEnabled(spriteExt_enabled);
    }
  }

  void SpriteExt::reset(int reset_level)
  {
    if (reset_level >= 2)
      std::memset(&(sd_ram_ext.front()), 0xFF, sd_ram_ext.size());
  }

  void SpriteExt::openImage(const char *sdimg_path)
  {
  }

  void SpriteExt::openROMFile(const char *fileName)
  {
  }

  uint8_t SpriteExt::readNamedPort(bool secondary)
  {
     uint8_t retval = 0xFF;
     uint8_t portAddr = secondary ? io_port_values[SPRITEEXT_SEC_REG_INDEX] : io_port_values[SPRITEEXT_REG_INDEX];
     switch (portAddr)
     {
       case REG_FW_VERSION_MAJOR:
         retval = REG_FW_VERSION_MAJOR_DEFAULT;
         break;
       case REG_FW_VERSION_MINOR:
         retval = REG_FW_VERSION_MINOR_DEFAULT;
         break;
       case REG_MEMORY_PSRAM_SIZE_IN_MB:
         retval = REG_MEMORY_PSRAM_SIZE_IN_MB_DEFAULT;
         break;
       case REG_USB_INIT:
         retval = REG_USB_INIT_DEFAULT;
         break;
       case REG_USB_MOUSE_SPEED:
         retval = namedPortValues[REG_USB_MOUSE_SPEED];
       break;
       default:
         // Video related ports are read instantly (lot of TODO here)
         if (portAddr <= REG_SCREEN_MAXY)
            retval = namedPortValues[portAddr];
         else
            retval = 0xFF;
         break;
     }
  // Increment register index after operation
  if (secondary)
    io_port_values[SPRITEEXT_SEC_REG_INDEX] += io_port_values[SPRITEEXT_SEC_REG_INCREMENT];
  else
    io_port_values[SPRITEEXT_REG_INDEX] += io_port_values[SPRITEEXT_REG_INCREMENT];

  return retval;
  }

  void SpriteExt::writeNamedPort(bool secondary, uint8_t value)
  {
     uint8_t portAddr = secondary ? io_port_values[SPRITEEXT_SEC_REG_INDEX] : io_port_values[SPRITEEXT_REG_INDEX];
     switch (portAddr)
     {
       case REG_USB_MOUSE_SPEED:
         namedPortValues[REG_USB_MOUSE_SPEED] = value;
         updateMouseSpeed(value);
       break;
       default:
         if (portAddr > 127)
           value = value & namedPortMasks[portAddr-128];
         // Video related ports are written instantly (lot of TODO here)
         if (portAddr <= REG_SCREEN_MAXY)
            namedPortValues[portAddr] = value;
     }

  // Increment register index after operation
  if (secondary)
    io_port_values[SPRITEEXT_SEC_REG_INDEX] += io_port_values[SPRITEEXT_SEC_REG_INCREMENT];
  else
    io_port_values[SPRITEEXT_REG_INDEX] += io_port_values[SPRITEEXT_REG_INCREMENT];

  }

  void SpriteExt::updateMouseSpeed(uint8_t binValue)
  {
     mouse_speed = ((binValue & 0xC) >> 6) + (float)(binValue & 0x1F) * 1/32;
  }

  
  const uint8_t* SpriteExt::combineLine(const uint8_t *buf, size_t *nBytes, uint8_t vsyncCnt, Ep128::Memory *mem)
  {

    const unsigned char *bufp = buf;
    const uint8_t *endp = buf + *nBytes;
    size_t outPos = 0;
    size_t currSlot = 0;
    if (vsyncCnt>0)
      curLine = 0;
    else 
      curLine++;
    if (!(*nBytes) || curLine < SPRITEEXT_FIRST_LINE || curLine > SPRITEEXT_LAST_LINE)
      return buf;
   // todo: screen height limit
   // todo: border color, content location
      
   //printf("combineLine, vsync: %03d %03d\n",vsyncCnt,curLine);
   // Note: line pixels are according to PAL (768). One TVC pixel is always at least 2 PAL pixels.
    do {
      switch (bufp[0]) {
      case 0x00:                        // 16 pixel blank coded on 1 byte
        do {
            buf_[outPos] = 0x00;
          bufp = bufp + 1;
          outPos++;
          if (bufp >= endp)
            break;
        } while (bufp[0] == 0x00);
        break;
      case 0x01:                        // 1x16 pixel, 256 colors coded on 2 bytes -- border
        do {
            buf_[outPos] = 0x01;
            buf_[outPos+1] = bufp[1];
          bufp = bufp + 2;
          outPos += 2;
          if (bufp >= endp)
            break;
        } while (bufp[0] == 0x01);
        break;
      case 0x02:                        // 2x8 pixels, 256 colors coded on 3 bytes -- not used for TVC
        do {
            buf_[outPos] = 0x02;
            buf_[outPos+1] = bufp[1];
            buf_[outPos+2] = bufp[2];

          bufp = bufp + 3;
          outPos += 3;
          if (bufp >= endp)
            break;
        } while (bufp[0] == 0x02);
        break;
      case 0x03:                        // 8x2 pixels, 2 colors coded on 4 bytes -- not used for TVC
        do {
          unsigned char c0 = bufp[1];
          unsigned char c1 = bufp[2];
          unsigned char b = bufp[3];
            buf_[outPos] = 0x03;
            buf_[outPos+1] = bufp[1];
            buf_[outPos+2] = bufp[2];
            buf_[outPos+3] = bufp[3];

          bufp = bufp + 4;
          outPos += 4;
          if (bufp >= endp)
            break;
        } while (bufp[0] == 0x03);
        break;
      case 0x04:                        // 4x4 pixels, 256 colors coded on 5 bytes -- TVC 16 color mode
        do {
            currSlot++;
          if (namedPortValues[REG_SCREEN_VIDEOMODE] == REG_SCREEN_VIDEOMODE_BITMAP)
          {
            buf_[outPos] = 0x08;
            uint32_t baseAddr = 
              ((TVC256_FASTRAM_START_SEGMENT + namedPortValues[REG_SCREEN_BITMAP_BASE_ADDR]*2)<<14) + 
              (curLine - SPRITEEXT_FIRST_LINE) * 128 + (currSlot-1)*4;
            buf_[outPos+1] = i4ToTVCRGB(((mem->readRaw(baseAddr)     & 0x0F)     ), bufp[1]);
            buf_[outPos+2] = i4ToTVCRGB(((mem->readRaw(baseAddr)     & 0xF0) >> 4), bufp[1]);
            buf_[outPos+3] = i4ToTVCRGB(((mem->readRaw(baseAddr + 1) & 0x0F)     ), bufp[2]);
            buf_[outPos+4] = i4ToTVCRGB(((mem->readRaw(baseAddr + 1) & 0xF0) >> 4), bufp[2]);
            buf_[outPos+5] = i4ToTVCRGB(((mem->readRaw(baseAddr + 2) & 0x0F)     ), bufp[3]);
            buf_[outPos+6] = i4ToTVCRGB(((mem->readRaw(baseAddr + 2) & 0xF0) >> 4), bufp[3]);
            buf_[outPos+7] = i4ToTVCRGB(((mem->readRaw(baseAddr + 3) & 0x0F)     ), bufp[4]);
            buf_[outPos+8] = i4ToTVCRGB(((mem->readRaw(baseAddr + 3) & 0xF0) >> 4), bufp[4]);
            bufp = bufp + 5;
            outPos += 9;
            *nBytes += 4;
          }
          else if (namedPortValues[REG_SCREEN_VIDEOMODE] == REG_SCREEN_VIDEOMODE_CHAR2)
          {
            div_t charLine = div(curLine - SPRITEEXT_FIRST_LINE, 8);
            uint32_t charAddr  = (TVC256_FASTRAM_START_SEGMENT<<14) + 
                                  namedPortValues[REG_SCREEN_SCREEN_BASE_ADDR]*0x400 +
                                  charLine.quot * 32 + (currSlot-1);
            uint8_t charVal    = mem->readRaw(charAddr);
            uint32_t colorAddr = (TVC256_FASTRAM_START_SEGMENT<<14) +
                                  namedPortValues[REG_SCREEN_SCREEN_COLOR_BASE_ADDR]*0x400 +
                                  charLine.quot * 32 + (currSlot-1);
            uint8_t colorVal   = mem->readRaw(colorAddr);
            uint32_t fontAddr  = (TVC256_FASTRAM_START_SEGMENT<<14) +
                                  namedPortValues[REG_SCREEN_FONT_BASE_ADDR]*0x800 +
                                  charVal * 8 + charLine.rem;
            uint8_t fontVal = mem->readRaw(fontAddr);
            buf_[outPos] = 0x08;
            buf_[outPos+1] = fontVal & 128 ? i4ToTVCRGB(colorVal,bufp[1]) : bufp[1];
            buf_[outPos+2] = fontVal &  64 ? i4ToTVCRGB(colorVal,bufp[1]) : bufp[1];
            buf_[outPos+3] = fontVal &  32 ? i4ToTVCRGB(colorVal,bufp[2]) : bufp[2];
            buf_[outPos+4] = fontVal &  16 ? i4ToTVCRGB(colorVal,bufp[2]) : bufp[2];
            buf_[outPos+5] = fontVal &   8 ? i4ToTVCRGB(colorVal,bufp[3]) : bufp[3];
            buf_[outPos+6] = fontVal &   4 ? i4ToTVCRGB(colorVal,bufp[3]) : bufp[3];
            buf_[outPos+7] = fontVal &   2 ? i4ToTVCRGB(colorVal,bufp[4]) : bufp[4];
            buf_[outPos+8] = fontVal &   1 ? i4ToTVCRGB(colorVal,bufp[4]) : bufp[4];
            bufp = bufp + 5;
            outPos += 9;
            *nBytes += 4;
          }
          else if (namedPortValues[REG_SCREEN_VIDEOMODE] == REG_SCREEN_VIDEOMODE_CHAR16)
          {
            div_t charLine = div(curLine - SPRITEEXT_FIRST_LINE, 8);
            uint32_t charAddr  = (TVC256_FASTRAM_START_SEGMENT<<14) + 
                                  namedPortValues[REG_SCREEN_SCREEN_BASE_ADDR]*0x400 +
                                  charLine.quot * 32 + (currSlot-1);
            uint8_t charVal    = mem->readRaw(charAddr);
            uint32_t fontAddr  = (TVC256_FASTRAM_START_SEGMENT<<14) +
                                  namedPortValues[REG_SCREEN_FONT_BASE_ADDR]*0x800 +
                                  charVal * 8 * 4 + charLine.rem * 4;
            uint8_t fontVal;
            buf_[outPos] = 0x08;
            fontVal = ((mem->readRaw(fontAddr  ) & 0xF0)>>4);
            buf_[outPos+1] = i4ToTVCRGB(fontVal,bufp[1]);
            fontVal = ((mem->readRaw(fontAddr  ) & 0x0F));
            buf_[outPos+2] = i4ToTVCRGB(fontVal,bufp[1]);
            fontVal = ((mem->readRaw(fontAddr+1) & 0xF0)>>4);
            buf_[outPos+3] = i4ToTVCRGB(fontVal,bufp[2]);
            fontVal = ((mem->readRaw(fontAddr+1) & 0x0F));
            buf_[outPos+4] = i4ToTVCRGB(fontVal,bufp[2]);
            fontVal = ((mem->readRaw(fontAddr+2) & 0xF0)>>4);
            buf_[outPos+5] = i4ToTVCRGB(fontVal,bufp[3]);
            fontVal = ((mem->readRaw(fontAddr+2) & 0x0F));
            buf_[outPos+6] = i4ToTVCRGB(fontVal,bufp[3]);
            fontVal = ((mem->readRaw(fontAddr+3) & 0xF0)>>4);
            buf_[outPos+7] = i4ToTVCRGB(fontVal,bufp[4]);
            fontVal = ((mem->readRaw(fontAddr+3) & 0x0F));
            buf_[outPos+8] = i4ToTVCRGB(fontVal,bufp[4]);
            bufp = bufp + 5;
            outPos += 9;
            *nBytes += 4;
          }
/*          else if (namedPortValues[REG_SPRITE_ENABLE])
          {
             uint16_t spriteX = namedPortValues[REG_SPRITE_X];
             uint16_t spriteY = namedPortValues[REG_SPRITE_Y];
             if (curLine < spriteX - REG_SPRITE_OFFSET_X || curLine > spriteX - REG_SPRITE_OFFSET_X + 21)
          }*/
          else {
            buf_[outPos] = 0x04;
            buf_[outPos+1] = bufp[1];
            buf_[outPos+2] = bufp[2];
            buf_[outPos+3] = bufp[3];
            buf_[outPos+4] = bufp[4];
            bufp = bufp + 5;
            outPos += 5;
          }
          if (bufp >= endp)
            break;
        } while (bufp[0] == 0x04);
        break;
      case 0x06:                        // 16 (2*8) pixels, 2*2 colors coded on 7 bytes -- TVC 2 color mode
        do {
            currSlot++;
          if (namedPortValues[REG_SCREEN_VIDEOMODE] == REG_SCREEN_VIDEOMODE_BITMAP)
          {
            unsigned char c0 = bufp[1];
            unsigned char c1 = bufp[2];
            unsigned char b = bufp[3];
            buf_[outPos] = 0x09;
            uint32_t baseAddr = 
              ((TVC256_FASTRAM_START_SEGMENT + namedPortValues[REG_SCREEN_BITMAP_BASE_ADDR]*2)<<14) + 
              (curLine - SPRITEEXT_FIRST_LINE) * 128 + (currSlot-1)*4;
            // Split pixels
            buf_[outPos+1] = i4ToTVCRGB(((mem->readRaw(baseAddr)     & 0x0F)     ), (b & 0x80) ? c1 : c0);
            buf_[outPos+2] = i4ToTVCRGB(((mem->readRaw(baseAddr)     & 0x0F)     ), (b & 0x40) ? c1 : c0);
            
            buf_[outPos+3] = i4ToTVCRGB(((mem->readRaw(baseAddr)     & 0xF0) >> 4), (b & 0x20) ? c1 : c0);
            buf_[outPos+4] = i4ToTVCRGB(((mem->readRaw(baseAddr)     & 0xF0) >> 4), (b & 0x10) ? c1 : c0);

            buf_[outPos+5] = i4ToTVCRGB(((mem->readRaw(baseAddr + 1) & 0x0F)     ), (b & 0x08) ? c1 : c0);
            buf_[outPos+6] = i4ToTVCRGB(((mem->readRaw(baseAddr + 1) & 0x0F)     ), (b & 0x04) ? c1 : c0);

            buf_[outPos+7] = i4ToTVCRGB(((mem->readRaw(baseAddr + 1) & 0xF0) >> 4), (b & 0x02) ? c1 : c0);
            buf_[outPos+8] = i4ToTVCRGB(((mem->readRaw(baseAddr + 1) & 0xF0) >> 4), (b & 0x01) ? c1 : c0);

            c0 = bufp[4];
            c1 = bufp[5];
            b = bufp[6];

            buf_[outPos+ 9] = i4ToTVCRGB(((mem->readRaw(baseAddr + 2) & 0x0F)     ), (b & 0x80) ? c1 : c0);
            buf_[outPos+10] = i4ToTVCRGB(((mem->readRaw(baseAddr + 2) & 0x0F)     ), (b & 0x40) ? c1 : c0);
            
            buf_[outPos+11] = i4ToTVCRGB(((mem->readRaw(baseAddr + 2) & 0xF0) >> 4), (b & 0x20) ? c1 : c0);
            buf_[outPos+12] = i4ToTVCRGB(((mem->readRaw(baseAddr + 2) & 0xF0) >> 4), (b & 0x10) ? c1 : c0);

            buf_[outPos+13] = i4ToTVCRGB(((mem->readRaw(baseAddr + 3) & 0x0F)     ), (b & 0x08) ? c1 : c0);
            buf_[outPos+14] = i4ToTVCRGB(((mem->readRaw(baseAddr + 3) & 0x0F)     ), (b & 0x04) ? c1 : c0);

            buf_[outPos+15] = i4ToTVCRGB(((mem->readRaw(baseAddr + 3) & 0xF0) >> 4), (b & 0x02) ? c1 : c0);
            buf_[outPos+16] = i4ToTVCRGB(((mem->readRaw(baseAddr + 3) & 0xF0) >> 4), (b & 0x01) ? c1 : c0);


            bufp = bufp + 7;
            outPos += 17;
            *nBytes += 10;
          }
          else if (namedPortValues[REG_SCREEN_VIDEOMODE] == REG_SCREEN_VIDEOMODE_CHAR2)
          {
            div_t charLine = div(curLine - SPRITEEXT_FIRST_LINE, 8);
            uint32_t charAddr  = (TVC256_FASTRAM_START_SEGMENT<<14) + 
                                  namedPortValues[REG_SCREEN_SCREEN_BASE_ADDR]*0x400 +
                                  charLine.quot * 32 + (currSlot-1);
            uint8_t charVal    = mem->readRaw(charAddr);
            uint32_t colorAddr = (TVC256_FASTRAM_START_SEGMENT<<14) +
                                  namedPortValues[REG_SCREEN_SCREEN_COLOR_BASE_ADDR]*0x400 +
                                  charLine.quot * 32 + (currSlot-1);
            uint8_t colorVal   = mem->readRaw(colorAddr);
            uint32_t fontAddr  = (TVC256_FASTRAM_START_SEGMENT<<14) +
                                  namedPortValues[REG_SCREEN_FONT_BASE_ADDR]*0x800 +
                                  charVal * 8 + charLine.rem;
            uint8_t fontVal = mem->readRaw(fontAddr);
            unsigned char c0 = bufp[1];
            unsigned char c1 = bufp[2];
            unsigned char b  = bufp[3];
            buf_[outPos] = 0x08;
            buf_[outPos+1] = fontVal & 128 ? i4ToTVCRGB(colorVal,(b & 0xC0) ? c1 : c0) : (b & 0xC0) ? c1 : c0;
            buf_[outPos+2] = fontVal &  64 ? i4ToTVCRGB(colorVal,(b & 0x30) ? c1 : c0) : (b & 0x30) ? c1 : c0;
            buf_[outPos+3] = fontVal &  32 ? i4ToTVCRGB(colorVal,(b & 0x0C) ? c1 : c0) : (b & 0x0C) ? c1 : c0;
            buf_[outPos+4] = fontVal &  16 ? i4ToTVCRGB(colorVal,(b & 0x03) ? c1 : c0) : (b & 0x03) ? c1 : c0;
            c0 = bufp[4];
            c1 = bufp[5];
            b = bufp[6];
            buf_[outPos+5] = fontVal &   8 ? i4ToTVCRGB(colorVal,(b & 0xC0) ? c1 : c0) : (b & 0xC0) ? c1 : c0;
            buf_[outPos+6] = fontVal &   4 ? i4ToTVCRGB(colorVal,(b & 0x30) ? c1 : c0) : (b & 0x30) ? c1 : c0;
            buf_[outPos+7] = fontVal &   2 ? i4ToTVCRGB(colorVal,(b & 0x0C) ? c1 : c0) : (b & 0x0C) ? c1 : c0;
            buf_[outPos+8] = fontVal &   1 ? i4ToTVCRGB(colorVal,(b & 0x03) ? c1 : c0) : (b & 0x03) ? c1 : c0;
            bufp = bufp + 7;
            outPos += 9;
            *nBytes += 2;
          }
          else if (namedPortValues[REG_SCREEN_VIDEOMODE] == REG_SCREEN_VIDEOMODE_CHAR16)
          {
            div_t charLine = div(curLine - SPRITEEXT_FIRST_LINE, 8);
            uint32_t charAddr  = (TVC256_FASTRAM_START_SEGMENT<<14) + 
                                  namedPortValues[REG_SCREEN_SCREEN_BASE_ADDR]*0x400 +
                                  charLine.quot * 32 + (currSlot-1);
            uint8_t charVal    = mem->readRaw(charAddr);
            uint32_t fontAddr  = (TVC256_FASTRAM_START_SEGMENT<<14) +
                                  namedPortValues[REG_SCREEN_FONT_BASE_ADDR]*0x800 +
                                  charVal * 8 * 4 + charLine.rem * 4;
            uint8_t fontVal;
            unsigned char c0 = bufp[1];
            unsigned char c1 = bufp[2];
            unsigned char b  = bufp[3];
            buf_[outPos] = 0x08;
            fontVal = ((mem->readRaw(fontAddr  ) & 0xF0)>>4);
            buf_[outPos+1] = i4ToTVCRGB(fontVal,(b & 0xC0) ? c1 : c0);
            fontVal = ((mem->readRaw(fontAddr  ) & 0x0F));
            buf_[outPos+2] = i4ToTVCRGB(fontVal,(b & 0x30) ? c1 : c0);
            fontVal = ((mem->readRaw(fontAddr+1) & 0xF0)>>4);
            buf_[outPos+3] = i4ToTVCRGB(fontVal,(b & 0x0C) ? c1 : c0);
            fontVal = ((mem->readRaw(fontAddr+1) & 0x0F));
            buf_[outPos+4] = i4ToTVCRGB(fontVal,(b & 0x03) ? c1 : c0);
            c0 = bufp[4];
            c1 = bufp[5];
            b = bufp[6];
            fontVal = ((mem->readRaw(fontAddr+2) & 0xF0)>>4);
            buf_[outPos+5] = i4ToTVCRGB(fontVal,(b & 0xC0) ? c1 : c0);
            fontVal = ((mem->readRaw(fontAddr+2) & 0x0F));
            buf_[outPos+6] = i4ToTVCRGB(fontVal,(b & 0x30) ? c1 : c0);
            fontVal = ((mem->readRaw(fontAddr+3) & 0xF0)>>4);
            buf_[outPos+7] = i4ToTVCRGB(fontVal,(b & 0x0C) ? c1 : c0);
            fontVal = ((mem->readRaw(fontAddr+3) & 0x0F));
            buf_[outPos+8] = i4ToTVCRGB(fontVal,(b & 0x03) ? c1 : c0);
            bufp = bufp + 7;
            outPos += 9;
            *nBytes += 2;
          }
          else {
            buf_[outPos] = 0x06;
            buf_[outPos+1] = bufp[1];
            buf_[outPos+2] = bufp[2];
            buf_[outPos+3] = bufp[3];
            buf_[outPos+4] = bufp[4];
            buf_[outPos+5] = bufp[5];
            buf_[outPos+6] = bufp[6];
          bufp = bufp + 7;
          outPos += 7;
          }
          if (bufp >= endp)
            break;
        } while (bufp[0] == 0x06);
        break;
      case 0x08:                        // 8*2 pixels, 256 colors coded on 9 bytes -- TVC 4 color mode
        do {
            currSlot++;
          if (namedPortValues[REG_SCREEN_VIDEOMODE] == REG_SCREEN_VIDEOMODE_BITMAP)
          {
            buf_[outPos] = 0x08;
            uint32_t baseAddr = 
              ((TVC256_FASTRAM_START_SEGMENT + namedPortValues[REG_SCREEN_BITMAP_BASE_ADDR]*2)<<14) + 
              (curLine - SPRITEEXT_FIRST_LINE) * 128 + (currSlot-1)*4;
            buf_[outPos+1] = i4ToTVCRGB(((mem->readRaw(baseAddr)     & 0x0F)     ), bufp[1]);
            buf_[outPos+2] = i4ToTVCRGB(((mem->readRaw(baseAddr)     & 0xF0) >> 4), bufp[2]);
            buf_[outPos+3] = i4ToTVCRGB(((mem->readRaw(baseAddr + 1) & 0x0F)     ), bufp[3]);
            buf_[outPos+4] = i4ToTVCRGB(((mem->readRaw(baseAddr + 1) & 0xF0) >> 4), bufp[4]);
            buf_[outPos+5] = i4ToTVCRGB(((mem->readRaw(baseAddr + 2) & 0x0F)     ), bufp[5]);
            buf_[outPos+6] = i4ToTVCRGB(((mem->readRaw(baseAddr + 2) & 0xF0) >> 4), bufp[6]);
            buf_[outPos+7] = i4ToTVCRGB(((mem->readRaw(baseAddr + 3) & 0x0F)     ), bufp[7]);
            buf_[outPos+8] = i4ToTVCRGB(((mem->readRaw(baseAddr + 3) & 0xF0) >> 4), bufp[8]);
            bufp = bufp + 9;
            outPos += 9;
          } 
          else if (namedPortValues[REG_SCREEN_VIDEOMODE] == REG_SCREEN_VIDEOMODE_CHAR2)
          {
            div_t charLine = div(curLine - SPRITEEXT_FIRST_LINE, 8);
            uint32_t charAddr  = (TVC256_FASTRAM_START_SEGMENT<<14) + 
                                  namedPortValues[REG_SCREEN_SCREEN_BASE_ADDR]*0x400 +
                                  charLine.quot * 32 + (currSlot-1);
            uint8_t charVal    = mem->readRaw(charAddr);
            uint32_t colorAddr = (TVC256_FASTRAM_START_SEGMENT<<14) +
                                  namedPortValues[REG_SCREEN_SCREEN_COLOR_BASE_ADDR]*0x400 +
                                  charLine.quot * 32 + (currSlot-1);
            uint8_t colorVal   = mem->readRaw(colorAddr);
            uint32_t fontAddr  = (TVC256_FASTRAM_START_SEGMENT<<14) +
                                  namedPortValues[REG_SCREEN_FONT_BASE_ADDR]*0x800 +
                                  charVal * 8 + charLine.rem;
            uint8_t fontVal = mem->readRaw(fontAddr);
            buf_[outPos] = 0x08;
            buf_[outPos+1] = fontVal & 128 ? i4ToTVCRGB(colorVal,bufp[1]) : bufp[1];
            buf_[outPos+2] = fontVal &  64 ? i4ToTVCRGB(colorVal,bufp[2]) : bufp[2];
            buf_[outPos+3] = fontVal &  32 ? i4ToTVCRGB(colorVal,bufp[3]) : bufp[3];
            buf_[outPos+4] = fontVal &  16 ? i4ToTVCRGB(colorVal,bufp[4]) : bufp[4];
            buf_[outPos+5] = fontVal &   8 ? i4ToTVCRGB(colorVal,bufp[5]) : bufp[5];
            buf_[outPos+6] = fontVal &   4 ? i4ToTVCRGB(colorVal,bufp[6]) : bufp[6];
            buf_[outPos+7] = fontVal &   2 ? i4ToTVCRGB(colorVal,bufp[7]) : bufp[7];
            buf_[outPos+8] = fontVal &   1 ? i4ToTVCRGB(colorVal,bufp[8]) : bufp[8];
            bufp = bufp + 9;
            outPos += 9;
          } 
          else if (namedPortValues[REG_SCREEN_VIDEOMODE] == REG_SCREEN_VIDEOMODE_CHAR16)
          {
            div_t charLine = div(curLine - SPRITEEXT_FIRST_LINE, 8);
            uint32_t charAddr  = (TVC256_FASTRAM_START_SEGMENT<<14) + 
                                  namedPortValues[REG_SCREEN_SCREEN_BASE_ADDR]*0x400 +
                                  charLine.quot * 32 + (currSlot-1);
            uint8_t charVal    = mem->readRaw(charAddr);
            uint32_t fontAddr  = (TVC256_FASTRAM_START_SEGMENT<<14) +
                                  namedPortValues[REG_SCREEN_FONT_BASE_ADDR]*0x800 +
                                  charVal * 8 * 4 + charLine.rem * 4;
            uint8_t fontVal;
            buf_[outPos] = 0x08;
            fontVal = ((mem->readRaw(fontAddr  ) & 0xF0)>>4);
            buf_[outPos+1] = i4ToTVCRGB(fontVal,bufp[1]);
            fontVal = ((mem->readRaw(fontAddr  ) & 0x0F));
            buf_[outPos+2] = i4ToTVCRGB(fontVal,bufp[2]);
            fontVal = ((mem->readRaw(fontAddr+1) & 0xF0)>>4);
            buf_[outPos+3] = i4ToTVCRGB(fontVal,bufp[3]);
            fontVal = ((mem->readRaw(fontAddr+1) & 0x0F));
            buf_[outPos+4] = i4ToTVCRGB(fontVal,bufp[4]);
            fontVal = ((mem->readRaw(fontAddr+2) & 0xF0)>>4);
            buf_[outPos+5] = i4ToTVCRGB(fontVal,bufp[5]);
            fontVal = ((mem->readRaw(fontAddr+2) & 0x0F));
            buf_[outPos+6] = i4ToTVCRGB(fontVal,bufp[6]);
            fontVal = ((mem->readRaw(fontAddr+3) & 0xF0)>>4);
            buf_[outPos+7] = i4ToTVCRGB(fontVal,bufp[7]);
            fontVal = ((mem->readRaw(fontAddr+3) & 0x0F));
            buf_[outPos+8] = i4ToTVCRGB(fontVal,bufp[8]);
            bufp = bufp + 9;
            outPos += 9;
          } else {
            buf_[outPos] = 0x08;
            buf_[outPos+1] = bufp[1];
            buf_[outPos+2] = bufp[2];
            buf_[outPos+3] = bufp[3];
            buf_[outPos+4] = bufp[4];
            buf_[outPos+5] = bufp[5];
            buf_[outPos+6] = bufp[6];
            buf_[outPos+7] = bufp[7];
            buf_[outPos+8] = bufp[8];

          bufp = bufp + 9;
          outPos += 9;
          }
          if (bufp >= endp)
            break;
        } while (bufp[0] == 0x08);
        break;
      default:                          // invalid flag byte
        do {
          buf_[outPos++] = 0x00;
        } while (outPos < sizeof(buf_));
        break;
      }
    } while (bufp < endp && outPos < *nBytes);
//    printf("Line converted, from %d to %d bytes, compare %d\n",*nBytes, outPos, std::memcmp(&buf_[0],buf, *nBytes));
    return &buf_[0];
  }

  static int safe_read(int fd, uint8_t *buffer, int size)
  {
    int all = 0;
    while (size) {
      int ret = read(fd, buffer, size);
      if (ret <= 0)
        break;
      all += ret;
      size -= ret;
      buffer += ret;
    }
    return all;
  }

  void SpriteExt::_block_read()
  {
  }

  /* SPI is a read/write in once stuff. We have only a single function ...
   * _write_b is the data value to put on MOSI
   * _read_b is the data read from MISO without spending _ANY_ SPI time to do
   * shifting!
   * This is not a real thing, but easier to code this way.
   * The implementation of the real behaviour is up to the caller of this
   * function.
   */
  void SpriteExt::_spi_shifting_with_sd_card()
  {
  }

  /* Warning:
   * Some resources mention addresses like 0xFC00 for the I/O area.
   * Here, I mean addresses within segment 7 only, so it becomes 0x3C00 ...
   */

  uint8_t SpriteExt::readCartP3(uint32_t addr)
  {
    return 0xFF;        // make GCC happy :)
  }

  void SpriteExt::writeCartP3(uint32_t addr, uint8_t data)
  {
  }

  uint8_t SpriteExt::readCartP3Debug(uint32_t addr) const
  {
  }

  // --------------------------------------------------------------------------

  uint8_t SpriteExt::flashRead(uint32_t addr)
  {
    return 0xFF;
  }

  void SpriteExt::flashWrite(uint32_t addr, uint8_t data)
  {
  }

  uint8_t SpriteExt::flashReadDebug(uint32_t addr) const
  {
    return 0xFF;
  }

  // --------------------------------------------------------------------------

  class ChunkType_SpriteExtSnapshot : public Ep128Emu::File::ChunkTypeHandler {
   private:
    SpriteExt&  ref;
   public:
    ChunkType_SpriteExtSnapshot(SpriteExt& ref_)
      : Ep128Emu::File::ChunkTypeHandler(),
        ref(ref_)
    {
    }
    virtual ~ChunkType_SpriteExtSnapshot()
    {
    }
    virtual Ep128Emu::File::ChunkType getChunkType() const
    {
      return Ep128Emu::File::EP128EMU_CHUNKTYPE_SPRITEEXT_STATE;
    }
    virtual void processChunk(Ep128Emu::File::Buffer& buf)
    {
      ref.loadState(buf);
    }
  };

  void SpriteExt::saveState(Ep128Emu::File::Buffer& buf)
  {
    buf.setPosition(0);
    buf.writeUInt32(0x01000001U);       // version number
    buf.writeBoolean(spriteExt_enabled);
  }

  void SpriteExt::saveState(Ep128Emu::File& f)
  {
    Ep128Emu::File::Buffer  buf;
    this->saveState(buf);
    f.addChunk(Ep128Emu::File::EP128EMU_CHUNKTYPE_SPRITEEXT_STATE, buf);
  }

  void SpriteExt::loadState(Ep128Emu::File::Buffer& buf)
  {
    buf.setPosition(0);
    // check version number
    unsigned int  version = buf.readUInt32();
    if (version != 0x01000001U) {
      buf.setPosition(buf.getDataSize());
      throw Ep128Emu::Exception("incompatible spriteext snapshot format");
    }
    try {
      // save flash ROM first if changed, reset it to erased state
      openROMFile((char *) 0);
      // reset the interface as most registers are not saved in the snapshot
      this->reset(1);
      // load saved state
      setEnabled(buf.readBoolean());
    }
    catch (...) {
      // reset spriteext
      this->reset(2);
      openROMFile((char *) 0);
      throw;
    }
  }

  void SpriteExt::registerChunkType(Ep128Emu::File& f)
  {
    ChunkType_SpriteExtSnapshot *p;
    p = new ChunkType_SpriteExtSnapshot(*this);
    try {
      f.registerChunkType(p);
    }
    catch (...) {
      delete p;
      throw;
    }
  }

}       // namespace Ep128

