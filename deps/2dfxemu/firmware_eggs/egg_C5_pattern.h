#ifndef EGG_C5_PATTERN_H
#define EGG_C5_PATTERN_H

#include "twodfx_api.h"

/*
 * Egg C5 — PATTERN_FILL showcase
 * =================================
 *
 * Start from the host computer with:
 *
 *     OUT 248,197        (197 = C5h)
 *
 * Purpose
 * -------
 * Demonstrates the public C API for:
 *
 * - uploading generated 16x8 packed-pixel patterns to RRAM;
 * - defining a pattern bank;
 * - filling rectangles with repeated patterns;
 * - combining PATTERN_FILL with ordinary colour and rectangle commands;
 * - rebuilding an animated background-layer playlist every update.
 *
 * What appears on screen
 * ----------------------
 * A large framed pattern area contains two wide pattern panels, three smaller
 * panels, an animated ticker strip and a moving solid highlight. The complete
 * background pattern changes slowly, while the ticker changes more quickly.
 *
 * The TVC layout is deliberately placed inside the measured VT-BASIC-safe
 * region. Its bottom edge leaves two complete 10-pixel VT-BASIC text rows
 * visible below the egg.
 *
 * Implementation style
 * --------------------
 * The .c file calls the public twodfx_* API directly. Small local helpers are
 * used only to generate packed pattern pixels; they do not hide public API
 * calls. All tunable resource, layout, animation and colour constants live in
 * this header.
 */

/* ------------------------------------------------------------------------- */
/* Resource allocation and descriptor                                        */
/* ------------------------------------------------------------------------- */

/** RRAM address of the first generated 16x8 pattern. */
#define EGG_C5_PATTERN_BANK_ADDRESS 0x590000u

/** Pattern descriptor used by every PATTERN_FILL command in this egg. */
#define EGG_C5_PATTERN_DESCRIPTOR   0u

/** Number of consecutive generated patterns in the bank. */
#define EGG_C5_PATTERN_COUNT        8u

/** Mask used by the animation counters to select one of eight patterns. */
#define EGG_C5_PATTERN_NUMBER_MASK  7u

/* Pattern numbers and their intended visual character. */
#define EGG_C5_PATTERN_DIAGONAL_BANDS  0u
#define EGG_C5_PATTERN_BLOCK_CHECKER   1u
#define EGG_C5_PATTERN_DOTS            2u
#define EGG_C5_PATTERN_BRICKS          3u
#define EGG_C5_PATTERN_VERTICAL_RAILS  4u
#define EGG_C5_PATTERN_CROSSHATCH      5u
#define EGG_C5_PATTERN_TEXTURE         6u
#define EGG_C5_PATTERN_BORDER          7u

/* ------------------------------------------------------------------------- */
/* Outer showcase rectangles                                                 */
/* ------------------------------------------------------------------------- */

/** Enterprise outer rectangle: top-left position and complete dimensions. */
#define EGG_C5_EP_X      TWODFX_ENTERPRISE_BASIC_SAFE_MIN_X
#define EGG_C5_EP_Y      TWODFX_ENTERPRISE_BASIC_SAFE_MIN_Y
/*
 * Pattern destinations align X and width down to even. Use the even 640-pixel
 * full corrected X=124..763 safe range so pattern panels and their stroked
 * frames share exact boundaries.
 */
#define EGG_C5_EP_WIDTH 640u
#define EGG_C5_EP_HEIGHT \
    (TWODFX_ENTERPRISE_BASIC_SAFE_MAX_Y - \
     TWODFX_ENTERPRISE_BASIC_SAFE_MIN_Y + 1u)

/*
 * TVC/VT-BASIC outer rectangle.
 *
 * X/Y start at the corrected VT-BASIC-safe top-left corner. Height 216 ends
 * at Y=264, leaving Y=265..288 free at the bottom for a deliberately open
 * lower margin while the upper content is moved closer to the safe-area top.
 */
#define EGG_C5_TVC_X      TWODFX_TVC_BASIC_SAFE_MIN_X
#define EGG_C5_TVC_Y      TWODFX_TVC_BASIC_SAFE_MIN_Y
#define EGG_C5_TVC_WIDTH \
    (TWODFX_TVC_BASIC_SAFE_MAX_X - TWODFX_TVC_BASIC_SAFE_MIN_X + 1u)
