#ifndef EGG_C0_LOGO_H
#define EGG_C0_LOGO_H

#include <stdint.h>

#include "twodfx_api.h"
#include "logo_2dfx_asset.h"

/*
 * Egg C0 — animated ENTERPRISE / 2dfx logo show
 * =================================================
 *
 * Start from either host computer with:
 *
 *     OUT 248,192        (192 = C0h)
 *
 * Purpose
 * -------
 * Demonstrates a large, completely sprite-driven presentation built through
 * the public C API. Ten independent letter sprites first move and bounce in a
 * chaotic field, then assemble into the word ENTERPRISE. Horizontal and
 * vertical card-turn animations are generated procedurally in resource RAM.
 * Six coloured stripe sprites form the familiar Enterprise rainbow, and the
 * large 2dfx logo finally drops from the top, bounces, settles and remains on
 * screen while the stripes circulate.
 *
 * What appears on screen
 * ----------------------
 * 1. Ten doubled letter sprites bounce independently.
 * 2. Boundary bounces trigger smooth generated card-turn phases instead of
 *    instant flip-bit changes.
 * 3. The letters assemble into ENTERPRISE and flipped letters are corrected
 *    from left to right only when their visual symmetry requires it.
 * 4. Six rainbow stripes sit underneath the word.
 * 5. After a short pause, the large 2dfx logo falls under simple fixed-point
 *    gravity, bounces, and settles at the machine-specific final Y position.
 * 6. The rainbow stripe positions then rotate forever.
 *
 * Collision demonstration
 * -----------------------
 * The T letter and the last stripe participate in the global sprite collision
 * detector. While their previous completed-frame collision result is true, a
 * temporary marker reusing the final stripe asset appears in the lower part of
 * the active machine profile.
 *
 * Implementation style
 * --------------------
 * All renderer interaction uses twodfx_api.h. Source artwork is converted in
 * ordinary MCU work buffers and uploaded with twodfx_upload_raw(); sprites are
 * submitted with twodfx_sprite_set(); collision is read with
 * twodfx_collision_detected(). The egg never accesses resource_ram, raw SPBs,
 * shadow tables, private machine profiles or 2dfx.h.
 *
 * Large source-art arrays live in egg_C0_logo_assets.h so this configuration
 * header remains readable. All tunable resource, geometry, timing, animation,
 * colour-remap and sprite-allocation constants live here.
 */

/* ------------------------------------------------------------------------- */
/* Animation state                                                           */
/* ------------------------------------------------------------------------- */

typedef enum {
    EGG_C0_PHASE_CHAOS = 0,
    EGG_C0_PHASE_ASSEMBLE,
    EGG_C0_PHASE_FLIPBACK,
    EGG_C0_PHASE_SETTLE,
    EGG_C0_PHASE_RAINBOW
} egg_C0_phase_t;

typedef enum {
    EGG_C0_GLYPH_E = 0,
    EGG_C0_GLYPH_N,
    EGG_C0_GLYPH_T,
    EGG_C0_GLYPH_R,
    EGG_C0_GLYPH_P,
    EGG_C0_GLYPH_I,
    EGG_C0_GLYPH_S,
    EGG_C0_UNIQUE_GLYPH_COUNT
} egg_C0_glyph_id_t;

/* ------------------------------------------------------------------------- */
/* Sprite allocation and transforms                                          */
/* ------------------------------------------------------------------------- */

#define EGG_C0_LETTER_COUNT 10u
#define EGG_C0_STRIPE_COUNT  6u

#define EGG_C0_LETTER_SPRITE_BASE 0u
#define EGG_C0_STRIPE_SPRITE_BASE \
    (EGG_C0_LETTER_SPRITE_BASE + EGG_C0_LETTER_COUNT)
#define EGG_C0_COLLISION_MARKER_SPRITE \
    (EGG_C0_STRIPE_SPRITE_BASE + EGG_C0_STRIPE_COUNT)
#define EGG_C0_2DFX_LOGO_SPRITE \
    (EGG_C0_COLLISION_MARKER_SPRITE + 1u)
#define EGG_C0_USED_SPRITE_COUNT \
    (EGG_C0_LETTER_COUNT + EGG_C0_STRIPE_COUNT + 2u)

