#ifndef EGG_C3_TEXT_H
#define EGG_C3_TEXT_H

#include <stdint.h>

#include "twodfx_api.h"

/*
 * C3h — text and fixed-cell font showcase
 * ========================================
 *
 * Start from the host with:
 *
 *     OUT 248,195
 *
 * The egg demonstrates five simultaneous font descriptors, transparent and
 * filled text cells, and a live counter whose string bytes change without
 * rebuilding the 2DPT. The visible panel contains:
 *
 *   - a 32x16 title;
 *   - normal 16x8, narrow 8x8 and wide 32x8 lines;
 *   - tall 16x16 and big 32x16 lines;
 *   - transparent-background and filled-background text examples;
 *   - a FRAME 0000..9999 counter updated directly in RRAM.
 *
 * Static font artwork is authored in egg_C3_text_fonts.h. The offline tool
 * tools/generate_egg_C3_assets.py expands each sparse source table into a
 * complete 128-glyph bank, compresses every bank as its own forward ZX1
 * stream, and writes egg_C3_text_assets.h. The runtime egg includes only the
 * generated asset header: it never builds fonts in MCU RAM and never accesses
 * private resource-RAM storage.
 *
 * One reusable RRAM staging address is used to install the five separately
 * named font streams in sequence. The screen remains blank during this short
 * asynchronous loading interval. Once all fonts are ready, descriptors and
 * the scene are submitted through the public twodfx_* API. Static strings are
 * small raw const flash assets; only the live counter is generated at runtime.
 */

/* ------------------------------------------------------------------------- */
/* Offline font-loading state                                                */
/* ------------------------------------------------------------------------- */

typedef enum {
    EGG_C3_FONT_LOAD_NARROW = 0,
    EGG_C3_FONT_LOAD_NORMAL,
    EGG_C3_FONT_LOAD_WIDE,
    EGG_C3_FONT_LOAD_TALL,
    EGG_C3_FONT_LOAD_BIG,
    EGG_C3_FONT_LOAD_READY,
    EGG_C3_FONT_LOAD_ERROR
} egg_C3_font_load_state_t;

/* ------------------------------------------------------------------------- */
/* Font descriptors and complete 128-glyph bank geometry                     */
/* ------------------------------------------------------------------------- */

#define EGG_C3_FONT_NARROW_ID 0u  /*  8x8  cell: 1 byte  x  8 rows */
#define EGG_C3_FONT_NORMAL_ID 1u  /* 16x8  cell: 2 bytes x  8 rows */
#define EGG_C3_FONT_WIDE_ID   2u  /* 32x8  cell: 4 bytes x  8 rows */
#define EGG_C3_FONT_TALL_ID   3u  /* 16x16 cell: 2 bytes x 16 rows */
#define EGG_C3_FONT_BIG_ID    4u  /* 32x16 cell: 4 bytes x 16 rows */

#define EGG_C3_FONT_GLYPH_COUNT TWODFX_FONT_GLYPHS

#define EGG_C3_FONT_NARROW_WIDTH_UNITS  1u
#define EGG_C3_FONT_NARROW_HEIGHT_UNITS 1u
#define EGG_C3_FONT_NORMAL_WIDTH_UNITS  2u
#define EGG_C3_FONT_NORMAL_HEIGHT_UNITS 1u
#define EGG_C3_FONT_WIDE_WIDTH_UNITS    4u
#define EGG_C3_FONT_WIDE_HEIGHT_UNITS   1u
#define EGG_C3_FONT_TALL_WIDTH_UNITS    2u
#define EGG_C3_FONT_TALL_HEIGHT_UNITS   2u
#define EGG_C3_FONT_BIG_WIDTH_UNITS     4u
#define EGG_C3_FONT_BIG_HEIGHT_UNITS    2u

#define EGG_C3_FONT_NARROW_BYTES \
    (EGG_C3_FONT_GLYPH_COUNT * EGG_C3_FONT_NARROW_WIDTH_UNITS * 8u)
#define EGG_C3_FONT_NORMAL_BYTES \
    (EGG_C3_FONT_GLYPH_COUNT * EGG_C3_FONT_NORMAL_WIDTH_UNITS * 8u)
#define EGG_C3_FONT_WIDE_BYTES \
    (EGG_C3_FONT_GLYPH_COUNT * EGG_C3_FONT_WIDE_WIDTH_UNITS * 8u)
