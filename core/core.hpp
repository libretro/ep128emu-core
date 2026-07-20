
// ep128emu-core -- libretro core version of the ep128emu emulator
// Copyright (C) 2022 Zoltan Balogh
// https://github.com/zoltanvb/ep128emu-core
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

#ifndef EP128EMU_CORE_HPP
#define EP128EMU_CORE_HPP
#include "ep128vm.hpp"
#include "tvc64vm.hpp"
#include "cpc464vm.hpp"
#include "zx128vm.hpp"
#include "vmthread.hpp"
#include "emucfg.hpp"
#include "libretro.h"
#include "libretro-funcs.hpp"
#include "libretrodisp.hpp"
#include "libretrosnd.hpp"

namespace Ep128Emu
{

// VM main type compatibility is checked by subtracting types
// so do not use type indexes in the x50-x99 range
const std::map<std::string, int> VM_config {
 { "VM_CONFIG_AUTO"   , 0},
 { "EP128_DISK"       , 100},
 { "EP128_TAPE"       , 101},
 { "EP128_FILE"       , 102},
 { "EP128_FILE_DTF"   , 103},
 { "EP128_TAPE_NOCART", 104},
 { "EP128_DISK_ISDOS" , 105},
 { "EP64_DISK"        , 110},
 { "EP64_TAPE"        , 111},
 { "EP64_FILE"        , 112},
 { "EP64_FILE_DTF"    , 113},
 { "EP64_TAPE_NOCART" , 114},
 { "EP64_DISK_ISDOS"  , 115},
 { "EP128_HDD"        , 120},
 { "TVC64_FILE"       , 200},
 { "TVC64_DISK"       , 201},
 { "TVC64_TAPE"       , 202},
 { "CPC_TAPE"         , 300}, // Default is CPC6128
 { "CPC_DISK"         , 301},
 { "CPC_464_TAPE"     , 302},
 { "CPC_664_DISK"     , 303},
 { "ZX16_TAPE"        , 400},
 { "ZX16_FILE"        , 401},
 { "ZX48_TAPE"        , 402},
 { "ZX48_FILE"        , 403},
 { "ZX128_TAPE"       , 404},
 { "ZX128_FILE"       , 405},
 { "VM_CONFIG_UNKNOWN" , INT_MAX},
};

enum LibretroCore_VM_type
{
  MACHINE_EP,
  MACHINE_TVC,
  MACHINE_CPC,
  MACHINE_ZX,
  MACHINE_UNKNOWN = INT_MAX
};

// Mapping of ROM names used in core.cpp (coming from ep128emu ROM package)
// to other sources like TOSEC compilations or other available downloads
// _p1, _p2 etc. denotes 16K pages
const std::multimap<std::string, std::string> rom_names_ep128emu_tosec = {
{"basic21.rom"    , "BASIC Interpreter v2.1 (1985)(Intelligent Software).bin"},
{"exos20.rom"     , "Expandible OS v2.0 (1984)(Intelligent Software).bin"},
{"exos21.rom"     , "Expandible OS v2.1 (1985)(Intelligent Software).bin"},
{"exos23.rom"     , "Expandible OS v2.3 (1987)(Intelligent Software).bin"},
{"exdos13.rom"    , "Disk Controller v1.3 (1985)(Intelligent Software).bin"}, // binary does not fully match
{"exos20.rom"     , "EXOS20.ROM"}, // enterprise-world.net
{"exos21.rom"     , "EXOS21.ROM"}, // enterprise-world.net
{"exos23.rom"     , "EXOS23.ROM"}, // enterprise-world.net
{"exdos13.rom"    , "EXDOS14.ROM"}, // enterprise-world.net, binary does not fully match
{"zt19uk.rom"     , "ZozoTools v1.8 (19xx)(Zoltan Nemeth).bin"}, // version does not match
{"zx48.rom"       , "ZX Spectrum 48K - 1 (1982)(Sinclair Research).rom"},
{"zx48.rom"       , "48.rom"}, // github.com/Abdess/retroarch_system
{"zx128.rom"      , "ZX Spectrum 128K (1986)(Sinclair Research)(128K)[aka Derby].rom"},
{"zx128.rom_p0"   , "128-0.rom"}, // github.com/Abdess/retroarch_system
{"zx128.rom_p1"   , "128-1.rom"}, // github.com/Abdess/retroarch_system
{"cpc_amsdos.rom" , "Amstrad CPC 664 Amsdos (1985)(Amstrad)[AMSDOS.ROM].rom"},
{"cpc464.rom_p0"  , "Amstrad CPC 464 OS (1985)(Amstrad)[OS.ROM].rom"},        // first half
{"cpc664.rom_p0"  , "Amstrad CPC 664 OS (1985)(Amstrad)[OS.ROM].rom"},        // first half
{"cpc6128.rom_p0" , "Amstrad CPC 6128 OS (1985)(Amstrad)[OS.ROM].rom"},       // first half
{"cpc464.rom_p1"  , "Amstrad CPC 464 BASIC (1985)(Amstrad)[BASIC.ROM].rom"},  // second half
{"cpc664.rom_p1"  , "Amstrad CPC 664 BASIC (1985)(Amstrad)[BASIC.ROM].rom"},  // second half
{"cpc6128.rom_p1" , "Amstrad CPC 6128 BASIC (1986)(Amstrad)[BASIC.ROM].rom"}, // second half
{"cpc464.rom_p0"  , "OS_464.ROM"},     // first half,  cpcwiki.eu naming
{"cpc664.rom_p0"  , "OS_664.ROM"},     // first half,  cpcwiki.eu naming
{"cpc6128.rom_p0" , "OS_6128.ROM"},    // first half,  cpcwiki.eu naming
{"cpc464.rom_p1"  , "BASIC_1.0.ROM"},  // second half, cpcwiki.eu naming
{"cpc664.rom_p1"  , "BASIC_664.ROM"},  // second half, cpcwiki.eu naming
{"cpc6128.rom_p1" , "BASIC_1.1.ROM"},  // second half, cpcwiki.eu naming
{"cpc_amsdos.rom" , "AMSDOS_0.5.ROM"}, // cpcwiki.eu naming
{"tvc_dos12d.rom" , "VT-DOS12-DISK.ROM"}, // tvc.homeserver.hu naming, binary does not fully match
{"tvc22_ext.rom"  , "TVC22_D7.64K"},      // tvc.homeserver.hu naming
{"tvc22_sys.rom"  , "TVC22_D6_D4.64K"},   // tvc.homeserver.hu naming. Combine the 8K dump files into one 16K file:
  // copy TVC22_D6.64K+TVC22_D4.64K  TVC22_D6_D4.64K (Windows)
  // cat  TVC22_D6.64K TVC22_D4.64K >TVC22_D6_D4.64K (Linux)
{"basic20.rom"    , "BASIC 1.0 (198x)(Enterprise).bin"}, // Combine the 8K dump files into one 16K file:
  // copy "BASIC 1.0 - EPROM 1-2 (198x)(Enterprise).bin"+"BASIC 1.0 - EPROM 2-2 (198x)(Enterprise).bin"  "BASIC 1.0 (198x)(Enterprise).bin" (Windows)
  // cat  "BASIC 1.0 - EPROM 1-2 (198x)(Enterprise).bin" "BASIC 1.0 - EPROM 2-2 (198x)(Enterprise).bin" >"BASIC 1.0 (198x)(Enterprise).bin" (Linux)
};

const std::map<std::string, int> joystick_type {
  {"",          0  },
  {"DEFAULT",   0  },
  {"INTERNAL",  1  },
  {"EXT1",      2  },
  {"EXT2",      3  },
  {"SINCLAIR1", 4  },
  {"SINCLAIR2", 5  },
  {"PROTEK",    6  },
  {"EXT3",      7  },
  {"EXT4",      8  },
  {"EXT5",      9  },
  {"EXT6",      10 },
  {"UNKNOWN", INT_MAX},
};
const int JOY_TYPE_AMOUNT = 11;

#define RETRO_DEVICE_EP_JOYSTICK_DEF  RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_JOYPAD, 1)
#define RETRO_DEVICE_EP_JOYSTICK_INT  RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_JOYPAD, 2)
#define RETRO_DEVICE_EP_JOYSTICK_EXT1 RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_JOYPAD, 3)
#define RETRO_DEVICE_EP_JOYSTICK_EXT2 RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_JOYPAD, 4)
#define RETRO_DEVICE_EP_JOYSTICK_SIN1 RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_JOYPAD, 5)
#define RETRO_DEVICE_EP_JOYSTICK_SIN2 RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_JOYPAD, 6)
#define RETRO_DEVICE_EP_JOYSTICK_PROT RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_JOYPAD, 7)
#define RETRO_DEVICE_EP_JOYSTICK_EXT3 RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_JOYPAD, 8)
#define RETRO_DEVICE_EP_JOYSTICK_EXT4 RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_JOYPAD, 9)
#define RETRO_DEVICE_EP_JOYSTICK_EXT5 RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_JOYPAD, 10)
#define RETRO_DEVICE_EP_JOYSTICK_EXT6 RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_JOYPAD, 11)

const std::map<unsigned, std::string> joystick_type_retrodev {
  {RETRO_DEVICE_NONE,             "DEFAULT"   },
  {RETRO_DEVICE_JOYPAD,           "DEFAULT"   },
  {RETRO_DEVICE_EP_JOYSTICK_DEF,  "DEFAULT"   },
  {RETRO_DEVICE_EP_JOYSTICK_INT,  "INTERNAL"  },
  {RETRO_DEVICE_EP_JOYSTICK_EXT1, "EXT1"      },
  {RETRO_DEVICE_EP_JOYSTICK_EXT2, "EXT2"      },
  {RETRO_DEVICE_EP_JOYSTICK_SIN1, "SINCLAIR1" },
  {RETRO_DEVICE_EP_JOYSTICK_SIN2, "SINCLAIR2" },
  {RETRO_DEVICE_EP_JOYSTICK_PROT, "PROTEK"    },
  {RETRO_DEVICE_EP_JOYSTICK_EXT3, "EXT3"      },
  {RETRO_DEVICE_EP_JOYSTICK_EXT4, "EXT4"      },
  {RETRO_DEVICE_EP_JOYSTICK_EXT5, "EXT5"      },
  {RETRO_DEVICE_EP_JOYSTICK_EXT6, "EXT6"      },
};

static const struct retro_controller_description controller_description[] = {
  { "Core default",        RETRO_DEVICE_EP_JOYSTICK_DEF  },
  { "Internal Joystick",   RETRO_DEVICE_EP_JOYSTICK_INT  },
  { "External Joystick 1 / Kempston", RETRO_DEVICE_EP_JOYSTICK_EXT1 },
  { "External Joystick 2", RETRO_DEVICE_EP_JOYSTICK_EXT2 },
  { "Sinclair Joystick 1", RETRO_DEVICE_EP_JOYSTICK_SIN1 },
  { "Sinclair Joystick 2", RETRO_DEVICE_EP_JOYSTICK_SIN2 },
  { "Protek Joystick",     RETRO_DEVICE_EP_JOYSTICK_PROT },
  { "External Joystick 3", RETRO_DEVICE_EP_JOYSTICK_EXT3 },
  { "External Joystick 4", RETRO_DEVICE_EP_JOYSTICK_EXT4 },
  { "External Joystick 5", RETRO_DEVICE_EP_JOYSTICK_EXT5 },
  { "External Joystick 6", RETRO_DEVICE_EP_JOYSTICK_EXT6 },
  { "None", RETRO_DEVICE_NONE },
};


// Order of codes: up - down - left - right - fire - fire2 - fire3
// Fire2 is used only for CPC.
// Actually there's nothing on any system that would use fire3 (maybe some mouse implementation on ep and cpc)
const unsigned char joystickCodesInt[7]       = { 0x3b, 0x39, 0x3d, 0x3a, 0x46, 0xff, 0xff };
const unsigned char joystickCodesExt1[7]      = { 0x73, 0x72, 0x71, 0x70, 0x74, 0x75, 0x76 };
const unsigned char joystickCodesExt2[7]      = { 0x7b, 0x7a, 0x79, 0x78, 0x7c, 0x7d, 0x7e };
// ZX: Kempston interface mapped to ext 1 already inside ep128emu so ext1 codes can be used.
// The 'left' Sinclair joystick maps the joystick directions and the fire button to the 1 (left), 2 (right), 3 (down), 4 (up) and 5 (fire) keys
const unsigned char joystickCodesSinclair1[7] = { 0x1b, 0x1d, 0x19, 0x1e, 0x1c, 0xff, 0xff };
// The 'right' Sinclair joystick maps to keys 6 (left), 7 (right), 8 (down), 9 (up) and 0 (fire)
const unsigned char joystickCodesSinclair2[7] = { 0x2a, 0x28, 0x1a, 0x18, 0x2c, 0xff, 0xff };
// A cursor joystick interfaces maps to keys 5 (left), 6 (down), 7 (up), 8 (right) and 0 (fire). (Protek and AGF)
const unsigned char joystickCodesSinclair3[7] = { 0x18, 0x1a, 0x1c, 0x28, 0x2c, 0xff, 0xff };
// External 3..6 joysticks for Enterprise. Very rarely used.
// 3/4: column K
const unsigned char joystickCodesExt3[7]      = { 0x63, 0x62, 0x61, 0x60, 0x64, 0x65, 0x66 };
const unsigned char joystickCodesExt4[7]      = { 0x6b, 0x6a, 0x69, 0x68, 0x6c, 0x6d, 0x6e };
// 5/6: column L
const unsigned char joystickCodesExt5[7]      = { 0x53, 0x52, 0x51, 0x50, 0x54, 0x55, 0x56 };
const unsigned char joystickCodesExt6[7]      = { 0x5b, 0x5a, 0x59, 0x58, 0x5c, 0x5d, 0x5e };

const unsigned char* const joystickCodeMap[JOY_TYPE_AMOUNT] =
{ nullptr, joystickCodesInt, joystickCodesExt1, joystickCodesExt2, joystickCodesSinclair1, joystickCodesSinclair2, joystickCodesSinclair3,
joystickCodesExt3, joystickCodesExt4, joystickCodesExt5, joystickCodesExt6 };

const char* const joystickNameMap[JOY_TYPE_AMOUNT] =
{ nullptr, "Internal", "External 1", "External 2", "Sinclair 1", "Sinclair 2", "Protek", "External 3", "External 4", "External 5", "External 6" };

enum LibretroCore_locale
{
  LOCALE_UK,
  LOCALE_HUN,
  LOCALE_GER,
  LOCALE_ESP,
  LOCALE_FRA,
  LOCALE_AMOUNT,
  LOCALE_UNKNOWN = INT_MAX
};

// HUN locale is required for "req zrom" games in Tosec collection
// BRD locale is required for "req brd-rom" games in Tosec collection
// Others are unused for now.
const std::string locale_identifiers[LOCALE_AMOUNT] = {
  "(uk)", "[req zrom]", "[req brd-rom]", "(es)", "(fr)"
};

const std::multimap<std::string, std::string> multidisk_replacements = {
{"Tape 1 of 2 Side A" , "Tape 2 of 2 Side B"},
{"Tape 1 of 4 Side A" , "Tape 2 of 4 Side B"},
{"Tape 1 of 4 Side A" , "Tape 3 of 4 Side A"},
{"Tape 1 of 4 Side A" , "Tape 4 of 4 Side B"},
{"Tape 1 of"      , "Tape 2 of"},
{"Tape 1 of"      , "Tape 3 of"},
{"Tape 1 of"      , "Tape 4 of"},
{"Tape 1 of"      , "Tape 5 of"},
{"Tape 1 of"      , "Tape 6 of"},
{"Tape 1 of"      , "Tape 7 of"},
{"Tape 1 of"      , "Tape 8 of"},
{"Tape 1 of"      , "Tape 9 of"},
{"Tape 1 of"      , "Tape 10 of"},
{"Disk 1 Side A"  , "Disk 1 Side B"},
{"Disk 1 Side A"  , "Disk 2 Side A"},
{"Disk 1 Side A"  , "Disk 2 Side B"},
{"Disk 1 Side A"  , "Disk 3 Side A"},
{"Disk 1 Side A"  , "Disk 3 Side B"},
{"Disk 1A"        , "Disk 1B"},
{"Disk 1A"        , "Disk 2A"},
{"Disk 1A"        , "Disk 2B"},
{"Disk 1A"        , "Disk 3A"},
{"Disk 1A"        , "Disk 3B"},
{"Disk 1 of"      , "Disk 2 of"},
{"Disk 1 of"      , "Disk 3 of"},
{"Disk 1 of"      , "Disk 4 of"},
{"Disk 1 of"      , "Disk 5 of"},
{"Disk 1 of"      , "Disk 6 of"},
{"Side 1A"        , "Side 1B"},
{"Side 1A"        , "Side 2A"},
{"Side 1A"        , "Side 2B"},
{"Side 1A"        , "Side 3A"},
{"Side 1A"        , "Side 3B"},
{"Side 1A"        , "Side 4A"},
{"Side 1A"        , "Side 4B"},
{"Side A"         , "Side B"},
};

class LibretroCore
{
private:
  retro_log_printf_t log_cb;
  int libretro_to_ep128emu_kbmap[RETROK_LAST];
  unsigned int bootframes;
  unsigned int autofireFrame;
  unsigned int autofireButtonId;
  unsigned int autofireFrameCycle;

public:
  uint16_t audioBuffer[EP128EMU_SAMPLE_RATE*1000*2];
  int inputJoyMap[256][EP128EMU_MAX_USERS];
  bool inputStateMap[256][EP128EMU_MAX_USERS];
  bool useHalfFrame;
  bool isHalfFrame;
  bool canSkipFrames;
  bool joypadConfigChanged;
  uint32_t prevFrameCount;
  size_t startSequenceIndex;
  int currWidth;
  int currHeight;
  int fullHeight;
  int halfHeight;
  int defaultHalfHeight;
  int defaultWidth;
  int borderSize;
  int machineType;
  int machineDetailedType;
  retro_usec_t totalTime;
  std::string startSequence;
  std::string infoMessage;

