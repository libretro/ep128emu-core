#ifndef EGG_CD_BLIT_FULL_H
#define EGG_CD_BLIT_FULL_H

#include "twodfx_api.h"

/*
 * Egg CD — full-frame BLIT benchmark
 * ==================================
 *
 * Start from the host computer with:
 *
 *     OUT 248,205        (205 = CDh)
 *
 * Purpose
 * -------
 * Measures the cost of the largest ordinary exact BLIT supported by the
 * active machine profile, first by itself and then with two doubled sprites.
 * It is useful for checking both raw BLIT timing and the extra cost of sprite
 * composition over a large copied background.
 *
 * What appears on screen
 * ----------------------
 * A Mondrian-like opaque image covers the complete stored render area. A
 * small marker bar near the top identifies stage 0. After 20 seconds a second
 * marker appears and two doubled smiley sprites are added. The two stages then
 * alternate every 20 seconds.
 *
 * Implementation style
 * --------------------
 * The bitmap is generated one packed row at a time in a small MCU buffer and
 * uploaded through twodfx_upload_raw(). The smileys are generated in another
 * small buffer. The scene uses one public exact-BLIT command plus public
 * sprite calls; this egg never accesses resource_ram, raw SPBs/2DPBs, shadow
 * tables or private renderer helpers.
 *
 * All resource, profile, artwork, stage and colour constants live here.
 */

/* ------------------------------------------------------------------------- */
/* Stage timing                                                              */
/* ------------------------------------------------------------------------- */

/** 20 seconds per stage at the normal 50 update opportunities per second. */
#define EGG_CD_STAGE_TICKS 1000u

/** Stage 0 = BLIT only, stage 1 = BLIT plus two smileys. */
#define EGG_CD_STAGE_COUNT 2u
#define EGG_CD_STAGE_BLIT_ONLY 0u
#define EGG_CD_STAGE_WITH_SPRITES 1u

/* ------------------------------------------------------------------------- */
/* Resource allocation                                                       */
/* ------------------------------------------------------------------------- */

#define EGG_CD_BITMAP_ADDRESS 0x560000u
#define EGG_CD_BITMAP_ID      11u

#define EGG_CD_SMILEY0_ADDRESS 0x610000u
#define EGG_CD_SMILEY1_ADDRESS 0x620000u

/* ------------------------------------------------------------------------- */
/* Complete stored render areas                                              */
/* ------------------------------------------------------------------------- */

/* Enterprise profile: full 744x288 renderer window at raw X=88, Y=16. */
#define EGG_CD_EP_BITMAP_X      88u
#define EGG_CD_EP_BITMAP_Y      16u
#define EGG_CD_EP_BITMAP_WIDTH 744u
#define EGG_CD_EP_BITMAP_HEIGHT 288u

/* TVC profile: complete 704x256 stored scanout area at raw X=0, Y=41. */
#define EGG_CD_TVC_BITMAP_X       0u
#define EGG_CD_TVC_BITMAP_Y      41u
#define EGG_CD_TVC_BITMAP_WIDTH 704u
#define EGG_CD_TVC_BITMAP_HEIGHT 256u

#define EGG_CD_EP_ROW_BYTES  (EGG_CD_EP_BITMAP_WIDTH / 2u)
#define EGG_CD_TVC_ROW_BYTES (EGG_CD_TVC_BITMAP_WIDTH / 2u)
#define EGG_CD_MAX_ROW_BYTES EGG_CD_EP_ROW_BYTES

/* ------------------------------------------------------------------------- */
/* Mondrian field geometry                                                   */
/* ------------------------------------------------------------------------- */

/*
 * Both profiles use the same normalized C1-style Mondrian composition. The
 * design is scaled into each machine's actual maximum-visible rectangle while
 * the stored full-screen bitmap may extend into invisible scanout padding.
 */
#define EGG_CD_C1_MONDRIAN_WIDTH  540u
#define EGG_CD_C1_MONDRIAN_HEIGHT 230u
#define EGG_CD_C1_MONDRIAN_GRID_THICKNESS 6u

/* Internal colour index 0 is deliberately avoided in the full bitmap. */
#define EGG_CD_EP_OPAQUE_BLACK_COLOUR  8u
#define EGG_CD_TVC_OPAQUE_BLACK_COLOUR 1u

