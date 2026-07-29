
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
#include "tvcmem.hpp"
#include <vector>

namespace Ep128 {

  class SpriteExt {
   protected:
    TVC64::Memory *hostMem;
    bool      spriteExt_enabled;    // only used in temporaryDisable()
    bool      anyGfxEnabled;
    uint32_t  spriteExtSegment;
    uint32_t  spriteExtAddress;
    uint8_t namedPortValues[256];
    uint8_t lastFunctionResult;
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
    void updateLineWithGfx(size_t outPos, uint8_t currSlot);
    void updateLineWithSprite(uint8_t *buf, uint8_t currSlot, size_t spriteNum);
    size_t curLine;
    size_t scrollX;
    size_t scrollY;
    bool scrollBorderX;
    bool scrollBorderY;
    uint8_t overlayBuffer[16];
    uint8_t overlayBufferPrev[16];
    uint16_t backgr_active_pixels;
    uint16_t sprite_active_pixels[16];
    uint8_t i4ToTVCRGB(uint8_t val, uint8_t transparent_val);
    uint8_t i4ToTVCRGB_coll(uint8_t val, uint8_t transparent_val, uint16_t *collision_mask, size_t collision_bit);
    void executeFunction(uint8_t funcCode);

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
    void setMemRef(TVC64::Memory *m);
    const uint8_t *combineLine(const uint8_t *buf, size_t *nBytes, uint8_t vsyncCnt, uint8_t *irqState);
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
#endif  // EP128EMU_spriteext_HPP

