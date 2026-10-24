#ifndef EGG_C4_BLIT_H
#define EGG_C4_BLIT_H

#include <stdint.h>

#include "twodfx_api.h"

/*
 * C4h — BLIT exact/transparent showcase
 * =====================================
 *
 * Start from the host with:
 *
 *     OUT 248,196
 *
 * The egg demonstrates four separately stored packed 4-bpp bitmap descriptors
 * with both BLIT modes:
 *
 *   - EXACT copies every source pixel, including transparent index 0;
 *   - TRANSPARENT preserves the destination wherever the source pixel is 0.
 *
 * The displayed copies sit over a filled stage and crossing background lines,
 * making the difference between the two modes easy to inspect. A final copy
 * moves horizontally near the bottom, while a small marker travels along the
 * stage edge.
 *
 * The four badges are deterministic animation artwork generated offline by
 * tools/generate_egg_C4_assets.py. Enterprise and TVC each have a separately
 * named ZX1 animation bank per object because their colour numbering and
 * transparent H detail are different. Each bank contains 64 precomputed
 * frames. The four objects keep phase offsets 0/16/32/48 while all diagonal
 * bands move continuously at runtime. No complete image is generated or
 * rewritten in MCU RAM.
 *
 * A cyan rectangle is part of every bitmap itself and occupies its outermost
 * pixels. Therefore every visible BLIT copy clearly shows the exact 128x64
 * object boundary, without relying on a separate RECT primitive around the
 * destination. Transparent corners and the profile-specific centre cuts remain
 * inside this cyan boundary.
 */

/* ------------------------------------------------------------------------- */
/* Offline asset-loading state                                               */
/* ------------------------------------------------------------------------- */

typedef enum {
    EGG_C4_ASSET_LOAD_OBJECT0 = 0,
    EGG_C4_ASSET_LOAD_OBJECT1,
    EGG_C4_ASSET_LOAD_OBJECT2,
    EGG_C4_ASSET_LOAD_OBJECT3,
    EGG_C4_ASSET_READY,
    EGG_C4_ASSET_ERROR
} egg_C4_asset_load_state_t;

/* ------------------------------------------------------------------------- */
/* Bitmap descriptor and RRAM allocation                                     */
/* ------------------------------------------------------------------------- */

#define EGG_C4_OBJECT_COUNT 4u

#define EGG_C4_OBJECT0_BITMAP_ID 0u
#define EGG_C4_OBJECT1_BITMAP_ID 1u
#define EGG_C4_OBJECT2_BITMAP_ID 2u
#define EGG_C4_OBJECT3_BITMAP_ID 3u

#define EGG_C4_BITMAP_WIDTH          128u
#define EGG_C4_BITMAP_HEIGHT          64u
#define EGG_C4_BITMAP_BYTES_PER_ROW  (EGG_C4_BITMAP_WIDTH / 2u)
#define EGG_C4_BITMAP_BYTES \
    (EGG_C4_BITMAP_BYTES_PER_ROW * EGG_C4_BITMAP_HEIGHT)

/* One bank contains every possible 6-bit diagonal phase. */
#define EGG_C4_ANIMATION_FRAME_COUNT 64u
#define EGG_C4_ANIMATION_FRAME_MASK  (EGG_C4_ANIMATION_FRAME_COUNT - 1u)
#define EGG_C4_ANIMATION_TICK_SHIFT  1u
#define EGG_C4_ANIMATION_BANK_BYTES (EGG_C4_BITMAP_BYTES * EGG_C4_ANIMATION_FRAME_COUNT)

/* Four independent 256 KiB banks occupy a contiguous 1 MiB RRAM window. */
#define EGG_C4_OBJECT0_BANK_ADDRESS 0x550000u
#define EGG_C4_OBJECT1_BANK_ADDRESS \
    (EGG_C4_OBJECT0_BANK_ADDRESS + EGG_C4_ANIMATION_BANK_BYTES)
#define EGG_C4_OBJECT2_BANK_ADDRESS \
    (EGG_C4_OBJECT1_BANK_ADDRESS + EGG_C4_ANIMATION_BANK_BYTES)
#define EGG_C4_OBJECT3_BANK_ADDRESS \
    (EGG_C4_OBJECT2_BANK_ADDRESS + EGG_C4_ANIMATION_BANK_BYTES)

/* One compressed bank is uploaded here, decompressed, then overwritten by
 * the next bank. The generator verifies every stream fits this staging area. */