#define EGG_CD_TVC_WHITE_COLOUR  15u
#define EGG_CD_TVC_RED_COLOUR     3u
#define EGG_CD_TVC_YELLOW_COLOUR  7u
#define EGG_CD_TVC_BLUE_COLOUR    9u
#define EGG_CD_TVC_CYAN_COLOUR   13u

/* Enterprise follows the handbook demo palette. There is no dedicated blue
 * in 0..7, so cyan is the closest stable lower-palette substitute. */
#define EGG_CD_EP_WHITE_COLOUR   7u
#define EGG_CD_EP_RED_COLOUR     1u
#define EGG_CD_EP_YELLOW_COLOUR  3u
#define EGG_CD_EP_BLUE_COLOUR    5u
#define EGG_CD_EP_CYAN_COLOUR    5u

/* ------------------------------------------------------------------------- */
/* Stage marker bars                                                         */
/* ------------------------------------------------------------------------- */

#define EGG_CD_MARKER_Y      22u
#define EGG_CD_MARKER_WIDTH  36u
#define EGG_CD_MARKER_HEIGHT  8u

#define EGG_CD_MARKER0_X      96u
#define EGG_CD_MARKER0_COLOUR  7u
#define EGG_CD_MARKER1_X     140u
#define EGG_CD_MARKER1_COLOUR  4u

/* ------------------------------------------------------------------------- */
/* Smiley sprite artwork and placement                                       */
/* ------------------------------------------------------------------------- */

#define EGG_CD_SMILEY_WIDTH  16u
#define EGG_CD_SMILEY_HEIGHT 16u
#define EGG_CD_SMILEY_ROW_BYTES (EGG_CD_SMILEY_WIDTH / 2u)
#define EGG_CD_SMILEY_BYTES \
    (EGG_CD_SMILEY_ROW_BYTES * EGG_CD_SMILEY_HEIGHT)

#define EGG_CD_SMILEY0_SPRITE 0u
#define EGG_CD_SMILEY1_SPRITE 1u

#define EGG_CD_SMILEY0_X 232u
#define EGG_CD_SMILEY0_Y  96u
#define EGG_CD_SMILEY1_X 584u
#define EGG_CD_SMILEY1_Y 176u

#define EGG_CD_EP_SMILEY0_FACE_COLOUR    3u
#define EGG_CD_EP_SMILEY0_DETAIL_COLOUR  7u
#define EGG_CD_EP_SMILEY1_FACE_COLOUR    4u
#define EGG_CD_EP_SMILEY1_DETAIL_COLOUR  7u
#define EGG_CD_EP_SMILEY_DARK_COLOUR     8u

#define EGG_CD_TVC_SMILEY0_FACE_COLOUR   6u
#define EGG_CD_TVC_SMILEY0_DETAIL_COLOUR 7u
#define EGG_CD_TVC_SMILEY1_FACE_COLOUR   4u
#define EGG_CD_TVC_SMILEY1_DETAIL_COLOUR 7u
#define EGG_CD_TVC_SMILEY_DARK_COLOUR    1u
#define EGG_CD_SMILEY_TRANSPARENT_COLOUR 0u

#define EGG_CD_SMILEY_CENTRE_X 7
#define EGG_CD_SMILEY_CENTRE_Y 7
#define EGG_CD_SMILEY_RADIUS_SQUARED 58
#define EGG_CD_SMILEY_LEFT_EYE_X   5u
#define EGG_CD_SMILEY_RIGHT_EYE_X 10u
#define EGG_CD_SMILEY_EYE_MIN_Y    5u
#define EGG_CD_SMILEY_EYE_MAX_Y    6u
#define EGG_CD_SMILEY_MOUTH_MIN_X   4u
#define EGG_CD_SMILEY_MOUTH_MAX_X  11u
#define EGG_CD_SMILEY_MOUTH_Y      10u
#define EGG_CD_SMILEY_MOUTH_CORNER0_X  3u
#define EGG_CD_SMILEY_MOUTH_CORNER1_X 12u
#define EGG_CD_SMILEY_MOUTH_CORNER_Y   9u

#define EGG_CD_SMILEY_TRANSFORMS \
    (TWODFX_SPRITE_X_DOUBLE | TWODFX_SPRITE_Y_DOUBLE)

void egg_CD_blit_full_start(void);
void egg_CD_blit_full_tick(void);

#endif /* EGG_CD_BLIT_FULL_H */