#define EGG_C0_COLLISION_LETTER_INDEX 2u
#define EGG_C0_COLLISION_STRIPE_INDEX (EGG_C0_STRIPE_COUNT - 1u)

#define EGG_C0_X_DOUBLE TWODFX_SPRITE_X_DOUBLE
#define EGG_C0_Y_DOUBLE TWODFX_SPRITE_Y_DOUBLE
#define EGG_C0_X_FLIP   TWODFX_SPRITE_X_FLIP
#define EGG_C0_Y_FLIP   TWODFX_SPRITE_Y_FLIP

/* ------------------------------------------------------------------------- */
/* Resource allocation and MCU work buffers                                  */
/* ------------------------------------------------------------------------- */

#define EGG_C0_RESOURCE_BASE 0x300000u

/* Largest unique glyph is N: 34 pixels x 52 lines = 884 packed bytes. */
#define EGG_C0_MAX_GLYPH_WIDTH      34u
#define EGG_C0_GLYPH_HEIGHT         52u
#define EGG_C0_MAX_GLYPH_ROW_BYTES \
    ((EGG_C0_MAX_GLYPH_WIDTH + 1u) / 2u)
#define EGG_C0_MAX_GLYPH_BYTES \
    (EGG_C0_MAX_GLYPH_ROW_BYTES * EGG_C0_GLYPH_HEIGHT)

#define EGG_C0_STRIPE_WIDTH  48u
#define EGG_C0_STRIPE_HEIGHT  6u

/* TVC uses the bright fixed palette for the rainbow.  The second stripe is
 * a single-raw-pixel red/yellow checkerboard to suggest orange, which TVC
 * cannot represent directly.  Its 96-pixel source width is intentionally not
 * X-doubled, so all six displayed stripes remain the same width. */
#define EGG_C0_TVC_STRIPE_RED_COLOUR      3u
#define EGG_C0_TVC_STRIPE_ORANGE_A_COLOUR 3u
#define EGG_C0_TVC_STRIPE_ORANGE_B_COLOUR 7u
#define EGG_C0_TVC_STRIPE_YELLOW_COLOUR   7u
#define EGG_C0_TVC_STRIPE_GREEN_COLOUR    5u
#define EGG_C0_TVC_STRIPE_BLUE_COLOUR     9u
#define EGG_C0_TVC_STRIPE_MAGENTA_COLOUR 11u
#define EGG_C0_TVC_ORANGE_STRIPE_WIDTH   (EGG_C0_STRIPE_WIDTH * 2u)

/* Individual unique-glyph dimensions. */
#define EGG_C0_GLYPH_E_WIDTH 26u
#define EGG_C0_GLYPH_N_WIDTH 34u
#define EGG_C0_GLYPH_T_WIDTH 32u
#define EGG_C0_GLYPH_R_WIDTH 30u
#define EGG_C0_GLYPH_P_WIDTH 30u
#define EGG_C0_GLYPH_I_WIDTH 14u
#define EGG_C0_GLYPH_S_WIDTH 26u

/*
 * Readable initializer tables used by egg_C0_logo.c.
 *
 * The source pointers are declared in egg_C0_logo_assets.h.  Keeping the
 * mapping and displayed ENTERPRISE sequence here makes the configuration
 * visible without burying it inside the implementation.
 */
#define EGG_C0_UNIQUE_LETTER_ASSET_VALUES \
    { \
        [EGG_C0_GLYPH_E] = \
            { EGG_C0_GLYPH_E_WIDTH, EGG_C0_GLYPH_HEIGHT, asset_e1 }, \
        [EGG_C0_GLYPH_N] = \
            { EGG_C0_GLYPH_N_WIDTH, EGG_C0_GLYPH_HEIGHT, asset_n  }, \
        [EGG_C0_GLYPH_T] = \
            { EGG_C0_GLYPH_T_WIDTH, EGG_C0_GLYPH_HEIGHT, asset_t  }, \
        [EGG_C0_GLYPH_R] = \
            { EGG_C0_GLYPH_R_WIDTH, EGG_C0_GLYPH_HEIGHT, asset_r1 }, \
        [EGG_C0_GLYPH_P] = \
            { EGG_C0_GLYPH_P_WIDTH, EGG_C0_GLYPH_HEIGHT, asset_p  }, \
        [EGG_C0_GLYPH_I] = \
            { EGG_C0_GLYPH_I_WIDTH, EGG_C0_GLYPH_HEIGHT, asset_i  }, \
        [EGG_C0_GLYPH_S] = \
            { EGG_C0_GLYPH_S_WIDTH, EGG_C0_GLYPH_HEIGHT, asset_s  } \
    }

