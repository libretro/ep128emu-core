
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
   - Fix standalone version joystick handling
   - Fake gamecard rom for presenting the ID string
   - State save/load support

   TVC256++ gfx:
   - Frame buffer overlay of 256x240
   - Write to port 0x00 to set sprite border as well
   - Sprite only gfx
   - 2-color char screen
   - 16-color char screen
   - 16-color bitmap screen
   - Scroll
   - Sprite 2-color
   - Sprite 16-color
   - Sprite foreground / background
   - Ext graphics calculation line-by-line
   - Sprite registers
   - Sprite interrupt
   - Screen height setting
   
   TVC256++ drives:
   - USB drive handling
   - Flash mem drive handling
   - PSRAM drive handling
   
   TVC256++ others:
   - Initial memory fill
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

#include "spriteext.hpp"
#include "ide.hpp"

namespace Ep128 {


  SpriteExt::SpriteExt()
    : spriteExt_enabled(false),
      spriteExtSegment(0xFFFFFFFFU),
      spriteExtAddress(0xFFFFFFFFU)
  {
    for (int i = 0; i < 16; i++)
      io_port_values[i] = 0xFF;
    io_port_values[SPRITEEXT_REG_INCREMENT] = 0x01;
    io_port_values[SPRITEEXT_SEC_REG_INCREMENT] = 0x01;

    for (int i = 0; i < 256; i++)
      namedPortValues[i] = 0x00;
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
     switch (secondary ? io_port_values[SPRITEEXT_SEC_REG_INDEX] : io_port_values[SPRITEEXT_REG_INDEX])
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
         retval = 0xFF;
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
     switch (secondary ? io_port_values[SPRITEEXT_SEC_REG_INDEX] : io_port_values[SPRITEEXT_REG_INDEX])
     {
       case REG_USB_MOUSE_SPEED:
         namedPortValues[REG_USB_MOUSE_SPEED] = value;
         updateMouseSpeed(value);
       break;
       default:
         ;
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

  
  const uint8_t* SpriteExt::combineLine(const uint8_t *buf, size_t *nBytes)
  {

    const unsigned char *bufp = buf;
    const uint8_t *endp = buf + *nBytes;
    size_t outPos = 0;
    if (!(*nBytes))
      return buf;

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
      case 0x01:                        // 1x16 pixel, 256 colors coded on 2 bytes
        do {
            buf_[outPos] = 0x01;
            buf_[outPos+1] = bufp[1];
          bufp = bufp + 2;
          outPos += 2;
          if (bufp >= endp)
            break;
        } while (bufp[0] == 0x01);
        break;
      case 0x02:                        // 2x8 pixels, 256 colors coded on 3 bytes
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
      case 0x03:                        // 8x2 pixels, 2 colors coded on 4 bytes
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
      case 0x04:                        // 4x4 pixels, 256 colors coded on 5 bytes -- TVC yes
        do {
            buf_[outPos] = 0x04;
            buf_[outPos+1] = bufp[1];
            buf_[outPos+2] = bufp[2];
            buf_[outPos+3] = bufp[3];
            buf_[outPos+4] = bufp[4];
          bufp = bufp + 5;
          outPos += 5;
          if (bufp >= endp)
            break;
        } while (bufp[0] == 0x04);
        break;
      case 0x06:                        // 16 (2*8) pixels, 2*2 colors coded on 7 bytes -- TVC yes
        do {
          unsigned char c0 = bufp[1];
          unsigned char c1 = bufp[2];
          unsigned char b = bufp[3];
            buf_[outPos] = 0x06;
            buf_[outPos+1] = bufp[1];
            buf_[outPos+2] = bufp[2];
            buf_[outPos+3] = bufp[3];
            buf_[outPos+4] = bufp[4];
            buf_[outPos+5] = bufp[5];
            buf_[outPos+6] = bufp[6];

          c0 = bufp[4];
          c1 = bufp[5];
          bufp = bufp + 7;
          outPos += 7;
          if (bufp >= endp)
            break;
        } while (bufp[0] == 0x06);
        break;
      case 0x08:                        // 8*2 pixels, 256 colors coded on 9 bytes -- TVC yes
        do {
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
          if (bufp >= endp)
            break;
        } while (bufp[0] == 0x08);
        break;
      default:                          // invalid flag byte
        do {
          buf_[outPos++] = 0x00;
        } while (outPos < 108);
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

