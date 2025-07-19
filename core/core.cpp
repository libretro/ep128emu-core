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

#include "core.hpp"
#include "libretro_keys_reverse.h"
#include "roms/roms.hpp"
namespace Ep128Emu {

LibretroCore::LibretroCore(retro_log_printf_t log_cb_, int machineDetailedType_, int contentLocale, bool canSkipFrames_, const char* romDirectory_, const char* saveDirectory_,
                           const char* startSequence_, const char* cfgFile, bool useCfgFile, bool useHalfFrame_, bool enhancedRom)
  : log_cb(log_cb_),
    autofireFrame(0),
    autofireButtonId(256),
    autofireFrameCycle(1),
    useHalfFrame(useHalfFrame_),
    isHalfFrame(useHalfFrame_),
    canSkipFrames(canSkipFrames_),
    joypadConfigChanged(false),
    prevFrameCount(0),
    startSequenceIndex(0),
    currWidth(EP128EMU_LIBRETRO_SCREEN_WIDTH),
    fullHeight(EP128EMU_LIBRETRO_SCREEN_HEIGHT),
    halfHeight(EP128EMU_LIBRETRO_SCREEN_HEIGHT/2),
    defaultHalfHeight(EP128EMU_LIBRETRO_SCREEN_HEIGHT/2),
    defaultWidth(EP128EMU_LIBRETRO_SCREEN_WIDTH),
    borderSize(0),
    machineType(MACHINE_EP),
    machineDetailedType(machineDetailedType_),
    totalTime(0),
    vmThread(NULL),
    config(NULL)
{
  std::string romBasePath(romDirectory_);
  std::string configBaseFile(romDirectory_);
  std::string configContentDirFile;
  std::string configContentDirTmp;
  std::string configOrigFilename(cfgFile);
  Ep128Emu::splitPath(configOrigFilename,configContentDirFile,configContentDirTmp);
  
#ifdef WIN32
  romBasePath.append("\\ep128emu\\roms\\");
  configBaseFile.append("\\ep128emu\\config\\");
#else
  romBasePath.append("/ep128emu/roms/");
  configBaseFile.append("/ep128emu/config/");
#endif
// check for existence of content dir cfg file, and override machineType accordingly

  configContentDirTmp = configContentDirFile + "tvc.ep128cfg";
  if(Ep128Emu::does_file_exist(configContentDirTmp.c_str()))
  {
    machineDetailedType = VM_config.at("TVC64_DISK");
    log_cb(RETRO_LOG_INFO, "VM type forced to TVC\n");
  }
  configContentDirTmp = configContentDirFile + "cpc.ep128cfg";
  if(Ep128Emu::does_file_exist(configContentDirTmp.c_str()))
  {
    machineDetailedType = VM_config.at("CPC_DISK");
    log_cb(RETRO_LOG_INFO, "VM type forced to CPC\n");
  }
  configContentDirTmp = configContentDirFile + "zx.ep128cfg";
  if(Ep128Emu::does_file_exist(configContentDirTmp.c_str()))
  {
    machineDetailedType = VM_config.at("ZX128_TAPE");
    log_cb(RETRO_LOG_INFO, "VM type forced to ZX\n");
  }
  configContentDirTmp = configContentDirFile + "enterprise.ep128cfg";
  if(Ep128Emu::does_file_exist(configContentDirTmp.c_str()))
  {
    machineDetailedType = VM_config.at("EP128_DISK");
    log_cb(RETRO_LOG_INFO, "VM type forced to EP\n");
  }

  if(machineDetailedType == VM_config.at("TVC64_DISK") || machineDetailedType == VM_config.at("TVC64_FILE")
     || machineDetailedType == VM_config.at("TVC64_TAPE"))
  {
    machineType = MACHINE_TVC;
    log_cb(RETRO_LOG_INFO, "Emulated machine: TVC\n");
    configBaseFile = configBaseFile + "tvc.ep128cfg";
    configContentDirFile = configContentDirFile + "tvc.ep128cfg";
  }
  else if(machineDetailedType == VM_config.at("CPC_DISK") || machineDetailedType == VM_config.at("CPC_TAPE"))
  {
    machineType = MACHINE_CPC;
    log_cb(RETRO_LOG_INFO, "Emulated machine: CPC\n");
    configBaseFile = configBaseFile + "cpc.ep128cfg";
    configContentDirFile = configContentDirFile + "cpc.ep128cfg";
  }
  else if(machineDetailedType == VM_config.at("ZX16_TAPE")  || machineDetailedType == VM_config.at("ZX16_FILE") ||
          machineDetailedType == VM_config.at("ZX48_TAPE")  || machineDetailedType == VM_config.at("ZX48_FILE") ||
          machineDetailedType == VM_config.at("ZX128_TAPE") || machineDetailedType == VM_config.at("ZX128_FILE") )
  {
    machineType = MACHINE_ZX;
    log_cb(RETRO_LOG_INFO, "Emulated machine: ZX\n");
    configBaseFile = configBaseFile + "zx.ep128cfg";
    configContentDirFile = configContentDirFile + "zx.ep128cfg";
  }
  else
  {
    log_cb(RETRO_LOG_INFO, "Emulated machine: EP\n");
    configBaseFile = configBaseFile + "enterprise.ep128cfg";
    configContentDirFile = configContentDirFile + "enterprise.ep128cfg";
  }
  if(machineDetailedType == VM_config.at("VM_CONFIG_UNKNOWN"))
  {
    log_cb(RETRO_LOG_ERROR, "VM_CONFIG_UNKNOWN passed!\n");
    throw Ep128Emu::Exception("Machine configuration not recognized!");
  }

  audioOutput = new Ep128Emu::AudioOutput_libretro();
  //audioOutput->setOutputFile("/tmp/core_sound.wav");
  w = new Ep128Emu::LibretroDisplay(32, 32, EP128EMU_LIBRETRO_SCREEN_WIDTH, EP128EMU_LIBRETRO_SCREEN_HEIGHT, "", useHalfFrame);
  if(machineType == MACHINE_TVC)
  {
    vm = new TVC64::TVC64VM(*(dynamic_cast<Ep128Emu::VideoDisplay *>(w)),
                            *audioOutput);
  }
  else if (machineType == MACHINE_CPC)
  {
    vm = new CPC464::CPC464VM(*(dynamic_cast<Ep128Emu::VideoDisplay *>(w)),
                              *audioOutput);
  }
  else if (machineType == MACHINE_ZX)
  {
    vm = new ZX128::ZX128VM(*(dynamic_cast<Ep128Emu::VideoDisplay *>(w)),
                            *audioOutput);
  }
  else
  {
    vm = new Ep128::Ep128VM(*(dynamic_cast<Ep128Emu::VideoDisplay *>(w)),
                            *audioOutput);
  }
  log_cb(RETRO_LOG_DEBUG, "VM created\n");
  config = new Ep128Emu::EmulatorConfiguration(
    *vm, *(dynamic_cast<Ep128Emu::VideoDisplay *>(w)), *audioOutput
#ifdef ENABLE_MIDI_PORT
    , *midiPort
#endif
  );

  //config->setErrorCallback(&LibretroCore::errorCallback, (void *) 0);
  currHeight = useHalfFrame ? EP128EMU_LIBRETRO_SCREEN_HEIGHT / 2 : EP128EMU_LIBRETRO_SCREEN_HEIGHT;

  // Load extra configuration files.
  // At this point, only machineDetailedType will be interpreted, all others will be overwritten a bit later.
  if(Ep128Emu::does_file_exist(configBaseFile.c_str()))
  {
    log_cb(RETRO_LOG_INFO, "Loading system wide configuration file: %s\n",configBaseFile.c_str());
    config->loadState(configBaseFile.c_str(),false);
  }
  if(Ep128Emu::does_file_exist(configContentDirFile.c_str()))
  {
    log_cb(RETRO_LOG_INFO, "Loading content dir wide configuration file: %s\n",configContentDirFile.c_str());
    config->loadState(configContentDirFile.c_str(),false);
  }
  if (useCfgFile)
  {
    log_cb(RETRO_LOG_INFO, "Loading content specific configuration file: %s\n",cfgFile);
    config->loadState(cfgFile,false);
  }

  startSequence = startSequence_;

  // Check if the forced configuration matches the main VM type.
  if (config->machineDetailedType != "")
  {
    int newDetailedType = VM_config.at(config->machineDetailedType);
    if(newDetailedType - machineDetailedType > 50 || newDetailedType - machineDetailedType < -50)
    {
      log_cb(RETRO_LOG_ERROR, "Incompatible machine type from config file: %s instead of %d\n",config->machineDetailedType.c_str(),machineDetailedType);
      throw Ep128Emu::Exception("Machine configuration incompatibilty!");
    }
    log_cb(RETRO_LOG_INFO, "Detailed machine type from configuration: %s instead of %d\n",config->machineDetailedType.c_str(),machineDetailedType);
    machineDetailedType = newDetailedType;
  }

  // RAM/ROM configuration begin
  // Practically all important parts of configuration are set up here, no other file is needed
  // Usage of configBaseFile is completely optional.
  if(machineType == MACHINE_EP)
  {
    bool is_EP64 = (machineDetailedType  == VM_config.at("EP64_DISK")  || machineDetailedType == VM_config.at("EP64_FILE") ||
                    machineDetailedType  == VM_config.at("EP64_TAPE")  || machineDetailedType == VM_config.at("EP64_FILE_DTF") ||
                    machineDetailedType  == VM_config.at("EP64_TAPE_NOCART") || machineDetailedType  == VM_config.at("EP64_DISK_ISDOS")) ? true : false;
    bool use_file = (machineDetailedType == VM_config.at("EP128_FILE") || machineDetailedType == VM_config.at("EP64_FILE")) ? true : false;
    bool use_disk = (machineDetailedType == VM_config.at("EP128_DISK") || machineDetailedType == VM_config.at("EP64_DISK") ||
                     machineDetailedType == VM_config.at("EP128_DISK_ISDOS") || machineDetailedType == VM_config.at("EP64_DISK_ISDOS")) ? true : false;
    bool use_dtf = (machineDetailedType  == VM_config.at("EP128_FILE_DTF") || machineDetailedType == VM_config.at("EP64_FILE_DTF")) ? true : false;
    bool use_cartridge = (machineDetailedType  == VM_config.at("EP128_TAPE_NOCART") || machineDetailedType  == VM_config.at("EP64_TAPE_NOCART")) ? false : true;
    bool use_isdos = (machineDetailedType == VM_config.at("EP128_DISK_ISDOS") || machineDetailedType  == VM_config.at("EP64_DISK_ISDOS")) ? true : false;

    if (is_EP64)
      config->memory.ram.size=64;
    else
      config->memory.ram.size=128;
    if (enhancedRom)
    {
      bootframes = 20*10;
      config->memory.rom[0x00].file="exos24uk.rom";
      config->memory.rom[0x00].offset=0;
      config->memory.rom[0x01].file="exos24uk.rom";
      config->memory.rom[0x01].offset=16384;
      config->memory.rom[0x02].file="exos24uk.rom";
      config->memory.rom[0x02].offset=32768;
      config->memory.rom[0x03].file="exos24uk.rom";
      config->memory.rom[0x03].offset=49152;
    }
    else if (is_EP64)
    {
      bootframes = 30*10;
      config->memory.rom[0x00].file="exos20.rom";
      config->memory.rom[0x00].offset=0;
      config->memory.rom[0x01].file="exos20.rom";
      config->memory.rom[0x01].offset=16384;
    }
    else
    {
      bootframes = 40*10;
      config->memory.rom[0x00].file="exos21.rom";
      config->memory.rom[0x00].offset=0;
      config->memory.rom[0x01].file="exos21.rom";
      config->memory.rom[0x01].offset=16384;
    }
    if(contentLocale == LOCALE_HUN)
    {
      // Locale support: HUN ROM goes to segment 4 and then Basic goes to segment 5
      config->memory.rom[0x04].file="hun.rom";
      config->memory.rom[0x04].offset=0;
      if (use_cartridge) {
        if (is_EP64)
          config->memory.rom[0x05].file="basic20.rom";
        else
          config->memory.rom[0x05].file="basic21.rom";
        config->memory.rom[0x05].offset=0;
      }
      // HFONT is used from epdos rom (and CLKOFF, as it is not present after this 1.6f version)
      config->memory.rom[0x06].file="epdos16f.rom";
      config->memory.rom[0x06].offset=0;
      config->memory.rom[0x07].file="epdos16f.rom";
      config->memory.rom[0x07].offset=16384;
    }
    else if(contentLocale == LOCALE_GER)
    {
      // Locale support: BRD ROM goes to segment 4 and then Basic goes to segment 5
      config->memory.rom[0x04].file="brd.rom";
      config->memory.rom[0x04].offset=0;
      if (use_cartridge) {
        if (is_EP64)
          config->memory.rom[0x05].file="basic20.rom";
        else
          config->memory.rom[0x05].file="basic21.rom";
        config->memory.rom[0x05].offset=0;
      }
    }
    else
    {
      if (use_cartridge) {
        if (is_EP64)
          config->memory.rom[0x04].file="basic20.rom";
        else
          config->memory.rom[0x05].file="basic21.rom";
        config->memory.rom[0x04].offset=0;
      }
    }

    if(use_file || use_dtf)
    {
      config->memory.rom[0x10].file="epfileio.rom";
      config->memory.rom[0x10].offset=0;
      if(use_dtf)
      {
        config->memory.rom[0x40].file="zt19uk.rom";
        config->memory.rom[0x40].offset=0;
        config->memory.rom[0x41].file="zt19uk.rom";
        config->memory.rom[0x41].offset=16384;
      }
    }
    if(use_isdos)
    {
      config->memory.rom[0x20].file="exdos14isdos10uk.rom";
      config->memory.rom[0x20].offset=0;
      config->memory.rom[0x21].file="exdos14isdos10uk.rom";
      config->memory.rom[0x21].offset=16384;
      startSequence += " \xff\xff\xff\xff\xff\xff\xff\xff\xff\xff""\x27""isdos\r""\xff\xff\xff\xff\xff\xff\xff""dir\r";
    }
    else if(use_file || use_disk)
    {
      config->memory.rom[0x20].file="exdos13.rom";
      config->memory.rom[0x20].offset=0;
      config->memory.rom[0x21].file="exdos13.rom";
      config->memory.rom[0x21].offset=16384;
    }
  }
  else if(machineType == MACHINE_TVC)
  {
    bootframes = 50*10;
    config->memory.ram.size=128;
    config->memory.rom[0x00].file="tvc22_sys.rom";
    config->memory.rom[0x00].offset=0;
    config->memory.rom[0x02].file="tvc22_ext.rom";
    config->memory.rom[0x02].offset=0;
    if(machineDetailedType == VM_config.at("TVC64_FILE"))
    {
      config->memory.rom[0x04].file="tvcfileio.rom";
      config->memory.rom[0x04].offset=0;
    }
    if(machineDetailedType == VM_config.at("TVC64_DISK"))
    {
      config->memory.rom[0x03].file="tvc_dos12d.rom";
      config->memory.rom[0x03].offset=0;
    }
  }
  else if(machineType == MACHINE_CPC)
  {
    // TODO locale support
    bootframes = 20*10;
    if(machineDetailedType == VM_config.at("CPC_464_TAPE"))
    {
      config->memory.ram.size=64;
      config->memory.rom[0x10].file="cpc464.rom";
      config->memory.rom[0x10].offset=0;
      config->memory.rom[0x00].file="cpc464.rom";
      config->memory.rom[0x00].offset=16384;
    }
    else if(machineDetailedType == VM_config.at("CPC_664_DISK"))
    {
      config->memory.ram.size=64;
      config->memory.rom[0x10].file="cpc664.rom";
      config->memory.rom[0x10].offset=0;
      config->memory.rom[0x00].file="cpc664.rom";
      config->memory.rom[0x00].offset=16384;
    }
    else
    {
      // 6128 as default
      config->memory.ram.size=128;
      config->memory.rom[0x10].file="cpc6128.rom";
      config->memory.rom[0x10].offset=0;
      config->memory.rom[0x00].file="cpc6128.rom";
      config->memory.rom[0x00].offset=16384;
    }
    if(machineDetailedType == VM_config.at("CPC_DISK") || machineDetailedType == VM_config.at("CPC_664_DISK"))
    {
      config->memory.rom[0x07].file="cpc_amsdos.rom";
      config->memory.rom[0x07].offset=0;
    }

  }
  else if(machineType == MACHINE_ZX)
  {
    bootframes = 20*10;
    if(machineDetailedType == VM_config.at("ZX16_TAPE") || machineDetailedType == VM_config.at("ZX16_FILE"))
    {
      config->memory.ram.size=16;
    }
    else if (machineDetailedType == VM_config.at("ZX48_TAPE") || machineDetailedType == VM_config.at("ZX48_FILE"))
    {
      config->memory.ram.size=48;
    }
    else
    {
      config->memory.ram.size=128;
    }

    if(machineDetailedType == VM_config.at("ZX128_TAPE") || machineDetailedType == VM_config.at("ZX128_FILE"))
    {
      config->memory.rom[0x00].file="zx128.rom";
      config->memory.rom[0x00].offset=0;
      config->memory.rom[0x01].file="zx128.rom";
      config->memory.rom[0x01].offset=16384;
    }
    else
    {
      config->memory.rom[0x00].file="zx48.rom";
      config->memory.rom[0x00].offset=0;
    }
  }

  if(machineType == MACHINE_EP)
  {
    config->vm.cpuClockFrequency=4000000;
    config->vm.enableMemoryTimingEmulation=true;
    config->vm.soundClockFrequency=500000;
    config->vm.videoClockFrequency=889846;
  }
  else if(machineType == MACHINE_TVC)
  {
    config->vm.cpuClockFrequency=3125000;
    config->vm.enableMemoryTimingEmulation=true;
    config->vm.soundClockFrequency=390625;
    config->vm.videoClockFrequency=1562500;
  }
  else if(machineType == MACHINE_CPC)
  {
    config->vm.cpuClockFrequency=4000000;
    config->vm.enableMemoryTimingEmulation=true;
    config->vm.soundClockFrequency=125000;
    config->vm.videoClockFrequency=1000000;
  }
  else if(machineType == MACHINE_ZX)
  {
    if(machineDetailedType == VM_config.at("ZX128_TAPE") || machineDetailedType == VM_config.at("ZX128_FILE"))
    {
      config->vm.cpuClockFrequency=3546896;
      config->vm.soundClockFrequency=221681;
      config->vm.videoClockFrequency=886724;
    }
    else
    {
      config->vm.cpuClockFrequency=3500000;
      config->vm.soundClockFrequency=218750;
      config->vm.videoClockFrequency=875000;
    }
    config->vm.enableMemoryTimingEmulation=true;
  }
  config->vmConfigurationChanged = true;

  config->sound.sampleRate = EP128EMU_SAMPLE_RATE_FLOAT;
  config->sound.hwPeriods = 16;
  //config->sound.swPeriods = 16;
  //config->sound.enabled = false;
  config->sound.highQuality = true;
  config->soundSettingsChanged = true;

  // Reload config files to enforce priority of settings.
  if(Ep128Emu::does_file_exist(configBaseFile.c_str()))
  {
    config->loadState(configBaseFile.c_str(),false);
  }
  if(Ep128Emu::does_file_exist(configContentDirFile.c_str()))
  {
    config->loadState(configContentDirFile.c_str(),false);
  }
  // highest: content-specific file
  if (useCfgFile)
  {
    config->loadState(cfgFile,false);
  }

  // ROM validity check: scan ROM definitions and check if files can be be found.
  // If not, try to use replacement file
  for(int i=0; i<68; i++)
  {
    if(config->memory.rom[i].file.length()>0)
    {
      // Prepend romBasePath if the rom name is not a path already (useful for rom settings read from .ep128cfg)
#ifdef WIN32
      size_t idx = config->memory.rom[i].file.rfind('\\');
#else
      size_t idx = config->memory.rom[i].file.rfind('/');
#endif
      if(idx == std::string::npos)
      {
        config->memory.rom[i].file=romBasePath+config->memory.rom[i].file;
      }
      
      if(!Ep128Emu::does_file_exist(config->memory.rom[i].file.c_str()))
      {
        std::map< std::string, std::string >::const_iterator  iter_altrom;
        std::string romShortName;
        int page_index = config->memory.rom[i].offset / 16384;
        std::string romPageName(std::to_string(page_index));
        std::string replacementFullName;
#ifdef WIN32
        size_t idx = config->memory.rom[i].file.rfind('\\');
#else
        size_t idx = config->memory.rom[i].file.rfind('/');
#endif
        if(idx != std::string::npos)
        {
          romShortName = config->memory.rom[i].file.substr(idx+1);
          romPageName = config->memory.rom[i].file.substr(idx+1) + "_p" + romPageName;
        }
        else
        {
          log_cb(RETRO_LOG_ERROR, "Invalid ROM name: %s \n",config->memory.rom[i].file.c_str());
          throw Ep128Emu::Exception("ROM file not found!");
        }

        bool romFound = false;

        for (iter_altrom = rom_names_ep128emu_tosec.begin(); iter_altrom != rom_names_ep128emu_tosec.end(); ++iter_altrom)
        {
          if ((*iter_altrom).first == romShortName || (*iter_altrom).first == romPageName )
          {
            replacementFullName = (*iter_altrom).second.c_str();
            replacementFullName = romBasePath + replacementFullName;
            if(Ep128Emu::does_file_exist(replacementFullName.c_str()))
            {
              log_cb(RETRO_LOG_INFO, "ROM file alternative found: %s => %s\n",romPageName.c_str(), (*iter_altrom).second.c_str());
              config->memory.rom[i].file = replacementFullName;
              romFound = true;
              // If split ROM is used, remove offset.
              if ((*iter_altrom).first == romPageName)
                config->memory.rom[i].offset=0;
              break;
            }
            else
            {
              //log_cb(RETRO_LOG_DEBUG, "ROM file alternative not found: %s => %s\n",romShortName.c_str(),replacementFullName.c_str());
            }
          }
        }
        if(!romFound)
        {
          log_cb(RETRO_LOG_DEBUG, "ROM file or any alternative not found: %s \n",config->memory.rom[i].file.c_str());
          replacementFullName = "_default_" + romShortName;
          config->memory.rom[i].file = replacementFullName;
          std::map<std::string, const unsigned char*>::const_iterator  iter_builtin_rom;
          iter_builtin_rom = Ep128Emu::builtin_rom.find(replacementFullName);
          if (iter_builtin_rom == Ep128Emu::builtin_rom.end()) {
            throw Ep128Emu::Exception("ROM file not found!");
          }
          log_cb(RETRO_LOG_DEBUG, "Using default ROM for: %s \n",config->memory.rom[i].file.c_str());
        }
      }
      else
      {
        log_cb(RETRO_LOG_DEBUG, "Loading ROM from system directory: %s \n",config->memory.rom[i].file.c_str());
      }
    }
  }
  config->memoryConfigurationChanged = true;  
  
  initialize_keyboard_map();
  initialize_joystick_map(std::string(""),std::string(""),std::string(""),-1,
                          joystick_type.at("DEFAULT"), joystick_type.at("DEFAULT"), joystick_type.at("DEFAULT"),
                          joystick_type.at("DEFAULT"), joystick_type.at("DEFAULT"), joystick_type.at("DEFAULT"));
  if (config->contentFileName != "")
  {
    bool injectedBeforeStart = false;
    if (startSequence.length()>0) {
      // For EP load sequences where F1 is pressed ("START"), inject "content file name" before that
      // to allow for some customization (like key click off)
      if((unsigned char)startSequence.at(startSequence.length()-1) == 253) {
        // ...except for no-cartridge types where first F1 is pressed (load), then file name
        if (machineDetailedType  == VM_config.at("EP128_TAPE_NOCART") || machineDetailedType  == VM_config.at("EP64_TAPE_NOCART")) {
          startSequence = startSequence + config->contentFileName + "\r";
          injectedBeforeStart = true;
          log_cb(RETRO_LOG_DEBUG, "Extended startup sequence (NOCART case)\n");
        } else {
          startSequence.pop_back();
          startSequence = startSequence + config->contentFileName + "\r" + "\xfd";
          injectedBeforeStart = true;
          log_cb(RETRO_LOG_DEBUG, "Extended startup sequence (START case)\n");
        }
      }
    }
    if (!injectedBeforeStart) {
      startSequence = startSequence + config->contentFileName + "\xfe\r";
      log_cb(RETRO_LOG_DEBUG, "Extended startup sequence (disk load case)\n");
    }
  }
  //config->display.enabled = false;
  //config->displaySettingsChanged = true;
  log_cb(RETRO_LOG_DEBUG, "Applying settings\n");
  config->applySettings();

  vmThread = new Ep128Emu::VMThread(*vm);
}

LibretroCore::~LibretroCore()
{
  if (vmThread)
    delete vmThread;
  if (vm)
    delete vm;
  if (w)
    delete w;
  if (config)
    delete config;
  if (audioOutput)
    delete audioOutput;
}

void LibretroCore::initialize_keyboard_map(void)
{
  for(int i=0; i<RETROK_LAST; i++)
  {
    libretro_to_ep128emu_kbmap[i]=-1;
  }
  for(int port=0; port<EP128EMU_MAX_USERS; port++)
  {
    for(int i=0; i<256; i++)
    {
      inputJoyMap[i][port] = -1;
    }
  }

  libretro_to_ep128emu_kbmap[RETROK_n]         = 0x00;
  libretro_to_ep128emu_kbmap[RETROK_OEM_102]   = 0x01; // positional mapping for ISO keyboard: kb í -> EP \ backslash
  libretro_to_ep128emu_kbmap[RETROK_HOME]      = 0x01; // additional mapping for non-ISO keyboard: kb home -> EP \ backslash
  libretro_to_ep128emu_kbmap[RETROK_b]         = 0x02;
  libretro_to_ep128emu_kbmap[RETROK_c]         = 0x03;
  libretro_to_ep128emu_kbmap[RETROK_v]         = 0x04;
  libretro_to_ep128emu_kbmap[RETROK_x]         = 0x05;
  libretro_to_ep128emu_kbmap[RETROK_z]         = 0x06;
  libretro_to_ep128emu_kbmap[RETROK_LSHIFT]    = 0x07;
  libretro_to_ep128emu_kbmap[RETROK_h]         = 0x08;
  libretro_to_ep128emu_kbmap[RETROK_CAPSLOCK]  = 0x09; // LOCK
  libretro_to_ep128emu_kbmap[RETROK_g]         = 0x0A;
  libretro_to_ep128emu_kbmap[RETROK_d]         = 0x0B;
  libretro_to_ep128emu_kbmap[RETROK_f]         = 0x0C;
  libretro_to_ep128emu_kbmap[RETROK_s]         = 0x0D;
  libretro_to_ep128emu_kbmap[RETROK_a]         = 0x0E;
  libretro_to_ep128emu_kbmap[RETROK_LCTRL]     = 0x0F;
  libretro_to_ep128emu_kbmap[RETROK_RCTRL]     = 0x0F;

  libretro_to_ep128emu_kbmap[RETROK_u]         = 0x10;
  libretro_to_ep128emu_kbmap[RETROK_q]         = 0x11;
  libretro_to_ep128emu_kbmap[RETROK_y]         = 0x12;
  libretro_to_ep128emu_kbmap[RETROK_r]         = 0x13;
  libretro_to_ep128emu_kbmap[RETROK_t]         = 0x14;
  libretro_to_ep128emu_kbmap[RETROK_e]         = 0x15;
  libretro_to_ep128emu_kbmap[RETROK_w]         = 0x16;
  libretro_to_ep128emu_kbmap[RETROK_TAB]       = 0x17;
  libretro_to_ep128emu_kbmap[RETROK_7]         = 0x18;
  libretro_to_ep128emu_kbmap[RETROK_1]         = 0x19;
  libretro_to_ep128emu_kbmap[RETROK_6]         = 0x1A;
  libretro_to_ep128emu_kbmap[RETROK_4]         = 0x1B;
  libretro_to_ep128emu_kbmap[RETROK_5]         = 0x1C;
  libretro_to_ep128emu_kbmap[RETROK_3]         = 0x1D;
  libretro_to_ep128emu_kbmap[RETROK_2]         = 0x1E;
  libretro_to_ep128emu_kbmap[RETROK_BACKQUOTE] = 0x1F; // positional mapping for ISO keyboard, kb ` (backquote) -> EP esc
  libretro_to_ep128emu_kbmap[RETROK_ESCAPE]    = 0x1F; // additional mapping for regular Esc on modern keyboards

  libretro_to_ep128emu_kbmap[RETROK_F4]        = 0x20;
  libretro_to_ep128emu_kbmap[RETROK_F8]        = 0x21;
  libretro_to_ep128emu_kbmap[RETROK_F3]        = 0x22;
  libretro_to_ep128emu_kbmap[RETROK_F6]        = 0x23;
  libretro_to_ep128emu_kbmap[RETROK_F5]        = 0x24;
  libretro_to_ep128emu_kbmap[RETROK_F7]        = 0x25;
  libretro_to_ep128emu_kbmap[RETROK_F2]        = 0x26;
  libretro_to_ep128emu_kbmap[RETROK_F1]        = 0x27;
  libretro_to_ep128emu_kbmap[RETROK_8]         = 0x28;
  //libretro_to_ep128emu_kbmap[???]            = 0x29;
  libretro_to_ep128emu_kbmap[RETROK_9]         = 0x2A;
  libretro_to_ep128emu_kbmap[RETROK_MINUS]     = 0x2B;
  libretro_to_ep128emu_kbmap[RETROK_0]         = 0x2C;
  libretro_to_ep128emu_kbmap[RETROK_CARET]     = 0x2D; // allowing any keyboard with specific caret key
  libretro_to_ep128emu_kbmap[RETROK_EQUALS]    = 0x2D; // positional mapping for ISO keyboard: kb = -> EP ^ caret
  libretro_to_ep128emu_kbmap[RETROK_BACKSPACE] = 0x2E;
  //libretro_to_ep128emu_kbmap[???]            = 0x2F;

  libretro_to_ep128emu_kbmap[RETROK_j]         = 0x30;
  //libretro_to_ep128emu_kbmap[???]            = 0x31;
  libretro_to_ep128emu_kbmap[RETROK_k]         = 0x32;
  libretro_to_ep128emu_kbmap[RETROK_SEMICOLON] = 0x33;
  libretro_to_ep128emu_kbmap[RETROK_l]         = 0x34;
  libretro_to_ep128emu_kbmap[RETROK_COLON]     = 0x35; // allowing any keyboard with specific colon key
  libretro_to_ep128emu_kbmap[RETROK_QUOTE]     = 0x35; // positional mapping for ISO keyboard, kb ' -> EP :
  libretro_to_ep128emu_kbmap[RETROK_BACKSLASH] = 0x36; // positional mapping for ISO keyboard, kb \ -> EP ]
  //libretro_to_ep128emu_kbmap[???]            = 0x37;
  libretro_to_ep128emu_kbmap[RETROK_F10]       = 0x38; // STOP -- no positional equivalent
  libretro_to_ep128emu_kbmap[RETROK_PAUSE]     = 0x38; // STOP -- no positional equivalent
  libretro_to_ep128emu_kbmap[RETROK_DOWN]      = 0x39;
  libretro_to_ep128emu_kbmap[RETROK_RIGHT]     = 0x3A;
  libretro_to_ep128emu_kbmap[RETROK_UP]        = 0x3B;
  libretro_to_ep128emu_kbmap[RETROK_F9]        = 0x3C; // HOLD -- no positional equivalent
  //libretro_to_ep128emu_kbmap[RETROK_SCROLLOCK] = 0x3C; // HOLD - conflicts with "game focus" default
  libretro_to_ep128emu_kbmap[RETROK_LEFT]      = 0x3D;
  libretro_to_ep128emu_kbmap[RETROK_RETURN]    = 0x3E;
  libretro_to_ep128emu_kbmap[RETROK_LALT]      = 0x3F;
  libretro_to_ep128emu_kbmap[RETROK_RALT]      = 0x3F;

  libretro_to_ep128emu_kbmap[RETROK_m]         = 0x40;
  libretro_to_ep128emu_kbmap[RETROK_DELETE]    = 0x41;
  libretro_to_ep128emu_kbmap[RETROK_COMMA]     = 0x42;
  libretro_to_ep128emu_kbmap[RETROK_SLASH]     = 0x43;
  libretro_to_ep128emu_kbmap[RETROK_PERIOD]    = 0x44;
  libretro_to_ep128emu_kbmap[RETROK_RSHIFT]    = 0x45;
  libretro_to_ep128emu_kbmap[RETROK_SPACE]     = 0x46;
  libretro_to_ep128emu_kbmap[RETROK_INSERT]    = 0x47;
  libretro_to_ep128emu_kbmap[RETROK_i]         = 0x48;
  //libretro_to_ep128emu_kbmap[???]            = 0x49;
  libretro_to_ep128emu_kbmap[RETROK_o]         = 0x4A;
  libretro_to_ep128emu_kbmap[RETROK_AT]        = 0x4B; // allowing any keyboard with specific "at" key
  libretro_to_ep128emu_kbmap[RETROK_LEFTBRACKET] = 0x4B; // positional mapping for ISO keyboard, kb [ -> EP @
  libretro_to_ep128emu_kbmap[RETROK_p]         = 0x4C;
  libretro_to_ep128emu_kbmap[RETROK_RIGHTBRACKET] = 0x4D; // positional mapping for ISO keyboard, kb ] -> EP [
  //libretro_to_ep128emu_kbmap[???]            = 0x4E;
  //libretro_to_ep128emu_kbmap[???]            = 0x4F;

  // Numeric keypad mapped to ext 1 joystick, fire = 0/ins
  libretro_to_ep128emu_kbmap[RETROK_KP6]        = 0x70;
  libretro_to_ep128emu_kbmap[RETROK_KP4]        = 0x71;
  libretro_to_ep128emu_kbmap[RETROK_KP2]        = 0x72;
  libretro_to_ep128emu_kbmap[RETROK_KP8]        = 0x73;
  libretro_to_ep128emu_kbmap[RETROK_KP0]        = 0x74;
  // fire 2 and 3, not really used on Enterprise
  libretro_to_ep128emu_kbmap[RETROK_KP_PERIOD]  = 0x75;
  libretro_to_ep128emu_kbmap[RETROK_KP_PLUS]    = 0x76;
  if(machineType == MACHINE_TVC)
  {
    /* tvc extra keys in original ep128emu (tvc key - EP key - default PC UK key):
    - ö - @ - `
    - í - Esc - Esc
    - @ - F1 - F1
    - ; - F2,Tab - F2,Tab
    - <> - F3 - F3
    - \ - F4 - F4
    - * - F5,Pause - F5,Home
    - ^ - F6 - F6
    - [ - F7 - F7
    - ] - F8 - F8
    - Esc - Stop - End
    */
    libretro_to_ep128emu_kbmap[RETROK_ESCAPE]     = 0x38; // move Esc back to Esc
    libretro_to_ep128emu_kbmap[RETROK_BACKQUOTE]  = 0x4B; // use the key left of 1 for 0
    libretro_to_ep128emu_kbmap[RETROK_AT]         = -1; // unmap @
    libretro_to_ep128emu_kbmap[RETROK_F10]        = 0x1F; // use F10 for í which can not be mapped from TVC layout to ISO layout directly
    libretro_to_ep128emu_kbmap[RETROK_OEM_102]    = 0x1F; // or the extra 102. key, which has no equivalent in TVC but is used for í anyway
    libretro_to_ep128emu_kbmap[RETROK_LEFTBRACKET]  = 0x4D; // positional mapping for ISO keyboard, kb [ -> TVC ő
    libretro_to_ep128emu_kbmap[RETROK_RIGHTBRACKET] = 0x36; // positional mapping for ISO keyboard, kb ] -> TVC ú
    libretro_to_ep128emu_kbmap[RETROK_BACKSLASH]    = 0x01; // positional mapping for ISO keyboard, kb \ -> TVC ú
    // other keys cannot be mapped to ISO keyboard logically so original ep128emu mapping is kept to function keys

  }
  else if (machineType == MACHINE_CPC)
  {
    /* CPC extra keys in original ep128emu:
       rshift mapped to Fn enter
       Hold to F0
       Stop to F9
       Insert to Copy
       Alt to Fn Dot
       Delete to Clear
       Erase to Delete */

    // f10-f9 swap back (EP default uses F9 for Hold and F10 for Stop)
    libretro_to_ep128emu_kbmap[RETROK_F9]        = 0x38; // map F9 to Fn9
    libretro_to_ep128emu_kbmap[RETROK_F10]       = 0x3C; // map F10 to Fn0
    libretro_to_ep128emu_kbmap[RETROK_F11]       = 0x3F; // map F11 to Fn Dot.
    // swap semicolon and quote to restore positional mapping
    libretro_to_ep128emu_kbmap[RETROK_SEMICOLON] = 0x35;
    libretro_to_ep128emu_kbmap[RETROK_QUOTE]     = 0x33;

    libretro_to_ep128emu_kbmap[RETROK_LALT]      = 0x47; // quasi-positional mapping: lAlt for copy button
    libretro_to_ep128emu_kbmap[RETROK_KP_ENTER]  = 0x45; // map keypad enter to Fn Enter
    libretro_to_ep128emu_kbmap[RETROK_END]       = 0x45; // map End to Fn Enter
    libretro_to_ep128emu_kbmap[RETROK_RSHIFT]    = 0x07; // rshift to lshift - reverse rshift to Fn Enter default of ep128emu
  }

  for(int i=0; i<RETROK_LAST; i++)
  {
    if(libretro_to_ep128emu_kbmap[i] >= 0)
    {
      if(config->keyboard[libretro_to_ep128emu_kbmap[i]][0] > 0)
      {
        if(config->keyboard[libretro_to_ep128emu_kbmap[i]][1] > 0) {
         log_cb(RETRO_LOG_WARN, "Triple key mapping, EP scancode 0x%x was set to key %d and %d, second mapping now replaced by key %d\n",
           libretro_to_ep128emu_kbmap[i],config->keyboard[libretro_to_ep128emu_kbmap[i]][0],config->keyboard[libretro_to_ep128emu_kbmap[i]][1],i);
        }
        config->keyboard[libretro_to_ep128emu_kbmap[i]][1] = i;
      }
      else
        config->keyboard[libretro_to_ep128emu_kbmap[i]][0] = i;
    }
  }
  config->keyboardMapChanged = true;
}

// TODO: split to key and joystick setup, maybe using user + index
void LibretroCore::initialize_joystick_map(std::string zoomKey, std::string infoKey, std::string autofireKey, int autofireSpeed, int user1, int user2, int user3, int user4, int user5, int user6)
{
  // Lowest priority joystick settings: machine dependent hardcoded defaults.
  infoMessage = "Joypad: ";
  if (machineType == MACHINE_CPC)
  {
    // external joystick 1: controller 1 (cursor keys usually not used for game control)
    update_joystick_map(joystickCodesExt1,0,6);
    infoMessage += joystickNameMap[2];
    // external joystick 2: controller 2
    update_joystick_map(joystickCodesExt2,1,6);
  }
  else if (machineType == MACHINE_ZX)
  {
    // Kempston interface mapped to ext 1
    update_joystick_map(joystickCodesExt1,0,5);
    infoMessage += joystickNameMap[2];
    // The 'left' Sinclair joystick maps the joystick directions and the fire button to the 1 (left), 2 (right), 3 (down), 4 (up) and 5 (fire) keys
    update_joystick_map(joystickCodesSinclair1,1,5);
    // The 'right' Sinclair joystick maps to keys 6 (left), 7 (right), 8 (down), 9 (up) and 0 (fire)
    update_joystick_map(joystickCodesSinclair2,2,5);
    // A cursor joystick interfaces maps to keys 5 (left), 6 (down), 7 (up), 8 (right) and 0 (fire). (Protek and AGF)
    update_joystick_map(joystickCodesSinclair3,3,5);
  }
  else
  {
    // EP or TVC: internal joystick: controller 0 / user 1
    update_joystick_map(joystickCodesInt,0,5);
    infoMessage += joystickNameMap[1];
    // external joystick 1: controller 1
    update_joystick_map(joystickCodesExt1,1,5);
    // external joystick 2: controller 2
    update_joystick_map(joystickCodesExt2,2,5);
    // external joystick 3: controller 3
    update_joystick_map(joystickCodesExt3,3,5);
    // external joystick 4: controller 4
    update_joystick_map(joystickCodesExt4,4,5);
    // external joystick 5: controller 5 / user 6
    update_joystick_map(joystickCodesExt5,5,5);
  }
  inputJoyMap[EPKEY_INFO][0] = RETRO_DEVICE_ID_JOYPAD_L3; // special: info button
  inputJoyMap[EPKEY_ZOOM][0] = RETRO_DEVICE_ID_JOYPAD_R3; // special: fit to content
  inputJoyMap[0x3e][0] = RETRO_DEVICE_ID_JOYPAD_Y;  // enter
  inputJoyMap[0x2c][0] = RETRO_DEVICE_ID_JOYPAD_L;  // 0
  inputJoyMap[0x19][0] = RETRO_DEVICE_ID_JOYPAD_R;  // 1
  inputJoyMap[0x1e][0] = RETRO_DEVICE_ID_JOYPAD_L2; // 2
  inputJoyMap[0x1d][0] = RETRO_DEVICE_ID_JOYPAD_R2; // 3

  // Second priority: assignment read from .ep128cfg file
  std::string buttonprefix("EPKEY_");
  std::string button;
  std::map< std::string, unsigned char >::const_iterator  iter_epkey;
  std::string joypadPrefix("RETRO_DEVICE_ID_JOYPAD_");
  std::string joypadButton;
  std::map< std::string, unsigned char >::const_iterator  iter_joypad;

  for (int i=0; i<16; i++)
  {
    if(config->joypad[i] != "")
    {
      button = buttonprefix + config->joypad[i];
      joypadButton = joypadPrefix + config->joypadButtons[i];
      iter_epkey = epkey_reverse.find(button);
      iter_joypad = retro_joypad_reverse.find(joypadButton);
      if (iter_epkey != epkey_reverse.end() && iter_joypad != retro_joypad_reverse.end())
      {
        reset_joystick_map(0, (*iter_joypad).second);
        inputJoyMap[((*iter_epkey).second)][0] = (*iter_joypad).second;
      }
    }
  }

  // Highest priority: core option settings (if they are not left on default)
  if(zoomKey != "")
  {
    joypadButton = joypadPrefix + zoomKey;
    iter_joypad = retro_joypad_reverse.find(joypadButton);
    if (iter_joypad != retro_joypad_reverse.end())
    {
      reset_joystick_map(0, (*iter_joypad).second);
      inputJoyMap[EPKEY_ZOOM][0] = (*iter_joypad).second;
    }
  }
  if(infoKey != "")
  {
    joypadButton = joypadPrefix + infoKey;
    iter_joypad = retro_joypad_reverse.find(joypadButton);
    if (iter_joypad != retro_joypad_reverse.end())
    {
      reset_joystick_map(0, (*iter_joypad).second);
      inputJoyMap[EPKEY_INFO][0] = (*iter_joypad).second;
    }
  }

  if(autofireKey != "")
  {
    joypadButton = joypadPrefix + autofireKey;
    iter_joypad = retro_joypad_reverse.find(joypadButton);
    if (iter_joypad != retro_joypad_reverse.end())
    {
      autofireButtonId = (*iter_joypad).second;
    } else {
      autofireButtonId = 256;
    }
    if (autofireSpeed > 0) autofireFrameCycle = (unsigned int) autofireSpeed;
  }

  // Override joypad users (received from core options) with config values, if they are left at default.
  int mappings[EP128EMU_MAX_USERS] =
  {
    user1 == joystick_type.at("DEFAULT") ? joystick_type.at(config->joypadUser[0]) : user1,
    user2 == joystick_type.at("DEFAULT") ? joystick_type.at(config->joypadUser[1]) : user2,
    user3 == joystick_type.at("DEFAULT") ? joystick_type.at(config->joypadUser[2]) : user3,
    user4 == joystick_type.at("DEFAULT") ? joystick_type.at(config->joypadUser[3]) : user4,
    user5 == joystick_type.at("DEFAULT") ? joystick_type.at(config->joypadUser[4]) : user5,
    user6 == joystick_type.at("DEFAULT") ? joystick_type.at(config->joypadUser[5]) : user6,
  };
  for (int i=0; i<EP128EMU_MAX_USERS; i++)
  {
    if(mappings[i]>0)
    {
      // Use 2nd fire button for CPC
      if(machineType == MACHINE_CPC)
        update_joystick_map(joystickCodeMap[mappings[i]],i,6);
      // Otherwise, only 1 fire button
      else
        update_joystick_map(joystickCodeMap[mappings[i]],i,5);
      if (i==0)
      {
        infoMessage = "Joypad: ";
        infoMessage += joystickNameMap[mappings[i]];
      }
    }
  }

  // Fill the info text.
  infoMessage += ", button map: ";
  for (int j=0; j<12; j++)
  {
    int btnIndex = buttonOrderInInfoMessage[j];
    for(int i=0; i<256; i++)
    {
      if(inputJoyMap[i][0] == btnIndex && i != EPKEY_NONE)
      {
        infoMessage += buttonNames[inputJoyMap[i][0]];
        infoMessage += "=";
        for (auto it = epkey_reverse.begin(); it != epkey_reverse.end(); ++it)
          if (it->second == i)
          {
            infoMessage += it->first.substr(6,6);
            break;
          }
        if (inputJoyMap[i][0] == (int)autofireButtonId)
          infoMessage += "(autofire)";
        infoMessage += " ";
        break;
      }
    }
  }
}

void LibretroCore::update_joystick_map(const unsigned char * joystickCodes, int port, int length)
{
  reset_joystick_map(port);
  inputJoyMap[joystickCodes[0]][port] = RETRO_DEVICE_ID_JOYPAD_UP;
  inputJoyMap[joystickCodes[1]][port] = RETRO_DEVICE_ID_JOYPAD_DOWN;
  inputJoyMap[joystickCodes[2]][port] = RETRO_DEVICE_ID_JOYPAD_LEFT;
  inputJoyMap[joystickCodes[3]][port] = RETRO_DEVICE_ID_JOYPAD_RIGHT;
  inputJoyMap[joystickCodes[4]][port] = RETRO_DEVICE_ID_JOYPAD_X;
  if(length>=6 && joystickCodes[5] < 0x80)
    inputJoyMap[joystickCodes[5]][port] = RETRO_DEVICE_ID_JOYPAD_A;
  if(length>=7  && joystickCodes[6] < 0x80)
    inputJoyMap[joystickCodes[6]][port] = RETRO_DEVICE_ID_JOYPAD_Y;
}

void LibretroCore::reset_joystick_map(int port, unsigned value)
{
  for(int i=0; i<255; i++)
  {
    if((unsigned)inputJoyMap[i][port] == value)
      inputJoyMap[i][port] = -1;
  }
}

void LibretroCore::reset_joystick_map(int port)
{
  for(int i=1; i<JOY_TYPE_AMOUNT; i++)
  {
    // reset only the one that was set previously
    if (inputJoyMap[joystickCodeMap[i][0]][port] == RETRO_DEVICE_ID_JOYPAD_UP)
    {
      inputJoyMap[joystickCodeMap[i][0]][port] = -1;
      inputJoyMap[joystickCodeMap[i][1]][port] = -1;
      inputJoyMap[joystickCodeMap[i][2]][port] = -1;
      inputJoyMap[joystickCodeMap[i][3]][port] = -1;
      inputJoyMap[joystickCodeMap[i][4]][port] = -1;
      if(joystickCodeMap[i][5] < 0x80)
        inputJoyMap[joystickCodeMap[i][5]][port] = -1;
      if(joystickCodeMap[i][6] < 0x80)
        inputJoyMap[joystickCodeMap[i][6]][port] = -1;
      break;
    }
  }
}

void LibretroCore::update_keyboard(bool down, unsigned keycode, uint32_t character, uint16_t key_modifiers)
{
  // printf("%s %d %d \n",down ? "down" : "up  ", keycode, character);
  int convertedKeycode = config->convertKeyCode(keycode);
  if (convertedKeycode >= 0)
  {
    vmThread->setKeyboardState((uint8_t)convertedKeycode,down);
  } else if (down && keycode == RETROK_F12) {
    w->scanBorders = true;
  }
  // TODO: if conversion does not succeed, try to apply literal mapping instead of default positional
  // it requires more work because upstrokes will not have character value so keypresses have to be stored separately
  // to match with up events
}

void LibretroCore::update_input(retro_input_state_t input_state_cb, retro_environment_t environ_cb, unsigned maxUsers)
{
  int i;
  unsigned port;
  bool currInputState;
  unsigned scanLimit = maxUsers < EP128EMU_MAX_USERS ? maxUsers : EP128EMU_MAX_USERS;

  for(port=0; port<scanLimit; port++)
  {
    for(i=0; i<256; i++)
    {
      if(inputJoyMap[i][port]>=0)
      {
        currInputState = input_state_cb(port, RETRO_DEVICE_JOYPAD, 0, inputJoyMap[i][port]);
        if (currInputState && !inputStateMap[i][port])
        {
          // Joystick map codes below 0x80 are interpreted as keyboard state
          // including all joystick types (int, ext)
          if(i<128)
          {
            vmThread->setKeyboardState(i,true);
          }
          // All other codes are interpreted by the libretro core itself.
          else
          {
            if(i == EPKEY_INFO)
            {
              struct retro_message message;
              std::string longMsg = "Wait for end of startup sequence... ";
              if (startSequenceIndex < startSequence.length())
              {
                longMsg += infoMessage;
                message.msg = longMsg.c_str();
              }
              else
              {
                message.msg = infoMessage.c_str();
              }
              message.frames = EP128EMU_MESSAGE_DISPLAY_FRAMES;
              environ_cb(RETRO_ENVIRONMENT_SET_MESSAGE, &message);
            }
            if(i == EPKEY_ZOOM)
            {
              w->scanBorders = true;
            }
            // EPKEY_NONE does nothing.
          }
        }
        else if (inputStateMap[i][port] && !currInputState)
        {
          if(i<128)
            vmThread->setKeyboardState(i,false);
        }
        // autofire - button is pressed already
        else if (inputJoyMap[i][port] == (int)autofireButtonId && inputStateMap[i][port] && currInputState) {
          bool shouldFire =    (w->frameCount >= autofireFrame + 2*autofireFrameCycle) ? true        : false;
          bool shouldRelease = (w->frameCount >= autofireFrame +   autofireFrameCycle) ? !shouldFire : false;
          if(shouldFire) {
            autofireFrame = w->frameCount;
            vmThread->setKeyboardState(i,true);
          }
          if(shouldRelease) {
            vmThread->setKeyboardState(i,false);
          }
        }

        inputStateMap[i][port] = currInputState;
      }
    }
  }
  // Mouse handling
  int16_t dX = input_state_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_X);
  int16_t dY = input_state_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_Y);
  uint8_t mouseButtonState = input_state_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_LEFT) ? 1 : 0;
  uint8_t mouseWheelEvent = 
    input_state_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_WHEELUP  ) ? 1<<0 : 0 |
    input_state_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_WHEELDOWN) ? 1<<1 : 0;
  vmThread->setMouseState((int8_t)dX*-1, (int8_t)dY*-1, mouseButtonState, mouseWheelEvent);
  // startSequence handling.
  // Send keyboard input at specific frames (down presses)
  if (startSequenceIndex < startSequence.length())
  {
    if (w->frameCount == (bootframes + startSequenceIndex*20))
    {
      // Double quote " is not available on the keyboard so it gets a special mapping
      // Generic solution was not designed as this startsequence is really limited
      if((unsigned char)startSequence.at(startSequenceIndex) == 254)
      {
        update_keyboard(true,RETROK_LSHIFT,0,0);
        update_keyboard(true,RETROK_2,0,0);
      }
      // F1 is available on the keyboard, but has no single-character map that would be
      // suitable for .ep128cfg file, special mapping again
      else if((unsigned char)startSequence.at(startSequenceIndex) == 253)
      {
        update_keyboard(true,RETROK_F1,0,0);
      }
      else
      {
        update_keyboard(true,startSequence.at(startSequenceIndex),0,0);
      }
      startSequenceIndex++;
    }
  }
  // Send keyboard input at specific frames (key releases)
  if (startSequenceIndex <= startSequence.length())
  {
    if (startSequenceIndex > 0 && w->frameCount == (bootframes + (startSequenceIndex-1)*20+10))
    {
      if((unsigned char)startSequence.at(startSequenceIndex-1) == 254)
      {
        update_keyboard(false,RETROK_LSHIFT,0,0);
        update_keyboard(false,RETROK_2,0,0);
      }
      else if((unsigned char)startSequence.at(startSequenceIndex-1) == 253)
      {
        update_keyboard(false,RETROK_F1,0,0);
      }
      else
      {
        update_keyboard(false,startSequence.at(startSequenceIndex-1),0,0);
      }
      if (startSequenceIndex == startSequence.length())
      {
        startSequenceIndex++;
        // Start tape for ZX at the end of startup sequence, or if there is forced tape play
        if(machineDetailedType == VM_config.at("ZX128_TAPE") || config->tape.forceMotorOn)
        {
          vm->tapePlay();
        }
      }
    }
  }
}

