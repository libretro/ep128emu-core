
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

#ifndef EP128EMU_EP128EMU_HPP
#define EP128EMU_EP128EMU_HPP

#include <exception>
#include <new>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#ifdef WIN32
#  undef WIN32
#endif
#if defined(_WIN32) || defined(_WIN64) || defined(_MSC_VER)
#  define WIN32 1
#endif

#if defined(HAVE_STDINT_H) || defined(__GNUC__)
#  include <stdint.h>
#else
typedef signed char         int8_t;
typedef unsigned char       uint8_t;
typedef short               int16_t;
typedef unsigned short      uint16_t;
typedef int                 int32_t;
typedef unsigned int        uint32_t;
#  ifdef WIN32
typedef __int64             int64_t;
typedef unsigned __int64    uint64_t;
#  else
typedef long long           int64_t;
typedef unsigned long long  uint64_t;
#  endif
#  ifdef _WIN64
typedef __int64             intptr_t;
typedef unsigned __int64    uintptr_t;
#  else
typedef long                intptr_t;
typedef unsigned long       uintptr_t;
#  endif
#endif

namespace Ep128Emu {

  class Exception : public std::exception {
   private:
    const char  *msg;
   public:
    Exception() throw()
      : std::exception()
    {
      msg = (char *) 0;
    }
    Exception(const char *msg_) throw()
      : std::exception()
    {
      msg = msg_;
    }
    Exception(const Exception& e) throw()
      : std::exception()
    {
      msg = e.msg;
    }
    virtual ~Exception() throw()
    {
    }
    void operator=(const Exception& e) throw()
    {
      msg = e.msg;
    }
    virtual const char * what() const throw()
    {
      return (msg == (char *) 0 ? "unknown error" : msg);
    }
  };

}       // namespace Ep128Emu

#if defined(__GNUC__) && (__GNUC__ >= 3) && defined(__i386__) && !defined(__ICC)
#  define EP128EMU_REGPARM1 __attribute__ ((__regparm__ (1)))
#  define EP128EMU_REGPARM2 __attribute__ ((__regparm__ (2)))
#  define EP128EMU_REGPARM3 __attribute__ ((__regparm__ (3)))
#else
#  define EP128EMU_REGPARM1
#  define EP128EMU_REGPARM2
#  define EP128EMU_REGPARM3
#endif
#if defined(__GNUC__) && (__GNUC__ >= 3) && !defined(__ICC)
#  define EP128EMU_INLINE         __attribute__ ((__always_inline__)) inline
#  define EP128EMU_EXPECT(x__)    __builtin_expect((x__), 1)
#  define EP128EMU_UNLIKELY(x__)  __builtin_expect((x__), 0)
#else
#  define EP128EMU_INLINE         inline
#  define EP128EMU_EXPECT(x__)    x__
#  define EP128EMU_UNLIKELY(x__)  x__
#endif

#include "fileio.hpp"

#define EP128EMU_MAX_TVC_ROM_SEGMENT 0x23
#define TVCGAMECARD_ROM_START_SEGMENT 0x20

#ifdef ENABLE_SPRITEEXT
#define SPRITEEXT_REG_INDEX 0x0
#define SPRITEEXT_REG_ACCESS 0x1
#define SPRITEEXT_REG_INCREMENT 0x2
#define SPRITEEXT_REG_INCREMENT_DEFAULT 0x1
#define SPRITEEXT_SEC_REG_INDEX 0x4
#define SPRITEEXT_SEC_REG_ACCESS 0x5
#define SPRITEEXT_SEC_REG_INCREMENT 0x6
#define REG_SCREEN_MAXY 0xA9
#define REG_SCREEN_MAXY_DEFAULT 240
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
#define REG_SPRITE_SP_COLLISION_LOW 0x96
#define REG_SPRITE_SP_COLLISION_HIGH 0x97
#define REG_SPRITE_BG_COLLISION_LOW 0x98
#define REG_SPRITE_BG_COLLISION_HIGH 0x99
#define REG_SPRITE_SP_IRQMASK_LOW 0x9A
#define REG_SPRITE_SP_IRQMASK_HIGH 0x9B
#define REG_SPRITE_BG_IRQMASK_LOW 0x9C
#define REG_SPRITE_BG_IRQMASK_HIGH 0x9D
#define REG_SPRITE_OFFSET_X 24
#define REG_SPRITE_OFFSET_Y 21+6
#define REG_SPRITE_BASE_ADDR 0x9E

#define REG_MEMORY_P2 0xB0
#define REG_MEMORY_P3 0xB1
#ifdef ENABLE_DEVTOOL
#define REG_MEMORY_P2_DEFAULT 0xFF /* changed for DevTool compatibility (program move to TVC RAM) */
#define REG_MEMORY_P3_DEFAULT 0xFF /* changed for DevTool compatibility (program move to TVC RAM) */
#else
#define REG_MEMORY_P2_DEFAULT 0x04
#define REG_MEMORY_P3_DEFAULT 0x05
#endif // ENABLE_DEVTOOL

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

#define REG_FUNCTION_EXECUTE 0xC0
#define REG_FUNCTION_PARAM_START 0xC1
#define REG_FUNCTION_RESULT 0xC2
#define REG_FUNCTION_MULTI_EXECUTE 0xC6
#define REG_FUNCTION_BITMAP_BASE 0xC7
#define REG_FUNCTION_BITMAP_BASE_DEFAULT 0xFF
#define SPRITEEXT_FUNCTION_RESULT_PENDING 0xFF

#define REG_SID_BASE 0xE0
#define REG_SID_LAST 0xF9
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
#define REG_FW_VERSION_MINOR_DEFAULT 0x04
#define REG_USB_INIT 0xD0
#define REG_USB_INIT_DEFAULT 0x01
#define REG_USB_MOUSE_BUTTONS 0xD1
#define REG_USB_MOUSE_DX 0xD2
#define REG_USB_MOUSE_DY 0xD3
#define REG_USB_MOUSE_DW 0xD4
#define REG_USB_MOUSE_SPEED 0xD5
#define REG_USB_MOUSE_SPEED_DEFAULT 0x10
#endif // ENABLE_SPRITEEXT
#endif  // EP128EMU_EP128EMU_HPP

