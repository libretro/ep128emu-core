/* This file is originated from tvc256++ firmware. The structure
   is kept to enable merge of later changes easier than re-implementing
   all the functions. */
#include <stdint.h>
#include <string.h>
//#include "hardware/sync.h"
//#include "hardware/clocks.h"
//#include "pico/critical_section.h"
//#include "TVC-IO-main-sys.h"
//#include "TVC-IO-main-gfx.h"
#include "tvc-routines.h"
//#include "tvc-fs.h"
//#include "tvc-usb.h"
//#include "tusb.h"
//#include "psram.h"
//#include "tvc-lfs-flash.h"
//#include "tvc-lfs-psram.h"
//#include "zx7Compress.h"
//#include "hardware/clocks.h"
//#include "lfs.h"
#include <stdbool.h>
#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#define abs(x) ((x) < 0 ? (x) : (-(x)))
#define __unused
// TVC256_FASTRAM_START_SEGMENT 0xE8
#define RAMBASE ((0xE8)<<14)
namespace TVC256 {

uint8_t registerScreenBaseAddr;
uint8_t registerBitmapBaseAddr;
uint8_t registerScreenColorBaseAddr;
TVC64::Memory *emuMem = NULL;

//uint8_t TVC_RAM[];
//uint8_t TVC_ROM[];
uint8_t registerFunctionResult;
uint16_t screenMaxY = 240;
uint8_t *psram_array;
uint8_t videoMode;
uint8_t registerInitializedUSBDevices;
uint8_t registerFunctionBitmapBase;
bool psram_drive_initialized;
uint32_t free_psram_start;

uint8_t screenTextPosX = 0;
uint8_t screenTextPosY = 0;
uint8_t screenTextColor = 0x0f;

#define SELECTED_DEVICE ( TVC_ROM[0x1810] )

uint8_t bitmap_byte_masks[256];

// Fills the text screen area with spaces and the set to current text color
uint8_t clear_text_screen(__unused uint8_t* bufferStart) {
    (void) bufferStart;
    emuMem->memsetRaw(RAMBASE + registerScreenBaseAddr      * 0x0400, 32*screenMaxY/8, 0x20);
    emuMem->memsetRaw(RAMBASE + registerScreenColorBaseAddr * 0x0400, 32*screenMaxY/8, screenTextColor);
/*    memset(&TVC_RAM[registerScreenBaseAddr * 0x0400], 0x20, 32*screenMaxY/8);
    memset(&TVC_RAM[registerScreenColorBaseAddr * 0x0400], screenTextColor, 32*screenMaxY/8);*/
    screenTextPosX = 0;
    screenTextPosY = 0;
    return 0;
}

// Fills the bitmap screen area with the transparent pattern
uint32_t functionBitmapBaseAddr = 0; 
uint8_t clear_bitmap_screen(__unused uint8_t* bufferStart) {
    (void) bufferStart;
    functionBitmapBaseAddr = (registerFunctionBitmapBase == 0xff) ?
            (uint32_t)(registerBitmapBaseAddr & 0x03) * 0x8000 : 
            (uint32_t)(registerFunctionBitmapBase & 0x1f) * 0x8000;
    //memset(&TVC_RAM[functionBitmapBaseAddr], 0x88, 128 * screenMaxY);
    emuMem->memsetRaw(RAMBASE + functionBitmapBaseAddr, 128*screenMaxY, 0x88);
    return 0;
}

// Scrolls the text screen up by one line
uint8_t scrollScreenUp() {
/*    uint32_t screenBaseAddr = (registerScreenBaseAddr & 0x3F) * 0x0400;
    uint32_t sColorBaseAddr = (registerScreenColorBaseAddr & 0x3F) * 0x0400;
    memmove(&TVC_RAM[screenBaseAddr], &TVC_RAM[screenBaseAddr + 32], 32*(screenMaxY/8 - 1));
    memset(&TVC_RAM[screenBaseAddr + 32*(screenMaxY/8 - 1)], 0x20, 32);
    memmove(&TVC_RAM[sColorBaseAddr], &TVC_RAM[sColorBaseAddr + 32], 32*(screenMaxY/8 - 1));
    memset(&TVC_RAM[sColorBaseAddr + 32*(screenMaxY/8 - 1)], screenTextColor, 32);*/
    return 0;
}

void print_char_scr_code(uint8_t ch) {
    uint32_t screenBaseAddr = (registerScreenBaseAddr & 0x3F) * 0x0400;
    uint32_t sColorBaseAddr = (registerScreenColorBaseAddr & 0x3F) * 0x0400;
    uint16_t pos = screenBaseAddr + screenTextPosY*32 + screenTextPosX;
    //TVC_RAM[pos] = ch;
    emuMem->memsetRaw(RAMBASE + pos, 1, ch);
    //TVC_RAM[sColorBaseAddr + screenTextPosY*32 + screenTextPosX] = screenTextColor;
    emuMem->memsetRaw(RAMBASE + sColorBaseAddr + screenTextPosY*32 + screenTextPosX, 1, screenTextColor);
    screenTextPosX++;
    if(screenTextPosX >= 32) {
        screenTextPosX = 0;
        screenTextPosY++;
        if(screenTextPosY >= (screenMaxY / 8)) {
            scrollScreenUp();
            screenTextPosY--;
        }
    }
}

// Prints a character as a screen code at the current text position and advances the cursor
uint8_t print_character_screen_code(uint8_t* bufferStart) {
    print_char_scr_code(*bufferStart);
    return 0;
};

void print_hex_digit(uint8_t digit) {
    uint8_t d;
    if(digit < 10) {
        d = (uint8_t)'0' + digit;
    } else {
        d = 1 + (digit - 10); // screen code for 'A' is 1
    }
    print_character_screen_code(&d);
}

uint8_t print_hex_byte(uint8_t* bufferStart) {
    uint8_t byte = *bufferStart;
    print_hex_digit((byte >> 4) & 0x0f);
    print_hex_digit(byte & 0x0f);
    return 0;
}

uint8_t print_hex_word(uint8_t* bufferStart) {
    uint16_t word = *(uint16_t*)bufferStart;
    print_hex_digit((word >> 12) & 0x0f);
    print_hex_digit((word >> 8) & 0x0f);
    print_hex_digit((word >> 4) & 0x0f);
    print_hex_digit(word & 0x0f);
    return 0;
}

uint8_t print_string_screen_code(uint8_t* bufferStart) {
    while(*bufferStart) {
        print_character_screen_code((uint8_t *)(bufferStart));
        bufferStart++;
    }
    return 0;
}

uint8_t accents[] = 
        {0xf0, 0xfc, 0xe1, 0xf6, 0xf4, 0xe5, 0xf5, 0xee, 0xfb,  // small
         0xeb, 0xf1, 0xe2, 0xf9, 0xf7, 0xe3, 0xf8, 0xf3, 0xf2}; // capital
        // á,    é,    í,    ó,    ö,    ő,    ú,    ü,    ű
uint8_t print_string_ascii(uint8_t* bufferStart) {
    uint8_t *str = bufferStart;
    while(*str) {
        if((*str >= 'A') && (*str <= (']') ) ) {
            print_char_scr_code(*str - 'A' + 1);
        } else if((*str >= 'a') && (*str <= 'z')) {
            print_char_scr_code(*str - 'a' + 0x81);
        } else if((*str >= 32) && (*str <= 64)) {
            print_char_scr_code(*str);
        } else if(*str == '_') {
            print_char_scr_code(0x52);
        } else if(*str == '^') {
            print_char_scr_code(0x1E);                
        } else if(*str == '~') {
            print_char_scr_code(0x1F);
        } else if(*str == 0x0a) { // LF
            screenTextPosY++;
            if(screenTextPosY >= (screenMaxY/8)) {
                scrollScreenUp();
                screenTextPosY--;
            }
        } else if(*str == 0x0d) { // CR
            screenTextPosX = 0;
        } else if(*str == 0x04) {   // cursor right
            screenTextPosX++;
            if(screenTextPosX > 31) {
                screenTextPosX = 0;
                screenTextPosY++;
                if(screenTextPosY >= (screenMaxY / 8)) {
                    screenTextPosY--;
                    screenTextPosX = 31;
                }
            }
        } else if(*str == 0x13) {   // cursor left
            screenTextPosX--;
            if(screenTextPosX == 255) { // Assuming 255 is the maximum value for screenTextPosX
                screenTextPosX = 31;
                screenTextPosY--;
                if(screenTextPosY == 255) {
                    screenTextPosY=0;
                    screenTextPosX=0;
                }
            }
        } else if(*str == 0x05) {   // cursor up
            screenTextPosY--;
            if(screenTextPosY == 255) {
                screenTextPosY = 0;
            }
        } else if(*str == 0x18) {   // cursor down
            screenTextPosY++;
            if(screenTextPosY >= (screenMaxY/8)) {
                screenTextPosY--;
            }
        // } else if(*str == 0x0c) {   // clear screen
        //     clear_text_screen(NULL);
        } else if( ( (*str) >=0x90) && ( (*str) <= 0x98) ) {
            print_char_scr_code(accents[*str - 0x90]);
        } else if( ( (*str) >=0x80) && ( (*str) <= 0x88) ) {
            print_char_scr_code(accents[*str - 0x80 + 9] );
        } else if( ((*str) >= 0xa0 ) && (*str < 0xe0)) {
            print_char_scr_code(*str - 0xa0 + 0x40);
        } else if ( *str >= 0xf0 ) {
            screenTextColor = *str - 0xf0;
        } else {
            print_char_scr_code('?');
        }
        str++;
    }
    return 0;
}

uint8_t set_xy(uint8_t* bufferStart) {
    uint8_t x = bufferStart[0];
    uint8_t y = bufferStart[1];
    if(x!=255) {
        if(x>31) {
            x=31;
        }
        screenTextPosX = x;
    }
    if(y!=255) {
        if(y>29) {
            y=29;
        }
        screenTextPosY = y;
    }
    return 0;
}

uint8_t get_xy(uint8_t* buf) {
    buf[0] = screenTextPosX;
    buf[1] = screenTextPosY;
    return 0;
}

uint8_t set_text_color(uint8_t *color) {
    screenTextColor = *color & 0x0f;
    return 0;
}

uint8_t get_text_color(uint8_t *buf) {
    *buf = screenTextColor;
    return 0;
}


/**
 * Move <16k memory block within one block, overlapping source and target areas are allowed, routine handles it
 * properly.
 * INPUT:
 *   memory block (1): the 16k memory page within the memory must be moved. Can be a paged in slow ram block
 *   destination (2) : destination position, relative to page start, to where the block must be moved (<16k)
 *   source (2)      : source position, relative to page start, from where the block must be moved (<16k)
 *   length (2)      : lengh of memory area to be moved. source+length and destination+length must be less than 16384
 */
uint8_t memory_move_short(uint8_t *buf) {
    uint8_t memoryBlock = buf[0];
    uint16_t destination = *(uint16_t *)&buf[1];
    uint16_t source = *(uint16_t *)&buf[3];
    uint16_t length = *(uint16_t *)&buf[5];
    
    if((source==destination) || (length == 0))
        return 0;

    if(memoryBlock>0x11)
        return 1;

    if(((source + length) >= 16384) || ((destination + length) >= 16384)) {
        return 2;
    }
    //memmove(&TVC_RAM[memoryBlock * 16384 + destination], &TVC_RAM[memoryBlock * 16384 + source], length);
    return 0;
}

/**
 * Move memory block within the full fast RAM area (256k), overlapping source and target areas are allowed, 
 * routine handles it properly.
 * INPUT:
 *   destination (3) : destination position, to where the block must be moved (<256k)
 *   source (3)      : source position, from where the block must be moved (<256k)
 *   length (3)      : lengh of memory area to be moved. source+length and destination+length must be less than 256k
 */
uint8_t memory_move_full(uint8_t *buf) {
    uint32_t destination = *(uint32_t *)&buf[0] & 0x00ffffff;
    uint32_t source = *(uint32_t *)&buf[3] & 0x00ffffff;
    uint32_t length = *(uint32_t *)&buf[6] & 0x00ffffff;
    if(((source + length) >= 256*1024) || ((destination + length) >= 256*1024)) {
        return 1;
    }
    //memmove(&TVC_RAM[destination], &TVC_RAM[source], length);
    return 0;
}

/**
 * Move memory block within the full fast RAM area (256k), overlapping source and target areas are allowed, 
 * routine handles it properly, 
 * INPUT:
 *   destination (3) : destination position, to where the block must be moved (<256k)
 *   source (3)      : source position, from where the block must be moved (<256k)
 *   length (3)      : lengh of memory area to be moved. source+length and destination+length must be less than 256k
 */
uint8_t memory_move_chunks_full(uint8_t *buf) {
    uint32_t destination = *(uint32_t *)&buf[0] & 0x00ffffff;
    uint32_t source = *(uint32_t *)&buf[3] & 0x00ffffff;
    uint32_t length = *(uint32_t *)&buf[6] & 0x00ffffff;
    if(((source + length) >= 256*1024) || ((destination + length) >= 256*1024)) {
        return 1;
    }
    //memmove(&TVC_RAM[destination], &TVC_RAM[source], length);
    return 0;
}

/**
 * Move memory block from the slow (PSRAM, 8MB) area to the fast RAM (256k).
 * INPUT:
 *   destination (3) : destination position to where the block must be moved (<256k)
 *   source (3)      : source position with PSRAM, from where the block must be moved (<8M)
 *   length (3)      : lengh of memory area to be moved. source+length must be less than 8M, 
 *                     destination+length must be less than 256k
 */

uint8_t memory_move_from_slow(uint8_t *buf) {
    uint32_t destination = *(uint32_t *)&buf[0] & 0x00ffffff;
    uint32_t source = *(uint32_t *)&buf[3] & 0x00ffffff;
    uint32_t length = *(uint32_t *)&buf[6] & 0x00ffffff;
    if(((source + length) >= 8192*1024) || ((destination + length) >= 256*1024)) {
        return 1;
    }
    //memcpy(&TVC_RAM[destination], &psram_array[source], length);
    return 0;
}

/**
 * Move memory block to the slow (PSRAM, 8MB) area from the fast RAM (256k).
 * INPUT:
 *   destination (3) : destination position with PSRAM, to where the block must be moved (<8M)
 *   source (3)      : source position from where the block must be moved (<256k)
 *   length (3)      : lengh of memory area to be moved. destination+length must be less than 8M, 
 *                     source+length must be less than 256k
 */
uint8_t memory_move_to_slow(uint8_t *buf) {
    uint32_t destination = *(uint32_t *)&buf[0] & 0x00ffffff;
    uint32_t source = *(uint32_t *)&buf[3] & 0x00ffffff;
    uint32_t length = *(uint32_t *)&buf[6] & 0x00ffffff;
    if(((source + length) >= 256*1024) || ((destination + length) >= 8192*1024)) {
        return 1;
    }
    //memcpy(&TVC_RAM[destination], &psram_array[source], length);
    return 0;
}
/*
DIR routinesDirHandle;
FILINFO routinesFileInfo;
FIL routinesFile;
lfs_file_t routinesLFSFile;
extern lfs_t lfs_psram;
void str_tolower(char *str);
uint8_t transferBuffer[4096];

uint8_t copy_dir_to_psram(uint8_t *bufStart) {
    // usb_printf("cache: psram is %d\n", psram_size());main_loop_task();sleep_ms(50);

    if(psram_size() == 0) {
        return 128;
    }

    if((registerInitializedUSBDevices & 2) == 0) {
        return 129;
    }

    if(SELECTED_DEVICE != 0) {
        return 130;
    }

    uint8_t screenColor = 15;
    set_text_color(&screenColor);
    clear_text_screen(NULL);
    uint8_t videoModeSave = videoMode;
    videoMode = 1;
    print_string_ascii((uint8_t *)"Caching dir: ");
    int res;
    if(bufStart[0] == 0) {
        res = f_opendir(&routinesDirHandle, "./");
        print_string_ascii((uint8_t *)"./\r\n");
    } else {
        uint8_t len = bufStart[0];
        memmove(&bufStart[0], &bufStart[1], len);
        bufStart[len] = 0;

        res = f_opendir(&routinesDirHandle, (char *)bufStart);
        print_string_ascii(bufStart);
        print_string_ascii((uint8_t *)"\r\n");
    }

    uint64_t sizeSum = 0;
    uint8_t numOfFiles = 0;
    while(res == FR_OK) {
        res = f_readdir(&routinesDirHandle, &routinesFileInfo);
        if(res!=FR_OK) {
            f_closedir(&routinesDirHandle);
            print_string_ascii((uint8_t *)"\nread direntry failed\r\n");
            return res;
        } 
        if(routinesFileInfo.fname[0] == 0) {
            f_closedir(&routinesDirHandle);
            print_string_ascii((uint8_t *)"\rScanning directory done\r\n");
            break;
        }
        if((routinesFileInfo.fattrib & AM_DIR) != 0) {
            continue;
        }
        numOfFiles++;
        sizeSum += routinesFileInfo.fsize;
        uint16_t posXY = 255*256;
        set_xy((uint8_t *)&posXY);
        print_hex_byte((uint8_t *)&numOfFiles);
        print_character_screen_code((uint8_t *)" ");
        print_hex_word((uint8_t *)&sizeSum + 2);
        print_hex_word((uint8_t *)&sizeSum);
    }
    uint32_t mappedSize = MIN(MAX(512*1024, 2*sizeSum), psram_size());
    if(mappedSize < sizeSum) {
        print_string_ascii((uint8_t *)"Not enough PSRAM space..\r\n");
        return 131;
    }
     
    print_string_ascii((uint8_t *)"Formatting PSRAM... ");

    uint8_t result = psram_drive_init((mappedSize - 1) / 4096 + 1);
    if( result != 0 ) {
        print_string_ascii((uint8_t *)"FAILEDs\r\n");
        print_hex_byte(&result);
        return 132;
    } else {
        print_string_ascii((uint8_t *)"OK\r\n");
    }

    if(bufStart[0] == 0) {
        res = f_opendir(&routinesDirHandle, "./");
    } else {
        res = f_opendir(&routinesDirHandle, (char *)bufStart);
    }
    if(res!=0) {
        print_string_ascii("2nd opendir failed");
        return res;
    }

    print_string_ascii("Copying files...\r\n");
    int prevLen = 0;
    while(res == FR_OK) {
        res = f_readdir(&routinesDirHandle, &routinesFileInfo);
        if(res!=FR_OK) {
            f_closedir(&routinesDirHandle);
            print_string_ascii((uint8_t *)"\r\nread direntry failed\r\n");
            return res;
        } 
        if(routinesFileInfo.fname[0] == 0) {
            f_closedir(&routinesDirHandle);
            print_string_ascii((uint8_t *)"\rCaching directory done. \r\n");
            TVC_ROM[0x1810] = 1;
            TVC_ROM[0x1824] = 1;
            videoMode = videoModeSave;
            break;
        }
        if((routinesFileInfo.fattrib & AM_DIR) != 0) {
            continue;
        }

        uint16_t posXY = 255*256;
        set_xy((uint8_t *)&posXY);
        print_string_ascii((uint8_t *)(routinesFileInfo.fname));
        int len = strlen(routinesFileInfo.fname);
        int posX = screenTextPosX;
        if(len<prevLen) {
            for(int i=0; i<prevLen - len; i++) {
                print_char_scr_code(' ');
            }
            screenTextPosX = posX;
        }
        prevLen = len;
        char temp[256];
        strcat(temp, "./");
        if(bufStart[0] !=0 ) {
            strcpy(temp, bufStart);
            if(bufStart[len-1] != '/')
                strcat(temp, "/");
        }
        strcat(temp, routinesFileInfo.fname);

        res = f_open(&routinesFile, temp, FA_READ);
        if(res != FR_OK) {
            print_string_ascii((uint8_t *)" failed to open");
            continue;
        }
        str_tolower((char *)routinesFileInfo.fname);
        res = lfs_file_open(&lfs_psram, &routinesLFSFile, (char *)routinesFileInfo.fname, LFS_O_CREAT | LFS_O_WRONLY);
        if(res<0) {
            print_string_ascii((uint8_t *)" failed to create");
            continue;
        }
        unsigned int remainingSize = routinesFileInfo.fsize;
        while(remainingSize>0) {
            unsigned int bytesRead = 0;
            unsigned int bytesToRead = MIN(sizeof(transferBuffer), remainingSize);
            res = f_read(&routinesFile, transferBuffer, bytesToRead, &bytesRead);
            if(res != FR_OK) {
                print_string_ascii((uint8_t *)" failed to read");
                break;
            }
            res = lfs_file_write(&lfs_psram, &routinesLFSFile, transferBuffer, bytesRead);
            if(res < 0) {
                print_string_ascii((uint8_t *)" failed to write");
                break;
            }
            res = FR_OK;

            remainingSize -= bytesRead;
        }
        f_close(&routinesFile);
        lfs_file_close(&lfs_psram, &routinesLFSFile);
    }
    return res;
*/
    /*
    psram_direntry_t *current_psram_direntry = (psram_direntry_t *)psram_array;
    current_psram_direntry->fName[0] = 0;
    
    uint32_t pos = PSRAM_FS_CONTENT_START;
    FRESULT res;
    DIR dirHandle;
    FILINFO fileInfo;
    FIL f;
    uint8_t screenColor = 15;
    set_text_color(&screenColor);
    clear_text_screen(NULL);
    uint8_t videoModeSave = videoMode;
    videoMode = 1;
    print_string_ascii((uint8_t *)"caching dir: ");
    if(bufStart[0] == 0) {
        res = f_opendir(&dirHandle, "./");
        print_string_ascii((uint8_t *)"./\n");
    } else {
        uint8_t len = bufStart[0];
        memmove(&bufStart[1], &bufStart[0], len);
        bufStart[len] = 0;

        res = f_opendir(&dirHandle, (char *)bufStart);
        print_string_ascii(bufStart);
        print_string_ascii((uint8_t *)"\n");
    }

    bool firstFile = true;
    while(res == FR_OK) {
        // usb_printf("cache: psram_de: %p, pos is %06x\n", current_psram_direntry, pos);main_loop_task();sleep_ms(50);
        if(firstFile) {
            firstFile = false;
        }
        res = f_readdir(&dirHandle, &fileInfo);
        if(res!=FR_OK) {
            f_closedir(&dirHandle);
            print_string_ascii((uint8_t *)"read direntry failed\n");
            return res;
        } 
        if(fileInfo.fname[0] == 0) {
            f_closedir(&dirHandle);
            print_string_ascii((uint8_t *)"caching directory done\n");
            videoMode = videoModeSave;
            TVC_ROM[0x1810] = 1;
            TVC_ROM[0x1824] = 1;
            break;
        }
        if((fileInfo.fattrib & AM_DIR) != 0) {
            continue;
        }
        if(strlen(fileInfo.fname)>12) {
            strncpy((char *)current_psram_direntry->fName, fileInfo.altname, 13);
            print_string_ascii((uint8_t *)fileInfo.altname);
        } else {
            strncpy((char *)current_psram_direntry->fName, fileInfo.fname, 13);
            print_string_ascii((uint8_t *)fileInfo.fname);
        }
        print_char_scr_code((uint8_t) ':');
        if((pos+fileInfo.fsize) < psram_size()) {
            current_psram_direntry->fSize = (uint32_t)fileInfo.fsize; // smaller than 4GB for sure
            current_psram_direntry->startPos = pos;
            uint8_t fNameBuf[256];
            fNameBuf[0] = 0;
            if(bufStart[0] == 0) {
                strcpy((char *)fNameBuf, "./");
            } else {
                strcpy((char *)fNameBuf, (char *)bufStart+1);
                strcat((char *)fNameBuf, "/");
            }
            strcat((char *)fNameBuf, fileInfo.fname);
            res = f_open(&f, (char *)fNameBuf, FA_READ);
            if(res != FR_OK) {
                f_closedir(&dirHandle);
                TVC_ROM[0x1824] = 0;
                return res;
            }
            if(fileInfo.fsize>65535) {
                print_hex_word((uint8_t *)&fileInfo.fsize + 2);
                print_hex_word((uint8_t *)&fileInfo.fsize);
            } else {
                print_hex_word( (uint8_t *)&fileInfo.fsize );
            }
            print_string_ascii((uint8_t *)"...        \r");
            UINT br = 0;
            uint32_t all_read = 0;
            do {
                // res = f_read(&f, &TVC_ROM[0x1900], MIN(fileInfo.fsize - all_read, 0x0700), &br);
                res = f_read(&f, &psram_array[pos + all_read], MIN(fileInfo.fsize - all_read, 0x0800), &br);
                if(res==FR_OK) {
                    // print_hex_word((uint8_t *)&all_read);
                    // print_string_ascii((uint8_t *)"\r");
                    // memcpy(&psram_array[pos + all_read], &TVC_ROM[0x1900], br);
                    all_read += br;
                }
            } while((res == FR_OK) && (all_read != fileInfo.fsize) && (br!=0));
            if(res != FR_OK) {
                f_close(&f);
                f_closedir(&dirHandle);
                TVC_ROM[0x1810] = 0;
                TVC_ROM[0x1824] = 0;
                return res;
            } else if(br == 0) {
                // print_string_ascii((uint8_t *)"0 bytes read");
                f_close(&f);
                f_closedir(&dirHandle);
                TVC_ROM[0x1810] = 0;
                TVC_ROM[0x1824] = 0;
                return res;
            }
            f_close(&f);
            pos += fileInfo.fsize;
            current_psram_direntry++;
            current_psram_direntry->fName[0] = 0;

        } else {
            // print_string_ascii((uint8_t *)"doesn't fit!\n");
            continue;
        }
    }
    psram_drive_initialized = true;
    // checkSum = 0;
    // for(int i=0; i<0x1800; i++)
    //     checkSum+=TVC_ROM[i];
    // usb_printf("cache: chkSum is %d\n", checkSum);main_loop_task();sleep_ms(50);
    */
//}

/**
 * Replaces the pixel data (4bits) in the buffer with the color defined in the parameter.
 * INPUT:
 *   start_address (3) : start address of the pixel data to be replaced, relative to the start of the buffer
 *   length (3)        : length of the pixel data to be replaced, must be less than 256k
 *   color_from (1)    : color to be replaced, lower 4 bits, higher 4 bits are ignored
 *   color_to (1)      : color to be used for replacement, lower 4 bits, higher 4 bits are ignored
 * 
 */
uint8_t replace_pixel_color(uint8_t *bufStart) {
    uint32_t start_address = *(uint32_t *)&bufStart[0] & 0x00ffffff;
    uint32_t length = *(uint32_t *)&bufStart[3] & 0x00ffffff;
    uint8_t color_from = bufStart[6] & 0x0f;
    uint8_t color_to = bufStart[7] & 0x0f;
    if((start_address + length) > 256*1024) {
        return 1;
    }
    for(uint32_t i=0; i<length; i++) {
/*        uint8_t pixel = TVC_RAM[start_address + i];
        if((pixel & 0x0f) == color_from) {
            pixel = (pixel & 0xf0) | color_to;
            TVC_RAM[start_address + i] = pixel;
        }
        if((pixel >> 4) == color_from) {
            pixel = (pixel & 0x0f) | (color_to << 4);
            TVC_RAM[start_address + i] = pixel;
        }*/
    }
    return 0;
}

/**
 * - gyakorlatilag minden másoló rutin blokkokat másol
 * - kell fastRam-fastRam és slowRam-fastRam másolás. Az első esetben mozgatás is kell, figyelve az irányra és overlap-ra
 * a) tudni kell folyamatos blokkból, darabokban lerakni 
 *    pl#1 240x4 byte hosszúságú tömbből minden sor végére 4 byte -ot odabiggyeszteni, azaz 4byte másol, sourceAddr+=4, destAddr+=128, ismétel 240x
 * b) tudni kell daraboKAT másolni dabarokra 
 *    pl#1: 4 byte -ot kimásolni, majd a forráscímet és a célcímet megnövelvel 128 byte -al ismét 4 byte -ot, ezt 240-szer
 *    pl#2: 128byte -ot kimásolni, majd a forrás és célcímet megnövelni 1024 -el ismét 128 byte -ot kimásolni, ezt 8-szor)
 * Ezeket tudni kell fastból és slow-ból
 * c) tudni kell blokkoKAT mozgatni
 *    pl#1: mozgatni 124 byte -ot egy soron belül, ahol a sourceAddr és a destAddr 4 byte-al tér el egymástól, 128-al novelni sAddr és dAddr -okat, ismételni 240x
 *    pl#2: mozgatni 128 byte-ot egy sorral följebb (lejjebb), ahol a sA és a dA 128byte-al tér el egymástól. 128-al növelni (csökkenteni) sA és dA -kat, ismételni 240x

 * Ezeket 5 fgv-nyel le lehet fedni (vagy trükközök egy extra flaggel, hogy hol van a source, ezt még kitalálom).
 */

/**
 * Given a continuous memory block. Move it to the destination in chunks, where the num of chunks and size of chunks
 * are defined in the parameters. After each chunk is moved, the destination address is incremented by the defined increment value.
 * Overlapping source and target areas - in fastRam - are allowed, routine handles it properly.
 * IN:
 *  destination (3) : destination position, to where the block must be moved (<256k)
 *  source (3)      : source position, from where the block must be moved. If bit 23 is set, source is in PSRAM (<8MB), otherwise in fast RAM (<256k)
 *  chunkSize(2)    : size of the chunk to be moved in each step, must be less than 65536
 *  count(2)        : number of chunks to be moved, must be less than 65536
 *  increment(2)    : increment to be added to destination addresses after each chunk is moved.
 * RETURN:
 *  0 if success, otherwise error code
 *  1: if any of the parameters are zero (chunkSize, count, increment must be non-zero)
 *  2: if the destination address with chunkSize and count is outside the limits
 *  3: if the source address with chunkSize and count is outside the limits
 */
uint8_t memory_move_chunks_from_block(uint8_t *bufStart) {
    uint32_t destination = *(uint32_t *)&bufStart[0] & 0x00ffffff;
    uint32_t source = *(uint32_t *)&bufStart[3] & 0x00ffffff;
    uint16_t chunkSize = *(uint16_t *)&bufStart[6];
    uint16_t count = *(uint16_t *)&bufStart[8];
    uint16_t increment = *(uint16_t *)&bufStart[10];
    bool sourceInPSRAM = (source & 0x00800000) != 0;
    source &= 0x007fffff;
    if((chunkSize == 0) || (increment == 0) || (count == 0)) {
        return 1;
    }
    if((uint32_t)chunkSize * count + destination > 256*1024) {
        return 2;
    }
    if(sourceInPSRAM) {
        if ((source + (uint32_t)chunkSize * count) > 8192*1024) {
            return 3;
        }
    } else {
        if ( (source + (uint32_t)chunkSize * count) > 256*1024) {
            return 3;
        }
    }

    if(sourceInPSRAM) {
        // source is in PSRAM
        for(int i=0; i<count; i++) {
            //memcpy(&TVC_RAM[destination], &psram_array[source], chunkSize);
            destination += increment;
            source += chunkSize;
        }
    } else {
        // source is in fast RAM
        for(int i=0; i<count; i++) {
            //memmove(&TVC_RAM[destination], &TVC_RAM[source], chunkSize);
            destination += increment;
            source += chunkSize;
        }
    }

    return 0;
}

/**
 * Given a number of blocks in source. Move them to the destination in blocks, where the number of blocks, size of blocks
 * are defined in the parameters. After each block is moved, the source and destination address is incremented by the defined 
 * increment value.
 * Overlapping source and target areas - in fastRam - are allowed, routine handles it properly.
 * IN:
 *  destination (3) : destination position, to where the block must be moved (<256k)
 *  source (3)      : source position, from where the block must be moved. If bit 23 is set, source is in PSRAM (<8MB), otherwise in fast RAM (<256k)
 *  chunkSize(2)    : size of the chunk to be moved in each step, must be less than 65536
 *  count(2)        : number of chunks to be moved, must be less than 65536
 *  increment(2)    : increment to be added to source and destination addresses after each chunk is moved.
 */
uint8_t memory_move_chunks(uint8_t *bufStart) {
    uint32_t destination = *(uint32_t *)&bufStart[0] & 0x00ffffff;
    uint32_t source = *(uint32_t *)&bufStart[3] & 0x00ffffff;
    uint16_t chunkSize = *(uint16_t *)&bufStart[6];
    uint16_t count = *(uint16_t *)&bufStart[8];
    uint16_t increment = *(uint16_t *)&bufStart[10];
    bool sourceInPSRAM = (source & 0x00800000) != 0;
    source &= 0x007fffff;
    if((chunkSize == 0) || (increment == 0) || (count == 0)) {
        return 1;
    }
    if(destination + chunkSize + (count-1)*increment > 256*1024) {
        return 2;
    }
    if(sourceInPSRAM && ((source + chunkSize + (count-1)*increment) > 8192*1024)) {
        return 3;
    }
    if(!sourceInPSRAM && ((source + chunkSize + (count-1)*increment) > 256*1024)) {
        return 3;
    }

    if(sourceInPSRAM) {
        // source is in PSRAM
        for(uint16_t i=0; i<count; i++) {
            //memcpy(&TVC_RAM[destination], &psram_array[source], chunkSize);
            destination += increment;
            source += increment;
        }
    } else {
        // source is in fast RAM
        if((destination < source + chunkSize + (count - 1) * increment) && (destination > source)) {
            // destination is within the source blocks, copy backwards to handle overlap
            destination += (count - 1) * increment;
            source += (count - 1) * increment;
            for(int i=count-1; i>=0; i--) {
                //memmove(&TVC_RAM[destination], &TVC_RAM[source], chunkSize);
                destination -= increment;
                source -= increment;
            }
        } else {
            for(uint16_t i=0; i<count; i++) {
                //memmove(&TVC_RAM[destination], &TVC_RAM[source], chunkSize);
                destination += increment;
                source += increment;
            }
        }
    }
    return 0;
}

uint8_t reverse8BitOrder(uint8_t b) {
    b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
    b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
    b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
    return b;
}

uint8_t reverseNibbles(uint8_t b) {
    b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
    return b;
}

/**
 * Mirrors sprite phases vertically or horizontally
 * INPUT:
 *   phase_address (3): the sprite's phase address in fastRAM
 *   phase_count (1) : how many continous phases to flip
 *   color_mode (1) : the mode of the phase: 0: 2c, non-0: 16c
 *   flip_mode (1) : 0: horizontal, non-0: vertical
 */
uint8_t mirror_sprite_phase(uint8_t *bufStart) {
    uint32_t phase_address = *(uint32_t *)&bufStart[0] & 0x000fffff;
    uint8_t phase_count = bufStart[3];
    uint8_t color_mode = bufStart[4];
    uint8_t flip_mode = bufStart[5];
    if(phase_count == 0)
        return 0;

    if((phase_address + (phase_count+1) * (color_mode?256:64) - 1) >= 256*1024) {
        return 1;
    }

    if (color_mode == 0) {
        // 2c sprite phase
        if(flip_mode == 0) {
            // horizontal flip
            for(int i=0; i<phase_count; i++) {
                for(int j=0; j<21; j++) {
                    uint32_t baseAddress = phase_address + i*64 + j*3;
                    //uint8_t leftByte = reverse8BitOrder(TVC_RAM[baseAddress + 2]);
                    //uint8_t rightByte = reverse8BitOrder(TVC_RAM[baseAddress]);
                    //TVC_RAM[baseAddress + 1] = reverse8BitOrder(TVC_RAM[baseAddress + 1]);
                    //TVC_RAM[baseAddress] = leftByte;
                    //TVC_RAM[baseAddress+2] = rightByte;
                }
            }
        } else {
            // vertical flip
            for(int i=0; i<phase_count; i++) {
                for(int j=0; j<10; j++) {
                    uint32_t topAddress = phase_address + i*64 + j*3;
                    uint32_t bottomAddress = phase_address + i*64 + (20-j)*3;
                    //uint32_t topLine = *(uint32_t *)&TVC_RAM[topAddress] & 0x00ffffff;
                    //uint32_t bottomLine = *(uint32_t *)&TVC_RAM[bottomAddress] & 0x00ffffff;
                    //*(uint32_t *)&TVC_RAM[topAddress] = ((*(uint32_t *)&TVC_RAM[topAddress]) & 0xff000000 ) | bottomLine;
                    //*(uint32_t *)&TVC_RAM[bottomAddress] = ((*(uint32_t *)&TVC_RAM[bottomAddress]) & 0xff000000 ) | topLine;
                }
            }
        }
    } else {
        // 16c sprite phase
        if(flip_mode == 0) {
            // horizontal flip
            for(int i=0; i<phase_count; i++) {
                for(int j=0; j<21; j++) {
                    uint32_t baseAddress = phase_address + i*256 + j*12;
                    for(int k = 0; k<6; k++) {
                        //uint8_t leftByte = reverseNibbles(TVC_RAM[baseAddress + (11-k)]);
                        //uint8_t rightByte = reverseNibbles(TVC_RAM[baseAddress + k]);
                        //TVC_RAM[baseAddress + k] = leftByte;
                        //TVC_RAM[baseAddress + (11-k)] = rightByte;
                    }
                }
            }
        } else {
            // vertical flip
            for(int i=0; i<phase_count; i++) {
                for(int j=0; j<10; j++) {
                    uint32_t topAddress = phase_address + i*256 + j*12;
                    uint32_t bottomAddress = phase_address + i*256 + (20-j)*12;
                    for(int k=0; k<12; k++) {
                        //uint8_t temp = TVC_RAM[topAddress + k];
                        //TVC_RAM[topAddress + k] = TVC_RAM[bottomAddress + k];
                        //TVC_RAM[bottomAddress + k] = temp;
                    }
                }
            }
        }
    }
    return 0;
}

/*
uint8_t zx7Decompress(uint8_t *bufStart) {
    uint32_t dest_offset = *(uint32_t *)&bufStart[0] & 0x00ffffff;
    uint32_t source_offset = *(uint32_t *)&bufStart[3] & 0x00ffffff;

    uint8_t* destAddress;
    uint8_t* sourceAddress;
    if(dest_offset & 0x00800000) {
        destAddress = psram_array + (dest_offset & 0x007fffff);
    } else {
        destAddress = TVC_RAM + dest_offset;
    }

    if(source_offset & 0x00800000) {
        sourceAddress = psram_array + (source_offset & 0x007fffff);
    } else {
        sourceAddress = TVC_RAM + source_offset;
    }
    // long startTime = time_us_64();
    uint32_t decompressSize = decompress((unsigned char *)sourceAddress, (unsigned char *)destAddress);

    *(uint32_t *)&bufStart[9] = decompressSize;

    // uint64_t endTime = time_us_64();
    // uint32_t elapsedTime = (uint32_t)(endTime - startTime);
    // usb_printf("zx7 elapsed time: %lu us, size: %04x\n", elapsedTime, decompressSize);

    return 0;
}
*/
uint8_t penColor = 0;
uint8_t set_pen_color(uint8_t *color) {
    penColor = *color & 0x0f;
    return 0;
}

uint8_t get_pen_color(uint8_t *color) {
    *color = penColor;
    return 0;
}

void set_dotc_impl(uint8_t x, uint8_t y, uint8_t color) {
    // draw pixel at (x, y) with the given color
    uint32_t offset = functionBitmapBaseAddr + y * 128 + (x >> 1);
    if(x & 1) {
        // odd pixel, color is in lower 4 bits
        //TVC_RAM[offset] = (TVC_RAM[offset] & 0xf0) | color;
    } else {
        // even pixel, color is in higher 4 bits
        //TVC_RAM[offset] = (TVC_RAM[offset] & 0x0f) | (color << 4);
    }
}



uint8_t set_dot_color(uint8_t *bufStart) {
    uint8_t x = bufStart[0];
    uint8_t y = bufStart[1];
    if(y>=screenMaxY) {
        return 1;
    }

/*    functionBitmapBaseAddr = (registerFunctionBitmapBase == 0xff) ?
            (registerBitmapBaseAddr & 0x03) * 0x8000 : 
            (registerFunctionBitmapBase & 0x1f) * 0x8000;*/

    set_dotc_impl(x, y, penColor);
    return 0;
}

uint8_t get_dot_color_impl(uint8_t x, uint8_t y) {
    //uint32_t offset = functionBitmapBaseAddr + y * 128 + (x >> 1);
    uint8_t retVal = 0;
    /*if(x & 1) {
        // odd pixel, color is in lower 4 bits
        retVal = TVC_RAM[offset] & 0x0f;
    } else {
        // even pixel, color is in higher 4 bits
        retVal = (TVC_RAM[offset] >> 4) & 0x0f;
    }*/
    return retVal;
}

uint8_t get_dot_color(uint8_t *bufStart) {
    uint8_t x = bufStart[0];
    uint8_t y = bufStart[1];
    if(y>=screenMaxY) {
        return 1;
    }
    /*functionBitmapBaseAddr = (registerFunctionBitmapBase == 0xff) ?
            (registerBitmapBaseAddr & 0x03) * 0x8000 : 
            (registerFunctionBitmapBase & 0x1f) * 0x8000;*/

    bufStart[0] = get_dot_color_impl(x, y);
    return 0;
}

void draw_line_impl(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint8_t color) {
    // Bresenham's line algorithm
    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;
    int err = dx - dy;

    while (true) {
        // draw pixel at (x1, y1) with the given color
        if((x1>=0) && (x1<256) && (y1>=0) && (y1<screenMaxY)) {
            set_dotc_impl(x1, y1, color);
        }

        if ((x1 == x2) && (y1 == y2))
            break;

        int err2 = err * 2;
        if (err2 > -dy) {
            err -= dy;
            x1 += sx;
        }
        if (err2 < dx) {
            err += dx;
            y1 += sy;
        }
    }
}

uint8_t draw_line(uint8_t *bufStart) {
    int16_t x1 = *(int16_t *)&bufStart[0];
    int16_t y1 = *(int16_t *)&bufStart[2];
    int16_t x2 = *(int16_t *)&bufStart[4];
    int16_t y2 = *(int16_t *)&bufStart[6];

    functionBitmapBaseAddr = (registerFunctionBitmapBase == 0xff) ?
            (registerBitmapBaseAddr & 0x03) * 0x8000 : 
            (registerFunctionBitmapBase & 0x1f) * 0x8000;
    
    draw_line_impl(x1, y1, x2, y2, penColor);
    
    return 0;
}

uint8_t draw_rectangle(uint8_t *bufStart) {
    int16_t x = *(int16_t *)&bufStart[0];
    int16_t y = *(int16_t *)&bufStart[2];
    uint8_t w = bufStart[4];
    uint8_t h = bufStart[5];

    functionBitmapBaseAddr = (registerFunctionBitmapBase == 0xff) ?
            (registerBitmapBaseAddr & 0x03) * 0x8000 : 
            (registerFunctionBitmapBase & 0x1f) * 0x8000;
    
    draw_line_impl(x, y, x+w, y, penColor);
    draw_line_impl(x+w, y, x+w, y+h, penColor);
    draw_line_impl(x+w, y+h, x, y+h, penColor);
    draw_line_impl(x, y+h, x, y, penColor);
    
    return 0;
}

uint8_t fill_rectangle(uint8_t *bufStart) {
    int16_t x = *(int16_t *)&bufStart[0];
    int16_t y = *(int16_t *)&bufStart[2];
    uint8_t w = bufStart[4];
    uint8_t h = bufStart[5];

    functionBitmapBaseAddr = (registerFunctionBitmapBase == 0xff) ?
            (registerBitmapBaseAddr & 0x03) * 0x8000 : 
            (registerFunctionBitmapBase & 0x1f) * 0x8000;

    for(uint8_t i=y; i<y+h; i++) {
        draw_line_impl(x, i, x+w, i, penColor);
    }
    
    return 0;
}

uint8_t draw_multi_line(uint8_t *bufStart) {
    uint8_t pointCount = bufStart[0];
    if(pointCount<2) {
        return 1;
    }

    functionBitmapBaseAddr = (registerFunctionBitmapBase == 0xff) ?
            (registerBitmapBaseAddr & 0x03) * 0x8000 : 
            (registerFunctionBitmapBase & 0x1f) * 0x8000;

    for(int i=0; i<pointCount-1; i++) {
        int16_t x1 = *(int16_t *)&bufStart[1 + i*4];
        int16_t y1 = *(int16_t *)&bufStart[3 + i*4];
        int16_t x2 = *(int16_t *)&bufStart[5 + i*4];
        int16_t y2 = *(int16_t *)&bufStart[7 + i*4];
        draw_line_impl(x1, y1, x2, y2, penColor);
    }
    return 0;
}

// A verem (stack) egy elemét leíró struktúra
typedef struct {
    uint8_t x;
    uint8_t y;
} Point;

Point stack[1024];

uint8_t scanline_flood_fill(uint8_t *bufStart) {
    // long startTime = time_us_64();
     
    uint8_t startX = bufStart[0];
    uint8_t startY = bufStart[1];
    uint8_t new_color = penColor;
    if(startY>=screenMaxY)
        return 1;

    functionBitmapBaseAddr = (registerFunctionBitmapBase == 0xff) ?
            (registerBitmapBaseAddr & 0x03) * 0x8000 : 
            (registerFunctionBitmapBase & 0x1f) * 0x8000;

    uint8_t target_color = get_dot_color_impl(startX, startY);
    
    // Ha a kitöltendő szín megegyezik az új színnel, nincs dolgunk
    if (target_color == new_color) 
        return 0;

    // Memória-optimalizálás: A verem maximális szükséges mérete ritkán haladja meg a kép magasságát.
    // Biztonsági okokból a magasság négyszeresét foglaljuk le, ami elhanyagolható méret (pár kilobájt).
    int stack_capacity = 1024;

    int stack_pointer = 0;

    // Első pont berakása a verembe
    stack[stack_pointer++] = (Point){startX, startY};

    // Fő ciklus
    while (stack_pointer > 0) {
        // Pont kivétele a veremből
        Point p = stack[--stack_pointer];
        int x = p.x;
        int y = p.y;

        // Elindulunk balra, amíg a cél színt találjuk
        int leftX = x;
        while ((leftX >= 0) && (get_dot_color_impl(leftX, y) == target_color)) {
            leftX--;
        }
        leftX++; // Visszalépés az utolsó érvényes pixelre

        // Elindulunk jobbra, amíg a cél színt találjuk
        int rightX = x;
        while ((rightX < 256) && (get_dot_color_impl(rightX, y) == target_color)) {
            rightX++;
        }
        rightX--; // Visszalépés az utolsó érvényes pixelre

        // A megtalált vízszintes vonalszakasz kiszínezése
        for (int i = leftX; i <= rightX; i++) {
            set_dotc_impl(i, y, new_color);
        }

        // Felső sor ellenőrzése (y - 1)
        if (y > 0) {
            bool in_segment = false;
            for (int i = leftX; i <= rightX; i++) {
                if (get_dot_color_impl(i, y - 1) == target_color) {
                    if (!in_segment) {
                        // Ha megtelt a verem (extrém eset), menet közben növeljük a méretét
                        if (stack_pointer >= stack_capacity) {
                            return 2;
                        }
                        stack[stack_pointer++] = (Point){i, y - 1};
                        in_segment = true;
                    }
                } else {
                    in_segment = false;
                }
            }
        }

        // Alsó sor ellenőrzése (y + 1)
        if (y < screenMaxY) {
            bool in_segment = false;
            for (int i = leftX; i <= rightX; i++) {
                if (get_dot_color_impl(i, y + 1) == target_color) {
                    if (!in_segment) {
                        if (stack_pointer >= stack_capacity) {
                            return 2;
                        }
                        stack[stack_pointer++] = (Point){i, y + 1};
                        in_segment = true;
                    }
                } else {
                    in_segment = false;
                }
            }
        }
    }
    // uint64_t endTime = time_us_64();
    // uint32_t elapsedTime = (uint32_t)(endTime - startTime);
    // usb_printf("Flood fill elapsed time: %lu us\n", elapsedTime);
    return 0;
}

void draw_ellipse_pixels(int16_t xc, int16_t yc, uint8_t x, uint8_t y, uint8_t color) {
    int16_t xx = xc + x;
    int16_t yy = yc + y;
    // if(((xc + x) < 256 ) && ((yc + y) < maxY)) {
    if((xx >= 0) && (xx < 256 ) && (yy>=0) && ((yy) < screenMaxY)) {        
        set_dotc_impl(xx, yy, color); // 1. negyed
    }

    xx = xc - x;
    if((xx >= 0) && (xx < 256) && (yy>=0) && ((yy) < screenMaxY)) {        
        set_dotc_impl(xx, yy, color); // 1. negyed
    }

    xx = xc + x;
    yy = yc - y;
    if((xx >= 0) && (xx < 256) && (yy>=0) && ((yy) < screenMaxY)) {        
        set_dotc_impl(xx, yy, color); // 1. negyed
    }

    xx = xc - x;
    if((xx >= 0) && (xx < 256) && (yy>=0) && ((yy) < screenMaxY)) {        
        set_dotc_impl(xx, yy, color); // 1. negyed
    }
}

void fill_ellipse_pixels(int16_t xc, int16_t yc, uint8_t x, uint8_t y, uint8_t color) {

    if((xc - x > 256) || (xc + x < 0) || (yc - y > screenMaxY) || (yc + y < 0) )
        return;
    int16_t leftX = (xc - x) > 0 ? (xc - x) : 0;
    int16_t rightX = (xc + x < 256) ? (xc + x) : 255;
    int16_t topY = (yc >= y) ? (yc - y) : 0;
    int16_t bottomY = (yc + y < screenMaxY) ? (yc + y) : (screenMaxY - 1);
    draw_line_impl(leftX, topY, rightX, topY, color);
    draw_line_impl(leftX, bottomY, rightX, bottomY, color);
}


// Bresenham-féle élsimítás mentes ellipszis rajzoló
uint8_t ellipse(uint8_t *bufStart, bool fill) {
    int16_t xc = *(int16_t *)&bufStart[0];
    int16_t yc = *(int16_t *)&bufStart[2];
    uint8_t a = bufStart[4];
    uint8_t b = bufStart[5];

    uint8_t x = 0;
    int32_t y = b;

    functionBitmapBaseAddr = (registerFunctionBitmapBase == 0xff) ?
            (registerBitmapBaseAddr & 0x03) * 0x8000 : 
            (registerFunctionBitmapBase & 0x1f) * 0x8000;
    
    // Előre kiszámolt négyzetek a túlcsordulás megelőzésére (long long)
    uint32_t a2 = (uint32_t)a * a;
    uint32_t b2 = (uint32_t)b * b;
    uint32_t two_a2 = 2 * a2;
    uint32_t two_b2 = 2 * b2;
    
    int32_t dx = two_b2 * x;
    int32_t dy = two_a2 * y;
    
    // 1. FÁZIS: Ahol az ív meredeksége < 1 (vízszintesebb szakasz)
    int32_t p = b2 - (a2 * b) + (a2/4); // Döntési paraméter kezdeti értéke
    
    while (dx < dy) {
        if(fill) {
            fill_ellipse_pixels(xc, yc, x, (uint8_t)y, penColor);
        } else {
            draw_ellipse_pixels(xc, yc, x, y, penColor);
        }
        
        x++;
        dx += two_b2;
        
        if (p < 0) {
            p += b2 + dx;
        } else {
            y--;
            dy -= two_a2;
            p += b2 + dx - dy;
        }
    }
    
    // 2. FÁZIS: Ahol az ív meredeksége >= 1 (függőlegesebb szakasz)
    p = b2 * (x * x + x) + b2 / 4 + a2 * (y - 1) * (y - 1) - a2 * b2;
    
    while (y >= 0) {
        if(fill) {
            fill_ellipse_pixels(xc, yc, x, (uint8_t)y, penColor);
        } else {
            draw_ellipse_pixels(xc, yc, x, (uint8_t)y, penColor);
        }    
        
        y--;
        dy -= two_a2;
        
        if (p > 0) {
            p += a2 - dy;
        } else {
            x++;
            dx += two_b2;
            p += a2 - dy + dx;
        }
    }
    return 0;
}

uint8_t draw_ellipse(uint8_t *bufStart) {
    return ellipse(bufStart, false);
}

uint8_t fill_ellipse(uint8_t *bufStart) {
    return ellipse(bufStart, true);
}

/**
 * Copies an image block from fastRAM to the selected bitmap. The image block is defined in
 * fastRAM as a byte block. The size of the block is ((width-1)/2 + 1) * height bytes. The transparent pixels
 * leaves the destination pixel untouched.
 * This is a sub-condition of the  copy_sub_image  function.
 * INPUT:
 *   destX  (2): X coordinate of the destination in the bitmap. Signed integer, only visible part is copied
 *   dextY  (2): Y coordinate of the destination in the bitmap. Signed integer, only visible part is copied
 *   width  (1): width of the image to be copied
 *   height (1): height of the image to be copied
 *   srcAdd (3): source address from fastRam from the image is copied from. 
 * RETURN:
 *   0: if the image is copied
 *   1: if width or height is 0
 */
uint8_t copy_image_block(uint8_t *bufStart) {
    int16_t destX = *(int16_t *)&bufStart[0];
    int16_t destY = *(int16_t *)&bufStart[2];
    uint8_t width = bufStart[4];
    uint8_t height = bufStart[5];
    uint32_t sourceAddress = *(uint32_t *)&bufStart[6] & 0x00ffffff;
    if((width==0) || (height == 0))
        return 0;

/*    uint8_t *source_array = sourceAddress & 0x00800000 ? 
                            psram_array : 
                            TVC_RAM;

    sourceAddress &= 0x007FFFFF;

    functionBitmapBaseAddr = (registerFunctionBitmapBase == 0xff) ?
            (registerBitmapBaseAddr & 0x03) * 0x8000 : 
            (registerFunctionBitmapBase & 0x1f) * 0x8000;

    uint8_t wmultiplier = ((width - 1) >> 1) + 1;
    uint8_t color;
    for(int y = MAX(0, -1 * destY); y < height; y++) {

        if((destY + y) >= screenMaxY)
            break;

        uint32_t offset = sourceAddress + y * wmultiplier ;
        for(int x = MAX(0, -1 * destX); x < width; x++) {

            if((destX + x) >= 256)
                break;

            if(x & 1) {
                // odd pixel, color is in lower 4 bits
                color = source_array[offset + (x>>1)] & 0x0f;
            } else {
                // even pixel, color is in higher 4 bits
                color = (source_array[offset + (x>>1)] >> 4) & 0x0f;
            }
            if(color == 0x08) {
                continue; // skip transparent color
            }

            set_dotc_impl(destX + x, destY + y, color);
        }
    }*/
    return 0;
}

void init_bitmap_byte_masks() {
    for(int i=0; i<256; i++) {
        bitmap_byte_masks[i] = 0xff;
        
        if((i & 0x0f) == 0x08)
            bitmap_byte_masks[i] &= 0xf0;

        if((i & 0xf0) == 0x80)
            bitmap_byte_masks[i] &= 0x0f;
    }
}

uint8_t copy_image_block_fast(uint8_t *bufStart) {
    int16_t destX = *(int16_t *)&bufStart[0];
    int16_t destY = *(int16_t *)&bufStart[2];
    uint8_t width = bufStart[4];
    uint8_t height = bufStart[5];
    uint32_t sourceAddress = *(uint32_t *)&bufStart[6] & 0x00ffffff;
    
    if((width==0) || (height == 0))
        return 0;

    int x0 = MAX(0, destX);
    if(((destX + width) < 0) || (destX > 255))
        return 0;
    
    int y = MAX(0, destY);
    if(((destY + height) < 0) || (destY >= screenMaxY))
        return 0;

/*    uint8_t *source_array = sourceAddress & 0x00800000 ? 
                            psram_array : 
                            TVC_RAM;

    sourceAddress &= 0x007FFFFF;

    functionBitmapBaseAddr = (registerFunctionBitmapBase == 0xff) ?
            (registerBitmapBaseAddr & 0x03) * 0x8000 : 
            (registerFunctionBitmapBase & 0x1f) * 0x8000;

    uint8_t srcLineWidthInBytes = ((width - 1) >> 1) + 1;
    int skippedLines = -1 * MIN(0, destY);
    uint32_t sourceOffset = sourceAddress + skippedLines * srcLineWidthInBytes;
    uint32_t targetOffset = functionBitmapBaseAddr + y * 128;

    int skippedPixels = -1 * MIN(0, destX);
    uint32_t sourceXOffset = skippedPixels >> 1;
    uint32_t targetXOffset = x0 >> 1;
    uint8_t *s0 = &source_array[sourceOffset + sourceXOffset];  // denotes the byte addr of the first line of the src data
    uint8_t *t0 = &TVC_RAM[targetOffset + targetXOffset];
    
    for(; (y < (destY + height)) && (y < screenMaxY); y++, s0 += srcLineWidthInBytes, t0 += 128) {
        int x = x0;
        uint8_t srcData = 0x00;
        uint8_t dstData = 0x00;
        uint8_t *s = s0;
        uint8_t *t = t0;
        
        if((x & 1) == (skippedPixels & 1)) {
            if(x & 1) {
                srcData = (*s & 0x0f) | 0x80;
                dstData = *t;
                *t = (dstData & ~bitmap_byte_masks[srcData]) | (srcData & bitmap_byte_masks[srcData]);
                s++;
                t++;
                x++;
            }
            for(; (x < (destX + width)) && (x < 256); x+=2, s++, t++) {
                *t = (*t & ~bitmap_byte_masks[*s]) | (*s & bitmap_byte_masks[*s]);
                // *t = *s;
            }
        } else {
            if(x & 1) {
                srcData = (*s >> 4) | 0x80;
            }
            for(; (x < 256) && (x < (destX + width)); x++) {
                if(x & 1) {
                    *t = (*t & ~bitmap_byte_masks[srcData]) | (srcData & bitmap_byte_masks[srcData]);
                    // *t = srcData;
                    t++;
                } else {
                    srcData = *s << 4;
                    s++;
                    srcData |= (*s >> 4);
                }
            }
            if(x & 1) {
                srcData = (srcData & 0xf0) | 0x08;
                *t = (*t & ~bitmap_byte_masks[srcData]) | (srcData & bitmap_byte_masks[srcData]);
            }
        }
    }*/
    return 0;   
}



uint8_t create_psram_drive(uint8_t *bufStart) {
    uint16_t num_of_blocks = *(uint16_t *)&bufStart;
    int res = 0 /*psram_drive_init(num_of_blocks)*/;
    return res >= 0 ? 0 : res;
}

uint8_t get_first_usable_psram_pos(uint8_t *bufStart) {
    uint32_t value = (*(uint32_t *)&bufStart & 0xff000000) | (free_psram_start & 0x00ffffff);
    *(uint32_t *)&bufStart = value;
    return 0;
}

uint8_t delete_psram_drive(__unused uint8_t *bufStart) {
    if(psram_drive_initialized) {
        int len = 0/*psram_size() * 1024 * 1024 - free_psram_start*/;
        memset(psram_array, 0xff, len);
        psram_drive_initialized = false;
        free_psram_start = 0;
    }
    
    return 0;
}


/**
 * Copies a sub-image from a larger image to the selected screen.
 * INPUT:
 *   destX (2): integer, the destination X coord of the sub-image. Only the visible pixels are copied.
 *   destY (2): integer, the destination Y coord of the sub-image. Only the visible pixels are copied.
 *   dWidth(1): positive number, the width of the sub-image
 *   dHeight(1):positive number, the height of the sub-image
 *   srcX  (2): integer, the source X coord of the original image, copied from. Can be negative,
 *              the missing pixels will be the lower 4 bits of the defColor.
 *   srcY  (2): integer, the source Y coord of the original image, copied from
 *   sWidth(2): the width of the source image
 *   sHeight(2):the height of the source image
 *   srcAddr(3):the source address of the image in memory. The highest bit selects PSRAM (1) or fastRAM (0).
 *   defColor(1): (lower 4 bits) the default color used, when the source pixel is out of the source image boundaries.
 *                               If there is no copy out of the source image boundaries, then not used at all.
 *                (higher 4 bits) when non-zero then the 0x08 colored pixels are skipped (transparent) or 
 *                                simply overwrite the original pixel.
 */
uint8_t copy_sub_image(uint8_t *bufStart) {
    int16_t dstX = *(int16_t *)&bufStart[0];
    int16_t dstY = *(int16_t *)&bufStart[2];
    uint8_t dWidth = bufStart[4];
    uint8_t dHeight = bufStart[5];
    int16_t srcX = *(int16_t *)&bufStart[6];
    int16_t srcY = *(int16_t *)&bufStart[8];
    uint16_t sWidth = *(int16_t *)&bufStart[10];
    uint16_t sHeight = *(int16_t *)&bufStart[12];
    uint32_t sourceAddress = *(uint32_t *)&bufStart[14] & 0x00ffffff;
    uint8_t defColor = bufStart[17] & 0x0f;
    uint8_t transparent = bufStart[17] & 0xf0;
    if((sWidth==0) || (sHeight == 0) || (dWidth == 0) || (dHeight == 0))
        return 0;

    functionBitmapBaseAddr = (registerFunctionBitmapBase == 0xff) ?
            (registerBitmapBaseAddr & 0x03) * 0x8000 : 
            (registerFunctionBitmapBase & 0x1f) * 0x8000;

/*    uint8_t *source_array = sourceAddress & 0x00800000 ? 
                            psram_array : 
                            TVC_RAM;

    sourceAddress &= 0x007FFFFF;

    uint8_t swMultiplier = ((sWidth - 1) >> 1) + 1;
    
    uint8_t color;
    int sy = srcY;
    uint32_t offset = 0;
    for(int dy = 0; dy < dHeight; dy++, sy++) {
        if((dstY+dy)<0) {
            continue;
        }
        if((dstY + dy) >= screenMaxY) {
            break;
        }

        int sx = srcX;
        if((sy>=0 ) && (sy<sHeight))
            offset = sourceAddress + sy * swMultiplier ;

        for(int dx = 0; dx < dWidth; dx++, sx++) {
            if((dstX+dx)<0) {
                continue;
            }
            if((dstX + dx) > 255) {
                break;
            }
            if((sy<0) || (sy>=sHeight) || (sx<0) || (sx>=sWidth)) {
                color = defColor;
            } else {
                if(sx & 1) {
                    // odd pixel, color is in lower 4 bits
                    color = source_array[offset + (sx >> 1)] & 0x0f;
                } else {
                    // even pixel, color is in higher 4 bits
                    color = (source_array[offset + (sx >> 1)] >> 4) & 0x0f;
                }
            }
            if(!(transparent && (color == 0x08)))
                set_dotc_impl(dstX + dx, dstY + dy, color);
        }
    }*/
    return 0;
}
/**
 * File related operations on the selected drive
 */


/**
 * Opens a file. Returns the ff.h defined error codes. FF_OK is 0, otherwise error. 
 * IN:
 *  [0]/1 - open mode (see ff.h)
 *  [1]/1 - file type (0x01 - buffered, 0x11 - unbuffered)
 *  [2]/1 - length of fileName
 *  [3]/len - fileName
 * OUT:
 *  [0]/4 - fileHandle
 *  [4]/4 - file size
 *  [8]/1 - found filename length
 *  [9]/len - fileName
 */
/*
uint8_t tvcfunc_open_file(uint8_t* bufferStart) {
    TVC_ROM[0x1900] = bufferStart[0];   // open file mode
    TVC_ROM[0x1901] = bufferStart[1];   // file type, only for creating/rewriting
    uint8_t len = bufferStart[2];       // filename length
    if(len == 0) {
        return (uint8_t)FR_INVALID_PARAMETER; // Invalid filename length
    }
    // usb_printf("tvcfunc_open_file: filename length: %d, filename: %s, flag: %d\n", len, &bufferStart[3], SELECTED_DEVICE );main_loop_task();sleep_ms(50);
    memcpy(&TVC_ROM[0x1902], &bufferStart[2], (uint16_t)len + 1);
    FRESULT res = 0;
    switch(SELECTED_DEVICE) {
        case 0x00:  // USB
            res = tvc_open_file();
            break;
        case 0x01:   // psram
            res = tvc_lfs_psram_file_open();
            break;
        case 0x02:    // internal flash drive
            res = tvc_lfs_flash_file_open();
            break;
        default:
            res = 3;
            break;
    }

    bufferStart[0] = (uint8_t)res;
    if(res == FR_OK) {
        // On success, store the file handle pointer back to bufferStart
        *(uint32_t *)&bufferStart[0] = *(uint32_t *)&TVC_ROM[0x1a00];
        *(uint32_t *)&bufferStart[4] = *(uint32_t *)&TVC_ROM[0x1a04];
        bufferStart[8] = TVC_ROM[0x1a08];
        memcpy(&bufferStart[9], &TVC_ROM[0x1a09], bufferStart[8]);
    }
    return (uint8_t)res;
}

uint8_t tvcfunc_close_file(uint8_t* bufferStart) {
    *(uint32_t *)&TVC_ROM[0x1900] = *(uint32_t *)&bufferStart[0];
    FRESULT res;


    switch(SELECTED_DEVICE) {
        case 0x00:
            res = tvc_close_file();
            break;
        case 0x01:   // psram
            res = tvc_lfs_psram_file_close();
            break;
        case 0x02:    // internal flash drive
            res = tvc_lfs_flash_file_close();
            break;
        default:
            res = 3;
            break;
    }
    
    return (uint8_t)res;
}

/**
 * Reaf file into INPUT buffer
 * fileHandle(4): the open file handle
 * lengh(2): the length of data to be readf
 */
/*
uint8_t tvcfunc_read_file(uint8_t* bufferStart) {
    *(uint32_t *)&TVC_ROM[0x1900] = *(uint32_t *)&bufferStart[0];
    *(uint32_t *)&TVC_ROM[0x1904] = *(uint16_t *)&bufferStart[4];
    
    FRESULT res;
    switch(SELECTED_DEVICE) {
        case 0x00:
            res = tvc_read_file();
            break;
        case 0x01:   // psram
            res = tvc_lfs_psram_file_read();
            break;
        case 0x02:    // internal flash drive
            res = tvc_lfs_flash_file_read();
            break;
        default:
            res = 3;
            break;
    }

    // if(TVC_ROM[0x1810]) {
    //     res = psram_read_file();
    // } else {
    //     res = tvc_read_file();
    // }
    if(res == FR_OK) {
        // On success, store the number of bytes read back to bufferStart
        uint16_t readBytes = *(uint16_t *)&TVC_ROM[0x1a00];
        *(uint16_t *)&bufferStart[4] = readBytes;
        if(readBytes != 0) {
            memcpy(&bufferStart[6], &TVC_ROM[0x1a02], readBytes);
        }
    }
    return (uint8_t)res;
}

uint8_t tvcfunc_read_file_dest(uint8_t* bufferStart) {
    *(uint32_t *)&TVC_ROM[0x1900] = *(uint32_t *)&bufferStart[0];
    *(uint32_t *)&TVC_ROM[0x1904] = (*(uint32_t *)&bufferStart[4]) & 0x00ffffff;
        // size
    *(uint32_t *)&TVC_ROM[0x1907] = (*(uint32_t *)&bufferStart[7]) & 0x00ffffff;    // fast ram destination (0-256k)
    
    // usb_printf("tvcfunc_read_file_dest: size: %d, dest: %06x\n", (*(uint32_t *)&TVC_ROM[0x1904]) & 0x00ffffff, (*(uint32_t *)&TVC_ROM[0x1907]) & 0x00ffffff);main_loop_task();sleep_ms(50);

    FRESULT res = 0xff;
    switch(SELECTED_DEVICE) {
        case 0:
            res = tvc_read_file_dest();
            break;
        case 1:
            res = tvc_lfs_psram_file_read_dest();
            break;
        case 2:
            res = tvc_lfs_flash_file_read_dest();
            break;
    }
    // usb_printf("tvcfunc_read_file_dest: after read, res: %d\n", res);main_loop_task();sleep_ms(50);

    if(res == FR_OK) {
        // On success, store the number of bytes read back to bufferStart, 4 bytes!
        memcpy(&bufferStart[4], &TVC_ROM[0x1a00], 3);
    }
    return (uint8_t)res;
}
*/
/*
 * Write bytes from routing parameter area 
 * IN:
 *  bufferStart[0]: fileHandle (4 bytes)
 *  bufferStart[4]: numOfBytesToWrite (2 bytes)
 *  bufferStart[6]: bytesToWrite (numOfBytesToWrite bytes)
 * OUT:
 *  bufferStart[0]: fileHandle (4 bytes)
 *  bufferStart[4]: numOfBytesWritten (2 bytes)
*/
/*
uint8_t  tvcfunc_write_file(uint8_t* bufferStart) {
    *(uint32_t *)&TVC_ROM[0x1900] = *(uint32_t *)&bufferStart[0];
    uint16_t length = (*(uint16_t *)&bufferStart[4]);
    *(uint16_t *)&TVC_ROM[0x1904] = length;
    *(uint32_t *)&TVC_ROM[0x1906] = (uint32_t)&bufferStart[6];
    if((length!=0) && (((uint32_t)length + 0x1a00) <= 0x2000)) {
        memcpy(&bufferStart[6], &TVC_ROM[0x1a00], length);
    } else {
        return FR_INVALID_PARAMETER;
    }

    FRESULT res = 0xFF;
    switch(SELECTED_DEVICE) {
        case 0x00:
            res = tvc_write_file();
            break;
        case 0x01:
            res = tvc_lfs_psram_file_write();
            break;
        case 0x02:    // internal flash drive
            res = tvc_lfs_flash_file_write();
            break;
        default:
            res = 3;
            break;
    }
    
    if(res == FR_OK) {
        // On success, store the number of bytes stored to bufferStart
        *(uint16_t *)&bufferStart[4] = *(uint16_t *)&TVC_ROM[0x1a00];
    }
    return (uint8_t)res;
}
*/
/*
 * Write bytes to file from fast RAM memory area
 * IN:
 *  bufferStart[0]: fileHandle (4 bytes)
 *  bufferStart[4]: numOfBytesToWrite (3 bytes)
 *  bufferStart[7]: index in TVC_RAM -> write byte
 * OUT:
 *  bufferStart[0]: fileHandle (4 bytes)
 *  bufferStart[4]: numOfBytesWritten (3 bytes)
*/
/*
uint8_t tvcfunc_write_file_source(uint8_t* bufferStart) {
    *(uint32_t *)&TVC_ROM[0x1900] = *(uint32_t *)&bufferStart[0];
    *(uint32_t *)&TVC_ROM[0x1904] = (*(uint32_t *)&bufferStart[4]) & 0x00ffffff;
    *(uint32_t *)&TVC_ROM[0x1907] = (*(uint32_t *)&bufferStart[7]) & 0x00ffffff;

    FRESULT res = 0;
    switch(SELECTED_DEVICE) {
        case 0x00:
            res = tvc_write_file_source();
            break;
        case 0x01:
            res = tvc_lfs_psram_file_write_source();
            break;
        case 0x02:
            res = tvc_lfs_flash_file_write_source();
            break;
    }
    if(res == FR_OK) {
        // On success, store the number of bytes stored to bufferStart
        memcpy(&bufferStart[4], &TVC_ROM[0x1a00], 3);
    }
    return (uint8_t)res;
}

uint8_t tvcfunc_open_dir(uint8_t* bufferStart) {
    uint8_t len = bufferStart[0];
    if(len == 0) {
        return (uint8_t)FR_INVALID_PARAMETER; // Invalid filename length
    }
    memcpy(&TVC_ROM[0x1900], &bufferStart[0], len+1);
    FRESULT res  = 0;
    switch(SELECTED_DEVICE) {
        case 0x00:
            res = tvc_open_dir();
            break;
        case 0x01:
            res = tvc_lfs_psram_dir_open();
            break;
        case 0x02:
            res = tvc_lfs_flash_dir_open();
            break;
        default:
            res = 3;
            break;
    }

    if(res == FR_OK) {
        // On success, store the dir handle pointer back to bufferStart
        *(uint32_t *)&bufferStart[0] = *(uint32_t *)&TVC_ROM[0x1a00];
    }
    return (uint8_t)res;
}

uint8_t tvcfunc_close_dir(uint8_t* bufferStart) {
    *(uint32_t *)&TVC_ROM[0x1900] = *(uint32_t *)&bufferStart[0];
    FRESULT res = 0;
    switch(SELECTED_DEVICE) {
        case 0x00:
            res = tvc_close_dir();
            break;
        case 0x01:
            res = tvc_lfs_psram_dir_close();
            break;
        case 0x02:
            res = tvc_lfs_flash_dir_close();
            break;
        default:
            res = 3;
            break;
    }
    return (uint8_t)res;
}

uint8_t tvcfunc_read_dir(uint8_t* bufferStart) {
    *(uint32_t *)&TVC_ROM[0x1900] = *(uint32_t *)&bufferStart[0];

    FRESULT res = 0;

    switch(SELECTED_DEVICE) {
        case 0x00:
            res = tvc_read_dir();
            break;
        case 0x01:
            res = tvc_lfs_psram_dir_read();
            break;
        case 0x02:
            res = tvc_lfs_flash_dir_read();
            break;
        default:
            res = 3;
            break;
    }

    if(res == FR_OK) {
        memcpy(&bufferStart[4], &TVC_ROM[0x1a00], sizeof(FILINFO)+1);
    }
    return (uint8_t)res;
}

uint8_t tvcfunc_file_seek(uint8_t *bufferStart) {

    *(uint32_t *)&TVC_ROM[0x1900] = *(uint32_t *)&bufferStart[0];
    *(uint32_t *)&TVC_ROM[0x1904] = *(uint32_t *)&bufferStart[4];
    FRESULT res;
    switch(SELECTED_DEVICE) {
        case 0: // USB
            res = tvc_file_seek();
            break;
        case 1: // psram
            res = tvc_lfs_psram_file_seek();
            break;
        case 2: // flash
            res = tvc_lfs_flash_file_seek();
            break;
        default:
            res = 3;
            break;
    }
    return res;
}

uint8_t tvcfunc_getcwd(uint8_t *bufferStart) {
    FRESULT res = 0;
    switch(SELECTED_DEVICE) {
        case 0x00:
            res = tvc_getcwd();
            break;
        case 0x01:
            res = tvc_lfs_psram_getcwd();
            break;
        case 0x02:
            res = tvc_lfs_flash_getcwd();
            break;
        default:
            res = 3;
            break;
    }
    if(res == FR_OK) {
        int len = TVC_ROM[0x1a00];
        memcpy(&bufferStart[0], &TVC_ROM[0x1a00], len+1);
    }
    return (uint8_t)res;
}

uint8_t tvcfunc_chdir(uint8_t *bufferStart) {
    if(bufferStart[0] != 0) {
        memcpy(&TVC_ROM[0x1900], &bufferStart[0], bufferStart[0]+1);
    } else {
        return FR_INVALID_PARAMETER;
    }
    FRESULT res = 0;
    switch(SELECTED_DEVICE) {
        case 0x00:
            res = tvc_chdir();
            break;
        case 0x01:
            res = tvc_lfs_psram_chdir();
            break;
        case 0x02:
            res = tvc_lfs_flash_chdir();
            break;
        default:
            res = 3;
            break;
    }
    return res;
}

uint8_t tvcfunc_mkdir(uint8_t *bufferStart) {
    if(bufferStart[0] != 0) {
        memcpy(&TVC_ROM[0x1900], &bufferStart[0], bufferStart[0]+1);
    } else {
        return FR_INVALID_PARAMETER;
    }
    // usb_printf("rtns/mkdir: len: %d, dir: %s\n", bufferStart[0], (char *)&bufferStart[1]);main_loop_task();sleep_ms(50);
    FRESULT res = 0;
    switch (SELECTED_DEVICE) {
        case 0x00:
            res = tvc_mkdir();
            break;
        case 0x01:
            res = tvc_lfs_psram_mkdir();
            break;
        case 0x02:
            res = tvc_lfs_flash_mkdir();
            break;
        default:
            res = 3;
            break;
    }
    return res;
}

uint8_t tvcfunc_delete(uint8_t *bufferStart) {
    TVC_ROM[0x1900] = bufferStart[0];
    if(bufferStart[0] != 0) {
        memcpy(&TVC_ROM[0x1901], &bufferStart[1], bufferStart[0]+1);
    } else {
        return FR_INVALID_PARAMETER;
    }
    
    FRESULT res = 0;
    switch(SELECTED_DEVICE) {
        case 0x00:
            res = tvc_delete();
            break;
        case 0x01:
            res = tvc_lfs_psram_delete();
            break;
        case 0x02:
            res = tvc_lfs_flash_delete();
            break;
        default:
            res = 3;
            break;
    }
    return res;
}

uint8_t tvcfunc_rename(uint8_t *bufferStart) {
    uint8_t len1 = bufferStart[0];
    uint8_t len2 = bufferStart[len1 + 1];
    memcpy(&TVC_ROM[0x1900], &bufferStart[0], (uint16_t)len1 + (uint16_t)len2 + 2);
    switch(SELECTED_DEVICE) {
        case 0x00:
            return (uint8_t) tvc_rename();
        case 0x01:
            return (uint8_t) tvc_lfs_psram_rename();
        case 0x02:
            return (uint8_t) tvc_lfs_flash_rename();
        default:
            return 3;
    }

}
*/

/**
 * Gets a file's stat. Returns non-zero if error, otherwise 0 and file info is stored in bufferStart
 * INPUT:
 * bufferStart[0]: length of the filename
 * bufferStart[1...]: filename
 * OUTPUT:
 * bufferStart[0]: length of the filename (the found file's name)
 * bufferStart[1...]: filename (the found file's name)
 * bufferStart[0x100]: file size (4 bytes)
 * bufferStart[0x104]: file attributes (1 byte)
 */
/*
uint8_t tvcfunc_getstat(uint8_t* bufferStart) {
    TVC_ROM[0x1900] = bufferStart[0];
    if(bufferStart[0] != 0)
        memcpy(&TVC_ROM[0x1901], &bufferStart[1], bufferStart[0]);

    FRESULT res = 0;
    switch(SELECTED_DEVICE) {
        case 0x00:
            res = tvc_getstat();
            break;
        case 0x01:
            res = tvc_lfs_psram_getstat();
            break;
        case 0x02:
            res = tvc_lfs_flash_getstat();
            break;
        default:
            res = 3;
            break;
    }

    if(res == FR_OK) {
        memcpy(&bufferStart[0], &TVC_ROM[0x1a00], sizeof(FILINFO));
    }
    return (uint8_t)res;
}


uint8_t tvcfunc_sync(uint8_t* bufferStart) {
    *(uint32_t *)&TVC_ROM[0x1900] = *(uint32_t *)&bufferStart[0];
    switch(SELECTED_DEVICE) {
        case 0x00:
            return (uint8_t) tvc_sync();
        case 0x01:
            return (uint8_t) tvc_lfs_psram_sync();
        case 0x02:
            return (uint8_t) tvc_lfs_flash_sync();
        default:
            return 3;
    }
}

uint8_t getFunctionParamSize(uint8_t *bufStart, uint8_t paramSize) {
    if((paramSize & 0x80) == 0)
        return paramSize;
    uint size = 1;
    switch (paramSize) {
        case 0x80:  // C style, 0 terminated string
            while(*bufStart++)
                size++;
            break;
        case 0x81:  // TVC style string, length + characters
            size = (*bufStart) + 1;
            break;
        case 0x82:  // draw multiline
            size = (*bufStart) * 4;
            break;
        case 0x83:  // open file
            size = bufStart[3] + 3;
            break;
        case 0x84:  // write file
            size = 4 + 2 + *(uint16_t *)&bufStart[4];
            break;
        case 0x85:  // delete - two pascal strings
            size = (*bufStart) + 1;
            size += bufStart[size] + 1;
            break;
    }
    return size;
}
*/
tvc_function_struct_t tvc256k_funct_struct_array[256];

void setStructArrayElement(int idx, tvc_function_t funct, uint8_t sizeOfParam) {
    tvc256k_funct_struct_array[idx] = (tvc_function_struct_t){.func = funct, .param_size = sizeOfParam};
}


void init_routines() {
    setStructArrayElement( 0, clear_text_screen,             0);
    setStructArrayElement( 1, clear_bitmap_screen,           0);
    setStructArrayElement( 2, print_character_screen_code,   1);
    setStructArrayElement( 3, print_hex_byte,                1);
    setStructArrayElement( 4, print_hex_word,                2);
    setStructArrayElement( 5, print_string_screen_code,      0x80),       // 05
    setStructArrayElement( 6, print_string_ascii,            0x80);
    setStructArrayElement( 7, get_text_color,                0);
    setStructArrayElement( 8, set_text_color,                1);
    setStructArrayElement( 9, get_xy,                        0);
    setStructArrayElement(10, set_xy,                        2);          // 10
    setStructArrayElement(11, memory_move_short,             7);
    setStructArrayElement(12, memory_move_full,              9);
    setStructArrayElement(13, memory_move_from_slow,         9);
    setStructArrayElement(14, memory_move_to_slow,           9);
//    setStructArrayElement(15, copy_dir_to_psram,             0x81);      // 15
    setStructArrayElement(16, replace_pixel_color,           8);
    setStructArrayElement(17, memory_move_chunks_from_block, 12);
    setStructArrayElement(18, memory_move_chunks,            12);
    setStructArrayElement(19, mirror_sprite_phase,           6);
//    setStructArrayElement(20, zx7Decompress,                 6);      // 20
    setStructArrayElement(21, get_pen_color,                 0);
    setStructArrayElement(22, set_pen_color,                 1);
    setStructArrayElement(23, get_dot_color,                 2);
    setStructArrayElement(24, set_dot_color,                 2);
    setStructArrayElement(25, draw_line,                     8);     // 25
    setStructArrayElement(26, draw_rectangle,                6);
    setStructArrayElement(27, fill_rectangle,                6);
    setStructArrayElement(28, draw_multi_line,               0x82);
    setStructArrayElement(29, scanline_flood_fill,           2);
    setStructArrayElement(30, draw_ellipse,                  6);     // 30
    setStructArrayElement(31, fill_ellipse,                  6);
    setStructArrayElement(32, copy_image_block_fast,         9);
    setStructArrayElement(33, copy_sub_image,                18);
    setStructArrayElement(34, create_psram_drive,            2);
    setStructArrayElement(35, get_first_usable_psram_pos,    0);
    setStructArrayElement(36, delete_psram_drive,            0);

/*
    setStructArrayElement(128+MSC_FOPENFILE,    tvcfunc_open_file,       0x83);
    setStructArrayElement(128+MSC_FCLOSEFILE,   tvcfunc_close_file,      4);
    setStructArrayElement(128+MSC_FREAD,        tvcfunc_read_file,       6);
    setStructArrayElement(128+MSC_FWRITE,       tvcfunc_write_file,      0x84);
    setStructArrayElement(128+MSC_FOPENDIR,     tvcfunc_open_dir,        0x81);
    setStructArrayElement(128+MSC_FCLOSEDIR,    tvcfunc_close_dir,       4);
    setStructArrayElement(128+MSC_FREADDIR,     tvcfunc_read_dir,        4);
    setStructArrayElement(128+MSC_FWRITE_SRC,   tvcfunc_write_file_source, 10);
    setStructArrayElement(128+MSC_FREAD_DEST,   tvcfunc_read_file_dest,  10);
    setStructArrayElement(128+MSC_FSEEK,        tvcfunc_file_seek,       8);
    setStructArrayElement(128+MSC_FGETCWD,      tvcfunc_getcwd,          0);
    setStructArrayElement(128+MSC_FCHDIR,       tvcfunc_chdir,           0x81);
    setStructArrayElement(128+MSC_FMKDIR,       tvcfunc_mkdir,           0x81);
    setStructArrayElement(128+MSC_FDELETE,      tvcfunc_delete,          0x81);
    setStructArrayElement(128+MSC_FRENAME,      tvcfunc_rename,          0x85);
    setStructArrayElement(128+MSC_FSTAT,        tvcfunc_getstat,         0x81);
    setStructArrayElement(128+MSC_FSYNC,        tvcfunc_sync,            4);
*/
    init_bitmap_byte_masks();
}
}
