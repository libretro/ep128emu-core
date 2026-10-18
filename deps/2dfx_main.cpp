#include <stdint.h>
#include "2dfx_main.h"
#include "twodfx_api.h"

namespace TWODFX {

// Soft 2DPT command-write session.
//
// A 2DPT slot command is started by writing 60h..7Fh for background or
// E0h..FFh for foreground to F8h.  The following F9h bytes are collected here.
// The first byte is always the 2DPB slot number; the remaining bytes are
// command-specific arguments.  Once the expected number of bytes has been
// received, the corresponding shadow_2dpt layer/slot is updated atomically
// from the command interpreter's point of view.
static bool    twod_write_active = false;
static uint8_t twod_write_opcode = 0;
static uint8_t twod_write_expected = 0;
static uint8_t twod_write_index = 0;
static bool twod_write_foreground = false;
static uint8_t twod_write_args[8];

// ============================================================
// 2D PARAMETER TABLES / 2DPT
// ============================================================
//
// The 2D renderer uses the same shadow/active philosophy as the sprite SPT:
//
//   shadow_2dpt[0]: background table modified by Enterprise OUT 60h..7Fh.
//   shadow_2dpt[1]: foreground table modified by Enterprise OUT E0h..FFh.
//   active_2dpt[]:  stable snapshots replayed by the renderer.
//
// At VSYNC, both shadow tables are copied to active if dirty.  This makes 2D
// updates frame-synchronous and prevents the renderer from seeing a partially
// updated command table.
//
// Each 2DPB is deliberately 8 bytes, remaining independent of SPB size.
// Opcode 7Fh means RET: stop processing the selected 2DPT.  Opcode 60h means
// NOP: do nothing and continue with the next 2DPB.
static uint8_t shadow_2dpt[NUM_2DPT_LAYERS][NUM_2DPBS][SIZE_2DPB];
static uint8_t active_2dpt[NUM_2DPT_LAYERS][NUM_2DPBS][SIZE_2DPB];
static volatile bool twodpt_dirty = false;

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
