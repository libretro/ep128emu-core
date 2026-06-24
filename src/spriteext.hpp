
// ep128emu -- portable Enterprise 128 emulator
// Copyright (C) 2003-2016 Istvan Varga <istvanv@users.sourceforge.net>
// https://github.com/istvan-v/ep128emu/
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

// Xep128: Minimalistic Enterprise-128 emulator with focus on "exotic" hardware
// Copyright (C)2015 LGB (Gábor Lénárt) <lgblgblgb@gmail.com>
// http://xep128.lgb.hu/
//
// http://elm-chan.org/docs/mmc/mmc_e.html
// http://www.mikroe.com/downloads/get/1624/microsd_card_spec.pdf
// http://users.ece.utexas.edu/~valvano/EE345M/SD_Physical_Layer_Spec.pdf

#ifndef EP128EMU_SPRITEEXT_HPP
#define EP128EMU_SPRITEEXT_HPP

#include "ep128emu.hpp"
#include <vector>

namespace Ep128 {

  class SpriteExt {
   protected:
    bool      spriteExt_enabled;    // only used in temporaryDisable()
    uint32_t  spriteExtSegment;
    uint32_t  spriteExtAddress;
    uint8_t namedPortValues[256];
    // 7K of useful SRAM
    std::vector< uint8_t >  sd_ram_ext;
    // 64K flash ROM
    std::vector< uint8_t >  sd_rom_ext;
   // ----------------
    void _block_read();
    void _spi_shifting_with_sd_card();
    uint8_t flashRead(uint32_t addr);
    void flashWrite(uint32_t addr, uint8_t data);
    uint8_t flashReadDebug(uint32_t addr) const;
   public:
    uint8_t io_port_values[16];
    SpriteExt();
    virtual ~SpriteExt();
    void setEnabled(bool isEnabled);
    // for demo recording/playback, calling with isDisabled == false
    // restores the state previously set with setEnabled()
    void temporaryDisable(bool isDisabled);
    // 0 = soft reset
    // 1 = simulate disk change
    // 2 = clear SRAM
    void reset(int reset_level);
    void openImage(const char *sdimg_path);
    void openROMFile(const char *fileName);
    uint8_t readNamedPort(bool secondary);
    void writeNamedPort(bool secondary, uint8_t value);
    uint8_t readCartP3(uint32_t addr);
    void writeCartP3(uint32_t addr, uint8_t data);
    uint8_t readCartP3Debug(uint32_t addr) const;
    EP128EMU_INLINE bool isSpriteExtSegment(uint8_t segment) const
    {
      return (segment == spriteExtSegment);
    }
    EP128EMU_INLINE bool isSpriteExtAddress(uint32_t addr) const
    {
      return ((addr & 0x003FC000U) == spriteExtAddress);
    }
    void saveState(Ep128Emu::File::Buffer&);
    void saveState(Ep128Emu::File&);
    void loadState(Ep128Emu::File::Buffer&);
    void registerChunkType(Ep128Emu::File&);
  };

}       // namespace Ep128

#define SPRITEEXT_REG_INDEX 0x0
#define SPRITEEXT_REG_ACCESS 0x1
#define SPRITEEXT_REG_INCREMENT 0x2
#define SPRITEEXT_SEC_REG_INDEX 0x4
#define SPRITEEXT_SEC_REG_ACCESS 0x5
#define SPRITEEXT_SEC_REG_INCREMENT 0x6
#define REG_MEMORY_P2 0xB0
// 4
#define REG_MEMORY_P2_DEFAULT 0x04
#define REG_MEMORY_P3 0xB1
// 5
#define REG_MEMORY_P3_DEFAULT 0x05
#define REG_MEMORY_MAP_8M_P2_LOW 0xB2
#define REG_MEMORY_MAP_8M_P2_LOW_DEFAULT 0x0
#define REG_MEMORY_MAP_8M_P2_HIGH 0xB3
#define REG_MEMORY_MAP_8M_P3_LOW 0xB4
#define REG_MEMORY_MAP_8M_P3_LOW_DEFAULT 0x0
#define REG_MEMORY_MAP_8M_P3_HIGH 0xB5
#define REG_MEMORY_PSRAM_SIZE_IN_MB 0xB6
#define REG_MEMORY_PSRAM_SIZE_IN_MB_DEFAULT 0x08
#define SPRITEEXT_MEM_PAGE_BASE_SEGMENT 0xE8
#define SPRITEEXT_MEM_PAGE_MAX 0x0F
#define SPRITEEXT_MEM_PAGE_PSRAM_BASE_SEGMENT 0x68
#define SPRITEEXT_MEM_PAGE_PSRAM_P2 0x10
#define SPRITEEXT_MEM_PAGE_PSRAM_P3 0x11
#define SPRITEEXT_MEM_DISABLE 0xFF
#define REG_FW_VERSION_MAJOR 0xC3
#define REG_FW_VERSION_MAJOR_DEFAULT 0x01
#define REG_FW_VERSION_MINOR 0xC4
#define REG_FW_VERSION_MINOR_DEFAULT 0x02
#endif  // EP128EMU_spriteext_HPP

