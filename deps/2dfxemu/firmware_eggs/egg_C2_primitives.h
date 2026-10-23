#ifndef EGG_C2_PRIMITIVES_H
#define EGG_C2_PRIMITIVES_H

#include <stdint.h>

#include "twodfx_api.h"

/*
 * Egg C2 — animated 2D primitive showcase
 * =========================================
 *
 * Start from either host computer with:
 *
 *     OUT 248,194        (194 = C2h)
 *
 * Purpose
 * -------
 * Demonstrates the normal 2DPT primitive family without using any sprite or
 * resource asset. The complete scene is rebuilt every tick from direct public
 * C API calls so this file also serves as a readable example of constructing
 * an animated primitive-only playlist.
 *
 * What appears on screen
 * ----------------------
 * - Layered coloured background bands.
 * - Filled and stroked ellipses.
 * - An outer frame and a perspective/grid floor.
 * - A continuously sweeping horizontal line.
 * - Two pulsing elliptical outlines.
 * - A horizontally moving rounded rectangle with a filled ellipse inside it.
 * - A filled rounded panel, ERASE_LINE demonstration, framed bucket-fill box
 *   and live BUCKET_FILL result.
 *
 * Machine geometry
 * ----------------
 * The scene was authored in the original 744x288 Enterprise design space.
 * Both profiles scale that design into their measured BASIC-safe rectangles:
 * Enterprise X=124..763, Y=38..262; TVC uses the corrected 512x240 BASIC-safe
 * beginning at X=126, Y=49. This preserves the composition while keeping the
 * host BASIC border/text area untouched.
 *
 * Implementation style
 * --------------------
 * Every visible command is an explicit public twodfx_2d_* call in
 * egg_C2_primitives.c. There are no wrappers hiding primitive calls, no raw
 * 2DPBs, no command opcodes, no shadow-table access and no 2dfx.h dependency.
 * Only the coordinate-scaling helpers remain because they express the single
 * meaningful cross-profile transformation used throughout the scene.
 *
 * BUCKET_FILL is intentionally included as a functionality demonstration. It
 * is one of the expensive primitives and should be used carefully in normal
 * real-time scenes; a prepared BLIT resource may be better for repeated
 * complex filled artwork.
 */

/* ------------------------------------------------------------------------- */
/* Authoring and target rectangles                                           */
/* ------------------------------------------------------------------------- */

#define EGG_C2_DESIGN_WIDTH  744u
#define EGG_C2_DESIGN_HEIGHT 288u

#define EGG_C2_ENTERPRISE_X      TWODFX_ENTERPRISE_BASIC_SAFE_MIN_X
#define EGG_C2_ENTERPRISE_Y      TWODFX_ENTERPRISE_BASIC_SAFE_MIN_Y
#define EGG_C2_ENTERPRISE_WIDTH \
    (TWODFX_ENTERPRISE_BASIC_SAFE_MAX_X - \
     TWODFX_ENTERPRISE_BASIC_SAFE_MIN_X + 1u)
#define EGG_C2_ENTERPRISE_HEIGHT \
    (TWODFX_ENTERPRISE_BASIC_SAFE_MAX_Y - \
     TWODFX_ENTERPRISE_BASIC_SAFE_MIN_Y + 1u)

#define EGG_C2_TVC_X      TWODFX_TVC_BASIC_SAFE_MIN_X
#define EGG_C2_TVC_Y      TWODFX_TVC_BASIC_SAFE_MIN_Y
#define EGG_C2_TVC_WIDTH \
    (TWODFX_TVC_BASIC_SAFE_MAX_X - TWODFX_TVC_BASIC_SAFE_MIN_X + 1u)
#define EGG_C2_TVC_HEIGHT \
    (TWODFX_TVC_BASIC_SAFE_MAX_Y - TWODFX_TVC_BASIC_SAFE_MIN_Y + 1u)

/* ------------------------------------------------------------------------- */
/* Colour indices                                                            */
/* ------------------------------------------------------------------------- */

