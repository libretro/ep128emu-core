/* This file is originated from tvc256++ firmware. The structure
   is kept to enable merge of later changes easier than re-implementing
   all the functions. */
#ifndef EP128EMU_TVCROUTINES_H
#define EP128EMU_TVCROUTINES_H

#include <stdint.h>
#include <dirent.h>
#include "tvcmem.hpp"

typedef uint8_t (*tvc_function_t)(uint8_t* bufferStart);

typedef struct {
    tvc_function_t func;
    uint8_t param_size;
} tvc_function_struct_t;

namespace TVC256 {

extern uint8_t registerScreenBaseAddr;
extern uint8_t registerBitmapBaseAddr;
extern uint8_t registerScreenColorBaseAddr;
extern uint8_t registerFunctionBitmapBase;
extern TVC64::Memory *emuMem;
extern uint16_t screenMaxY;
extern tvc_function_struct_t tvc256k_funct_struct_array[256];

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