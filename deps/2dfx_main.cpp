#include <stdint.h>
#include "2dfx_main.h"

namespace TWODFX {

uint8_t nick_to_sprite_lut[256];
uint8_t tvc_to_sprite_lut[256];

void build_source_conversion_luts(void)
{
    for (uint32_t i = 0u; i < 256u; i++) {
        uint8_t n = (uint8_t)i;

        uint8_t nick_left =
            (((n >> 7) & 1u) << 3) |
            (((n >> 5) & 1u) << 2) |
            (((n >> 3) & 1u) << 1) |
            (((n >> 1) & 1u) << 0);

        uint8_t nick_right =
            (((n >> 6) & 1u) << 3) |
            (((n >> 4) & 1u) << 2) |
            (((n >> 2) & 1u) << 1) |
            (((n >> 0) & 1u) << 0);

        nick_to_sprite_lut[i] =
            (uint8_t)((nick_left & 0x0Fu) | (uint8_t)(nick_right << 4));

        /*
         * TVC Graphics-16 bytes are interleaved IIGGRRBB, just like the
         * Enterprise/NICK two-pixel layout: odd-numbered source bits belong
         * to the left pixel and even-numbered bits to the right pixel.
         *
         *   left  IGRB = source bits 7,5,3,1
         *   right IGRB = source bits 6,4,2,0
         *
         * The TVC external-colour/internal numeric bit order is BGRI, so I
         * and B exchange bit positions while G/R remain in bits 2/1.
         */
        uint8_t tvc_left_igrb =
            (uint8_t)((((n >> 7) & 1u) << 3) |
                      (((n >> 5) & 1u) << 2) |
                      (((n >> 3) & 1u) << 1) |
                      (((n >> 1) & 1u) << 0));

        uint8_t tvc_right_igrb =
            (uint8_t)((((n >> 6) & 1u) << 3) |
                      (((n >> 4) & 1u) << 2) |
                      (((n >> 2) & 1u) << 1) |
                      (((n >> 0) & 1u) << 0));

        uint8_t tvc_left_bgri =
            (uint8_t)(((tvc_left_igrb & 0x01u) << 3) |
                      (tvc_left_igrb & 0x06u) |
                      ((tvc_left_igrb & 0x08u) >> 3));
        uint8_t tvc_right_bgri =
            (uint8_t)(((tvc_right_igrb & 0x01u) << 3) |
                      (tvc_right_igrb & 0x06u) |
                      ((tvc_right_igrb & 0x08u) >> 3));

        /* Internal packed format stores the left pixel in the low nibble. */
        tvc_to_sprite_lut[i] =
            (uint8_t)((tvc_left_bgri & 0x0Fu) | (uint8_t)(tvc_right_bgri << 4));
    }
}

}