#define EGG_C0_WORD_GLYPH_VALUES \
    { \
        EGG_C0_GLYPH_E, EGG_C0_GLYPH_N, EGG_C0_GLYPH_T, \
        EGG_C0_GLYPH_E, EGG_C0_GLYPH_R, EGG_C0_GLYPH_P, \
        EGG_C0_GLYPH_R, EGG_C0_GLYPH_I, EGG_C0_GLYPH_S, \
        EGG_C0_GLYPH_E \
    }

#define EGG_C0_STRIPE_ASSET_VALUES \
    { \
        { EGG_C0_STRIPE_WIDTH, EGG_C0_STRIPE_HEIGHT, asset_red }, \
        { EGG_C0_STRIPE_WIDTH, EGG_C0_STRIPE_HEIGHT, asset_org }, \
        { EGG_C0_STRIPE_WIDTH, EGG_C0_STRIPE_HEIGHT, asset_yel }, \
        { EGG_C0_STRIPE_WIDTH, EGG_C0_STRIPE_HEIGHT, asset_grn }, \
        { EGG_C0_STRIPE_WIDTH, EGG_C0_STRIPE_HEIGHT, asset_cya }, \
        { EGG_C0_STRIPE_WIDTH, EGG_C0_STRIPE_HEIGHT, asset_mag } \
    }

/* ------------------------------------------------------------------------- */
/* Machine-profile geometry used by this established animation               */
/* ------------------------------------------------------------------------- */

#define EGG_C0_ENTERPRISE_SCREEN_WIDTH      832u
#define EGG_C0_ENTERPRISE_VISIBLE_FIRST_Y    16u
#define EGG_C0_ENTERPRISE_VISIBLE_HEIGHT    288u

#define EGG_C0_TVC_SCREEN_WIDTH             704u
#define EGG_C0_TVC_VISIBLE_FIRST_Y           41u
#define EGG_C0_TVC_VISIBLE_HEIGHT           256u

/* Final large-logo resting Y values preserved from the original profile. */
#define EGG_C0_ENTERPRISE_2DFX_TARGET_Y \
    (EGG_C0_ENTERPRISE_VISIBLE_FIRST_Y + 79u)
#define EGG_C0_TVC_2DFX_TARGET_Y \
    (EGG_C0_TVC_VISIBLE_FIRST_Y + \
     ((EGG_C0_TVC_VISIBLE_HEIGHT > LOGO_2DFX_HEIGHT) \
        ? ((EGG_C0_TVC_VISIBLE_HEIGHT - LOGO_2DFX_HEIGHT) / 2u) \
        : 0u))

/* Global raw-X correction shared visually with SHOW_FW. */
#define EGG_C0_GLOBAL_X_OFFSET LOGO_GLOBAL_X_OFFSET
#define EGG_C0_RAW_COORDINATE_MAX 1023u

/*
 * Chaos-phase bounce limits.  Maximum values are exclusive right/bottom
 * edges, because the bounce code subtracts each sprite's displayed size.
 *
 * Enterprise X is expressed before the historical +28 raw-X correction, so
 * the displayed sprite edges touch the measured NICK rectangle exactly:
 * 88..811 horizontally and 18..305 vertically.  TVC retains its established
 * presentation bounds.
 */
#define EGG_C0_ENTERPRISE_CHAOS_MIN_X \
    ((int32_t)TWODFX_ENTERPRISE_NICK_VISIBLE_MIN_X - \
     (int32_t)EGG_C0_GLOBAL_X_OFFSET)
