#ifndef EGG_CF_TVC_BITPATTERN_H
#define EGG_CF_TVC_BITPATTERN_H

#include "twodfx_api.h"

/*
 * Egg CF — TVC external-colour bit-pattern timing test
 * ===================================================
 *
 * Start from the TVC with:
 *
 *     OUT 248,207        (207 = CFh)
 *
 * Purpose
 * -------
 * Provides a deliberately sharp, repetitive EC0..EC3 timing pattern for
 * oscilloscope, logic-analyser and display-path checks on the TVC profile.
 * The registry marks this egg as TVC-only, so selecting CFh on Enterprise
 * leaves no egg active.
 *
 * What appears on screen
 * ----------------------
 * Sixteen horizontal bars are shown, one for every four-bit value 0000..1111.
 * Inside each bar the four bits repeat continuously across the line. A zero
 * bit is black and a one bit is white, producing only the two extreme TVC
 * external-colour levels.
 *
 * The 604-pixel-wide timing bitmap begins at X=84 and spans Y=49..288. It is
 * centred vertically inside the stored TVC 704x256 framebuffer and sits
 * inside the measured TVC visible window. The surrounding visible area is
 * filled with opaque black.
 *
 * Implementation style
 * --------------------
 * One packed 302-byte row is generated in MCU RAM for each four-bit pattern
 * and uploaded repeatedly for that bar's fifteen rows. The complete result is
 * then presented by one exact BLIT. The public API remains visible directly
 * in the scene builder; this egg never accesses resource_ram, raw 2DPBs,
 * shadow tables, profile internals or private renderer helpers.
 *
 * All resource, geometry, pattern and colour constants live in this header.
 */

/* ------------------------------------------------------------------------- */
/* Resource allocation                                                       */
/* ------------------------------------------------------------------------- */

/** Bitmap descriptor used by the prebuilt timing image. */
#define EGG_CF_BITMAP_ID 15u

/** Start address of the complete packed timing bitmap in resource RAM. */
#define EGG_CF_BITMAP_ADDRESS 0x5A0000u

/* ------------------------------------------------------------------------- */
/* TVC stored-frame and measured visible geometry                            */
/* ------------------------------------------------------------------------- */

/** Complete stored TVC framebuffer width. */
#define EGG_CF_FRAMEBUFFER_WIDTH 704u

/** First stored TVC scanline and number of stored lines. */
#define EGG_CF_FRAMEBUFFER_Y      41u
#define EGG_CF_FRAMEBUFFER_HEIGHT 256u

/** Measured TVC visible window filled with opaque black behind the bitmap. */
#define EGG_CF_VISIBLE_X      83u
#define EGG_CF_VISIBLE_Y      EGG_CF_FRAMEBUFFER_Y
#define EGG_CF_VISIBLE_WIDTH 605u
#define EGG_CF_VISIBLE_HEIGHT EGG_CF_FRAMEBUFFER_HEIGHT

/* ------------------------------------------------------------------------- */
/* Timing bitmap                                                             */
/* ------------------------------------------------------------------------- */

/** Sixteen bars cover every possible EC0..EC3 value. */
#define EGG_CF_BAR_COUNT 16u

/** Height of each logic-value bar in scanlines. */
#define EGG_CF_BAR_HEIGHT 15u

/** Total bitmap height: 16 patterns x 15 scanlines. */
#define EGG_CF_BITMAP_HEIGHT (EGG_CF_BAR_COUNT * EGG_CF_BAR_HEIGHT)

/** Packed timing image placement and size. */
#define EGG_CF_BITMAP_X     84u
#define EGG_CF_BITMAP_WIDTH 604u
#define EGG_CF_BITMAP_Y \
    (EGG_CF_FRAMEBUFFER_Y + \
     ((EGG_CF_FRAMEBUFFER_HEIGHT - EGG_CF_BITMAP_HEIGHT) / 2u))

/** Two pixels per byte in internal packed 4-bpp format. */
#define EGG_CF_BITMAP_ROW_BYTES (EGG_CF_BITMAP_WIDTH / 2u)
#define EGG_CF_BITMAP_BYTES \
    ((uint32_t)EGG_CF_BITMAP_ROW_BYTES * EGG_CF_BITMAP_HEIGHT)

/** Four successive pixels display bits 3, 2, 1 and 0, then repeat. */
#define EGG_CF_PATTERN_PIXEL_PERIOD 4u
#define EGG_CF_PATTERN_MSB          3u

/* ------------------------------------------------------------------------- */
/* Colours and update mode                                                   */
/* ------------------------------------------------------------------------- */

/** TVC palette extremes used for logic zero and logic one. */
#define EGG_CF_ZERO_COLOUR 0u
#define EGG_CF_ONE_COLOUR 15u

/** Opaque-black twin selected as transparency so colours 0 and 15 are driven. */
#define EGG_CF_TRANSPARENT_COLOUR 1u

void egg_CF_tvc_bitpattern_start(void);
void egg_CF_tvc_bitpattern_tick(void);

#endif /* EGG_CF_TVC_BITPATTERN_H */