#define EGG_C4_ZX1_STAGING_ADDRESS 0x7D8000u
#define EGG_C4_ZX1_STAGING_CAPACITY (TWODFX_RRAM_SIZE_BYTES - EGG_C4_ZX1_STAGING_ADDRESS)

#define EGG_C4_TRANSPARENT_COLOR 0u

/* ------------------------------------------------------------------------- */
/* Offline badge-artwork design                                              */
/* ------------------------------------------------------------------------- */

#define EGG_C4_BORDER_X_THICKNESS 2u
#define EGG_C4_BORDER_Y_THICKNESS 2u
#define EGG_C4_ENTERPRISE_BORDER_CYAN  6u
#define EGG_C4_TVC_BORDER_CYAN        13u

#define EGG_C4_BADGE_CENTRE_X 63
#define EGG_C4_BADGE_CENTRE_Y 31
#define EGG_C4_BADGE_ELLIPSE_LIMIT 9200

#define EGG_C4_BAND0_END_Y 12u
#define EGG_C4_BAND1_END_Y 24u
#define EGG_C4_BAND2_END_Y 40u
#define EGG_C4_BAND3_END_Y 52u
#define EGG_C4_BAND0_COLOR  4u
#define EGG_C4_BAND1_COLOR  3u
#define EGG_C4_BAND2_COLOR  5u
#define EGG_C4_BAND3_COLOR  6u
#define EGG_C4_BAND4_COLOR  2u

/* Brighter TVC-only badge bands; Enterprise keeps the established artwork. */
#define EGG_C4_TVC_BAND0_COLOR  5u
#define EGG_C4_TVC_BAND1_COLOR  3u
#define EGG_C4_TVC_BAND2_COLOR 13u
#define EGG_C4_TVC_BAND3_COLOR  7u
#define EGG_C4_TVC_BAND4_COLOR 11u

/* Four animation banks retain different diagonal-band phase offsets. */
#define EGG_C4_OBJECT0_DIAGONAL_PHASE  0u
#define EGG_C4_OBJECT1_DIAGONAL_PHASE 16u
#define EGG_C4_OBJECT2_DIAGONAL_PHASE 32u
#define EGG_C4_OBJECT3_DIAGONAL_PHASE 48u
#define EGG_C4_DIAGONAL_MASK       0x3Fu
#define EGG_C4_DIAGONAL_FIRST      26u
#define EGG_C4_DIAGONAL_LAST       34u
#define EGG_C4_ENTERPRISE_DIAGONAL_COLOR 7u
#define EGG_C4_TVC_DIAGONAL_COLOR0       14u
#define EGG_C4_TVC_DIAGONAL_COLOR1       15u

#define EGG_C4_H_LEFT_X0   18u
#define EGG_C4_H_LEFT_X1   19u
#define EGG_C4_H_RIGHT_X0 108u
#define EGG_C4_H_RIGHT_X1 109u
#define EGG_C4_H_VERTICAL_MIN_Y 13u
#define EGG_C4_H_VERTICAL_MAX_Y 51u
#define EGG_C4_H_ROW0 31u
#define EGG_C4_H_ROW1 32u
#define EGG_C4_H_HORIZONTAL_MIN_X 15u
#define EGG_C4_H_HORIZONTAL_MAX_X 113u
#define EGG_C4_ENTERPRISE_H_COLOR 1u
#define EGG_C4_TVC_H_COLOR EGG_C4_TRANSPARENT_COLOR

#define EGG_C4_CHECKER_ORIGIN_X 46u
#define EGG_C4_CHECKER_ORIGIN_Y 22u
#define EGG_C4_CHECKER_MIN_X 47u
#define EGG_C4_CHECKER_MAX_X 81u
#define EGG_C4_CHECKER_MIN_Y 23u
#define EGG_C4_CHECKER_MAX_Y 41u
#define EGG_C4_CHECKER_CELL_W 6u
#define EGG_C4_CHECKER_CELL_H 4u
#define EGG_C4_CHECKER_COLOR  7u

/* ------------------------------------------------------------------------- */
/* Enterprise and TVC stage geometry                                         */
/* ------------------------------------------------------------------------- */

#define EGG_C4_ENTERPRISE_STAGE_X TWODFX_ENTERPRISE_BASIC_SAFE_MIN_X
#define EGG_C4_ENTERPRISE_STAGE_Y TWODFX_ENTERPRISE_BASIC_SAFE_MIN_Y
#define EGG_C4_ENTERPRISE_STAGE_W \
    (TWODFX_ENTERPRISE_BASIC_SAFE_MAX_X - \
     TWODFX_ENTERPRISE_BASIC_SAFE_MIN_X + 1u)
