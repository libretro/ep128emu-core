#ifndef EGG_D1_RULER_H
#define EGG_D1_RULER_H

#include <stdint.h>

#include "twodfx_api.h"

/*
 * Egg D1 — cross-profile visible-area coordinate probe
 * =====================================================
 *
 * Start from either host computer with:
 *
 *     OUT 248,209        (209 = D1h)
 *
 * Purpose
 * -------
 * Provides an exact, animated ruler for measuring where raw 2dfx coordinates
 * first become visible and where they disappear on real Enterprise or TVC
 * hardware. It was designed for practical display calibration rather than as
 * a decorative demo.
 *
 * What appears on screen
 * ----------------------
 * A one-pixel bright probe line with red neighbour lines sweeps through the
 * configured raw measurement range while a central seven-segment display
 * shows the exact coordinate of the bright line. On Enterprise the X sweep
 * extends
 * deliberately to X=820, beyond the measured NICK-visible right edge at 811,
 * so the disappearance point remains easy to identify.
 *
 * Sequence
 * --------
 * 1. Vertical line: X=0 to the profile's configured final probe X.
 * 2. Endpoint pause.
 * 3. Vertical line: final probe X back to X=0.
 * 4. Endpoint pause.
 * 5. Horizontal line: Y=0 to the profile's final pre-wrap scanline.
 * 6. Endpoint pause.
 * 7. Horizontal line: final Y back to Y=0.
 * 8. Endpoint pause, then repeat forever.
 *
 * The red neighbours are only visibility aids. The number always identifies
 * the central bright coordinate exactly. The box border is cyan during X
 * sweeps and yellow during Y sweeps, with a matching axis marker at its left.
 *
 * Implementation style
 * --------------------
 * Every frame is rebuilt using direct public SET_COLOR, FILL_RECT and RET
 * calls. Machine-specific stored geometry is selected through
 * twodfx_get_machine(); the egg never accesses private profiles, raw 2DPBs,
 * shadow tables, renderer internals or 2dfx.h.
 *
 * All geometry, timing, colours, digit masks and state definitions live here.
 */

/* ------------------------------------------------------------------------- */
/* Configured raw geometry swept by the probe                                */
/* ------------------------------------------------------------------------- */

#define EGG_D1_ENTERPRISE_MAX_X             820u
#define EGG_D1_ENTERPRISE_FRAMEBUFFER_WIDTH \
    (EGG_D1_ENTERPRISE_MAX_X + 1u)
#define EGG_D1_ENTERPRISE_MAX_Y             314u
#define EGG_D1_ENTERPRISE_FIRST_VISIBLE_Y    18u
#define EGG_D1_ENTERPRISE_VISIBLE_LINES     287u

#define EGG_D1_TVC_FRAMEBUFFER_WIDTH        704u
#define EGG_D1_TVC_MAX_Y                    311u
#define EGG_D1_TVC_FIRST_VISIBLE_Y           41u
#define EGG_D1_TVC_VISIBLE_LINES            256u

/* ------------------------------------------------------------------------- */
/* Sweep timing                                                              */
/* ------------------------------------------------------------------------- */

/** Hold each coordinate for four 50 Hz updates so it can be read reliably. */
#define EGG_D1_FRAMES_PER_PIXEL 4u

/** Two-second pause at every forward/reverse endpoint. */
#define EGG_D1_PAUSE_TICKS 100u

/* ------------------------------------------------------------------------- */
/* Colours                                                                   */
/* ------------------------------------------------------------------------- */

#define EGG_D1_BACKGROUND_COLOUR             0u
#define EGG_D1_COLOUR_STATE_BACKGROUND       0u
#define EGG_D1_BOX_FILL_COLOUR               0u
#define EGG_D1_TRANSPARENT_COLOUR            0u

/* Enterprise uses only the fixed EXOS/IS-BASIC palette indices 0..7. */
#define EGG_D1_EP_BOX_BORDER_X_COLOUR         6u
#define EGG_D1_EP_BOX_BORDER_Y_COLOUR         3u
#define EGG_D1_EP_DIGIT_COLOUR                7u
#define EGG_D1_EP_NEIGHBOUR_LINE_COLOUR       2u
#define EGG_D1_EP_CORE_LINE_COLOUR            7u