#define EGG_C3_FONT_TALL_BYTES \
    (EGG_C3_FONT_GLYPH_COUNT * EGG_C3_FONT_TALL_WIDTH_UNITS * 16u)
#define EGG_C3_FONT_BIG_BYTES \
    (EGG_C3_FONT_GLYPH_COUNT * EGG_C3_FONT_BIG_WIDTH_UNITS * 16u)

/* ------------------------------------------------------------------------- */
/* Resource-RAM allocation                                                   */
/* ------------------------------------------------------------------------- */

#define EGG_C3_FONT_NARROW_ADDRESS 0x5C0000u
#define EGG_C3_FONT_NORMAL_ADDRESS 0x5C0400u
#define EGG_C3_FONT_WIDE_ADDRESS   0x5C0C00u
#define EGG_C3_FONT_TALL_ADDRESS   0x5C1C00u
#define EGG_C3_FONT_BIG_ADDRESS    0x5C2C00u

/* Reused for one compressed font stream at a time. */
#define EGG_C3_FONT_ZX1_STAGING_ADDRESS 0x7D0000u

#define EGG_C3_STRING_BASE_ADDRESS    0x5F0000u
#define EGG_C3_STRING_TITLE_ADDRESS   (EGG_C3_STRING_BASE_ADDRESS + 0x000u)
#define EGG_C3_STRING_NORMAL_ADDRESS  (EGG_C3_STRING_BASE_ADDRESS + 0x040u)
#define EGG_C3_STRING_NARROW_ADDRESS  (EGG_C3_STRING_BASE_ADDRESS + 0x080u)
#define EGG_C3_STRING_WIDE_ADDRESS    (EGG_C3_STRING_BASE_ADDRESS + 0x0C0u)
#define EGG_C3_STRING_TALL_ADDRESS    (EGG_C3_STRING_BASE_ADDRESS + 0x100u)
#define EGG_C3_STRING_BIG_ADDRESS     (EGG_C3_STRING_BASE_ADDRESS + 0x140u)
#define EGG_C3_STRING_TRANS_ADDRESS   (EGG_C3_STRING_BASE_ADDRESS + 0x180u)
#define EGG_C3_STRING_FILLED_ADDRESS  (EGG_C3_STRING_BASE_ADDRESS + 0x1C0u)
#define EGG_C3_STRING_COUNTER_ADDRESS (EGG_C3_STRING_BASE_ADDRESS + 0x200u)

/* ------------------------------------------------------------------------- */
/* Human-readable string source used by the offline generator                */
/* ------------------------------------------------------------------------- */

#define EGG_C3_TEXT_TITLE       "TEXT DEMO"
#define EGG_C3_TEXT_NORMAL      "16X8 NORMAL FONT"
#define EGG_C3_TEXT_NARROW      "8X8 NARROW FONT"
#define EGG_C3_TEXT_WIDE        "WIDE 32X8"
#define EGG_C3_TEXT_TALL        "TALL 16X16"
#define EGG_C3_TEXT_BIG         "BIG 32X16"
#define EGG_C3_TEXT_TRANSPARENT "TRANSPARENT BG"
#define EGG_C3_TEXT_FILLED      "FILLED BACKGROUND"
#define EGG_C3_TEXT_COUNTER     "FRAME 0000"

#define EGG_C3_TEXT_TERMINATOR 0x80u
#define EGG_C3_COUNTER_THOUSANDS_INDEX 6u
#define EGG_C3_COUNTER_HUNDREDS_INDEX  7u
#define EGG_C3_COUNTER_TENS_INDEX      8u
#define EGG_C3_COUNTER_ONES_INDEX      9u
#define EGG_C3_COUNTER_MAXIMUM      9999u

/* ------------------------------------------------------------------------- */
/* Enterprise and TVC panel geometry                                         */
/* ------------------------------------------------------------------------- */

#define EGG_C3_ENTERPRISE_PANEL_X TWODFX_ENTERPRISE_BASIC_SAFE_MIN_X
#define EGG_C3_ENTERPRISE_PANEL_Y TWODFX_ENTERPRISE_BASIC_SAFE_MIN_Y
#define EGG_C3_ENTERPRISE_PANEL_W \
    (TWODFX_ENTERPRISE_BASIC_SAFE_MAX_X - TWODFX_ENTERPRISE_BASIC_SAFE_MIN_X + 1u)