#define EGG_C4_ENTERPRISE_STAGE_H \
    (TWODFX_ENTERPRISE_BASIC_SAFE_MAX_Y - \
     TWODFX_ENTERPRISE_BASIC_SAFE_MIN_Y + 1u)

#define EGG_C4_TVC_STAGE_X TWODFX_TVC_BASIC_SAFE_MIN_X
#define EGG_C4_TVC_STAGE_Y TWODFX_TVC_BASIC_SAFE_MIN_Y
#define EGG_C4_TVC_STAGE_W \
    (TWODFX_TVC_BASIC_SAFE_MAX_X - TWODFX_TVC_BASIC_SAFE_MIN_X + 1u)
#define EGG_C4_TVC_STAGE_H \
    (TWODFX_TVC_BASIC_SAFE_MAX_Y - TWODFX_TVC_BASIC_SAFE_MIN_Y + 1u)

/* ------------------------------------------------------------------------- */
/* Static copy positions                                                     */
/* ------------------------------------------------------------------------- */

#define EGG_C4_TVC_COLUMN0_OFFSET  16u
#define EGG_C4_TVC_COLUMN1_OFFSET 192u
#define EGG_C4_TVC_COLUMN2_OFFSET 368u
#define EGG_C4_TVC_EXACT_ROW_OFFSET       18u
#define EGG_C4_TVC_TRANSPARENT_ROW_OFFSET 88u
#define EGG_C4_TVC_MOVING_ROW_OFFSET     158u

#define EGG_C4_ENTERPRISE_EXACT0_X_OFFSET  26u
#define EGG_C4_ENTERPRISE_EXACT1_X_OFFSET 180u
#define EGG_C4_ENTERPRISE_EXACT2_X_OFFSET 334u
#define EGG_C4_ENTERPRISE_EXACT3_X_OFFSET 488u
#define EGG_C4_ENTERPRISE_EXACT_ROW_OFFSET 10u

#define EGG_C4_ENTERPRISE_TRANS0_X_OFFSET  64u
#define EGG_C4_ENTERPRISE_TRANS1_X_OFFSET 256u
#define EGG_C4_ENTERPRISE_TRANS2_X_OFFSET 448u
#define EGG_C4_ENTERPRISE_TRANS_ROW_OFFSET 84u
#define EGG_C4_ENTERPRISE_MOVING_ROW_OFFSET 154u

/* ------------------------------------------------------------------------- */
/* Stage decoration and motion                                               */
/* ------------------------------------------------------------------------- */

#define EGG_C4_STAGE_FILL_COLOR       8u
#define EGG_C4_STAGE_FILL_BACKGROUND  2u
#define EGG_C4_STAGE_BORDER_COLOR     4u
#define EGG_C4_STAGE_BORDER_STROKE    2u
#define EGG_C4_STAGE_LINE_COLOR       5u
#define EGG_C4_STAGE_LINE_COUNT       8u
#define EGG_C4_STAGE_LINE_LEFT_INSET 16u
#define EGG_C4_STAGE_LINE_TOP_OFFSET 24u
#define EGG_C4_STAGE_LINE_TOP_STEP   22u
#define EGG_C4_STAGE_LINE_BOTTOM_INSET 24u
#define EGG_C4_STAGE_LINE_BOTTOM_STEP 12u
#define EGG_C4_STAGE_LINE_STROKE      1u

#define EGG_C4_MOVING_MIN_INSET 24
#define EGG_C4_MOVING_INITIAL_DX 4
#define EGG_C4_MOVING_BOUNCE_SPEED 2

#define EGG_C4_MARKER_COLOR 7u
#define EGG_C4_MARKER_LEFT_INSET 24u
#define EGG_C4_MARKER_TRAVEL_MARGIN 48u
#define EGG_C4_MARKER_SPEED 4u
#define EGG_C4_MARKER_BOTTOM_INSET 18u
#define EGG_C4_MARKER_WIDTH  24u
#define EGG_C4_MARKER_HEIGHT  6u

/* TVC stage decoration uses bright fixed-palette entries. */
#define EGG_C4_TVC_STAGE_FILL_COLOR       9u
#define EGG_C4_TVC_STAGE_FILL_BACKGROUND  3u
#define EGG_C4_TVC_STAGE_BORDER_COLOR    15u
#define EGG_C4_TVC_STAGE_LINE_COLOR      13u
#define EGG_C4_TVC_MARKER_COLOR           7u

void egg_C4_blit_start(void);
void egg_C4_blit_tick(void);

#endif /* EGG_C4_BLIT_H */
