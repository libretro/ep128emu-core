#ifndef EGG_C8_PONG_WOW_H
#define EGG_C8_PONG_WOW_H

/*
 * Egg C8 — WOW Pong
 * =================
 *
 * Start from the host computer with:
 *
 *     OUT 248,200        (200 = C8h)
 *
 * Purpose
 * -------
 * Builds a deliberately extravagant version of Pong through the public C API
 * while preserving the same game geometry and input model as C7. It combines:
 *
 * - generated striped paddle and circular ball sprites;
 * - one shared generated 16x8 1-bpp font and live score strings;
 * - a generated 16x8 tileset and animated tilemap background;
 * - four generated PATTERN_FILL patterns;
 * - TILEMAP, PATTERN_FILL, DRAW_TEXT, RECT and FILL_RECT commands;
 * - independent 2DPT and sprite shake after a score;
 * - ordered, nonblocking idle-F9 input.
 *
 * What appears on screen
 * ----------------------
 * A neon-styled Pong arena with animated patterned bars, a changing tilemap,
 * score panels, labels, a ball trail and hit/score flashes. All decoration is
 * intentionally kept inside the selected BASIC-safe playfield.
 *
 * Input
 * -----
 * Write one bit-mask byte with OUT 249,value while no normal 2dfx command
 * session is consuming F9h:
 *
 *     bit 7  N  new game, edge-triggered
 *     bit 3  Q  left paddle up
 *     bit 2  A  left paddle down
 *     bit 1  O  right paddle up
 *     bit 0  L  right paddle down
 *
 * The latest queued byte is treated as the current held-key state.
 *
 * Implementation style
 * --------------------
 * The .c file accesses 2dfx only through twodfx_api.h and twodfx_egg.h.
 * Deterministic artwork is generated offline by
 * tools/generate_egg_C8_assets.py and stored as separately named const arrays
 * in egg_C8_pong_wow_assets.h. It is uploaded directly from flash to RRAM, so
 * no persistent MCU-side font, pattern, tile or sprite work buffer exists.
 * Only the animated tilemap rows and changing score digits are constructed at
 * runtime in tiny stack buffers. All public 2DPT calls remain visible in the
 * scene builder; there are no raw command-packing wrappers, private
 * resource_ram writes or shadow-table access. All tunable resource, geometry,
 * timing, input, pattern, tile and colour constants live in this header.
 */

/* ------------------------------------------------------------------------- */
/* Resource allocation and descriptors                                       */
/* ------------------------------------------------------------------------- */

#define EGG_C8_LEFT_PADDLE_SPRITE  0u
#define EGG_C8_RIGHT_PADDLE_SPRITE 1u
#define EGG_C8_BALL_SPRITE         2u

#define EGG_C8_LEFT_PADDLE_ADDRESS  0x400000u
#define EGG_C8_RIGHT_PADDLE_ADDRESS 0x400400u
#define EGG_C8_BALL_ADDRESS         0x400800u

#define EGG_C8_FONT_DESCRIPTOR 1u
#define EGG_C8_FONT_ADDRESS    0x5D0000u

#define EGG_C8_TITLE_STRING_ADDRESS 0x5E0000u
#define EGG_C8_P1_STRING_ADDRESS    0x5E0020u
#define EGG_C8_P2_STRING_ADDRESS    0x5E0040u
#define EGG_C8_HINT_STRING_ADDRESS  0x5E0060u

#define EGG_C8_TILESET_DESCRIPTOR 1u
#define EGG_C8_TILEMAP_DESCRIPTOR 1u
#define EGG_C8_PATTERN_DESCRIPTOR 1u
#define EGG_C8_TILESET_ADDRESS    0x5E1000u
#define EGG_C8_TILEMAP_ADDRESS    0x5E3000u
#define EGG_C8_PATTERN_ADDRESS    0x5E4000u

#define EGG_C8_FONT_WIDTH_UNITS      2u
#define EGG_C8_FONT_HEIGHT_UNITS     1u
#define EGG_C8_FONT_BYTES_PER_GLYPH 16u

#define EGG_C8_PATTERN_COUNT 4u
#define EGG_C8_TILE_COUNT    8u

/* Largest row width. C8 keeps only one short tilemap row on the stack. */
#define EGG_C8_MAX_TILEMAP_COLUMNS 39u
#define EGG_C8_MAX_TILEMAP_ROWS    24u
#define EGG_C8_MAX_TILEMAP_CELLS \
    (EGG_C8_MAX_TILEMAP_COLUMNS * EGG_C8_MAX_TILEMAP_ROWS)

/* ------------------------------------------------------------------------- */
/* Machine-specific playfield and tilemap geometry                           */
/* ------------------------------------------------------------------------- */