/* TVC retains the already hardware-proven fixed 16-colour presentation. */
#define EGG_D1_TVC_BOX_BORDER_X_COLOUR       13u
#define EGG_D1_TVC_BOX_BORDER_Y_COLOUR        7u
#define EGG_D1_TVC_DIGIT_COLOUR              15u
#define EGG_D1_TVC_NEIGHBOUR_LINE_COLOUR      3u
#define EGG_D1_TVC_CORE_LINE_COLOUR          15u

/* ------------------------------------------------------------------------- */
/* Seven-segment coordinate display                                          */
/* ------------------------------------------------------------------------- */

#define EGG_D1_SEGMENT_THICKNESS 2u
#define EGG_D1_DIGIT_WIDTH      10u
#define EGG_D1_DIGIT_HEIGHT     18u
#define EGG_D1_DIGIT_ADVANCE    14u
#define EGG_D1_DIGIT_COUNT       4u

#define EGG_D1_SEGMENT_A 0x01u
#define EGG_D1_SEGMENT_B 0x02u
#define EGG_D1_SEGMENT_C 0x04u
#define EGG_D1_SEGMENT_D 0x08u
#define EGG_D1_SEGMENT_E 0x10u
#define EGG_D1_SEGMENT_F 0x20u
#define EGG_D1_SEGMENT_G 0x40u

#define EGG_D1_DIGIT_0_SEGMENTS 0x3Fu
#define EGG_D1_DIGIT_1_SEGMENTS 0x06u
#define EGG_D1_DIGIT_2_SEGMENTS 0x5Bu
#define EGG_D1_DIGIT_3_SEGMENTS 0x4Fu
#define EGG_D1_DIGIT_4_SEGMENTS 0x66u
#define EGG_D1_DIGIT_5_SEGMENTS 0x6Du
#define EGG_D1_DIGIT_6_SEGMENTS 0x7Du
#define EGG_D1_DIGIT_7_SEGMENTS 0x07u
#define EGG_D1_DIGIT_8_SEGMENTS 0x7Fu
#define EGG_D1_DIGIT_9_SEGMENTS 0x6Fu

/* Ordered lookup table used by the seven-segment digit helper. */
#define EGG_D1_DIGIT_SEGMENT_VALUES \
    { \
        EGG_D1_DIGIT_0_SEGMENTS, EGG_D1_DIGIT_1_SEGMENTS, \
        EGG_D1_DIGIT_2_SEGMENTS, EGG_D1_DIGIT_3_SEGMENTS, \
        EGG_D1_DIGIT_4_SEGMENTS, EGG_D1_DIGIT_5_SEGMENTS, \
        EGG_D1_DIGIT_6_SEGMENTS, EGG_D1_DIGIT_7_SEGMENTS, \
        EGG_D1_DIGIT_8_SEGMENTS, EGG_D1_DIGIT_9_SEGMENTS \
    }

/* ------------------------------------------------------------------------- */
/* Coordinate box                                                           */
/* ------------------------------------------------------------------------- */

#define EGG_D1_BOX_PADDING_X     8u
#define EGG_D1_BOX_PADDING_Y     6u
#define EGG_D1_BOX_AXIS_MARKER_W 6u
#define EGG_D1_BOX_DIGIT_GAP     4u
#define EGG_D1_BOX_BORDER        1u

#define EGG_D1_BOX_WIDTH \
    ((EGG_D1_BOX_PADDING_X * 2u) + EGG_D1_BOX_AXIS_MARKER_W + \
     EGG_D1_BOX_DIGIT_GAP + (EGG_D1_DIGIT_COUNT * EGG_D1_DIGIT_ADVANCE))
#define EGG_D1_BOX_HEIGHT \
    ((EGG_D1_BOX_PADDING_Y * 2u) + EGG_D1_DIGIT_HEIGHT)

/* ------------------------------------------------------------------------- */
/* Sweep state machine                                                       */
/* ------------------------------------------------------------------------- */

typedef enum {
    EGG_D1_MODE_X_FORWARD = 0,
    EGG_D1_MODE_X_REVERSE,
    EGG_D1_MODE_Y_FORWARD,
    EGG_D1_MODE_Y_REVERSE,
    EGG_D1_MODE_PAUSE_AFTER_X_FORWARD,
    EGG_D1_MODE_PAUSE_AFTER_X_REVERSE,
    EGG_D1_MODE_PAUSE_AFTER_Y_FORWARD,
    EGG_D1_MODE_PAUSE_AFTER_Y_REVERSE
} egg_D1_mode_t;

void egg_D1_ruler_start(void);
void egg_D1_ruler_tick(void);

#endif /* EGG_D1_RULER_H */
