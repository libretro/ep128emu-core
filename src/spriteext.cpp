
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
      namedPortValues[i] = 0xFF;
    namedPortValues[REG_MEMORY_P2] = REG_MEMORY_P2_DEFAULT;
    namedPortValues[REG_MEMORY_P3] = REG_MEMORY_P3_DEFAULT;
    namedPortValues[REG_MEMORY_MAP_8M_P2_LOW] = REG_MEMORY_MAP_8M_P2_LOW_DEFAULT;
    namedPortValues[REG_MEMORY_MAP_8M_P3_LOW] = REG_MEMORY_MAP_8M_P3_LOW_DEFAULT;

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
       default:
         ;
     }

  // Increment register index after operation
  if (secondary)
    io_port_values[SPRITEEXT_SEC_REG_INDEX] += io_port_values[SPRITEEXT_SEC_REG_INCREMENT];
  else
    io_port_values[SPRITEEXT_REG_INDEX] += io_port_values[SPRITEEXT_REG_INCREMENT];

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