  Ep128Emu::VMThread              *vmThread    ;
  Ep128Emu::LibretroDisplay       *w           ;
  Ep128Emu::EmulatorConfiguration *config      ;
  Ep128Emu::VirtualMachine        *vm          ;
  Ep128Emu::AudioOutput           *audioOutput ;

  // ----------------

  LibretroCore(retro_log_printf_t log_cb_, int machineDetailedType, int contentLocale, bool canSkipFrames_, const char* romDirectory_, const char* saveDirectory_,
  const char* startSequence_, const char* cfgFile, bool useCfgFile, bool useHalfFrame, bool enhancedRom);
  virtual ~LibretroCore();

  void initialize_keyboard_map(void);
  void update_keyboard(bool down, unsigned keycode, uint32_t character, uint16_t key_modifiers);
  void initialize_joystick_map(std::string zoomKey, std::string infoKey, std::string autofireKey, int autofireSpeed, int user1, int user2, int user3, int user4, int user5, int user6);
  void update_joystick_map(const unsigned char * joystickCodes, int port, int length);
  void reset_joystick_map(int port, unsigned value);
  void reset_joystick_map(int port);
  void start(void);
  void run_for(retro_usec_t frameTime, float waitPeriod, void * fb);
  void sync_display();
  char* get_current_message(void);
  void update_input(retro_input_state_t input_state_cb, retro_environment_t environ_cb, unsigned maxUsers);
  void render(retro_video_refresh_t video_cb, retro_environment_t environ_cb);
  void change_resolution(int width, int height, retro_environment_t environ_cb);
  void errorCallback(void *userData, const char *msg);
};
}

#endif