#define EGG_C5_TVC_HEIGHT 216u

/* ------------------------------------------------------------------------- */
/* Shared panel layout, relative to the outer rectangle                      */
/* ------------------------------------------------------------------------- */

#define EGG_C5_PANEL_INSET_X       16u  /**< Left/right padding inside frame. */
#define EGG_C5_TOP_Y_OFFSET        16u  /**< First row of two large panels. */
#define EGG_C5_TOP_PANEL_HEIGHT    72u
#define EGG_C5_MIDDLE_Y_OFFSET    112u  /**< Second row of three panels. */
#define EGG_C5_MIDDLE_PANEL_HEIGHT 64u
#define EGG_C5_TICKER_Y_OFFSET    192u  /**< Animated strip at the bottom. */
#define EGG_C5_TICKER_HEIGHT       16u
#define EGG_C5_HIGHLIGHT_WIDTH     16u  /**< Moving solid marker width. */
#define EGG_C5_HIGHLIGHT_HEIGHT    16u

/* TVC panel widths and relative X positions. */
#define EGG_C5_TVC_TOP_Y_OFFSET               8u
#define EGG_C5_TVC_MIDDLE_Y_OFFSET          104u
#define EGG_C5_TVC_TICKER_Y_OFFSET          184u
#define EGG_C5_TVC_TOP_PANEL_WIDTH          232u
#define EGG_C5_TVC_TOP_LEFT_X_OFFSET         16u
#define EGG_C5_TVC_TOP_RIGHT_X_OFFSET       264u
#define EGG_C5_TVC_MIDDLE_PANEL_WIDTH       108u
#define EGG_C5_TVC_MIDDLE_X0_OFFSET          16u
#define EGG_C5_TVC_MIDDLE_X1_OFFSET         136u
#define EGG_C5_TVC_MIDDLE_X2_OFFSET         256u
#define EGG_C5_TVC_MIDDLE_X3_OFFSET         376u
#define EGG_C5_TVC_TICKER_WIDTH             480u
#define EGG_C5_TVC_HIGHLIGHT_TRAVEL         464u

/* Enterprise panel widths and relative X positions. */
#define EGG_C5_EP_TOP_PANEL_WIDTH          296u
#define EGG_C5_EP_TOP_LEFT_X_OFFSET         16u
#define EGG_C5_EP_TOP_RIGHT_X_OFFSET       328u
#define EGG_C5_EP_MIDDLE_PANEL_WIDTH       186u
#define EGG_C5_EP_MIDDLE_X0_OFFSET          16u
#define EGG_C5_EP_MIDDLE_X1_OFFSET         226u
#define EGG_C5_EP_MIDDLE_X2_OFFSET         438u
#define EGG_C5_EP_TICKER_WIDTH             608u
#define EGG_C5_EP_HIGHLIGHT_TRAVEL         592u

/* ------------------------------------------------------------------------- */
/* Animation and frame appearance                                            */
/* ------------------------------------------------------------------------- */

/** Change the full-area background pattern every 32 update ticks. */
#define EGG_C5_BACKGROUND_PATTERN_SHIFT 5u

/** Change the ticker pattern every 8 update ticks. */
#define EGG_C5_TICKER_PATTERN_SHIFT     3u

/** Horizontal movement of the solid ticker highlight per update tick. */
#define EGG_C5_HIGHLIGHT_STEP_X         4u

/** Foreground colour used for the outer frame and moving highlight. */
#define EGG_C5_FRAME_COLOUR             7u

/** Foreground colour used for the five inner panel frames. */
#define EGG_C5_PANEL_FRAME_COLOUR       4u
#define EGG_C5_TVC_FRAME_COLOUR        15u
#define EGG_C5_TVC_PANEL_FRAME_COLOUR  13u

/** Transparent background colour used by stroked rectangle commands. */
#define EGG_C5_FRAME_BACKGROUND_COLOUR  0u

#define EGG_C5_OUTER_FRAME_STROKE       2u
#define EGG_C5_PANEL_FRAME_STROKE       1u

void egg_C5_pattern_start(void);
void egg_C5_pattern_tick(void);

#endif /* EGG_C5_PATTERN_H */
