#ifndef EP128EMU_DEVTOOL_HPP
#define EP128EMU_DEVTOOL_HPP
#ifdef WIN32

typedef void machine_functions_t;
typedef void z80_state_t;

typedef struct MACHINE_STATE
{
	z80_state_t* cpu;
	unsigned char* ram; /* 65536 elements */
	unsigned char* video_ram; /* 16384 elements */
	unsigned char* io_shadow; /* 256 elements */
	unsigned char* crtc_shadow; /* 32 elements */
	unsigned char* rom;
	unsigned char* ext_rom;
} machine_state_t;



extern "C" __declspec(dllexport) int __stdcall dtInit(HWND hWnd, machine_state_t* machine_state,machine_functions_t *machine_functions);
extern "C" __declspec(dllexport) int __stdcall dtExecInstr(int last_cycle);
extern "C" __declspec(dllexport) int __stdcall dtWriteCallback(unsigned short addr, unsigned char byte, unsigned int type);

#endif  // WIN32
#endif  // EP128EMU_GUI_HPP
