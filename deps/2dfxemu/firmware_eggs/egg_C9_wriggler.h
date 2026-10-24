#ifndef EGG_C9_WRIGGLER_H
#define EGG_C9_WRIGGLER_H

#include "twodfx_api.h"

/*
 * Egg C9 — Wriggler invasion
 * ==========================
 *
 * Start from the host computer with:
 *
 *     OUT 248,201        (201 = C9h)
 *
 * Purpose
 * -------
 * Demonstrates a large animated sprite scene built entirely through the
 * public C API:
 *
 * - up to 63 independently animated normal sprites;
 * - per-sprite source-address animation across eight artwork phases;
 * - X flipping for worms entering from the right;
 * - pre-generated colour variants in resource RAM;
 * - a giant sprite using X_DOUBLE and Y_DOUBLE;
 * - profile-specific staging for Enterprise and TVC.
 *
 * What appears on screen
 * ----------------------
 * Both machine profiles start directly with the edge-head reveal, followed by
 * the full invasion and the giant worm entering from the right. The obsolete
 * Enterprise-only pair of single-worm crossings has been removed completely.
 *
 * Implementation style
 * --------------------
 * The source artwork in egg_C9_wriggler_assets.h uses readable/import packing
 * (high nibble = left pixel). The .c file converts one normal phase or one four-row giant strip at a time into a
 * small MCU work buffer, then uploads it with
 * twodfx_upload_raw(). It never touches resource_ram, raw SPBs, shadow tables
 * or private renderer helpers. Every visible sprite is submitted with the
 * public twodfx_sprite_set() call.
 *
 * All tunable resource, geometry, timing, colour and animation constants live
 * in this header.
 */

/* ------------------------------------------------------------------------- */
/* Resource allocation and sprite slots                                      */
/* ------------------------------------------------------------------------- */

/** First byte of the 15-colour normal-worm phase bank in RRAM. */
#define EGG_C9_NORMAL_BASE_ADDRESS 0x200000u

/** Number of normal sprite slots; sprite 63 is reserved for the giant. */
#define EGG_C9_NORMAL_SPRITE_COUNT 63u

/** Sprite slot used by the giant worm. */
#define EGG_C9_GIANT_SPRITE 63u

/* ------------------------------------------------------------------------- */
/* Source animation dimensions                                               */
/* ------------------------------------------------------------------------- */

#define EGG_C9_PHASE_COUNT          8u
#define EGG_C9_WORM_WIDTH          64u
#define EGG_C9_WORM_HEIGHT         16u
#define EGG_C9_WORM_BYTES_PER_ROW  32u
#define EGG_C9_NORMAL_PHASE_BYTES \
    (EGG_C9_WORM_BYTES_PER_ROW * EGG_C9_WORM_HEIGHT)

/**
 * Pre-generated body-colour banks 1..15. Colour 0 remains transparent.
 * Both machine profiles select the complete visible bank.
 */
#define EGG_C9_BODY_COLOUR_COUNT 15u

/** Bytes occupied by all eight phases of one normal body colour. */
#define EGG_C9_COLOUR_SET_BYTES \
    (EGG_C9_PHASE_COUNT * EGG_C9_NORMAL_PHASE_BYTES)

/** First byte of the eight pre-scaled giant phases. */
#define EGG_C9_GIANT_BASE_ADDRESS \
    (EGG_C9_NORMAL_BASE_ADDRESS + \
     EGG_C9_BODY_COLOUR_COUNT * EGG_C9_COLOUR_SET_BYTES)

/** Body colour used by the giant worm. */
#define EGG_C9_EP_GIANT_BODY_COLOUR  7u
#define EGG_C9_TVC_GIANT_BODY_COLOUR 15u

/** Source-artwork colour treated as transparent during conversion. */
#define EGG_C9_SOURCE_TRANSPARENT_COLOUR 0x0Bu

/** Fallback body colour if a caller accidentally requests colour zero. */
#define EGG_C9_FALLBACK_BODY_COLOUR 7u

/* ------------------------------------------------------------------------- */
/* Profile framebuffer limits used only for off-screen staging               */
/* ------------------------------------------------------------------------- */

/*
 * These are renderer-window limits used by this animation, not claimed
 * practical visible-area measurements. They preserve the established C9
 * staging and escape behavior without including private 2dfx.h internals.
 */
#define EGG_C9_EP_SCANOUT_WIDTH       832u
#define EGG_C9_EP_FIRST_RENDER_Y       16u
#define EGG_C9_EP_LAST_RENDER_Y       303u
#define EGG_C9_TVC_FRAMEBUFFER_WIDTH  704u
#define EGG_C9_TVC_FIRST_RENDER_Y      41u
#define EGG_C9_TVC_LAST_RENDER_Y      296u

/* Shared 50 Hz timing base. */
#define EGG_C9_FRAMES_PER_SECOND 50u

/* ------------------------------------------------------------------------- */
/* Edge-head reveal                                                          */
/* ------------------------------------------------------------------------- */

#define EGG_C9_WORM_COUNT            63u
#define EGG_C9_HEAD_APPEAR_EVERY     10u
#define EGG_C9_HEAD_WAIT_AFTER      (10u * EGG_C9_FRAMES_PER_SECOND)