#define EGG_C0_ENTERPRISE_CHAOS_MAX_X \
    ((int32_t)TWODFX_ENTERPRISE_NICK_VISIBLE_MAX_X + 1 - \
     (int32_t)EGG_C0_GLOBAL_X_OFFSET)
#define EGG_C0_ENTERPRISE_CHAOS_MIN_Y \
    ((int32_t)TWODFX_ENTERPRISE_NICK_VISIBLE_MIN_Y)
#define EGG_C0_ENTERPRISE_CHAOS_MAX_Y \
    ((int32_t)TWODFX_ENTERPRISE_NICK_VISIBLE_MAX_Y + 1)

#define EGG_C0_TVC_CHAOS_MIN_X 88
#define EGG_C0_TVC_CHAOS_MAX_X 800
#define EGG_C0_TVC_CHAOS_MIN_Y 30
#define EGG_C0_TVC_CHAOS_MAX_Y 280

/* Final assembled layout. */
#define EGG_C0_LETTER_BASE_Y_OFFSET 70
#define EGG_C0_LETTER_ADVANCE_GAP    3
#define EGG_C0_STRIPE_Y_OFFSET      58
#define EGG_C0_STRIPE_ADVANCE_GAP    3
#define EGG_C0_COLLISION_MARKER_Y_NUMERATOR   2u
#define EGG_C0_COLLISION_MARKER_Y_DENOMINATOR 3u

/* ------------------------------------------------------------------------- */
/* Card-turn animation                                                       */
/* ------------------------------------------------------------------------- */

#define EGG_C0_XTURN_PHASES 11u
#define EGG_C0_XTURN_STEPS  21u
#define EGG_C0_YTURN_PHASES 11u
#define EGG_C0_YTURN_STEPS  21u
#define EGG_C0_TURN_MIN_VISIBLE_PIXELS 2u

/* Left-to-right delay after each letter has been visually corrected. */
#define EGG_C0_FLIPBACK_WAIT_FRAMES 100u

/* ------------------------------------------------------------------------- */
/* Main timing and gravity                                                   */
/* ------------------------------------------------------------------------- */

#define EGG_C0_CHAOS_FRAMES              (100u * 50u)
#define EGG_C0_2DFX_PRE_DROP_WAIT_FRAMES (10u * 50u)
#define EGG_C0_RAINBOW_SWAP_FRAMES       42u

/* Fixed-point values use eight fractional bits. */
#define EGG_C0_FIXED_POINT_SHIFT       8u
#define EGG_C0_2DFX_START_Y            0
#define EGG_C0_2DFX_INITIAL_VELOCITY   (5 * 256)
#define EGG_C0_2DFX_GRAVITY            160
#define EGG_C0_2DFX_BOUNCE_NUMERATOR    55
#define EGG_C0_2DFX_BOUNCE_DENOMINATOR 100
#define EGG_C0_2DFX_SETTLE_VELOCITY    (1 * 256)

/* ------------------------------------------------------------------------- */
/* Randomized chaos motion                                                   */
/* ------------------------------------------------------------------------- */

#define EGG_C0_RNG_INITIAL_FALLBACK 0xC0DEC0DEu
#define EGG_C0_RNG_SEED_XOR         0x2DFA2026u
#define EGG_C0_RNG_MULTIPLIER       1664525u
#define EGG_C0_RNG_INCREMENT        1013904223u
#define EGG_C0_RANDOM_SPEED_MIN     1u
#define EGG_C0_RANDOM_SPEED_RANGE   3u

/* ------------------------------------------------------------------------- */
/* TVC recolouring of the shared large 2dfx logo                             */
/* ------------------------------------------------------------------------- */

#define EGG_C0_SOURCE_TRANSPARENT_COLOUR 0u
#define EGG_C0_SOURCE_LOGO_TEXT_COLOUR   7u
#define EGG_C0_SOURCE_LOGO_ACCENT_COLOUR 1u
#define EGG_C0_TVC_LOGO_TEXT_COLOUR     15u
#define EGG_C0_TVC_LOGO_ACCENT_COLOUR    3u

void egg_C0_logo_start(void);
void egg_C0_logo_tick(void);

#endif /* EGG_C0_LOGO_H */
