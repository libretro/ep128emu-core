#ifndef EGG_DF_GRAPHICS_TEST_H
#define EGG_DF_GRAPHICS_TEST_H

#include "twodfx_api.h"

/*
 * Egg DF — eight-phase graphics validation (PATTERN_BUCKET_FILL, BLIT, clipping)
 * ========================================
 *
 * Start from the host computer with:
 *
 *     OUT 248,223        (223 = DFh)
 *
 * Purpose
 * -------
 * Hardware validation for the experimental 74h/F4h PATTERN_BUCKET_FILL
 * command.  The test deliberately uses patterns that contain the original
 * target colour.  That is the case which breaks colour-change-only flood-fill
 * algorithms and is the reason this implementation keeps an independent
 * visited bitmap.
 *
 * Two phase invariants are exercised at once:
 *
 * 1. the fill seed moves inside each enclosed region on every update; changing
 *    the seed must not change pattern alignment;
 * 2. an animated SET_XY_OFFSET moves both enclosed shapes while the dither
 *    pattern remains locked to absolute screen coordinates. The outlines move
 *    across a visually stationary pattern field, so a one-pixel movement
 *    naturally changes ABAB... at the edge to BABA... rather than shifting the
 *    entire dither field.
 *
 * Phases 1–5: legacy pattern-fill and four bitmap phases.
 * Phases 6–8: the same graphical workload over maximum visible raster,
 *   unclipped, fixed screen-anchored clip, and 5s cyclic expanding/shrinking
 *   clip respectively. Each phase lasts 20s, eight 17x5 foreground markers.
 *
 * Any whole-pattern crawl/flicker, holes or runaway fill indicates a bug.
 */

#define EGG_DF_PATTERN_BANK_ADDRESS 0x5A0000u
#define EGG_DF_PATTERN_DESCRIPTOR   1u
#define EGG_DF_PATTERN_COUNT        2u

#define EGG_DF_OFFSET_X_PERIOD      96u
#define EGG_DF_OFFSET_Y_PERIOD      124u
#define EGG_DF_OFFSET_X_AMPLITUDE   10
#define EGG_DF_OFFSET_Y_AMPLITUDE   5

void egg_DF_graphics_test_start(void);
void egg_DF_graphics_test_tick(void);

#endif /* EGG_DF_GRAPHICS_TEST_H */
