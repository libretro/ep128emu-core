#ifndef EGG_C6_TILEMAP_H
#define EGG_C6_TILEMAP_H

#include "twodfx_api.h"

/*
 * Egg C6 — TILEMAP showcase
 * =========================
 *
 * Start from the host computer with:
 *
 *     OUT 248,198        (198 = C6h)
 *
 * Purpose
 * -------
 * Demonstrates the public C API for:
 *
 * - generating and uploading a bank of fixed 16x8 tiles;
 * - generating and updating a row-major tilemap in RRAM;
 * - defining tileset and tilemap descriptors;
 * - drawing a complete tilemap viewport with one TILEMAP command;
 * - changing only map bytes while leaving the 2DPT playlist and descriptors
 *   unchanged.
 *
 * What appears on screen
 * ----------------------
 * A framed 16x12-cell map contains a patterned border and several repeating
 * decorative tiles. One bright marker tile moves through the map. A solid bar
 * below the map makes profile-specific colour and geometry easy to inspect.
 *
 * Implementation style
 * --------------------
 * The .c file accesses 2dfx only through twodfx_api.h. Tiles and map cells are
 * assembled in ordinary MCU arrays and uploaded with twodfx_upload_raw(); it
 * does not touch resource_ram, raw 2DPBs, shadow tables or private renderer
 * helpers. All tunable resource, geometry, animation, tile and colour
 * constants live in this header.
 */

/* ------------------------------------------------------------------------- */
/* Resource allocation and descriptors                                       */
/* ------------------------------------------------------------------------- */

/** RRAM address of tile 0 in the generated tileset. */
#define EGG_C6_TILESET_ADDRESS 0x5A0000u

/** RRAM address of cell (0,0) in the generated tilemap. */
#define EGG_C6_TILEMAP_ADDRESS 0x5B0000u

/** Public tileset descriptor used by this egg. */
#define EGG_C6_TILESET_DESCRIPTOR 0u

/** Public tilemap descriptor used by this egg. */
#define EGG_C6_TILEMAP_DESCRIPTOR 0u

/** Number of allocated/generated tiles in the tileset. */
#define EGG_C6_TILE_COUNT 8u

/** Complete generated map dimensions in 16x8 cells. */
#define EGG_C6_MAP_COLUMNS 16u
#define EGG_C6_MAP_ROWS    12u

/** First source cell and complete viewport drawn by the TILEMAP command. */
#define EGG_C6_SOURCE_TILE_X 0u
#define EGG_C6_SOURCE_TILE_Y 0u
#define EGG_C6_VIEW_COLUMNS  EGG_C6_MAP_COLUMNS
#define EGG_C6_VIEW_ROWS     EGG_C6_MAP_ROWS

/* ------------------------------------------------------------------------- */
/* Tile numbers                                                              */
/* ------------------------------------------------------------------------- */

#define EGG_C6_TILE_CHECKER       0u  /**< Default map floor. */
#define EGG_C6_TILE_WAVES         1u  /**< Repeating decorative stripe tile. */
#define EGG_C6_TILE_PANEL         2u  /**< Generated for visual variety. */
#define EGG_C6_TILE_SPARKLES      3u  /**< Sparse interior decoration. */
#define EGG_C6_TILE_BORDER        4u  /**< Outer map boundary. */
#define EGG_C6_TILE_MOVING_MARKER 5u  /**< Animated bright marker. */
#define EGG_C6_TILE_RESERVED_6    6u
#define EGG_C6_TILE_RESERVED_7    7u

/* ------------------------------------------------------------------------- */
/* Scene geometry                                                            */
/* ------------------------------------------------------------------------- */

/* Enterprise-only opaque backing rectangle behind the complete showcase. */
#define EGG_C6_EP_BACKING_X \
    (TWODFX_ENTERPRISE_BASIC_SAFE_MIN_X + \
     ((TWODFX_ENTERPRISE_BASIC_SAFE_MAX_X - TWODFX_ENTERPRISE_BASIC_SAFE_MIN_X + 1u) - EGG_C6_FRAME_WIDTH) / 2u)
#define EGG_C6_EP_BACKING_Y      38u
#define EGG_C6_EP_BACKING_WIDTH 272u
#define EGG_C6_EP_BACKING_HEIGHT 216u

/* Tilemap destination coordinates. */
#define EGG_C6_EP_MAP_X \
    (TWODFX_ENTERPRISE_BASIC_SAFE_MIN_X + \
     ((TWODFX_ENTERPRISE_BASIC_SAFE_MAX_X - TWODFX_ENTERPRISE_BASIC_SAFE_MIN_X + 1u) - (EGG_C6_MAP_COLUMNS * TWODFX_TILE_WIDTH)) / 2u)
#define EGG_C6_EP_MAP_Y  62u
#define EGG_C6_TVC_MAP_X \
    (TWODFX_TVC_BASIC_SAFE_MIN_X + \
     ((TWODFX_TVC_BASIC_SAFE_MAX_X - TWODFX_TVC_BASIC_SAFE_MIN_X + 1u - \
       (EGG_C6_MAP_COLUMNS * TWODFX_TILE_WIDTH)) / 2u))
#define EGG_C6_TVC_MAP_Y  73u