void LibretroCore::render(retro_video_refresh_t video_cb, retro_environment_t environ_cb)
{
  // Transition from half frame (normal video mode) to interlaced
  if (useHalfFrame && isHalfFrame && w->interlacedFrameCount > 0)
  {
    isHalfFrame = false;
    change_resolution(defaultWidth, fullHeight, environ_cb);
    w->resetViewport();
  }
  // Transition from interlaced back to half frame
  else if (useHalfFrame && (!isHalfFrame && w->interlacedFrameCount == 0))
  {
    isHalfFrame = true;
    change_resolution(defaultWidth, defaultHalfHeight, environ_cb);
    w->resetViewport();
  }
  else if (useHalfFrame && isHalfFrame && w->bordersScanned)
  {
    w->bordersScanned = false;
    // Transition from zoomed-in view back to default
    if(!w->isViewportDefault())
    {
      change_resolution(defaultWidth, defaultHalfHeight, environ_cb);
      w->resetViewport();
    }
    else
    {
      int proposedY1 = w->contentTopEdge - borderSize > 0 ? w->contentTopEdge - borderSize : 0;
      int proposedY2 = w->contentBottomEdge + borderSize < EP128EMU_LIBRETRO_SCREEN_HEIGHT ? w->contentBottomEdge + borderSize : EP128EMU_LIBRETRO_SCREEN_HEIGHT-1;
      int detectedHeight = proposedY2 - proposedY1 + 2;

      int proposedX1 = w->contentLeftEdge - borderSize > 0 ? w->contentLeftEdge - borderSize : 0;
      int proposedX2 = w->contentRightEdge + borderSize < EP128EMU_LIBRETRO_SCREEN_WIDTH ? w->contentRightEdge + borderSize : EP128EMU_LIBRETRO_SCREEN_WIDTH-1;
      int detectedWidth = proposedX2 - proposedX1 + 1;
      // todo: if the detected height is small, adjust it to be centered

      if (detectedHeight >= 150 && detectedWidth >= 200)
      {
        bool setSuccess = w->setViewport(proposedX1,proposedY1,proposedX2,proposedY2);
        if (setSuccess)
        {
          log_cb(RETRO_LOG_DEBUG, "Viewport reduced: %d,%d / %d,%d\n",proposedX1,proposedY1,proposedX2,proposedY2);
          change_resolution(detectedWidth, detectedHeight/2, environ_cb);
        }
        else
        {
          log_cb(RETRO_LOG_WARN, "Viewport setting error: %d,%d / %d,%d\n",proposedX1,proposedY1,proposedX2,proposedY2);
        }
      }
      else
      {
        log_cb(RETRO_LOG_INFO, "Viewport not set, detected: %d,%d / %d,%d\n",proposedX1,proposedY1,proposedX2,proposedY2);
      }
    }
  }
  unsigned stride  = currWidth;

  if (canSkipFrames && prevFrameCount == w->frameCount)
  {
    log_cb(RETRO_LOG_DEBUG, "frame dupe %d \n",prevFrameCount);
    video_cb(NULL, 0, 0, 0);
  }
  else
  {
    prevFrameCount = w->frameCount;
    {
      // Video callback, stride depends on pixel format (4 byte / 2 byte)
#ifdef EP128EMU_USE_XRGB8888
      video_cb(w->frame_bufActive, currWidth, currHeight, stride << 2);
#else
      video_cb(w->frame_bufActive, currWidth, currHeight, stride << 1);
#endif
    }
  }
}

