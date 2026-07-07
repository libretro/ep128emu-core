
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
#include "memory.hpp"
#include <vector>

namespace Ep128 {

  class SpriteExt {
   protected:
    bool      spriteExt_enabled;    // only used in temporaryDisable()
    bool      anyGfxEnabled;
    uint32_t  spriteExtSegment;
    uint32_t  spriteExtAddress;
    uint8_t namedPortValues[256];
     unsigned int  nBytes_;
     // a line of 768 pixels needs a maximum space of 768 * (9 / 16) = 432
     // ( = 108 * 4) bytes in compressed format
     uint8_t  buf_[800*4];

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
    void updateMouseSpeed(uint8_t binValue);
    void updateAnyGfxEnabled();
    void updateLineWithGfx(size_t outPos, uint8_t currSlot, Ep128::Memory *mem);
    void updateLineWithSprite(uint8_t *buf, uint8_t currSlot, Ep128::Memory *mem, size_t spriteNum);
    size_t curLine;
    size_t scrollX;
    size_t scrollY;
    bool scrollBorderX;
    bool scrollBorderY;
    uint8_t i4ToTVCRGB(uint8_t val, uint8_t transparent_val);

   public:
    uint8_t io_port_values[16];
    float mouse_speed;
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
    void   writeNamedPort(bool secondary, uint8_t value);
    uint8_t readNamedPortDebug(uint8_t portIndex);
    void   writeNamedPortDebug(uint8_t portIndex, uint8_t value);

    const uint8_t *combineLine(const uint8_t *buf, size_t *nBytes, uint8_t vsyncCnt, Ep128::Memory *mem);
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
#define SPRITEEXT_REG_INCREMENT_DEFAULT 0x1
#define SPRITEEXT_SEC_REG_INDEX 0x4
#define SPRITEEXT_SEC_REG_ACCESS 0x5
#define SPRITEEXT_SEC_REG_INCREMENT 0x6
#define REG_SCREEN_MAXY 0xA9
#define SPRITEEXT_FIRST_LINE 27
#define SPRITEEXT_LAST_LINE 266
#define SPRITEEXT_TRANSPARENT_COLOR 0x08

#define SPRITEEXT_SPRITE_MAX 0x0F
#define REG_SPRITE_X 0x00
#define REG_SPRITE_Y 0x20
#define REG_SPRITE_COLOR 0x40
#define REG_SPRITE_PHASE 0x50
#define REG_SPRITE_FOREGROUND 0x60
#define REG_SPRITE_ENABLE 0x70
#define REG_SPRITE_COLORMODE 0x80
#define REG_SPRITE_ENABLE_LOW 0x90
#define REG_SPRITE_ENABLE_HIGH 0x91
#define REG_SPRITE_FOREGROUND_LOW 0x92
#define REG_SPRITE_FOREGROUND_HIGH 0x93
#define REG_SPRITE_COLORMODE_LOW 0x94
#define REG_SPRITE_COLORMODE_HIGH 0x95


#define REG_SPRITE_OFFSET_X 24
#define REG_SPRITE_OFFSET_Y 21+6
#define REG_SPRITE_BASE_ADDR 0x9E

#define REG_MEMORY_P2 0xB0
#define REG_MEMORY_P2_DEFAULT 0xFF /* default would be 0x04, changed for DevTool compatibility */
#define REG_MEMORY_P3 0xB1
#define REG_MEMORY_P3_DEFAULT 0xFF /* default would be 0x05, changed for DevTool compatibility */
#define REG_MEMORY_MAP_8M_P2_LOW 0xB2
#define REG_MEMORY_MAP_8M_P2_LOW_DEFAULT 0x0
#define REG_MEMORY_MAP_8M_P2_HIGH 0xB3
#define REG_MEMORY_MAP_8M_P3_LOW 0xB4
#define REG_MEMORY_MAP_8M_P3_LOW_DEFAULT 0x0
#define REG_MEMORY_MAP_8M_P3_HIGH 0xB5
#define REG_MEMORY_PSRAM_SIZE_IN_MB 0xB6
// Hardware default: 8 MB / 0x08, emulated: 2 MB / 0x02
#define REG_MEMORY_PSRAM_SIZE_IN_MB_DEFAULT 0x02
// Some constants moved to tvcmem.hpp:
// #define TVC256_FASTRAM_START_SEGMENT 0xE8
// #define TVC256_SLOWRAM_START_SEGMENT 0x68
#define SPRITEEXT_MEM_PAGE_MAX 0x0F
#define SPRITEEXT_MEM_PAGE_PSRAM_P2 0x10
#define SPRITEEXT_MEM_PAGE_PSRAM_P3 0x11

#define REG_SCREEN_BITMAP_BASE_ADDR 0xA4
#define REG_SCREEN_BITMAP_BASE_ADDR_DEFAULT 0x01
#define REG_SCREEN_VIDEOMODE 0xA0
#define REG_SCREEN_VIDEOMODE_NONE 0x0
#define REG_SCREEN_VIDEOMODE_CHAR2 0x1
#define REG_SCREEN_VIDEOMODE_CHAR16 0x2
#define REG_SCREEN_VIDEOMODE_BITMAP 0x3
#define REG_SCREEN_SCREEN_BASE_ADDR 0xA1
#define REG_SCREEN_SCREEN_BASE_ADDR_DEFAULT 0x01
#define REG_SCREEN_SCREEN_COLOR_BASE_ADDR 0xA2
#define REG_SCREEN_SCREEN_COLOR_BASE_ADDR_DEFAULT 0x02
#define REG_SCREEN_FONT_BASE_ADDR 0xA3
#define REG_SCREEN_FONT_BASE_ADDR_DEFAULT 0x02
#define REG_SCREEN_SCROLL_X 0xA5
#define REG_SCREEN_SCROLL_Y 0xA6
#define REG_SCREEN_BORDER_COLOR 0xA7

#define REG_FW_VERSION_MAJOR 0xC3
#define REG_FW_VERSION_MAJOR_DEFAULT 0x01
#define REG_FW_VERSION_MINOR 0xC4
#define REG_FW_VERSION_MINOR_DEFAULT 0x03
#define REG_USB_INIT 0xD0
#define REG_USB_INIT_DEFAULT 0x01
#define REG_USB_MOUSE_BUTTONS 0xD1
#define REG_USB_MOUSE_DX 0xD2
#define REG_USB_MOUSE_DY 0xD3
#define REG_USB_MOUSE_DW 0xD4
#define REG_USB_MOUSE_SPEED 0xD5
#define REG_USB_MOUSE_SPEED_DEFAULT 0x10
#endif  // EP128EMU_spriteext_HPP