#define EGG_C3_ENTERPRISE_PANEL_H \
    (TWODFX_ENTERPRISE_BASIC_SAFE_MAX_Y - TWODFX_ENTERPRISE_BASIC_SAFE_MIN_Y + 1u)
#define EGG_C3_ENTERPRISE_BODY_X  (EGG_C3_ENTERPRISE_PANEL_X + 12u)
#define EGG_C3_ENTERPRISE_TITLE_X \
    (EGG_C3_ENTERPRISE_PANEL_X + (EGG_C3_ENTERPRISE_PANEL_W - 288u) / 2u)
#define EGG_C3_ENTERPRISE_RIGHT_X \
    (EGG_C3_ENTERPRISE_PANEL_X + EGG_C3_ENTERPRISE_PANEL_W - 1u)

#define EGG_C3_TVC_PANEL_X TWODFX_TVC_BASIC_SAFE_MIN_X
#define EGG_C3_TVC_PANEL_Y TWODFX_TVC_BASIC_SAFE_MIN_Y
#define EGG_C3_TVC_PANEL_W \
    (TWODFX_TVC_BASIC_SAFE_MAX_X - TWODFX_TVC_BASIC_SAFE_MIN_X + 1u)
#define EGG_C3_TVC_PANEL_H \
    (TWODFX_TVC_BASIC_SAFE_MAX_Y - TWODFX_TVC_BASIC_SAFE_MIN_Y + 1u)
#define EGG_C3_TVC_BODY_X  (EGG_C3_TVC_PANEL_X + 12u)
#define EGG_C3_TVC_TITLE_X \
    (EGG_C3_TVC_PANEL_X + (EGG_C3_TVC_PANEL_W - 288u) / 2u)
#define EGG_C3_TVC_RIGHT_X \
    (EGG_C3_TVC_PANEL_X + EGG_C3_TVC_PANEL_W - 1u)

#define EGG_C3_SEPARATOR_Y 76u
#define EGG_C3_TITLE_Y      54u
#define EGG_C3_NORMAL_Y     86u
#define EGG_C3_NARROW_Y    102u
#define EGG_C3_WIDE_Y      118u
#define EGG_C3_TALL_Y      142u
#define EGG_C3_BIG_Y       170u
#define EGG_C3_TRANS_Y     202u
#define EGG_C3_FILLED_Y    222u
#define EGG_C3_COUNTER_Y   244u

#define EGG_C3_PANEL_BORDER_STROKE 2u
#define EGG_C3_SEPARATOR_STROKE    1u

/* ------------------------------------------------------------------------- */
/* Scene colours                                                             */
/* ------------------------------------------------------------------------- */

#define EGG_C3_PANEL_FILL_COLOR       8u
#define EGG_C3_PANEL_BORDER_COLOR     4u
#define EGG_C3_TITLE_COLOR            7u
#define EGG_C3_NORMAL_COLOR           6u
#define EGG_C3_NARROW_COLOR           5u
#define EGG_C3_EP_WIDE_COLOR          5u
#define EGG_C3_TVC_WIDE_COLOR        13u
#define EGG_C3_TALL_COLOR             4u
#define EGG_C3_BIG_COLOR              7u
#define EGG_C3_TRANSPARENT_TEXT_COLOR 6u
#define EGG_C3_FILLED_TEXT_COLOR      7u
#define EGG_C3_FILLED_BACKGROUND      2u
#define EGG_C3_COUNTER_COLOR          5u
#define EGG_C3_TRANSPARENT_BACKGROUND 0u

/* TVC demo palette: intentionally bright and varied. */
#define EGG_C3_TVC_PANEL_FILL_COLOR        1u
#define EGG_C3_TVC_PANEL_BORDER_COLOR     13u
#define EGG_C3_TVC_TITLE_COLOR             7u
#define EGG_C3_TVC_NORMAL_COLOR           15u
#define EGG_C3_TVC_NARROW_COLOR            5u
#define EGG_C3_TVC_TALL_COLOR             11u
#define EGG_C3_TVC_BIG_COLOR               3u
#define EGG_C3_TVC_TRANSPARENT_TEXT_COLOR  9u
#define EGG_C3_TVC_FILLED_TEXT_COLOR      15u
#define EGG_C3_TVC_FILLED_BACKGROUND       3u
#define EGG_C3_TVC_COUNTER_COLOR          13u

void egg_C3_text_start(void);
void egg_C3_text_tick(void);

#endif /* EGG_C3_TEXT_H */
