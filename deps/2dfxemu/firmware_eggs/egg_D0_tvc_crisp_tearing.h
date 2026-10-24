#ifndef EGG_D0_TVC_CRISP_TEARING_H
#define EGG_D0_TVC_CRISP_TEARING_H

#include "twodfx_api.h"

/*
 * Egg D0 — TVC/CRISP framebuffer tearing test
 * ============================================
 *
 * Start from the TVC with:
 *
 *     OUT 248,208        (208 = D0h)
 *
 * Purpose
 * -------
 * Produces intentionally hostile full-frame changes for testing whether an
 * external TVC capture/display device such as CRISP presents one coherent
 * source frame or a mixture of two successive frames. The registry marks D0
 * as TVC-only.
 *
 * What appears on screen
 * ----------------------
 * Mode 0 runs for 25 seconds and alternates the complete stored TVC frame
 * between dark red and cyan on every 50 Hz update. A mixed capture frame would
 * show a sharp horizontal division between the two colours.
 *
 * Mode 1 then runs for 25 seconds. The frame is black with a full-height white
 * area to the right of a vertical boundary. The boundary moves sixteen raw
 * 2dfx pixels per update and bounces between X=80 and X=688. A mixed capture
 * frame would show different boundary positions above and below a horizontal
 * tear line.
 *
 * The two modes repeat indefinitely.
 *
 * Implementation style
 * --------------------
 * Every frame is described through ordinary public SET_COLOR, FILL_RECT and
 * RET calls. The egg writes only the shadow playlist; normal 2dfx rendering
 * and VSYNC framebuffer publication remain responsible for producing and
 * presenting the frame. It never writes a framebuffer directly and never
 * accesses raw 2DPBs, shadow tables, profile internals or private helpers.
 *
 * All mode, timing, geometry, movement and colour constants live here.
 */

/* ------------------------------------------------------------------------- */
/* Stored TVC framebuffer                                                    */
/* ------------------------------------------------------------------------- */

/** Complete TVC stored render area used by the tearing test. */
#define EGG_D0_FRAMEBUFFER_X       0u
#define EGG_D0_FRAMEBUFFER_Y      41u
#define EGG_D0_FRAMEBUFFER_WIDTH 704u
#define EGG_D0_FRAMEBUFFER_HEIGHT 256u

/* ------------------------------------------------------------------------- */
/* Mode timing                                                               */
/* ------------------------------------------------------------------------- */

/** The test deliberately updates on every 50 Hz TVC frame. */
#define EGG_D0_UPDATES_PER_SECOND 50u

/** Each presentation mode remains active for 25 seconds. */
#define EGG_D0_MODE_SECONDS 25u
#define EGG_D0_MODE_TICKS \
    (EGG_D0_UPDATES_PER_SECOND * EGG_D0_MODE_SECONDS)

/* ------------------------------------------------------------------------- */
/* Moving-boundary geometry                                                  */
/* ------------------------------------------------------------------------- */

/** Boundary movement quantum in raw 2dfx pixels per update. */
#define EGG_D0_BOUNDARY_STEP_PIXELS 16u

/** Left and right turning points of the moving split. */
#define EGG_D0_BOUNDARY_MIN_X  80u
#define EGG_D0_BOUNDARY_MAX_X 688u

/* ------------------------------------------------------------------------- */
/* TVC colours                                                               */
/* ------------------------------------------------------------------------- */

#define EGG_D0_COLOUR_BLACK     0u
#define EGG_D0_COLOUR_DARK_RED  2u
#define EGG_D0_COLOUR_CYAN     13u
#define EGG_D0_COLOUR_WHITE    15u

/** Opaque-black twin kept distinct from every visible test colour. */
#define EGG_D0_TRANSPARENT_COLOUR 1u

/* ------------------------------------------------------------------------- */
/* Presentation modes                                                        */
/* ------------------------------------------------------------------------- */

typedef enum {
    /** Alternate complete dark-red and cyan frames. */
    EGG_D0_MODE_FLASH = 0,

    /** Move a full-height black/white vertical boundary. */
    EGG_D0_MODE_MOVING_BOUNDARY = 1
} egg_D0_mode_t;

void egg_D0_tvc_crisp_tearing_start(void);
void egg_D0_tvc_crisp_tearing_tick(void);

#endif /* EGG_D0_TVC_CRISP_TEARING_H */