#define EGG_C2_COLOUR_STATE_BACKGROUND 0u
#define EGG_C2_BACKGROUND_COLOUR       1u
#define EGG_C2_TOP_BAND_COLOUR         2u
#define EGG_C2_MIDDLE_BAND_COLOUR      3u
#define EGG_C2_GRID_COLOUR_A           3u
#define EGG_C2_GRID_COLOUR_B           4u
#define EGG_C2_FRAME_COLOUR            4u
#define EGG_C2_PULSE_OUTER_COLOUR      5u
#define EGG_C2_FILLED_ELLIPSE_COLOUR   6u
#define EGG_C2_ACCENT_COLOUR           7u
#define EGG_C2_EP_ROUNDED_PANEL_COLOUR 6u
#define EGG_C2_TVC_ROUNDED_PANEL_COLOUR 8u
#define EGG_C2_EP_BUCKET_FILL_COLOUR    4u
#define EGG_C2_TVC_BUCKET_FILL_COLOUR  10u
#define EGG_C2_EP_SMALL_FRAME_COLOUR    7u
#define EGG_C2_TVC_SMALL_FRAME_COLOUR  15u

/* TVC presentation deliberately favours the bright half of its fixed palette. */
#define EGG_C2_TVC_BACKGROUND_COLOUR       1u
#define EGG_C2_TVC_TOP_BAND_COLOUR         3u
#define EGG_C2_TVC_MIDDLE_BAND_COLOUR     11u
#define EGG_C2_TVC_GRID_COLOUR_A           3u
#define EGG_C2_TVC_GRID_COLOUR_B          13u
#define EGG_C2_TVC_FRAME_COLOUR            5u
#define EGG_C2_TVC_PULSE_OUTER_COLOUR     13u
#define EGG_C2_TVC_FILLED_ELLIPSE_COLOUR   7u
#define EGG_C2_TVC_ACCENT_COLOUR          15u
#define EGG_C2_TVC_BLOCK_BACKGROUND_COLOUR 9u

/* ------------------------------------------------------------------------- */
/* Background bands and upper ellipse composition                           */
/* ------------------------------------------------------------------------- */

#define EGG_C2_TOP_BAND_HEIGHT_DESIGN       72u
#define EGG_C2_MIDDLE_BAND_Y_DESIGN         66u
#define EGG_C2_MIDDLE_BAND_HEIGHT_DESIGN    58u
#define EGG_C2_LOWER_BACKGROUND_Y_DESIGN   124u
#define EGG_C2_LOWER_BACKGROUND_H_DESIGN   164u

#define EGG_C2_FILLED_ELLIPSE_X_DESIGN      278u
#define EGG_C2_FILLED_ELLIPSE_Y_DESIGN       34u
#define EGG_C2_FILLED_ELLIPSE_W_DESIGN      188u
#define EGG_C2_FILLED_ELLIPSE_H_DESIGN       86u

#define EGG_C2_OUTER_ELLIPSE_X_DESIGN       268u
#define EGG_C2_OUTER_ELLIPSE_Y_DESIGN        28u
#define EGG_C2_OUTER_ELLIPSE_W_DESIGN       208u
#define EGG_C2_OUTER_ELLIPSE_H_DESIGN        98u
#define EGG_C2_OUTER_ELLIPSE_STROKE           2u

/* ------------------------------------------------------------------------- */
/* Main frame and perspective floor                                          */
/* ------------------------------------------------------------------------- */

#define EGG_C2_HORIZON_Y_DESIGN       140u
#define EGG_C2_HORIZON_STROKE           2u

#define EGG_C2_OUTER_FRAME_INSET_X_DESIGN 18u
#define EGG_C2_OUTER_FRAME_INSET_Y_DESIGN 14u
#define EGG_C2_OUTER_FRAME_STROKE          2u

#define EGG_C2_GRID_Y_VALUES \
    { 162u, 181u, 198u, 213u, 227u, 240u, 252u, 263u, 273u, 282u }
#define EGG_C2_GRID_LINE_COUNT 10u
#define EGG_C2_GRID_LINE_STROKE 1u

#define EGG_C2_VANISH_Y_DESIGN        143u
#define EGG_C2_PERSPECTIVE_SEGMENTS     8u
#define EGG_C2_PERSPECTIVE_LINE_STROKE  1u

/* ------------------------------------------------------------------------- */
/* Animated sweep line                                                       */
/* ------------------------------------------------------------------------- */

#define EGG_C2_SWEEP_SPEED_DESIGN_PIXELS 3u
#define EGG_C2_SWEEP_STROKE              2u