void LibretroCore::change_resolution(int width, int height, retro_environment_t environ_cb)
{

  float aspect = 4.0f / 3.0f;
  // Adjust aspect ratio if viewport is set to something else,
  // as border detection is done independently for all 4 edges
  // If half height is used, it needs to be taken into account
  aspect = 4.0f / (3.0f /
                   (float) (isHalfFrame ? EP128EMU_LIBRETRO_SCREEN_HEIGHT/2/(float)height : EP128EMU_LIBRETRO_SCREEN_HEIGHT/(float)height) *
                   (float) (EP128EMU_LIBRETRO_SCREEN_WIDTH/(float)width)
                  );
  retro_game_geometry g =
  {
    .base_width   = (unsigned int) width,
    .base_height  = (unsigned int) height,
    .max_width    = EP128EMU_LIBRETRO_SCREEN_WIDTH,
    .max_height   = EP128EMU_LIBRETRO_SCREEN_HEIGHT,
    .aspect_ratio = aspect,
  };
  log_cb(RETRO_LOG_DEBUG, "Changing resolution: %d x %d\n",width,height);
  environ_cb(RETRO_ENVIRONMENT_SET_GEOMETRY, &g);
  currWidth = width;
  currHeight = height;
}

void LibretroCore::start(void)
{
  vmThread->setSpeedPercentage(0);
  vmThread->lock(0x7FFFFFFF);
  vmThread->unlock();
  vmThread->pause(false);
  log_cb(RETRO_LOG_DEBUG, "Core started\n");
}

