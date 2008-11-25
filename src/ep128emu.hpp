
// ep128emu -- portable Enterprise 128 emulator
// Copyright (C) 2003-2008 Istvan Varga <istvanv@users.sourceforge.net>
// http://sourceforge.net/projects/ep128emu/
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

#if defined(HAVE_STDINT_H) || defined(__GNUC__)
#  include <stdint.h>
#else
typedef signed char         int8_t;
typedef unsigned char       uint8_t;
typedef short               int16_t;
typedef unsigned short      uint16_t;
typedef int                 int32_t;
typedef unsigned int        uint32_t;
#  if defined(_WIN32) || defined(_WIN64) || defined(_MSC_VER)
typedef __int64             int64_t;
typedef unsigned __int64    uint64_t;
#  else
typedef long long           int64_t;
typedef unsigned long long  uint64_t;
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
#  define EP128EMU_INLINE   __attribute__ ((__always_inline__)) inline
#else
#  define EP128EMU_REGPARM1
#  define EP128EMU_REGPARM2
#  define EP128EMU_REGPARM3
#  define EP128EMU_INLINE   inline
#endif

#include "fileio.hpp"

#endif  // EP128EMU_EP128EMU_HPP