/*
 * The phase-0 artwork occupies source X=0..47; X=48..63 is transparent.
 * Both profiles stage side/bottom worms from the visible artwork edge, using
 * the active BASIC-safe rectangle rather than profile-specific magic numbers.
 */
#define EGG_C9_PHASE0_VISIBLE_WIDTH      48u
#define EGG_C9_HEAD_VISIBLE_PIXELS       12u
#define EGG_C9_HEAD_VERTICAL_MARGIN       4u
#define EGG_C9_HEAD_BOTTOM_MARGIN         8u
#define EGG_C9_BOTTOM_EXTRA_OFFSET_Y      8u

/* ------------------------------------------------------------------------- */
/* Attack animation                                                          */
/* ------------------------------------------------------------------------- */

#define EGG_C9_ATTACK_FRAMES \
    (35u * EGG_C9_FRAMES_PER_SECOND)
#define EGG_C9_ATTACK_STEP_X 16

/** Deterministic seed so every run follows the same invasion layout. */
#define EGG_C9_RANDOM_SEED 0x2DFACE01u

/** Minimum and maximum phase-hold values assigned to normal worms. */
#define EGG_C9_PHASE_HOLD_MIN 1u
#define EGG_C9_PHASE_HOLD_MAX 4u

/* ------------------------------------------------------------------------- */
/* Giant worm                                                                */
/* ------------------------------------------------------------------------- */

/*
 * Each giant phase is stored at 4x source scale and then doubled by the SPB,
 * producing the established 8x visible size without regenerating pixels while
 * the giant is on screen.
 */
#define EGG_C9_GIANT_STORED_SCALE    4u
#define EGG_C9_GIANT_STORED_WIDTH \
    (EGG_C9_WORM_WIDTH * EGG_C9_GIANT_STORED_SCALE)
#define EGG_C9_GIANT_STORED_HEIGHT \
    (EGG_C9_WORM_HEIGHT * EGG_C9_GIANT_STORED_SCALE)
#define EGG_C9_GIANT_VISIBLE_WIDTH  (EGG_C9_GIANT_STORED_WIDTH * 2u)
#define EGG_C9_GIANT_VISIBLE_HEIGHT (EGG_C9_GIANT_STORED_HEIGHT * 2u)
#define EGG_C9_GIANT_BYTES_PER_ROW  (EGG_C9_GIANT_STORED_WIDTH / 2u)
#define EGG_C9_GIANT_PHASE_BYTES \
    (EGG_C9_GIANT_BYTES_PER_ROW * EGG_C9_GIANT_STORED_HEIGHT)

#define EGG_C9_EP_GIANT_START_X \
    (EGG_C9_EP_SCANOUT_WIDTH + 8u)
#define EGG_C9_TVC_GIANT_START_X \
    (EGG_C9_TVC_FRAMEBUFFER_WIDTH + 8u)
#define EGG_C9_EP_GIANT_TARGET_X \
    ((EGG_C9_EP_SCANOUT_WIDTH - EGG_C9_GIANT_VISIBLE_WIDTH) / 2u)
/*
 * Final phase 5 has visible source artwork at X=4..63. With the stored 4x
 * scale, X_DOUBLE and X_FLIP that is 480 visible raw pixels, followed by a
 * 32-pixel transparent tail inside the nominal 512-pixel sprite box.
 * Moving the TVC giant 30 pixels right leaves its visible artwork at X=635,
 * two pixels inside the corrected BASIC safe-area right edge (637).
 */
#define EGG_C9_GIANT_FINAL_VISIBLE_SOURCE_PIXELS 60u
#define EGG_C9_GIANT_FINAL_VISIBLE_WIDTH \
    (EGG_C9_GIANT_FINAL_VISIBLE_SOURCE_PIXELS * \
     EGG_C9_GIANT_STORED_SCALE * 2u)
#define EGG_C9_TVC_GIANT_TARGET_OFFSET_X 30u
#define EGG_C9_TVC_GIANT_TARGET_X \
    (TWODFX_TVC_BASIC_SAFE_MIN_X + EGG_C9_TVC_GIANT_TARGET_OFFSET_X)
#define EGG_C9_GIANT_Y              92u

_Static_assert(EGG_C9_TVC_GIANT_TARGET_X +
                   EGG_C9_GIANT_FINAL_VISIBLE_WIDTH - 1u <=
               TWODFX_TVC_BASIC_SAFE_MAX_X,
               "TVC giant worm visible artwork must remain inside safe area");

/** One visible 8x-scaled character step after a complete phase cycle. */
#define EGG_C9_GIANT_ADVANCE_PIXELS \
    (EGG_C9_ATTACK_STEP_X * EGG_C9_GIANT_STORED_SCALE * 2u)
#define EGG_C9_GIANT_PHASE_HOLD  5u
#define EGG_C9_GIANT_FINAL_PHASE 5u

/* ------------------------------------------------------------------------- */
/* Scene state                                                               */
/* ------------------------------------------------------------------------- */

typedef enum {
    EGG_C9_SCENE_HEADS_APPEAR = 0,
    EGG_C9_SCENE_WAIT_ATTACK,
    EGG_C9_SCENE_ATTACK,
    EGG_C9_SCENE_GIANT_ENTER,
    EGG_C9_SCENE_FOREVER
} egg_C9_scene_t;

void egg_C9_wriggler_start(void);
void egg_C9_wriggler_tick(void);

#endif /* EGG_C9_WRIGGLER_H */
