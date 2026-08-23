/* This file is originated from tvc256++ firmware. The structure
   is kept to enable merge of later changes easier than re-implementing
   all the functions. */
#ifndef EP128EMU_TVCROUTINES_H
#define EP128EMU_TVCROUTINES_H

#define MSC_FOPENFILE  0x01
#define MSC_FCLOSEFILE 0x02
#define MSC_FREAD      0x03
#define MSC_FWRITE     0x04
#define MSC_FOPENDIR   0x05
#define MSC_FCLOSEDIR  0x06
#define MSC_FREADDIR   0x07
#define MSC_FWRITE_SRC 0x08
#define MSC_FREAD_DEST 0x09
#define MSC_FSEEK      0x0A
#define MSC_FGETCWD    0x0B
#define MSC_FCHDIR     0x0C
#define MSC_FMKDIR     0x0D
#define MSC_FDELETE    0x0E
#define MSC_FRENAME    0x0F
#define MSC_FSTAT      0x10
#define MSC_FSYNC      0x11
#define MSC_MOUNT_DSK  0x12
#define MSC_UMOUNT_DSK 0x13

#include <stdint.h>
#include <dirent.h>
#include "vm.hpp"
#include "tvcmem.hpp"

typedef uint8_t (*tvc_function_t)(uint8_t* bufferStart);

typedef struct {
    tvc_function_t func;
    uint8_t param_size;
} tvc_function_struct_t;

typedef struct {
    uint8_t len;
    uint8_t str[255];
} TVCString;

namespace TVC256 {

extern uint8_t registerScreenBaseAddr;
extern uint8_t registerBitmapBaseAddr;
extern uint8_t registerScreenColorBaseAddr;
extern uint8_t registerFunctionBitmapBase;
extern TVC64::Memory *emuMem;
extern Ep128Emu::VirtualMachine *emuVm;
extern uint16_t screenMaxY;
extern tvc_function_struct_t tvc256k_funct_struct_array[256];
extern DIR *routinesDirHandle;
extern TVCString currDir;
extern uint8_t* tvcRomBufferIn;
extern uint8_t* tvcRomBufferOut;
extern uint16_t maxBufLen;
extern uint8_t* bufferNextSegment;

uint8_t clear_text_screen(uint8_t* bufferStart);
uint8_t clear_bitmap_screen(uint8_t* bufferStart);
uint8_t print_character_screen_code(uint8_t* bufferStart);
uint8_t print_string_screen_code(uint8_t* bufferStart);
uint8_t print_string_ascii(uint8_t* bufferStart);
uint8_t print_hex_byte(uint8_t* bufferStart);
uint8_t print_hex_word(uint8_t* bufferStart);
uint8_t goto_xy(uint8_t* bufferStart);
uint8_t set_xy(uint8_t* bufferStart);
uint8_t get_xy(uint8_t* bufferStart);
uint8_t set_text_color(uint8_t* bufferStart);
uint8_t get_text_color(uint8_t* bufferStart);
uint8_t replace_pixel_color(uint8_t* bufferStart);
uint8_t getFunctionParamSize(uint8_t *bufStart, uint8_t paramSize);


/*
void print_hex_digit(uint8_t digit);
void clear_text_screen(uint8_t* bufferStart);
void clear_bitmap_screen(uint8_t* bufferStart);
void print_character_screen_code(uint8_t* bufferStart);
void print_hex_byte(uint8_t* bufferStart);
void print_hex_word(uint8_t* bufferStart);
void print_string_screen_code(uint8_t* bufferStart);
void print_string_ascii(uint8_t* bufferStart);
void goto_xy(uint8_t* bufferStart);
*/


void init_routines();
}
#endif