void LibretroCore::run_for(retro_usec_t frameTime, float waitPeriod, void * fb)
{
  //Ep128Emu::VMThread::VMThreadStatus  vmThreadStatus(*vmThread);
  //log_cb(RETRO_LOG_DEBUG, "Running core for %d ms\n",frameTime);
  totalTime += frameTime;
  // Direct framebuffer usage
  if(fb)
  {
#ifdef EP128EMU_USE_XRGB8888
    w->frame_bufActive = (uint32_t*)fb;
#else
    w->frame_bufActive = (uint16_t*)fb;
#endif // EP128EMU_USE_XRGB8888
  }
  // Own framebuffer usage
  else
  {
#ifdef EP128EMU_USE_XRGB8888
    w->frame_bufActive = (uint32_t*) w->frame_buf1;
#else
    w->frame_bufActive = (uint16_t*) w->frame_buf1;
#endif // EP128EMU_USE_XRGB8888
  }

  vmThread->allowRunFor(frameTime);
  do
  {
    w->wakeDisplay(false);
    if (vmThread->isReady()) break;
    if (waitPeriod > 0)
      Timer::wait(waitPeriod);
  }
  while(true);
}

void LibretroCore::sync_display(void)
{
  w->wakeDisplay(true);
}

void LibretroCore::errorCallback(void *userData, const char *msg)
{
  (void) userData;
  log_cb(RETRO_LOG_ERROR, "Core error callback: %s\n", msg);
}

}       // namespace Ep128Emu
