#ifndef EGG_CB_ARCPIE_BENCHMARK_H
#define EGG_CB_ARCPIE_BENCHMARK_H

#include "twodfx_api.h"

/*
 * Egg CB — ARC/PIE render-time stress benchmark
 * =============================================
 *
 * Start from the host computer with:
 *
 *     OUT 248,203        (203 = CBh)
 *
 * Purpose
 * -------
 * Exercises the renderer's most expensive ellipse-sector primitives with a
 * deliberately heavy, completely static 2DPT scene:
 *
 * - two large stroked ellipses provide visual reference rings;
 * - six large filled PIE sectors include both ordinary and wrapping sweeps;
 * - eight large/thick ARC commands stress the optimized inner-ring path;
 * - no sprites or per-tick scene rebuilds disturb render-time measurements.
 *
 * What appears on screen
 * ----------------------
 * A dark full renderer-window background covered by overlapping coloured
 * ellipses, sectors and arcs. The same 2DPT is replayed every frame, so the
 * STATUS0 render pulse remains stable enough for oscilloscope comparison.
 *
 * Implementation style
 * --------------------
 * The .c file uses only direct public twodfx_2d_* calls. The original scene
 * was authored as top-left/width/height bounding boxes; each call visibly
 * converts that box to the centre/radius form required by ELLIPSE, ARC and
 * PIE. There are no raw 2DPBs, private shadow tables or renderer helpers.
 *
 * ARC and PIE are intentionally expensive primitives. This egg is a stress
 * benchmark, not a recommendation to rebuild such a scene every game frame.
 * Repeated artwork is normally better prepared once as a bitmap and BLITted.
 *
 * All profile, colour, geometry, angle and stroke constants live here.
 */

/* ------------------------------------------------------------------------- */
/* Profile renderer-window geometry                                          */
/* ------------------------------------------------------------------------- */

/* These are renderer-window limits, not practical safe-area measurements. */
#define EGG_CB_EP_RAW_X      88u
#define EGG_CB_EP_RAW_Y      16u
#define EGG_CB_EP_RAW_WIDTH 744u
#define EGG_CB_EP_RAW_HEIGHT 288u

#define EGG_CB_TVC_RAW_X       0u
#define EGG_CB_TVC_RAW_Y      41u
#define EGG_CB_TVC_RAW_WIDTH 704u
#define EGG_CB_TVC_RAW_HEIGHT 256u

/* ------------------------------------------------------------------------- */
/* Scene colours                                                             */
/* ------------------------------------------------------------------------- */

#define EGG_CB_EP_BACKGROUND_COLOUR  8u
#define EGG_CB_TVC_BACKGROUND_COLOUR 1u
#define EGG_CB_TRANSPARENT_COLOUR    0u

#define EGG_CB_REFERENCE_OUTER_COLOUR 2u
#define EGG_CB_REFERENCE_INNER_COLOUR 3u

#define EGG_CB_EP_PIE_COLOURS  { 2u, 3u, 4u, 6u, 5u, 1u }
#define EGG_CB_TVC_PIE_COLOURS { 9u, 10u, 11u, 12u, 13u, 14u }
#define EGG_CB_PIE_COLOUR_COUNT 6u

#define EGG_CB_EP_ARC_COLOURS  { 7u, 3u, 6u, 5u, 4u, 2u, 1u, 7u }
#define EGG_CB_TVC_ARC_COLOURS { 15u, 7u, 6u, 5u, 4u, 3u, 2u, 8u }
#define EGG_CB_ARC_COLOUR_COUNT 8u

/* ------------------------------------------------------------------------- */
/* Bounding-box geometry                                                     */
/* ------------------------------------------------------------------------- */

/* Reference ellipses: offsets are relative to the profile raw origin. */
#define EGG_CB_REFERENCE_OUTER_X  10u
#define EGG_CB_REFERENCE_OUTER_Y   8u
#define EGG_CB_REFERENCE_OUTER_W 720u
#define EGG_CB_REFERENCE_OUTER_H 270u
#define EGG_CB_REFERENCE_OUTER_STROKE 1u

#define EGG_CB_REFERENCE_INNER_X  70u
#define EGG_CB_REFERENCE_INNER_Y  38u
#define EGG_CB_REFERENCE_INNER_W 600u
#define EGG_CB_REFERENCE_INNER_H 210u
#define EGG_CB_REFERENCE_INNER_STROKE 2u

/* Filled PIE sectors. */
#define EGG_CB_PIE0_X  20u
#define EGG_CB_PIE0_Y  14u
#define EGG_CB_PIE0_W 360u
#define EGG_CB_PIE0_H 180u
#define EGG_CB_PIE0_START   0u
#define EGG_CB_PIE0_END    96u

