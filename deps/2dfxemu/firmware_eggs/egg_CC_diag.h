#ifndef EGG_CC_DIAG_H
#define EGG_CC_DIAG_H

#include "twodfx_api.h"

/*
 * Egg CC — five-stage renderer/resource diagnostic suite
 * ======================================================
 *
 * Start from the host computer with:
 *
 *     OUT 248,204        (204 = CCh)
 *
 * Purpose
 * -------
 * Holds five deliberately diagnostic scenes for approximately 20 seconds each
 * so visual behavior and STATUS0 render timing can be inspected separately:
 *
 *   stage 0 — 23-bit sprite addresses and cache-tag separation;
 *   stage 1 — all useful SPB transform-flag combinations;
 *   stage 2 — left/right/top/bottom clipping and transformed clipping;
 *   stage 3 — mixed sprites, BLIT, PATTERN_FILL and TILEMAP resources;
 *   stage 4 — asynchronous raw ZX1 decompression smoke test.
 *
 * What appears on screen
 * ----------------------
 * Every stage has a dark renderer-window background and a small coloured stage
 * marker at the top. The diagnostic graphics then change every 20 seconds.
 * The final stage initially shows an empty destination and then displays the
 * decompressed doubled sprite after the core1 resource job completes.
 *
 * Implementation style
 * --------------------
 * Every deterministic diagnostic resource is generated offline by
 * tools/generate_egg_CC_assets.py and stored as a separately named const array
 * in egg_CC_diag_assets.h. Each stage uploads those final-format bytes directly
 * from flash to the deliberately selected RRAM addresses, preserving the
 * address/cache tests without a mutable MCU pixel work buffer. Descriptors,
 * sprites and 2DPT commands are submitted only through the public API. The egg
 * never touches resource_ram, shadow tables, raw SPBs/2DPBs or private
 * renderer/resource helpers. All tunable addresses, sizes, coordinates,
 * colours, timings and descriptor IDs live in this header.
 */

/* ------------------------------------------------------------------------- */
/* Stage timing and state                                                    */
/* ------------------------------------------------------------------------- */

#define EGG_CC_FRAMES_PER_SECOND 50u
#define EGG_CC_STAGE_SECONDS     20u
#define EGG_CC_STAGE_TICKS \
    (EGG_CC_STAGE_SECONDS * EGG_CC_FRAMES_PER_SECOND)
#define EGG_CC_STAGE_COUNT 5u

typedef enum {
    EGG_CC_STAGE_ADDRESS_AND_CACHE = 0,
    EGG_CC_STAGE_TRANSFORMS,
    EGG_CC_STAGE_CLIPPING,
    EGG_CC_STAGE_MIXED_RESOURCES,
    EGG_CC_STAGE_ZX1
} egg_CC_stage_t;

/* ------------------------------------------------------------------------- */
/* Profile renderer-window sizes                                             */
/* ------------------------------------------------------------------------- */

/* The original diagnostic frame begins at this common raw coordinate. */
#define EGG_CC_FRAME_X 88u
#define EGG_CC_FRAME_Y 16u

#define EGG_CC_EP_FRAME_WIDTH  744u
#define EGG_CC_EP_FRAME_HEIGHT 288u
#define EGG_CC_TVC_FRAME_WIDTH  704u
#define EGG_CC_TVC_FRAME_HEIGHT 256u

/* Stage marker geometry. */
#define EGG_CC_MARKER_X       96u
#define EGG_CC_MARKER_Y       22u
#define EGG_CC_MARKER_W       24u
#define EGG_CC_MARKER_H        8u
#define EGG_CC_PROGRESS_X     128u
#define EGG_CC_PROGRESS_Y      22u
#define EGG_CC_PROGRESS_STEP_W 12u
#define EGG_CC_PROGRESS_H       8u

/* Stage/background colours. */
#define EGG_CC_EP_BACKGROUND_COLOUR  8u
#define EGG_CC_TVC_BACKGROUND_COLOUR 1u
#define EGG_CC_STAGE0_COLOUR     2u
#define EGG_CC_STAGE1_COLOUR     3u
#define EGG_CC_STAGE2_COLOUR     4u
#define EGG_CC_STAGE3_COLOUR     5u
#define EGG_CC_STAGE4_COLOUR     6u
#define EGG_CC_FRAME_COLOUR      7u
#define EGG_CC_TRANSPARENT_COLOUR 0u

/* ------------------------------------------------------------------------- */
/* Sprite addresses chosen to exercise 23-bit boundaries/cache tags          */
/* ------------------------------------------------------------------------- */