/* ------------------------------------------------------------------------- */
/* Pulsing elliptical outlines                                               */
/* ------------------------------------------------------------------------- */

#define EGG_C2_PULSE_PHASE_MASK      0x3Fu
#define EGG_C2_PULSE_HALF_PERIOD       32u
#define EGG_C2_PULSE_BASE_WIDTH       150u
#define EGG_C2_PULSE_WIDTH_STEP         4u
#define EGG_C2_PULSE_BASE_HEIGHT       60u
#define EGG_C2_PULSE_HEIGHT_STEP        2u
#define EGG_C2_PULSE_BASE_Y             90u
#define EGG_C2_PULSE_Y_DIVISOR           2u
#define EGG_C2_PULSE_OUTER_STROKE        2u
#define EGG_C2_PULSE_INNER_STROKE        1u
#define EGG_C2_PULSE_INNER_INSET_X      40u
#define EGG_C2_PULSE_INNER_INSET_Y      18u
#define EGG_C2_PULSE_INNER_WIDTH_TRIM   80u
#define EGG_C2_PULSE_INNER_HEIGHT_TRIM  32u
#define EGG_C2_PULSE_INNER_MIN_WIDTH    16u
#define EGG_C2_PULSE_INNER_MIN_HEIGHT    8u

/* ------------------------------------------------------------------------- */
/* Moving rounded block                                                      */
/* ------------------------------------------------------------------------- */

#define EGG_C2_BLOCK_START_X_DESIGN     40u
#define EGG_C2_BLOCK_SPEED_DESIGN        5u
#define EGG_C2_BLOCK_RIGHT_MARGIN_DESIGN 160u
#define EGG_C2_BLOCK_Y_DESIGN           226u
#define EGG_C2_BLOCK_WIDTH_DESIGN       112u
#define EGG_C2_BLOCK_HEIGHT_DESIGN       36u
#define EGG_C2_BLOCK_RADIUS_X_DESIGN     18u
#define EGG_C2_BLOCK_RADIUS_Y_DESIGN      8u
#define EGG_C2_BLOCK_STROKE               2u
#define EGG_C2_BLOCK_BACKGROUND_COLOUR    5u

#define EGG_C2_BLOCK_ELLIPSE_X_OFFSET_DESIGN 22u
#define EGG_C2_BLOCK_ELLIPSE_Y_DESIGN       232u
#define EGG_C2_BLOCK_ELLIPSE_W_DESIGN        68u
#define EGG_C2_BLOCK_ELLIPSE_H_DESIGN        24u

/* ------------------------------------------------------------------------- */
/* Lower-right primitive demonstrations                                      */
/* ------------------------------------------------------------------------- */

#define EGG_C2_FILLED_ROUND_X_DESIGN      552u
#define EGG_C2_FILLED_ROUND_Y_DESIGN      174u
#define EGG_C2_FILLED_ROUND_W_DESIGN      128u
#define EGG_C2_FILLED_ROUND_H_DESIGN       48u
#define EGG_C2_FILLED_ROUND_RX_DESIGN      24u
#define EGG_C2_FILLED_ROUND_RY_DESIGN      12u

#define EGG_C2_ERASE_LINE_X0_DESIGN       560u
#define EGG_C2_ERASE_LINE_Y0_DESIGN       198u
#define EGG_C2_ERASE_LINE_X1_DESIGN       670u
#define EGG_C2_ERASE_LINE_Y1_DESIGN       198u
#define EGG_C2_ERASE_LINE_STROKE            2u
#define EGG_C2_ERASE_LINE_BACKGROUND        1u

#define EGG_C2_BUCKET_FRAME_X_DESIGN      590u
#define EGG_C2_BUCKET_FRAME_Y_DESIGN      230u
#define EGG_C2_BUCKET_FRAME_W_DESIGN       72u
#define EGG_C2_BUCKET_FRAME_H_DESIGN       30u
#define EGG_C2_BUCKET_FRAME_STROKE          1u
#define EGG_C2_BUCKET_SEED_X_DESIGN       610u
#define EGG_C2_BUCKET_SEED_Y_DESIGN       244u

void egg_C2_primitives_start(void);
void egg_C2_primitives_tick(void);

#endif /* EGG_C2_PRIMITIVES_H */
