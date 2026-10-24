#ifndef EGG_D3_ELITE_SHOWCASE_H
#define EGG_D3_ELITE_SHOWCASE_H
#include <stdbool.h>
#include <stdint.h>
typedef struct { int32_t x, y, z; } egg3d_vec_q8_t;
typedef struct { int16_t sy, cy, sp, cp, sr, cr; } egg3d_angles_q12_t;


static inline int32_t egg3d_round_div(int32_t value, int32_t positive_den)
{
    return value >= 0 ? (value + positive_den / 2) / positive_den
                      : -((-value + positive_den / 2) / positive_den);
}

static inline egg3d_vec_q8_t egg3d_rotate_q8(
    const egg3d_angles_q12_t *r, int16_t vx, int16_t vy, int16_t vz)
{
    const int32_t xx = (int32_t)vx * 256;
    const int32_t yy = (int32_t)vy * 256;
    const int32_t zz = (int32_t)vz * 256;
    const int32_t x = (xx * r->cy - zz * r->sy) / 4096;
    const int32_t z = (xx * r->sy + zz * r->cy) / 4096;
    const int32_t y = (yy * r->cp - z * r->sp) / 4096;
    const int32_t z2 = (yy * r->sp + z * r->cp) / 4096;
    const int32_t rx = (x * r->cr - y * r->sr) / 4096;
    const int32_t ry = (x * r->sr + y * r->cr) / 4096;
    return (egg3d_vec_q8_t){rx, ry, z2};
}

/* Perspective division is the ONLY point where subpixel projected coordinates
 * are rounded to integer pixels.  This is a deliberate nearest-pixel rule.
 */
static inline uint16_t egg3d_project_axis(
    int32_t centre, int32_t rotated_q8, int32_t camera_q8,
    int32_t z_q8, int32_t scale, uint16_t maximum, int sign)
{
    const int32_t denom = camera_q8 - z_q8; /* Positive for both demo ships. */
    const int32_t numerator = rotated_q8 * scale;
    int32_t value = centre + sign * egg3d_round_div(numerator, denom);
    if (value < 0) value = 0;
    if (value > maximum) value = maximum;
    return (uint16_t)value;
}


/* D3: Elite showcase, cooperative main-loop geometry builder.
 * v1.0.4l: five Elite ships, 30 seconds each, no diagnostics.
 * egg_D3_elite_tick() executes at VSYNC only AFTER publication. */
void egg_D3_elite_start(void);
void egg_D3_elite_tick(void);
void egg_D3_elite_step(void);
bool egg_D3_elite_ready(void);
/* Q12 sine lookup table shared by all showcase models. */
static const int16_t d3_sine_q12[256] = {
        0,   101,   201,   301,   401,   501,   601,   700,
      799,   897,   995,  1092,  1189,  1285,  1380,  1474,
     1567,  1660,  1751,  1842,  1931,  2019,  2106,  2191,
     2276,  2359,  2440,  2520,  2598,  2675,  2751,  2824,
     2896,  2967,  3035,  3102,  3166,  3229,  3290,  3349,
     3406,  3461,  3513,  3564,  3612,  3659,  3703,  3745,
     3784,  3822,  3857,  3889,  3920,  3948,  3973,  3996,
     4017,  4036,  4052,  4065,  4076,  4085,  4091,  4095,
     4096,  4095,  4091,  4085,  4076,  4065,  4052,  4036,
     4017,  3996,  3973,  3948,  3920,  3889,  3857,  3822,
     3784,  3745,  3703,  3659,  3612,  3564,  3513,  3461,
     3406,  3349,  3290,  3229,  3166,  3102,  3035,  2967,
     2896,  2824,  2751,  2675,  2598,  2520,  2440,  2359,
     2276,  2191,  2106,  2019,  1931,  1842,  1751,  1660,
     1567,  1474,  1380,  1285,  1189,  1092,   995,   897,
      799,   700,   601,   501,   401,   301,   201,   101,
        0,  -101,  -201,  -301,  -401,  -501,  -601,  -700,
     -799,  -897,  -995, -1092, -1189, -1285, -1380, -1474,
    -1567, -1660, -1751, -1842, -1931, -2019, -2106, -2191,
    -2276, -2359, -2440, -2520, -2598, -2675, -2751, -2824,
    -2896, -2967, -3035, -3102, -3166, -3229, -3290, -3349,
    -3406, -3461, -3513, -3564, -3612, -3659, -3703, -3745,
    -3784, -3822, -3857, -3889, -3920, -3948, -3973, -3996,
    -4017, -4036, -4052, -4065, -4076, -4085, -4091, -4095,
    -4096, -4095, -4091, -4085, -4076, -4065, -4052, -4036,
    -4017, -3996, -3973, -3948, -3920, -3889, -3857, -3822,
    -3784, -3745, -3703, -3659, -3612, -3564, -3513, -3461,
    -3406, -3349, -3290, -3229, -3166, -3102, -3035, -2967,
    -2896, -2824, -2751, -2675, -2598, -2520, -2440, -2359,
    -2276, -2191, -2106, -2019, -1931, -1842, -1751, -1660,
    -1567, -1474, -1380, -1285, -1189, -1092,  -995,  -897,
     -799,  -700,  -601,  -501,  -401,  -301,  -201,  -101,
};

#endif
