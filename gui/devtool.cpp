#include "devtool.hpp"
#ifdef ENABLE_DEVTOOL

#include "fldisp.hpp"
#include "ep128vm.hpp"
#include "zx128vm.hpp"
#include "cpc464vm.hpp"
#include "tvc64vm.hpp"
#include <iostream>
#include <typeinfo>

namespace dtBridge {

// Globals for state transfer to devtool
machine_state_t machine_state_actual = {0};
machine_functions_t machine_functions_bridge = {0};
z80_state_t z80_state_bridge = {0};
void * ram_segment_table[256];
Ep128Emu::VirtualMachine *runningVm;
// Function pointers from DLL
INITPTR dtInitFromDll;
EXECINSTRPTR dtExecInstrFromDll;
WRITECBPTR dtWriteCallbackFromDll;

static void placeholderDummy()
{
   std::cout << "[devtool] dummy placeholder" << std::endl;
   return;
}

static void placeholderDummy2(char * tmp)
{
   std::cout << "[devtool] dummy placeholder 2" << std::endl;
   return;
}

static void placeholderDummy3(int tmp1, int tmp2)
{
   std::cout << "[devtool] dummy placeholder 3" << std::endl;
   return;
}

static void placeholderDummy4(char * tmp1, int tmp2)
{
   std::cout << "[devtool] dummy placeholder 4" << std::endl;
   return;
}

static void placeholderDummy5(unsigned int tmp)
{
   std::cout << "[devtool] dummy placeholder 5" << std::endl;
   return;
}

static machine_t getModel()
{
   /* Placeholder - to be seen if model will be used for EP */
   std::cout << "[devtool] get model" << std::endl;
   if      (typeid(runningVm) == typeid(Ep128::Ep128VM))
      return MODEL_64;
   else if (typeid(runningVm) == typeid(TVC64::TVC64VM))
      return MODEL_64;
   else if (typeid(runningVm) == typeid(CPC464::CPC464VM))
      return MODEL_64;
   else if (typeid(runningVm) == typeid(ZX128::ZX128VM))
      return MODEL_64;
   else
      return MODEL_64;
}

static void resetSoft()
{
   runningVm->reset(false);
   return;
}

static void resetHard()
{
   runningVm->reset(true);
   return;
}

static void typeTextDummy(char* text)
{
   std::cout << "[devtool] dummy type text" << std::endl;
   return;
}

static void createTransferStructures(Ep128Emu::VirtualMachine *vm)
{
   int i;
   // Segment table: 256 pointers to the 16k sized ROM/RAM segments.
   // The whole 256 items only makes sense for Enterprise, but for compatibility it is used always.
   for (i=0;i<256;i++)
   {
      ram_segment_table[i] = (void*)runningVm->getSegmentPtr(i);
   }
   machine_state_actual.ram = reinterpret_cast<unsigned char *>(ram_segment_table);

   // Video RAM is currently a dummy placeholder only.
   machine_state_actual.video_ram = (unsigned char *) calloc(16384, sizeof(char));
   /* vm->getSegmentPtr(vm->getVideoMemory());*/ 
   
   // Initial port value is read to have a default, but later on it will be updated to the last value written to port (it may not be the same)
   machine_state_actual.io_shadow = (unsigned char *) calloc(256, sizeof(char));
   for(i=0;i<256;i++)
   {
     machine_state_actual.io_shadow[i] = runningVm->readIOPort(i);
   }

   // Video register exposure is direct
   machine_state_actual.crtc_shadow  = (unsigned char *) runningVm->getVideoRegisters();
   // TVC definitions, ROM on segment 0, extension ROM on segment 2.
   machine_state_actual.rom =          (unsigned char *) vm->getSegmentPtr(0);
   machine_state_actual.ext_rom =      (unsigned char *) vm->getSegmentPtr(2);
   
   machine_state_actual.cpu = &z80_state_bridge;

   machine_functions_bridge.machine_init =           (TMachine_init)           placeholderDummy2;
	machine_functions_bridge.Machine_reset =          (TMachine_reset)          resetSoft;
	machine_functions_bridge.Machine_hard_reset =     (TMachine_hard_reset)     resetHard;
	machine_functions_bridge.Machine_run =            (TMachine_run)            placeholderDummy;
	machine_functions_bridge.Machine_do_some_frames = (TMachine_do_some_frames) placeholderDummy3;
	machine_functions_bridge.Machine_type_text =      (TMachine_type_text)      typeTextDummy;
	machine_functions_bridge.machine_autostart_file = (TMachine_autostart_file) placeholderDummy4;
	machine_functions_bridge.Machine_set_model =      (TMachine_set_model)      placeholderDummy5;
	machine_functions_bridge.Machine_get_model =      (TMachine_get_model)      getModel;
	machine_functions_bridge.Machine_update_screen =  (TMachine_update_screen)  placeholderDummy;
}


void loadDevtoolDLL(HWND hwnd1, HWND hwnd2, Ep128Emu::VirtualMachine *vm)
{
  int retval;
  unsigned int vmType = ID_EP128_TVC;
  runningVm = vm;
  HINSTANCE hGetProcIDDLL = LoadLibrary("dtdebug.dll");
  if (!hGetProcIDDLL) {
    std::cout << "could not load the dynamic library" << std::endl;
    return;
  }

#ifdef DEVTOOL_DLL_0420
  dtInitFromDll = (INITPTR)GetProcAddress(hGetProcIDDLL, "dtInit");
#else
  dtInitFromDll = (INITPTR)GetProcAddress(hGetProcIDDLL, "dtInitEP");
#endif
  if (!dtInitFromDll) {
    std::cout << "Could not locate the dtInit procedure" << std::endl;
    return;
  }
  dtExecInstrFromDll = (EXECINSTRPTR)GetProcAddress(hGetProcIDDLL, "dtExecInstr");
  if (!dtExecInstrFromDll) {
    std::cout << "Could not locate the dtExecInstr procedure" << std::endl;
    return;
  }
  dtWriteCallbackFromDll = (WRITECBPTR)GetProcAddress(hGetProcIDDLL, "dtWriteCallback");
  if (!dtWriteCallbackFromDll) {
    std::cout << "Could not locate the dtWriteCallback procedure" << std::endl;
    return;
  }
  createTransferStructures(vm);
  std::cout << "[devtool] Transfer structures created" << std::endl;

  if      (typeid(runningVm) == typeid(Ep128::Ep128VM))
    vmType = ID_EP128_EP;
  else if (typeid(runningVm) == typeid(TVC64::TVC64VM))
    vmType = ID_EP128_TVC;
  else if (typeid(runningVm) == typeid(CPC464::CPC464VM))
    vmType = ID_EP128_CPC;
  else if (typeid(runningVm) == typeid(ZX128::ZX128VM))
    vmType = ID_EP128_SPECCY;

#ifdef DEVTOOL_DLL_0420
  retval = dtInitFromDll(hwnd1, &machine_state_actual, &machine_functions_bridge);
#else
  retval = dtInitFromDll(hwnd1, &machine_state_actual, &machine_functions_bridge, vmType, hwnd2);
#endif
  std::cout << "[devtool] dtInit returned " << retval << std::endl;

  return;
}

/* Actual bridge called before each z80 instruction.
 * Z80 state is converted, I/O ports accounted in this file, RAM and video registers are currently exposed directly. */
void dtExecInstrBridge(z80_state_t *currStateConverted)
{
  if (!dtExecInstrFromDll)
   return;
  z80_state_bridge = *currStateConverted;
  runningVm->denyDisplayRefresh(true);
  /*if (machine_state_actual.cpu->pc.word == 0xdad3)
   std::cout << "[devtool] dtExec ping should be OK, pc: " << machine_state_actual.cpu->pc.word << std::endl;*/
  dtExecInstrFromDll(0);
  runningVm->denyDisplayRefresh(false);
}

void dtWriteCallbackBridge(unsigned short addr, unsigned char byte, unsigned int type)
{
  if (!dtWriteCallbackFromDll)
   return;
  // I/O port writes are accounted locally. Note: this shows the last value written to the ports,
  // which may not be the same as normal emulated read (if read is allowed at all).
  if (type == 1 && addr < 256)
    machine_state_actual.io_shadow[addr] = byte;
  dtWriteCallbackFromDll(addr, byte, type);
}

}
#endif // ENABLE_DEVTOOL