#define EGG_C8_EP_FIELD_X      128u
#define EGG_C8_EP_FIELD_Y       48u
#define EGG_C8_EP_FIELD_WIDTH  634u
#define EGG_C8_EP_FIELD_HEIGHT 212u

#define EGG_C8_TVC_FIELD_X      132u
#define EGG_C8_TVC_FIELD_Y       53u
#define EGG_C8_TVC_FIELD_WIDTH  500u
#define EGG_C8_TVC_FIELD_HEIGHT 232u

#define EGG_C8_EP_TILEMAP_COLUMNS 39u
#define EGG_C8_EP_TILEMAP_ROWS    22u
#define EGG_C8_EP_TILEMAP_X_INSET  4u
#define EGG_C8_EP_TILEMAP_Y_INSET 18u

#define EGG_C8_TVC_TILEMAP_COLUMNS 30u
#define EGG_C8_TVC_TILEMAP_ROWS    24u
#define EGG_C8_TVC_TILEMAP_X_INSET 10u
#define EGG_C8_TVC_TILEMAP_Y_INSET 18u

#define EGG_C8_PADDLE_WIDTH     12u
#define EGG_C8_PADDLE_HEIGHT    48u
#define EGG_C8_BALL_WIDTH  12u
#define EGG_C8_BALL_HEIGHT  6u

#define EGG_C8_PADDLE_X_INSET  8u
#define EGG_C8_PADDLE_Y_INSET 10u
#define EGG_C8_BALL_WALL_INSET 4u

/* ------------------------------------------------------------------------- */
/* Input bits and game motion                                                */
/* ------------------------------------------------------------------------- */

#define EGG_C8_INPUT_NEW_GAME         0x80u
#define EGG_C8_INPUT_LEFT_PADDLE_UP    0x08u
#define EGG_C8_INPUT_LEFT_PADDLE_DOWN  0x04u
#define EGG_C8_INPUT_RIGHT_PADDLE_UP   0x02u
#define EGG_C8_INPUT_RIGHT_PADDLE_DOWN 0x01u

#define EGG_C8_PADDLE_SPEED       5
#define EGG_C8_BALL_SPEED_X       4
#define EGG_C8_BALL_INITIAL_DY    3
#define EGG_C8_BALL_MIN_DY        1
#define EGG_C8_PADDLE_DEFLECT_DIV 8
#define EGG_C8_SERVE_DELAY_TICKS 30u
#define EGG_C8_SCORE_FLASH_TICKS 20u
#define EGG_C8_WALL_HIT_TICKS     4u
#define EGG_C8_PADDLE_HIT_TICKS   6u
#define EGG_C8_SCORE_MODULUS      10u

/* ------------------------------------------------------------------------- */
/* Text strings and live score digit                                        */
/* ------------------------------------------------------------------------- */

#define EGG_C8_TITLE_TEXT "PONG"
#define EGG_C8_P1_TEXT    "P1 0"
#define EGG_C8_P2_TEXT    "P2 0"
#define EGG_C8_HINT_TEXT  "N NEW GAME"
#define EGG_C8_SCORE_DIGIT_OFFSET 3u
#define EGG_C8_TITLE_CHARACTERS    4u
#define EGG_C8_SCORE_CHARACTERS    4u
#define EGG_C8_HINT_CHARACTERS    10u

/* ------------------------------------------------------------------------- */
/* Pattern and tile numbers                                                  */
/* ------------------------------------------------------------------------- */

#define EGG_C8_PATTERN_ENERGY_A 0u
#define EGG_C8_PATTERN_ENERGY_B 1u
#define EGG_C8_PATTERN_PANEL    2u
#define EGG_C8_PATTERN_CENTRE   3u
#define EGG_C8_PATTERN_PHASE_SHIFT 3u
#define EGG_C8_PATTERN_PHASE_MASK  3u

#define EGG_C8_TILE_FLOOR      0u
#define EGG_C8_TILE_WAVE       1u
#define EGG_C8_TILE_SPARKLE    2u
#define EGG_C8_TILE_BORDER     3u
#define EGG_C8_TILE_H_BAR      4u
#define EGG_C8_TILE_V_BAR      5u

#define EGG_C8_TILEMAP_H_BAR_ROW_A  5u
#define EGG_C8_TILEMAP_H_BAR_ROW_B 20u
#define EGG_C8_TILEMAP_V_BAR_COL_A 10u
#define EGG_C8_TILEMAP_V_BAR_COL_B 21u
#define EGG_C8_TILEMAP_V_BAR_COL_C 32u
#define EGG_C8_TILEMAP_ANIMATION_SHIFT 4u
#define EGG_C8_TILEMAP_SPARKLE_MASK 11u
#define EGG_C8_TILEMAP_WAVE_MASK 3u

/* ------------------------------------------------------------------------- */
/* Scene layout                                                              */
/* ------------------------------------------------------------------------- */