#define EGG_CC_SPR_ADDR0 0x000000u
#define EGG_CC_SPR_ADDR1 0x010000u
#define EGG_CC_SPR_ADDR2 0x03FFC0u
#define EGG_CC_SPR_ADDR3 0x040000u
#define EGG_CC_SPR_ADDR4 0x100000u
#define EGG_CC_SPR_ADDR5 0x200000u
#define EGG_CC_SPR_ADDR6 0x400000u
#define EGG_CC_SPR_ADDR7 0x7FF000u
#define EGG_CC_ADDRESS_VALUES \
    { \
        EGG_CC_SPR_ADDR0, EGG_CC_SPR_ADDR1, EGG_CC_SPR_ADDR2, EGG_CC_SPR_ADDR3, \
        EGG_CC_SPR_ADDR4, EGG_CC_SPR_ADDR5, EGG_CC_SPR_ADDR6, EGG_CC_SPR_ADDR7  \
    }

/* Same low address bits in different 1 MiB regions for cache-tag checks. */
#define EGG_CC_CACHE_ADDR0 0x000120u
#define EGG_CC_CACHE_ADDR1 0x100120u
#define EGG_CC_CACHE_ADDR2 0x200120u
#define EGG_CC_CACHE_ADDR3 0x300120u
#define EGG_CC_CACHE_ADDRESS_VALUES \
    { EGG_CC_CACHE_ADDR0, EGG_CC_CACHE_ADDR1, \
      EGG_CC_CACHE_ADDR2, EGG_CC_CACHE_ADDR3 }

/* Transform, clipping, mixed-resource and raw-ZX1 allocations. */
#define EGG_CC_ARROW_ADDR   0x500000u
#define EGG_CC_CLIP_ADDR    0x510000u
#define EGG_CC_MIX_SPR_ADDR 0x520000u
#define EGG_CC_ZX1_SRC_ADDR 0x530000u
#define EGG_CC_ZX1_DST_ADDR 0x540000u

/* BLIT/PATTERN/TILEMAP resources used by the mixed stage. */
#define EGG_CC_BLIT_ADDR    0x580000u
#define EGG_CC_PATTERN_ADDR 0x581000u
#define EGG_CC_TILESET_ADDR 0x582000u
#define EGG_CC_TILEMAP_ADDR 0x584000u

/* Descriptor IDs reserved by the diagnostic egg. */
#define EGG_CC_BITMAP_ID  10u
#define EGG_CC_PATTERN_ID 10u
#define EGG_CC_TILESET_ID 10u
#define EGG_CC_TILEMAP_ID 10u

/* ------------------------------------------------------------------------- */
/* Offline-generated assets                                                  */
/* ------------------------------------------------------------------------- */

/*
 * All fixed resources are emitted in final internal packed format in
 * egg_CC_diag_assets.h. The largest individual asset is the 96x64 clipping
 * sprite, but it occupies flash and RRAM only—not MCU BSS.
 */

/* ------------------------------------------------------------------------- */
/* Stage 0: address and cache-tag sprites                                     */
/* ------------------------------------------------------------------------- */

#define EGG_CC_ADDRESS_SPRITE_W 32u
#define EGG_CC_ADDRESS_SPRITE_H 24u
#define EGG_CC_ADDRESS_FIRST_X  96u
#define EGG_CC_ADDRESS_STEP_X   86u
#define EGG_CC_ADDRESS_Y        58u

#define EGG_CC_CACHE_SPRITE_W 32u
#define EGG_CC_CACHE_SPRITE_H 24u
#define EGG_CC_CACHE_FIRST_X  160u
#define EGG_CC_CACHE_STEP_X   132u
#define EGG_CC_CACHE_Y        132u

/* ------------------------------------------------------------------------- */
/* Stage 1: transform matrix                                                 */
/* ------------------------------------------------------------------------- */

