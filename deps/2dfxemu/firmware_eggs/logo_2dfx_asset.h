#ifndef LOGO_2DFX_ASSET_H
#define LOGO_2DFX_ASSET_H

#include <stdint.h>


/*
 * Large 2dfx logo shared by the optional C0 logo egg and the always-compiled
 * 8Eh SHOW_FW page.  Keeping the bitmap in one translation unit avoids a
 * second ~51 KiB flash copy when the logo egg is enabled.
 */
#define LOGO_2DFX_WIDTH          515u
#define LOGO_2DFX_HEIGHT         198u
#define LOGO_2DFX_BYTES_PER_ROW  258u
#define LOGO_2DFX_DATA_SIZE      (LOGO_2DFX_BYTES_PER_ROW * LOGO_2DFX_HEIGHT)

/* Final placement shared by the C0 logo egg and SHOW_FW. */
#define LOGO_GLOBAL_X_OFFSET     28u

/*
 * These formulas are expanded only by firmware code that already provides
 * FIRST_VISIBLE_Y and VISIBLE_LINES.  The bitmap-only header deliberately no
 * longer includes private 2dfx.h itself, so public eggs can include the asset
 * declaration without inheriting private renderer definitions.
 */
#define LOGO_2DFX_TARGET_Y_ENTERPRISE  (FIRST_VISIBLE_Y + 79u)
#define LOGO_2DFX_TARGET_Y_TVC         (FIRST_VISIBLE_Y + \
                                        ((VISIBLE_LINES > LOGO_2DFX_HEIGHT) ? \
                                         ((VISIBLE_LINES - LOGO_2DFX_HEIGHT) / 2u) : 0u))

extern const uint8_t logo_2dfx_data[LOGO_2DFX_DATA_SIZE];

#endif