/* Frame surrounding the 256x96-pixel tilemap with an 8-pixel inset. */
#define EGG_C6_EP_FRAME_X \
    (TWODFX_ENTERPRISE_BASIC_SAFE_MIN_X + \
     ((TWODFX_ENTERPRISE_BASIC_SAFE_MAX_X - TWODFX_ENTERPRISE_BASIC_SAFE_MIN_X + 1u) - EGG_C6_FRAME_WIDTH) / 2u)
#define EGG_C6_EP_FRAME_Y  54u
#define EGG_C6_TVC_FRAME_X \
    (TWODFX_TVC_BASIC_SAFE_MIN_X + \
     ((TWODFX_TVC_BASIC_SAFE_MAX_X - TWODFX_TVC_BASIC_SAFE_MIN_X + 1u - \
       EGG_C6_FRAME_WIDTH) / 2u))
#define EGG_C6_TVC_FRAME_Y  65u
#define EGG_C6_FRAME_WIDTH  272u
#define EGG_C6_FRAME_HEIGHT 112u
#define EGG_C6_FRAME_STROKE   2u

/* Solid profile-colour bar drawn below the map. */
#define EGG_C6_EP_BAR_X \
    (TWODFX_ENTERPRISE_BASIC_SAFE_MIN_X + \
     ((TWODFX_ENTERPRISE_BASIC_SAFE_MAX_X - TWODFX_ENTERPRISE_BASIC_SAFE_MIN_X + 1u) - EGG_C6_BAR_WIDTH) / 2u)
#define EGG_C6_EP_BAR_Y  182u
#define EGG_C6_TVC_BAR_X \
    (TWODFX_TVC_BASIC_SAFE_MIN_X + \
     ((TWODFX_TVC_BASIC_SAFE_MAX_X - TWODFX_TVC_BASIC_SAFE_MIN_X + 1u - \
       EGG_C6_BAR_WIDTH) / 2u))
#define EGG_C6_TVC_BAR_Y 193u
#define EGG_C6_BAR_WIDTH  224u
#define EGG_C6_BAR_HEIGHT   8u

/* ------------------------------------------------------------------------- */
/* Scene colours                                                             */
/* ------------------------------------------------------------------------- */

#define EGG_C6_TRANSPARENT_COLOUR 0u
#define EGG_C6_EP_BACKING_COLOUR  8u
#define EGG_C6_EP_FRAME_COLOUR    7u
#define EGG_C6_TVC_FRAME_COLOUR  15u
#define EGG_C6_EP_BAR_COLOUR      4u
#define EGG_C6_TVC_BAR_COLOUR    13u

/* Tile 0: checker/floor colours. */
#define EGG_C6_EP_TILE0_BASE      1u
#define EGG_C6_EP_TILE0_ACCENT    2u
#define EGG_C6_TVC_TILE0_BASE     5u
#define EGG_C6_TVC_TILE0_ACCENT  11u

/* Tile 1: horizontal-edge/wave colours. */
#define EGG_C6_EP_TILE1_BASE      3u
#define EGG_C6_EP_TILE1_ACCENT    4u
#define EGG_C6_TVC_TILE1_BASE     7u
#define EGG_C6_TVC_TILE1_ACCENT  13u

/* Tile 2: panel border, body and occasional accent. */
#define EGG_C6_EP_TILE2_BODY      5u
#define EGG_C6_EP_TILE2_BORDER    7u
#define EGG_C6_EP_TILE2_ACCENT    2u
#define EGG_C6_TVC_TILE2_BODY    11u
#define EGG_C6_TVC_TILE2_BORDER  15u
#define EGG_C6_TVC_TILE2_ACCENT   9u

/* Tile 3: dark field with feature points and bright sparkles. */
#define EGG_C6_EP_TILE3_BASE      1u
#define EGG_C6_TVC_TILE3_BASE     9u
#define EGG_C6_EP_TILE3_FEATURE   6u
#define EGG_C6_EP_TILE3_SPARKLE   7u
#define EGG_C6_TVC_TILE3_FEATURE 13u
#define EGG_C6_TVC_TILE3_SPARKLE 15u

/* Tile 4: transparent-centre map border with diagonal decoration. */
#define EGG_C6_TILE4_INTERIOR      0u
#define EGG_C6_EP_TILE4_BORDER     7u
#define EGG_C6_EP_TILE4_DIAGONAL   4u
#define EGG_C6_TVC_TILE4_BORDER   15u
#define EGG_C6_TVC_TILE4_DIAGONAL 13u

/* Tile 5: transparent-centre animated marker. */
#define EGG_C6_TILE5_INTERIOR      0u
#define EGG_C6_EP_TILE5_BORDER     6u
#define EGG_C6_EP_TILE5_BLOCK      7u
#define EGG_C6_TVC_TILE5_BORDER    7u
#define EGG_C6_TVC_TILE5_BLOCK    15u

/* ------------------------------------------------------------------------- */
/* Map generation and animation                                              */
/* ------------------------------------------------------------------------- */

/** Repeating floor decoration period. */
#define EGG_C6_WAVE_CELL_MASK 3u

/** Secondary sparse decoration mask. */
#define EGG_C6_SPARKLE_CELL_MASK 7u

/** Move the marker one map column every four update ticks. */
#define EGG_C6_MARKER_X_TICK_SHIFT 2u

/** Move the marker one map row every thirty-two update ticks. */
#define EGG_C6_MARKER_Y_TICK_SHIFT 5u

void egg_C6_tilemap_start(void);
void egg_C6_tilemap_tick(void);

#endif /* EGG_C6_TILEMAP_H */