#define EGG_CC_ARROW_W 32u
#define EGG_CC_ARROW_H 24u
#define EGG_CC_TRANSFORM_COLUMNS 5u
#define EGG_CC_TRANSFORM_FIRST_X 104u
#define EGG_CC_TRANSFORM_STEP_X  136u
#define EGG_CC_TRANSFORM_FIRST_Y  58u
#define EGG_CC_TRANSFORM_STEP_Y  104u
#define EGG_CC_TRANSFORM_COUNT    10u
#define EGG_CC_TRANSFORM_FLAG_VALUES \
    { \
        0u, \
        TWODFX_SPRITE_X_DOUBLE, \
        TWODFX_SPRITE_Y_DOUBLE, \
        TWODFX_SPRITE_X_DOUBLE | TWODFX_SPRITE_Y_DOUBLE, \
        TWODFX_SPRITE_X_FLIP, \
        TWODFX_SPRITE_Y_FLIP, \
        TWODFX_SPRITE_X_FLIP | TWODFX_SPRITE_Y_FLIP, \
        TWODFX_SPRITE_X_DOUBLE | TWODFX_SPRITE_X_FLIP, \
        TWODFX_SPRITE_Y_DOUBLE | TWODFX_SPRITE_Y_FLIP, \
        TWODFX_SPRITE_X_DOUBLE | TWODFX_SPRITE_Y_DOUBLE | \
            TWODFX_SPRITE_X_FLIP | TWODFX_SPRITE_Y_FLIP \
    }

/* ------------------------------------------------------------------------- */
/* Stage 2: clipping                                                         */
/* ------------------------------------------------------------------------- */

#define EGG_CC_CLIP_SPRITE_W 96u
#define EGG_CC_CLIP_SPRITE_H 64u

#define EGG_CC_CLIP_LEFT_X     18u
#define EGG_CC_CLIP_LEFT_Y     64u
#define EGG_CC_CLIP_RIGHT_X   760u
#define EGG_CC_CLIP_RIGHT_Y    64u
#define EGG_CC_CLIP_TOP_X     168u
#define EGG_CC_CLIP_TOP_Y       4u
#define EGG_CC_CLIP_BOTTOM_X  300u
#define EGG_CC_CLIP_BOTTOM_Y  260u
#define EGG_CC_CLIP_DOUBLE_X  520u
#define EGG_CC_CLIP_DOUBLE_Y  120u

#define EGG_CC_CLIP_CACHE_W       24u
#define EGG_CC_CLIP_CACHE_H       24u
#define EGG_CC_CLIP_CACHE_FIRST_X 120u
#define EGG_CC_CLIP_CACHE_STEP_X   72u
#define EGG_CC_CLIP_CACHE_Y       210u

/* ------------------------------------------------------------------------- */
/* Stage 3: mixed resources                                                  */
/* ------------------------------------------------------------------------- */

#define EGG_CC_BLIT_W     64u
#define EGG_CC_BLIT_H     32u
#define EGG_CC_BLIT_BYTES ((EGG_CC_BLIT_W / 2u) * EGG_CC_BLIT_H)

#define EGG_CC_PATTERN_COUNT 4u
#define EGG_CC_PATTERN_BANK_BYTES \
    (EGG_CC_PATTERN_COUNT * TWODFX_PATTERN_BYTES)

#define EGG_CC_TILE_COUNT 8u
#define EGG_CC_TILESET_BYTES (EGG_CC_TILE_COUNT * TWODFX_TILE_BYTES)
#define EGG_CC_TILEMAP_COLUMNS 16u
#define EGG_CC_TILEMAP_ROWS    10u
#define EGG_CC_TILEMAP_BYTES \
    (EGG_CC_TILEMAP_COLUMNS * EGG_CC_TILEMAP_ROWS)

#define EGG_CC_MIX_SPRITE_W 48u
#define EGG_CC_MIX_SPRITE_H 32u

#define EGG_CC_PATTERN0_X  96u
#define EGG_CC_PATTERN0_Y  32u
#define EGG_CC_PATTERN0_W 256u
#define EGG_CC_PATTERN0_H  96u
#define EGG_CC_PATTERN1_X 376u
#define EGG_CC_PATTERN1_Y  32u
#define EGG_CC_PATTERN1_W 336u
#define EGG_CC_PATTERN1_H  96u

#define EGG_CC_TILEMAP_X 104u
#define EGG_CC_TILEMAP_Y 152u

#define EGG_CC_BLIT0_X 448u
#define EGG_CC_BLIT0_Y 168u
#define EGG_CC_BLIT1_X 568u
#define EGG_CC_BLIT1_Y 208u

#define EGG_CC_MIX_SPRITE0_X 500u
#define EGG_CC_MIX_SPRITE0_Y  58u
#define EGG_CC_MIX_SPRITE1_X 600u
#define EGG_CC_MIX_SPRITE1_Y  84u

/* ------------------------------------------------------------------------- */
/* Stage 4: raw ZX1 smoke test                                               */
/* ------------------------------------------------------------------------- */

#define EGG_CC_ZX1_SPRITE_X 360u
#define EGG_CC_ZX1_SPRITE_Y 112u

void egg_CC_diag_start(void);
void egg_CC_diag_tick(void);

#endif /* EGG_CC_DIAG_H */
