// Integration with TVCDeveloperTool
// Currently based on the existing interface between Devtool and WinTVC.

#ifndef EP128EMU_DEVTOOL_HPP
#define EP128EMU_DEVTOOL_HPP
#ifdef ENABLE_DEVTOOL

#include <windows.h>
#include "vm.hpp"

namespace dtBridge {

#ifdef __GNUWIN32__ /*__GNUC__*/
typedef unsigned long long tstate_t;
#define TSTATE_T_MAX ULLONG_MAX
#define TSTATE_T_MID (((long long) -1LL)/2ULL)
#define TSTATE_T_LEN "Lu"
#else
#include <limits.h>
typedef unsigned __int64 tstate_t;
#define TSTATE_T_MAX _UI64_MAX
#define TSTATE_T_MID (((unsigned __int64) -1L)/2UL)
#define TSTATE_T_LEN "lu"
#endif

#define ID_WINTVC       0
#define ID_EP128_TVC    1
#define ID_EP128_EP    	2
#define ID_EP128_SPECCY	3
#define ID_EP128_CPC	   4
#define ID_HOMELAB		5
#define ID_PRIMO		   6
#define ID_HT1080       7

struct twobyte
{
    unsigned char low, high;
};

typedef union
{
	struct twobyte byte;
	unsigned short word;
} wordregister;

typedef struct //_Z80_STATE // DEVTOOL uses only the registers
{
	wordregister af;
	wordregister bc;
	wordregister de;
	wordregister hl;
	wordregister ix;
	wordregister iy;
	wordregister sp;
	wordregister pc;

	wordregister af_prime;
	wordregister bc_prime;
	wordregister de_prime;
	wordregister hl_prime;

	unsigned char i;	/* interrupt-page address register */
	unsigned char r;   /* memory-refresh register */

	unsigned char iff1, iff2;
	unsigned char interrupt_mode; /* IM */

	int irq;

	int nmi, nmi_seen;

	/* Cyclic T-state counter */
	tstate_t t_count;

	/* Clock in MHz = T-states per microsecond */
	tstate_t clock;

	/* Breakpoint address */
	wordregister breakpoint; // NOT USED BY DEVTOOL
	int bp_active;           // NOT USED BY DEVTOOL
	int bp_reached;          // NOT USED BY DEVTOOL
} z80_state_t;

/* TVC model types from WinTVC. Not mapped to ep128emu configs yet. */
enum machine_t {
	MODEL_32,
	MODEL_32_64,
	MODEL_64,
	MODEL_64_RUSSIAN,
	MODEL_64_BASIC21,
	MODEL_64PLUS,
	MODEL_CUSTOM,
	MODEL_DUMMY_ITEM
};

typedef void (WINAPI* TMachine_init)(char* exe_path);                      // NOT USED BY DEVTOOL
typedef void (WINAPI* TMachine_reset)();
typedef void (WINAPI* TMachine_hard_reset)();
typedef void (WINAPI* TMachine_run)();                                     // NOT USED BY DEVTOOL
typedef void (WINAPI* TMachine_do_some_frames)(int nr, int update_video);  // NOT USED BY DEVTOOL
typedef void (WINAPI* TMachine_type_text)(char* text);                     // Maybe TODO - a way to start the program execution ("RUN\r")
typedef void (WINAPI* TMachine_autostart_file)(char* filename, int reset); // NOT USED BY DEVTOOL
typedef void (WINAPI* TMachine_set_model)(unsigned int m);                 // NOT USED BY DEVTOOL
typedef machine_t (WINAPI* TMachine_get_model)();
typedef void (WINAPI* TMachine_update_screen)();                           // NOT USED BY DEVTOOL
//typedef unsigned char* (WINAPI* memory_get_ext_rom)();
//typedef unsigned char* (WINAPI* memory_get_rom_ptr)();

/* typedef unsigned char (*memory_get_pointer_callback)();*/

typedef struct
{
	TMachine_init machine_init;                     // NOT USED BY DEVTOOL
	TMachine_reset Machine_reset;
	TMachine_hard_reset Machine_hard_reset;
	TMachine_run Machine_run;                       // NOT USED BY DEVTOOL
	TMachine_do_some_frames Machine_do_some_frames; // NOT USED BY DEVTOOL
	TMachine_type_text Machine_type_text;
	TMachine_autostart_file machine_autostart_file; // NOT USED BY DEVTOOL
	TMachine_set_model Machine_set_model;           // NOT USED BY DEVTOOL
	TMachine_get_model Machine_get_model;
	TMachine_update_screen Machine_update_screen;   // NOT USED BY DEVTOOL
} machine_functions_t;

typedef struct MACHINE_STATE
{
	z80_state_t* cpu;
	unsigned char* ram;          /* 65536 elements */ /* Modified: this is currently a segment table */
	unsigned char* video_ram;    /* 16384 elements */ /* Not filled currently */
	unsigned char* io_shadow;    /* 256 elements */   /* Readonly copy passed */
	unsigned char* crtc_shadow;  /* 32 elements */    /* Direct CRTC register structure passed */
	unsigned char* rom;          /* Filled as segment table 00, TVC default ROM segment   */
	unsigned char* ext_rom;      /* Filled as segment table 02, TVC extension ROM segment */

} machine_state_t;

// Functions are very dynamically imported from DLL and used via pointers, to avoid compile time dependency.
// This also means a jump in the dark and hope that functions in the dll are really as described here.
// Tries to find following functions from DLL:
// extern "C" __declspec(dllimport) int __stdcall dtInit(HWND hWnd, machine_state_t* machine_state,machine_functions_t *machine_functions, unsigned int emulatorID, HWND hWnd);
// extern "C" __declspec(dllimport) int __stdcall dtExecInstr(int last_cycle);
// extern "C" __declspec(dllimport) int __stdcall dtWriteCallback(unsigned short addr, unsigned char byte, unsigned int type);

typedef int (__stdcall *INITPTR)(HWND,machine_state_t*,machine_functions_t*,unsigned int,HWND);
typedef int (__stdcall *EXECINSTRPTR)(int);
typedef int (__stdcall *WRITECBPTR)(unsigned short, unsigned char, unsigned int);

// Bridge counterparts
void loadDevtoolDLL(HWND hwnd1, HWND hwnd2, Ep128Emu::VirtualMachine  *vm);
bool dtExecInstrBridge(z80_state_t *currStateConverted);
void dtWriteCallbackBridge(unsigned short addr, unsigned char byte, unsigned int type);
}

#endif  // ENABLE_DEVTOOL
#endif  // EP128EMU_DEVTOOL_HPP