#define EGG_CB_PIE1_X 230u
#define EGG_CB_PIE1_Y  28u
#define EGG_CB_PIE1_W 360u
#define EGG_CB_PIE1_H 220u
#define EGG_CB_PIE1_START  64u
#define EGG_CB_PIE1_END   180u

#define EGG_CB_PIE2_X  72u
#define EGG_CB_PIE2_Y  78u
#define EGG_CB_PIE2_W 560u
#define EGG_CB_PIE2_H 198u
#define EGG_CB_PIE2_START 128u
#define EGG_CB_PIE2_END    24u

#define EGG_CB_PIE3_X 330u
#define EGG_CB_PIE3_Y  46u
#define EGG_CB_PIE3_W 380u
#define EGG_CB_PIE3_H 224u
#define EGG_CB_PIE3_START 192u
#define EGG_CB_PIE3_END    72u

#define EGG_CB_PIE4_X  34u
#define EGG_CB_PIE4_Y 118u
#define EGG_CB_PIE4_W 620u
#define EGG_CB_PIE4_H 158u
#define EGG_CB_PIE4_START  32u
#define EGG_CB_PIE4_END   224u

#define EGG_CB_PIE5_X 410u
#define EGG_CB_PIE5_Y   8u
#define EGG_CB_PIE5_W 318u
#define EGG_CB_PIE5_H 270u
#define EGG_CB_PIE5_START 220u
#define EGG_CB_PIE5_END    40u

/* Stroked ARCs. */
#define EGG_CB_ARC0_X  28u
#define EGG_CB_ARC0_Y   6u
#define EGG_CB_ARC0_W 690u
#define EGG_CB_ARC0_H 274u
#define EGG_CB_ARC0_START   0u
#define EGG_CB_ARC0_END   255u
#define EGG_CB_ARC0_STROKE  1u

#define EGG_CB_ARC1_X  54u
#define EGG_CB_ARC1_Y  30u
#define EGG_CB_ARC1_W 636u
#define EGG_CB_ARC1_H 230u
#define EGG_CB_ARC1_START  32u
#define EGG_CB_ARC1_END   210u
#define EGG_CB_ARC1_STROKE  2u

#define EGG_CB_ARC2_X 110u
#define EGG_CB_ARC2_Y  42u
#define EGG_CB_ARC2_W 528u
#define EGG_CB_ARC2_H 194u
#define EGG_CB_ARC2_START  75u
#define EGG_CB_ARC2_END   250u
#define EGG_CB_ARC2_STROKE  3u

#define EGG_CB_ARC3_X 176u
#define EGG_CB_ARC3_Y  70u
#define EGG_CB_ARC3_W 420u
#define EGG_CB_ARC3_H 150u
#define EGG_CB_ARC3_START 150u
#define EGG_CB_ARC3_END    50u
#define EGG_CB_ARC3_STROKE  4u

#define EGG_CB_ARC4_X 250u
#define EGG_CB_ARC4_Y  96u
#define EGG_CB_ARC4_W 360u
#define EGG_CB_ARC4_H 130u
#define EGG_CB_ARC4_START 208u
#define EGG_CB_ARC4_END    92u
#define EGG_CB_ARC4_STROKE  2u

#define EGG_CB_ARC5_X   8u
#define EGG_CB_ARC5_Y 128u
#define EGG_CB_ARC5_W 720u
#define EGG_CB_ARC5_H 144u
#define EGG_CB_ARC5_START  96u
#define EGG_CB_ARC5_END   192u
#define EGG_CB_ARC5_STROKE  1u

#define EGG_CB_ARC6_X 140u
#define EGG_CB_ARC6_Y  18u
#define EGG_CB_ARC6_W 480u
#define EGG_CB_ARC6_H 254u
#define EGG_CB_ARC6_START 240u
#define EGG_CB_ARC6_END   120u
#define EGG_CB_ARC6_STROKE  3u

#define EGG_CB_ARC7_X 300u
#define EGG_CB_ARC7_Y 108u
#define EGG_CB_ARC7_W 390u
#define EGG_CB_ARC7_H 132u
#define EGG_CB_ARC7_START  16u
#define EGG_CB_ARC7_END   160u
#define EGG_CB_ARC7_STROKE  4u

void egg_CB_arcpie_benchmark_start(void);
void egg_CB_arcpie_benchmark_tick(void);

#endif /* EGG_CB_ARCPIE_BENCHMARK_H */