#define EGG_C8_ENERGY_BAR_X_INSET    10u
#define EGG_C8_ENERGY_BAR_Y_INSET     4u
#define EGG_C8_ENERGY_BAR_BOTTOM_INSET 16u
#define EGG_C8_ENERGY_BAR_HEIGHT     12u

#define EGG_C8_LEFT_PANEL_X_INSET   24u
#define EGG_C8_RIGHT_PANEL_X_INSET 104u
#define EGG_C8_CENTRE_PANEL_HALF_WIDTH 56u
#define EGG_C8_SIDE_PANEL_WIDTH      80u
#define EGG_C8_CENTRE_PANEL_WIDTH   112u
#define EGG_C8_PANEL_Y_INSET         18u
#define EGG_C8_PANEL_HEIGHT          16u

#define EGG_C8_INNER_BORDER_INSET 8u
#define EGG_C8_OUTER_BORDER_STROKE 2u
#define EGG_C8_INNER_BORDER_STROKE 1u

#define EGG_C8_CENTER_DASH_X_HALF 4u
#define EGG_C8_CENTER_DASH_TOP    42u
#define EGG_C8_CENTER_DASH_BOTTOM 34u
#define EGG_C8_CENTER_DASH_STEP   18u
#define EGG_C8_CENTER_DASH_WIDTH   8u
#define EGG_C8_CENTER_DASH_HEIGHT  9u

#define EGG_C8_TRAIL_MULTIPLIER 2
#define EGG_C8_TRAIL_MIN_INSET 12u
#define EGG_C8_TRAIL_MAX_INSET 24u

#define EGG_C8_TITLE_Y_INSET        22u
#define EGG_C8_P1_X_INSET           40u
#define EGG_C8_P2_RIGHT_MARGIN      40u
#define EGG_C8_SCORE_Y_INSET        22u
#define EGG_C8_HINT_BOTTOM_INSET    13u

#define EGG_C8_FLASH_LINE_X_INSET      20u
#define EGG_C8_FLASH_LINE_Y_INSET      40u
#define EGG_C8_FLASH_LINE_BOTTOM_INSET 44u
#define EGG_C8_FLASH_LINE_HEIGHT        4u

/* ------------------------------------------------------------------------- */
/* Shake sequences                                                           */
/* ------------------------------------------------------------------------- */

#define EGG_C8_SCENE_SHAKE_ACTIVE_ABOVE 10u
#define EGG_C8_SCENE_SHAKE_X_VALUES { -3, 2, -2, 3, -2, 2, -1, 1 }
#define EGG_C8_SCENE_SHAKE_Y_VALUES { -1, 1,  1,-1,  1,-1,  1,-1 }

#define EGG_C8_SPRITE_SHAKE_ACTIVE_ABOVE 10u
#define EGG_C8_SPRITE_SHAKE_X_VALUES { -4, 3, -2, 4, -3, 2, -1, 1 }
#define EGG_C8_SPRITE_SHAKE_Y_VALUES { -2, 1,  2,-1,  1,-2,  2,-1 }
#define EGG_C8_SHAKE_PHASE_MASK 7u

/* ------------------------------------------------------------------------- */
/* Colours                                                                   */
/* ------------------------------------------------------------------------- */

#define EGG_C8_TRANSPARENT_COLOUR 0u

/*
 * Enterprise eggs assume the handbook pre-demo palette 0..7:
 * black, red, orange, yellow, green, cyan, magenta, white. Bias 8..15 stays
 * at the host default; 8/9 are deliberately used as darker background tones.
 */
#define EGG_C8_EP_DARK_COLOUR      8u
#define EGG_C8_EP_DARK_ACCENT      9u
#define EGG_C8_EP_RED_COLOUR       1u
#define EGG_C8_EP_GREEN_COLOUR     4u
#define EGG_C8_EP_CYAN_COLOUR      5u
#define EGG_C8_EP_YELLOW_COLOUR    3u
#define EGG_C8_EP_WHITE_COLOUR     7u
#define EGG_C8_EP_GREY_COLOUR      8u

/* TVC WOW mode deliberately uses the bright/full fixed palette. */
#define EGG_C8_TVC_DARK_COLOUR     9u
#define EGG_C8_TVC_DARK_ACCENT    11u
#define EGG_C8_TVC_RED_COLOUR      3u
#define EGG_C8_TVC_GREEN_COLOUR    5u
#define EGG_C8_TVC_CYAN_COLOUR    13u
#define EGG_C8_TVC_YELLOW_COLOUR   7u
#define EGG_C8_TVC_WHITE_COLOUR   15u
#define EGG_C8_TVC_GREY_COLOUR    14u

void egg_C8_pong_wow_start(void);
void egg_C8_pong_wow_tick(void);

#endif /* EGG_C8_PONG_WOW_H */
