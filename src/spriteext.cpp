
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
   - Write to port 0x00 to set scroll border as well
   - Scroll
   - 2-byte sprite position handling
   - Ext graphics calculation delay from prev line
   - Sprite collision
   - Sprite interrupt
   - Screen height setting
   
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

#define SET_BIT(p,n) (p) |=  (1 << (n))
#define CLR_BIT(p,n) (p) &= ~(1 << (n))
#define UPD_BIT(p,n,v) if ((v)) (p) |=  (1 << (n)); else (p) &= ~(1 << (n))
namespace Ep128 {

  // Port masks to prevent unreasonable values on write
  static const uint8_t namedPortMasks[256] = {
      0,    0,    0,    0,   0,    0,    0,    0,   0,  0,  0,  0,   0,  0,  0,  0,
      0,    0,    0,    0,   0,    0,    0,    0,   0,  0,  0,  0,   0,  0,  0,  0,
      0,    0,    0,    0,   0,    0,    0,    0,   0,  0,  0,  0,   0,  0,  0,  0,
      0,    0,    0,    0,   0,    0,    0,    0,   0,  0,  0,  0,   0,  0,  0,  0,
    0xF,  0xF,  0xF,  0xF, 0xF,  0xF,  0xF,  0xF, 0xF,0xF,0xF,0xF, 0xF,0xF,0xF,0xF,
      0,    0,    0,    0,   0,    0,    0,    0,   0,  0,  0,  0,   0,  0,  0,  0,
      1,    1,    1,    1,   1,    1,    1,    1,   1,  1,  1,  1,   1,  1,  1,  1,
      1,    1,    1,    1,   1,    1,    1,    1,   1,  1,  1,  1,   1,  1,  1,  1,
      1,    1,    1,    1,   1,    1,    1,    1,   1,  1,  1,  1,   1,  1,  1,  1,
      0,    0,    0,    0,   0,    0,    0,    0,   0,  0,  0,  0,   0,  0,  3,  0,
   0x03, 0x3f, 0x3f, 0x1f,   1, 0x87, 0x87,  0xF,   0,  0,  0,  0,   0,  0,  0,  0,
      0,    0,    0,    1,   0,    1,    0,    0,   0,  0,  0,  0,   0,  0,  0,  0,
      0,    0,    0,    0,   0,    0,    0,    0,   0,  0,  0,  0,   0,  0,  0,  0,
      0,    0,    0,    0,   0,    0,    0,    0,   0,  0,  0,  0,   0,  0,  0,  0,
      0,    0,    0,    0,   0,    0,    0,    0,   0,  0,  0,  0,   0,  0,  0,  0,
      0,    0,    0,    0,   0,    0,    0,    0,   0,  0,  0,  0,   0,  0,  0,  0
};


