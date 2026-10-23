#ifndef MESH3D_ANGLES_H
#define MESH3D_ANGLES_H
#include <stdint.h>
/* Public 3D 10-bit angle format: one full turn = 1024 units.
 * Signed directions use modulo-1024 two's-complement arithmetic.
 * Four packed bytes are a LITTLE-ENDIAN 32-bit bitstream:
 * bits  0.. 9 = roll, 10..19 = yaw, 20..29 = pitch,
 * bits 30..31 reserved and transmitted as zero.
 * The exact same representation is used by camera and mesh poses.
 */
#define TWODFX_3D_ANGLE_MASK 1023u
static inline void twodfx_pack_angles10(uint8_t out[4],
                                         uint16_t roll,uint16_t yaw,uint16_t pitch)
{
    uint32_t b=((uint32_t)roll&1023u) |
               (((uint32_t)yaw&1023u)<<10) |
               (((uint32_t)pitch&1023u)<<20);
    out[0]=(uint8_t)b;out[1]=(uint8_t)(b>>8);
    out[2]=(uint8_t)(b>>16);out[3]=(uint8_t)(b>>24);
}
static inline void twodfx_unpack_angles10(const uint8_t in[4],
                                           uint16_t *roll,uint16_t *yaw,uint16_t *pitch)
{
    uint32_t b=(uint32_t)in[0]|((uint32_t)in[1]<<8)|
               ((uint32_t)in[2]<<16)|((uint32_t)in[3]<<24);
    *roll=(uint16_t)(b&1023u);
    *yaw=(uint16_t)((b>>10)&1023u);
    *pitch=(uint16_t)((b>>20)&1023u);
}
#endif
