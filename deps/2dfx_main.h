/* This file is originated from tvc256++ firmware. The structure
   is kept to enable merge of later changes easier than re-implementing
   all the functions. */
#ifndef EP128EMU_2DFX_H
#define EP128EMU_2DFX_H


namespace TWODFX {

extern uint8_t nick_to_sprite_lut[256];
extern uint8_t tvc_to_sprite_lut[256];

void build_source_conversion_luts(void);

}
#endif