  SpriteExt::SpriteExt()
    : spriteExt_enabled(false),
      anyGfxEnabled(false),
      spriteExtSegment(0xFFFFFFFFU),
      spriteExtAddress(0xFFFFFFFFU),
      curLine(0),
      scrollX(0),
      scrollY(0),
      scrollBorderX(false),
      scrollBorderY(false)
  {
    for (int i = 0; i < 16; i++)
      io_port_values[i] = 0xFF;

    io_port_values[SPRITEEXT_REG_INCREMENT]     = SPRITEEXT_REG_INCREMENT_DEFAULT;
    io_port_values[SPRITEEXT_SEC_REG_INCREMENT] = SPRITEEXT_REG_INCREMENT_DEFAULT;

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

  // Convert one 4-bit 16-color value to a 8-bit TVCemu specific format
  // except when color is transparent - then fall back to provided value
  uint8_t SpriteExt::i4ToTVCRGB(uint8_t val, uint8_t transparent_val)
  {
    // igrb with double bits - see TVCVideo::convertPixelToRGB
    // todo: replace with fixed palette conversion array for emulation speed
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
       case REG_MEMORY_P2:
       case REG_MEMORY_P3:
       case REG_MEMORY_MAP_8M_P2_LOW:
       case REG_MEMORY_MAP_8M_P2_HIGH:
       case REG_MEMORY_MAP_8M_P3_LOW:
       case REG_MEMORY_MAP_8M_P3_HIGH:
         retval = namedPortValues[portAddr];
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
     
     // Apply mask to prevent illegal values
     if (namedPortMasks[portAddr])
        value = value & namedPortMasks[portAddr];

     switch (portAddr)
     {
       // Note: rest of mouse handling is done in TVC64VM::ioPortReadCallback
       // to avoid passing emulated mouse data
       case REG_USB_MOUSE_SPEED:
         namedPortValues[REG_USB_MOUSE_SPEED] = value;
         updateMouseSpeed(value);
       break;
       // Combined enable/disable registers.
       // Note: these are delayed on real HW
       case REG_SPRITE_FOREGROUND_LOW:
         namedPortValues[portAddr] = value;
         for (int i = 0; i< 8; i++)
         {
            namedPortValues[REG_SPRITE_FOREGROUND + i] = value & (1<<i) ? 1 : 0;
         }
       break;
       case REG_SPRITE_FOREGROUND_HIGH:
         namedPortValues[portAddr] = value;
         for (int i = 0; i< 8; i++)
         {
            namedPortValues[REG_SPRITE_FOREGROUND + 8 + i] = value & (1<<i) ? 1 : 0;
         }
         break;
       case REG_SPRITE_ENABLE_LOW:
         namedPortValues[portAddr] = value;
         for (int i = 0; i< 8; i++)
         {
            namedPortValues[REG_SPRITE_ENABLE + i] = value & (1<<i) ? 1 : 0;
         }
       break;
       case REG_SPRITE_ENABLE_HIGH:
         namedPortValues[portAddr] = value;
         for (int i = 0; i< 8; i++)
         {
            namedPortValues[REG_SPRITE_ENABLE + 8 + i] = value & (1<<i) ? 1 : 0;
         }
         break;
       case REG_SPRITE_COLORMODE_LOW:
         namedPortValues[portAddr] = value;
         for (int i = 0; i< 8; i++)
         {
            namedPortValues[REG_SPRITE_COLORMODE + i] = value & (1<<i) ? 1 : 0;
         }
       break;
       case REG_SPRITE_COLORMODE_HIGH:
         namedPortValues[portAddr] = value;
         for (int i = 0; i< 8; i++)
         {
            namedPortValues[REG_SPRITE_COLORMODE + 8 + i] = value & (1<<i) ? 1 : 0;
         }
         break;
       case REG_SCREEN_SCROLL_X:
         namedPortValues[portAddr] = value;
         scrollX = value & 0x07;
         scrollBorderX = value & 0x80;
         break;
       case REG_SCREEN_SCROLL_Y:
         namedPortValues[portAddr] = value;
         scrollY = value & 0x07;
         scrollBorderY = value & 0x80;
         break;
       default:
         // Video related ports are written instantly (lot of TODO here)
         if (portAddr <= REG_SCREEN_MAXY)
            namedPortValues[portAddr] = value;

         // Update bits of combined registers separately
         if (portAddr >= REG_SPRITE_FOREGROUND && portAddr < REG_SPRITE_FOREGROUND + SPRITEEXT_SPRITE_MAX)
         {
            if (portAddr < REG_SPRITE_FOREGROUND + 0x08)
               UPD_BIT(namedPortValues[REG_SPRITE_FOREGROUND_LOW ], portAddr - REG_SPRITE_FOREGROUND,        value);
            else
               UPD_BIT(namedPortValues[REG_SPRITE_FOREGROUND_HIGH], portAddr - REG_SPRITE_FOREGROUND - 0x08, value);
         }
         else if (portAddr >= REG_SPRITE_ENABLE && portAddr < REG_SPRITE_ENABLE + SPRITEEXT_SPRITE_MAX)
         {
            if (portAddr < REG_SPRITE_ENABLE     + 0x08)
               UPD_BIT(namedPortValues[REG_SPRITE_ENABLE_LOW     ], portAddr - REG_SPRITE_ENABLE,            value);
            else
               UPD_BIT(namedPortValues[REG_SPRITE_ENABLE_HIGH    ], portAddr - REG_SPRITE_ENABLE     - 0x08, value);
         }
         else if (portAddr >= REG_SPRITE_COLORMODE && portAddr < REG_SPRITE_COLORMODE + SPRITEEXT_SPRITE_MAX)
         {
            if (portAddr < REG_SPRITE_COLORMODE  + 0x08)
               UPD_BIT(namedPortValues[REG_SPRITE_COLORMODE_LOW  ], portAddr - REG_SPRITE_COLORMODE,         value);
            else
               UPD_BIT(namedPortValues[REG_SPRITE_COLORMODE_HIGH ], portAddr - REG_SPRITE_COLORMODE  - 0x08, value);
         }
     }

  // Increment register index after operation
  if (secondary)
    io_port_values[SPRITEEXT_SEC_REG_INDEX] += io_port_values[SPRITEEXT_SEC_REG_INCREMENT];
  else
    io_port_values[SPRITEEXT_REG_INDEX] += io_port_values[SPRITEEXT_REG_INCREMENT];

  updateAnyGfxEnabled();

  }

  void SpriteExt::updateMouseSpeed(uint8_t binValue)
  {
     // Special format: top 2 bits whole + bottom 5(!) bits fraction
     mouse_speed = ((binValue & 0xC0) >> 6) + (float)(binValue & 0x1F) * 1/32;
  }

  // Speed shortcut: if there isn't any gfx enhancement enabled, whole updateLine can be skipped
  void SpriteExt::updateAnyGfxEnabled()
  {
     for (int i=0; i<16; i++)
     {
      if (namedPortValues[REG_SPRITE_ENABLE + i])
      {
        anyGfxEnabled = true;
        return;
      }      
     }
     anyGfxEnabled = (bool) namedPortValues[REG_SCREEN_VIDEOMODE] || (bool) scrollX || (bool) scrollY;
  }
  
  // Sprite handling. Note that "slot" consists of 16 pixels, but in the full PAL resolution
  // to process transparency correctly also in case of graphics 2,
  // while sprites only support vertical 256 pixels, so each result pixel is doubled
  void SpriteExt::updateLineWithSprite(uint8_t *buf, uint8_t currSlot, Ep128::Memory *mem, size_t spriteNum)
  {
     for (size_t j=0; j<8; j++)
     {
        int spritePosX = ((currSlot - 1) * 16 + j*2 - (namedPortValues[REG_SPRITE_X+spriteNum*2] - 24)*2)/2;
        if (spritePosX < 0 || spritePosX > 23)
         continue;
        int spritePosY = curLine - SPRITEEXT_FIRST_LINE - (namedPortValues[REG_SPRITE_Y+spriteNum*2] - 21);
        if (spritePosY < 0 || spritePosY > 20)
         continue;
        // 2-color sprite: one line of 24 pixels is 3 bytes, bitmapped, color from register
        if (namedPortValues[REG_SPRITE_COLORMODE + spriteNum] == 0)
        {
           uint32_t spriteBaseAddr  = 
                      ((TVC256_FASTRAM_START_SEGMENT+namedPortValues[REG_SPRITE_BASE_ADDR])<<14) + 
                      spriteNum * 0x400 +
                      namedPortValues[REG_SPRITE_PHASE+spriteNum] * 0x40 +
                      spritePosY * 3;
           uint32_t spriteBits = mem->readRaw(spriteBaseAddr) << 16 | mem->readRaw(spriteBaseAddr+1) << 8 | mem->readRaw(spriteBaseAddr+2);
           if (spriteBits & (uint32_t)(1 << 23-spritePosX))
              buf[j*2] = buf[j*2+1] = i4ToTVCRGB(namedPortValues[REG_SPRITE_COLOR+spriteNum], buf[j*2]);
        // 16-color sprite: one line of 24 pixels is 12 bytes, one per nibble incl. color w/transparency
        } else {
           uint32_t spriteBaseAddr  = 
                      ((TVC256_FASTRAM_START_SEGMENT+namedPortValues[REG_SPRITE_BASE_ADDR])<<14) + 
                      spriteNum * 0x1000 +
                      namedPortValues[REG_SPRITE_PHASE+spriteNum] * 0x100 +
                      spritePosY * 12 + (spritePosX >> 1);
           uint8_t spriteBits = (spritePosX) % 2 ? (mem->readRaw(spriteBaseAddr)>>4) & 0x0F : mem->readRaw(spriteBaseAddr) & 0x0F;
           buf[j*2    ] = i4ToTVCRGB(spriteBits, buf[j*2    ]);
           buf[j*2 + 1] = i4ToTVCRGB(spriteBits, buf[j*2 + 1]);
        }
     }
  }

  // Handle one slot (16 PAL "pixels"), in-place overwrite pixels with overlay content where needed.
  void SpriteExt::updateLineWithGfx(size_t outPos, uint8_t currSlot, Ep128::Memory *mem)
  {
     uint8_t * buf = &buf_[outPos];

     // Scroll border top/bottom
     if (scrollBorderY && (curLine - SPRITEEXT_FIRST_LINE < 8 || SPRITEEXT_LAST_LINE - curLine < 8))
     {
        for (size_t i=0; i<16; i++)
        {
          buf[i] = i4ToTVCRGB(namedPortValues[REG_SCREEN_BORDER_COLOR],0x00);
        }
        return;
     }

     for (size_t i=0; i<16; i++)
     {
        if (namedPortValues[REG_SPRITE_ENABLE+i] && !namedPortValues[REG_SPRITE_FOREGROUND+i])
        {
              updateLineWithSprite(buf, currSlot, mem, i);
        }
     }

     // Bitmap mode: one nibble (half byte) -- one pixel
     if (namedPortValues[REG_SCREEN_VIDEOMODE] == REG_SCREEN_VIDEOMODE_BITMAP)
     {
        uint32_t baseAddr = 
              ((TVC256_FASTRAM_START_SEGMENT + namedPortValues[REG_SCREEN_BITMAP_BASE_ADDR]*2)<<14) + 
              (curLine + scrollY - SPRITEEXT_FIRST_LINE) * 128 + (currSlot-1)*4;
        buf[ 0] = i4ToTVCRGB(((mem->readRaw(baseAddr)     & 0x0F)     ), buf[ 0]);
        buf[ 1] = i4ToTVCRGB(((mem->readRaw(baseAddr)     & 0x0F)     ), buf[ 1]);
        buf[ 2] = i4ToTVCRGB(((mem->readRaw(baseAddr)     & 0xF0) >> 4), buf[ 2]);
        buf[ 3] = i4ToTVCRGB(((mem->readRaw(baseAddr)     & 0xF0) >> 4), buf[ 3]);
        buf[ 4] = i4ToTVCRGB(((mem->readRaw(baseAddr + 1) & 0x0F)     ), buf[ 4]);
        buf[ 5] = i4ToTVCRGB(((mem->readRaw(baseAddr + 1) & 0x0F)     ), buf[ 5]);
        buf[ 6] = i4ToTVCRGB(((mem->readRaw(baseAddr + 1) & 0xF0) >> 4), buf[ 6]);
        buf[ 7] = i4ToTVCRGB(((mem->readRaw(baseAddr + 1) & 0xF0) >> 4), buf[ 7]);
        buf[ 8] = i4ToTVCRGB(((mem->readRaw(baseAddr + 2) & 0x0F)     ), buf[ 8]);
        buf[ 9] = i4ToTVCRGB(((mem->readRaw(baseAddr + 2) & 0x0F)     ), buf[ 9]);
        buf[10] = i4ToTVCRGB(((mem->readRaw(baseAddr + 2) & 0xF0) >> 4), buf[10]);
        buf[11] = i4ToTVCRGB(((mem->readRaw(baseAddr + 2) & 0xF0) >> 4), buf[11]);
        buf[12] = i4ToTVCRGB(((mem->readRaw(baseAddr + 3) & 0x0F)     ), buf[12]);
        buf[13] = i4ToTVCRGB(((mem->readRaw(baseAddr + 3) & 0x0F)     ), buf[13]);
        buf[14] = i4ToTVCRGB(((mem->readRaw(baseAddr + 3) & 0xF0) >> 4), buf[14]);
        buf[15] = i4ToTVCRGB(((mem->readRaw(baseAddr + 3) & 0xF0) >> 4), buf[15]);
     }
     // 2-color char mode: char value points to font definition, if bit is set then use color map
     else if (namedPortValues[REG_SCREEN_VIDEOMODE] == REG_SCREEN_VIDEOMODE_CHAR2)
     {
      div_t charLine = div(curLine + scrollY - SPRITEEXT_FIRST_LINE, 8);
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
      buf[ 0] = fontVal & 0x80 ? i4ToTVCRGB(colorVal,buf[ 0]) : buf[ 0];
      buf[ 1] = fontVal & 0x80 ? i4ToTVCRGB(colorVal,buf[ 1]) : buf[ 1];
      buf[ 2] = fontVal & 0x40 ? i4ToTVCRGB(colorVal,buf[ 2]) : buf[ 2];
      buf[ 3] = fontVal & 0x40 ? i4ToTVCRGB(colorVal,buf[ 3]) : buf[ 3];
      buf[ 4] = fontVal & 0x20 ? i4ToTVCRGB(colorVal,buf[ 4]) : buf[ 4];
      buf[ 5] = fontVal & 0x20 ? i4ToTVCRGB(colorVal,buf[ 5]) : buf[ 5];
      buf[ 6] = fontVal & 0x10 ? i4ToTVCRGB(colorVal,buf[ 6]) : buf[ 6];
      buf[ 7] = fontVal & 0x10 ? i4ToTVCRGB(colorVal,buf[ 7]) : buf[ 7];
      buf[ 8] = fontVal & 0x08 ? i4ToTVCRGB(colorVal,buf[ 8]) : buf[ 8];
      buf[ 9] = fontVal & 0x08 ? i4ToTVCRGB(colorVal,buf[ 9]) : buf[ 9];
      buf[10] = fontVal & 0x04 ? i4ToTVCRGB(colorVal,buf[10]) : buf[10];
      buf[11] = fontVal & 0x04 ? i4ToTVCRGB(colorVal,buf[11]) : buf[11];
      buf[12] = fontVal & 0x02 ? i4ToTVCRGB(colorVal,buf[12]) : buf[12];
      buf[13] = fontVal & 0x02 ? i4ToTVCRGB(colorVal,buf[13]) : buf[13];
      buf[14] = fontVal & 0x01 ? i4ToTVCRGB(colorVal,buf[14]) : buf[14];
      buf[15] = fontVal & 0x01 ? i4ToTVCRGB(colorVal,buf[15]) : buf[15];
    }
    // 16-color char mode: char value points to font definition, one nibble (half byte) of font definition -- one pixel
    else if (namedPortValues[REG_SCREEN_VIDEOMODE] == REG_SCREEN_VIDEOMODE_CHAR16)
    {
      div_t charLine = div(curLine + scrollY - SPRITEEXT_FIRST_LINE, 8);
      uint32_t charAddr  = (TVC256_FASTRAM_START_SEGMENT<<14) + 
                            namedPortValues[REG_SCREEN_SCREEN_BASE_ADDR]*0x400 +
                            charLine.quot * 32 + (currSlot-1);
      uint8_t charVal    = mem->readRaw(charAddr);
      uint32_t fontAddr  = (TVC256_FASTRAM_START_SEGMENT<<14) +
                            namedPortValues[REG_SCREEN_FONT_BASE_ADDR]*0x800 +
                            charVal * 8 * 4 + charLine.rem * 4;
      uint8_t fontVal;
      fontVal = ((mem->readRaw(fontAddr  ) & 0xF0)>>4);
      buf[ 0] = i4ToTVCRGB(fontVal, buf[ 0]);
      buf[ 1] = i4ToTVCRGB(fontVal, buf[ 1]);

      fontVal = ((mem->readRaw(fontAddr  ) & 0x0F));
      buf[ 2] = i4ToTVCRGB(fontVal, buf[ 2]);
      buf[ 3] = i4ToTVCRGB(fontVal, buf[ 3]);

      fontVal = ((mem->readRaw(fontAddr+1) & 0xF0)>>4);
      buf[ 4] = i4ToTVCRGB(fontVal, buf[ 4]);
      buf[ 5] = i4ToTVCRGB(fontVal, buf[ 5]);

      fontVal = ((mem->readRaw(fontAddr+1) & 0x0F));
      buf[ 6] = i4ToTVCRGB(fontVal, buf[ 6]);
      buf[ 7] = i4ToTVCRGB(fontVal, buf[ 7]);

      fontVal = ((mem->readRaw(fontAddr+2) & 0xF0)>>4);
      buf[ 8] = i4ToTVCRGB(fontVal, buf[ 8]);
      buf[ 9] = i4ToTVCRGB(fontVal, buf[ 9]);

      fontVal = ((mem->readRaw(fontAddr+2) & 0x0F));
      buf[10] = i4ToTVCRGB(fontVal, buf[10]);
      buf[11] = i4ToTVCRGB(fontVal, buf[11]);

      fontVal = ((mem->readRaw(fontAddr+3) & 0xF0)>>4);
      buf[12] = i4ToTVCRGB(fontVal, buf[12]);
      buf[13] = i4ToTVCRGB(fontVal, buf[13]);

      fontVal = ((mem->readRaw(fontAddr+3) & 0x0F));
      buf[14] = i4ToTVCRGB(fontVal, buf[14]);
      buf[15] = i4ToTVCRGB(fontVal, buf[15]);
    }

     // Repeat sprite calc for foreground
     for (size_t i=0; i<16; i++)
     {
        if (namedPortValues[REG_SPRITE_ENABLE+i] && namedPortValues[REG_SPRITE_FOREGROUND+i])
        {
           updateLineWithSprite(buf, currSlot, mem, i);
        }
     }
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
    if (!(*nBytes) || curLine < SPRITEEXT_FIRST_LINE || curLine > SPRITEEXT_LAST_LINE || !anyGfxEnabled)
      return buf;
   // todo: screen height limit
   // todo: border color
   // Note: line pixels are according to PAL (768).
    do {
      switch (bufp[0]) {
      // Several modes do not occur in content area (or at all in case of TVC), those can be just copied
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
           std::memcpy(&(buf_[outPos]), bufp, 2);
          bufp = bufp + 2;
          outPos += 2;
          if (bufp >= endp)
            break;
        } while (bufp[0] == 0x01);
        break;
      case 0x02:                        // 2x8 pixels, 256 colors coded on 3 bytes -- not used for TVC
        do {
           std::memcpy(&(buf_[outPos]), bufp, 3);
          bufp = bufp + 3;
          outPos += 3;
          if (bufp >= endp)
            break;
        } while (bufp[0] == 0x02);
        break;
      case 0x03:                        // 8x2 pixels, 2 colors coded on 4 bytes -- not used for TVC
        do {
           std::memcpy(&(buf_[outPos]), bufp, 4);

          bufp = bufp + 4;
          outPos += 4;
          if (bufp >= endp)
            break;
        } while (bufp[0] == 0x03);
        break;
      // To simplify the overlay logic, convert all content modes to a new mode 0x09 which can cover 
      // all resolution with 256 colors (16 would be enough for TVC, but let's not complicate it)
      // This way, overlay pixel calculation logic can be done only once for all modes.
      case 0x04:                        // 4x4 pixels, 256 colors coded on 5 bytes -- TVC 16 color mode
        do {
            currSlot++;
            buf_[outPos] = 0x09;
            buf_[outPos +  1] = bufp[1];
            buf_[outPos +  2] = bufp[1];
            buf_[outPos +  3] = bufp[1];
            buf_[outPos +  4] = bufp[1];
            buf_[outPos +  5] = bufp[2];
            buf_[outPos +  6] = bufp[2];
            buf_[outPos +  7] = bufp[2];
            buf_[outPos +  8] = bufp[2];
            buf_[outPos +  9] = bufp[3];
            buf_[outPos + 10] = bufp[3];
            buf_[outPos + 11] = bufp[3];
            buf_[outPos + 12] = bufp[3];
            buf_[outPos + 13] = bufp[4];
            buf_[outPos + 14] = bufp[4];
            buf_[outPos + 15] = bufp[4];
            buf_[outPos + 16] = bufp[4];
            
            updateLineWithGfx(outPos+1, currSlot, mem);
            bufp    +=  5;
            outPos  += 17;
            *nBytes += 12;

          if (bufp >= endp)
            break;
        } while (bufp[0] == 0x04);
        break;
      case 0x06:                        // 2*8*2 pixels, 2*2 colors coded on 7 bytes -- TVC 2 color mode
        do {
            currSlot++;
            unsigned char c0 = bufp[1];
            unsigned char c1 = bufp[2];
            unsigned char b = bufp[3];
            buf_[outPos] = 0x09;
            buf_[outPos+1] = (b & 0x80) ? c1 : c0;
            buf_[outPos+2] = (b & 0x40) ? c1 : c0;
            buf_[outPos+3] = (b & 0x20) ? c1 : c0;
            buf_[outPos+4] = (b & 0x10) ? c1 : c0;
            buf_[outPos+5] = (b & 0x08) ? c1 : c0;
            buf_[outPos+6] = (b & 0x04) ? c1 : c0;
            buf_[outPos+7] = (b & 0x02) ? c1 : c0;
            buf_[outPos+8] = (b & 0x01) ? c1 : c0;
            c0 = bufp[4];
            c1 = bufp[5];
            b = bufp[6];
            buf_[outPos+ 9] = (b & 0x80) ? c1 : c0;
            buf_[outPos+10] = (b & 0x40) ? c1 : c0;
            buf_[outPos+11] = (b & 0x20) ? c1 : c0;
            buf_[outPos+12] = (b & 0x10) ? c1 : c0;
            buf_[outPos+13] = (b & 0x08) ? c1 : c0;
            buf_[outPos+14] = (b & 0x04) ? c1 : c0;
            buf_[outPos+15] = (b & 0x02) ? c1 : c0;
            buf_[outPos+16] = (b & 0x01) ? c1 : c0;

            updateLineWithGfx(outPos+1, currSlot, mem);
            bufp    +=  7;
            outPos  += 17;
            *nBytes += 10;

          if (bufp >= endp)
            break;
        } while (bufp[0] == 0x06);
        break;
      case 0x08:                        // 8*2 pixels, 256 colors coded on 9 bytes -- TVC 4 color mode
        do {
            currSlot++;
            buf_[outPos] = 0x09;
            buf_[outPos +  1] = bufp[1];
            buf_[outPos +  2] = bufp[1];
            buf_[outPos +  3] = bufp[2];
            buf_[outPos +  4] = bufp[2];
            buf_[outPos +  5] = bufp[3];
            buf_[outPos +  6] = bufp[3];
            buf_[outPos +  7] = bufp[4];
            buf_[outPos +  8] = bufp[4];
            buf_[outPos +  9] = bufp[5];
            buf_[outPos + 10] = bufp[5];
            buf_[outPos + 11] = bufp[6];
            buf_[outPos + 12] = bufp[6];
            buf_[outPos + 13] = bufp[7];
            buf_[outPos + 14] = bufp[7];
            buf_[outPos + 15] = bufp[8];
            buf_[outPos + 16] = bufp[8];
            
            updateLineWithGfx(outPos+1, currSlot, mem);
            bufp    +=  9;
            outPos  += 17;
            *nBytes +=  8;

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

