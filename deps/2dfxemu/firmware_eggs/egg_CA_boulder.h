#ifndef EGG_CA_BOULDER_H
#define EGG_CA_BOULDER_H

#include "twodfx_api.h"

/*
 * Egg CA — Boulder-style smooth TILEMAP viewport
 * =================================================
 *
 * Start from the host computer with:
 *
 *     OUT 248,202        (202 = CAh)
 *
 * Purpose
 * -------
 * Demonstrates the practical large-level/small-window TILEMAP model:
 *
 * - one complete 160x100-cell level remains resident in RRAM;
 * - one tileset and one tilemap descriptor describe that resident data;
 * - the TILEMAP command selects a moving source-cell viewport;
 * - drawing one extra row and column allows sub-tile destination shifting;
 * - mask rectangles hide the shifted overscan strips;
 * - a normal sprite stays fixed at the viewport centre while the world moves.
 *
 * What appears on screen
 * ----------------------
 * A framed Boulder-style cave scrolls through a deterministic route. The
 * player remains fixed in the centre while the map moves underneath. TVC uses
 * a smaller VT-BASIC-safe viewport and additionally visits source positions
 * near the map edges to demonstrate clipping without wrapping.
 *
 * Implementation style
 * --------------------
 * Editable graphics remain in egg_CA_boulder_tiles.h in readable/import nibble
 * order. The .c file converts one tile and one two-row map strip at a time in small
 * MCU work buffers, uploads them with twodfx_upload_raw(), defines public
 * descriptors, and builds the scene with direct twodfx_2d_* calls. It never
 * touches resource_ram, raw SPBs/2DPBs, shadow tables or private renderer
 * helpers.
 *
 * All tunable resource, geometry, route, timing, colour and slot constants
 * live in this header.
 */

/* ------------------------------------------------------------------------- */
/* Resource allocation and descriptors                                       */
/* ------------------------------------------------------------------------- */

#define EGG_CA_TILESET_ADDRESS       0x5C0000u
#define EGG_CA_LEVEL_ADDRESS         0x5D0000u
#define EGG_CA_PLAYER_SPRITE_ADDRESS 0x5E0000u

#define EGG_CA_TILESET_DESCRIPTOR 2u
#define EGG_CA_TILEMAP_DESCRIPTOR 2u
#define EGG_CA_PLAYER_SPRITE      0u

/* Hardware tile dimensions used by TILEMAP resources. */
#define EGG_CA_TILE_WIDTH  TWODFX_TILE_WIDTH
#define EGG_CA_TILE_HEIGHT TWODFX_TILE_HEIGHT
#define EGG_CA_TILE_BYTES  TWODFX_TILE_BYTES

/* Complete resident level dimensions in hardware tiles. */
#define EGG_CA_LEVEL_COLUMNS 160u
#define EGG_CA_LEVEL_ROWS    100u
#define EGG_CA_LEVEL_BYTES \
    (EGG_CA_LEVEL_COLUMNS * EGG_CA_LEVEL_ROWS)

/* One Boulder-style object is a 2x2 hardware-tile block. */
#define EGG_CA_OBJECT_COLUMNS 2u
#define EGG_CA_OBJECT_ROWS    2u
#define EGG_CA_LEVEL_OBJECT_COLUMNS \
    (EGG_CA_LEVEL_COLUMNS / EGG_CA_OBJECT_COLUMNS)
#define EGG_CA_LEVEL_OBJECT_ROWS \
    (EGG_CA_LEVEL_ROWS / EGG_CA_OBJECT_ROWS)

/* Player sprite dimensions reconstructed from one 2x2 object. */
#define EGG_CA_PLAYER_WIDTH \
    (EGG_CA_OBJECT_COLUMNS * EGG_CA_TILE_WIDTH)
#define EGG_CA_PLAYER_HEIGHT \
    (EGG_CA_OBJECT_ROWS * EGG_CA_TILE_HEIGHT)
#define EGG_CA_PLAYER_BYTES \
    ((EGG_CA_PLAYER_WIDTH / 2u) * EGG_CA_PLAYER_HEIGHT)

/* ------------------------------------------------------------------------- */
/* Profile-specific viewport geometry                                        */
/* ------------------------------------------------------------------------- */

#define EGG_CA_EP_VIEW_COLUMNS 38u
#define EGG_CA_EP_VIEW_ROWS    25u
#define EGG_CA_EP_SCREEN_X    140u
#define EGG_CA_EP_SCREEN_Y     46u
#define EGG_CA_EP_PLAYER_X_CORRECTION  0u
#define EGG_CA_EP_PLAYER_Y_CORRECTION  4u

/*
 * 30x26 tiles = 480x208 pixels. With the 16/8-pixel masks, all commands stay
 * inside the measured TVC VT-BASIC-safe composition used by this egg.
 */
#define EGG_CA_TVC_VIEW_COLUMNS 30u
#define EGG_CA_TVC_VIEW_ROWS    26u
#define EGG_CA_TVC_SCREEN_X    142u
#define EGG_CA_TVC_SCREEN_Y     65u
#define EGG_CA_TVC_PLAYER_X_CORRECTION 0u
#define EGG_CA_TVC_PLAYER_Y_CORRECTION 0u

/* Extra cells drawn while the destination is shifted inside one tile. */
#define EGG_CA_EXTRA_DRAW_COLUMNS 1u
#define EGG_CA_EXTRA_DRAW_ROWS    1u

/* Mask strips surrounding the fixed visible viewport. */
#define EGG_CA_MASK_HORIZONTAL 16u
#define EGG_CA_MASK_VERTICAL    8u

/* White frame surrounding the viewport. */
#define EGG_CA_FRAME_INSET   2u
#define EGG_CA_FRAME_STROKE  4u

/* ------------------------------------------------------------------------- */
/* Scene colours                                                             */
/* ------------------------------------------------------------------------- */

#define EGG_CA_TRANSPARENT_COLOUR 0u
#define EGG_CA_EP_MASK_COLOUR     8u
#define EGG_CA_TVC_MASK_COLOUR    1u
#define EGG_CA_EP_FRAME_COLOUR    7u
#define EGG_CA_TVC_FRAME_COLOUR  15u

/* TVC recolouring of original Enterprise-oriented source indices 0..7. */
#define EGG_CA_TVC_COLOUR_LUT \
    { 0u, 1u, 7u, 3u, 9u, 13u, 5u, 15u }

/* ------------------------------------------------------------------------- */
/* Camera route                                                              */
/* ------------------------------------------------------------------------- */

/** One route point expressed in source hardware-tile coordinates. */
typedef struct {
    uint8_t x;
    uint8_t y;
} egg_CA_view_target_t;

/*
 * Shared route for both profiles. It deliberately visits the right and bottom
 * edges so TILEMAP clipping is demonstrated identically on Enterprise/TVC.
 * The right-edge source X is even because Boulder objects occupy 2x2 tiles.
 */
#define EGG_CA_RIGHT_CLIP_SOURCE_X 146u
#define EGG_CA_ROUTE_VALUES \
    { \
        {   0u,  0u }, {  40u,  0u }, {  80u, 24u }, \
        { 130u, 48u }, \
        { EGG_CA_RIGHT_CLIP_SOURCE_X, 74u }, \
        { EGG_CA_RIGHT_CLIP_SOURCE_X, 88u }, \
        {  80u, 88u }, {   0u, 88u }, {   0u, 48u }, \
        {   0u,  0u } \
    }
#define EGG_CA_ROUTE_COUNT 10u

/* ------------------------------------------------------------------------- */
/* Camera animation                                                          */
/* ------------------------------------------------------------------------- */

#define EGG_CA_FRAMES_PER_SECOND  50u
#define EGG_CA_PAUSE_FRAMES \
    (2u * EGG_CA_FRAMES_PER_SECOND)
#define EGG_CA_SCROLL_HOLD_FRAMES 1u

/** Horizontal raw-pixel step; visually one pixel in the doubled artwork. */
#define EGG_CA_CAMERA_STEP_X 2u

/** Vertical raw-pixel step. */
#define EGG_CA_CAMERA_STEP_Y 1u

/* ------------------------------------------------------------------------- */
/* Deterministic level generation                                            */
/* ------------------------------------------------------------------------- */

#define EGG_CA_VERTICAL_BRICK_PERIOD   20u
#define EGG_CA_HORIZONTAL_BRICK_PERIOD 13u
#define EGG_CA_DIAMOND_MODULUS          37u
#define EGG_CA_ROCK_MODULUS             19u
#define EGG_CA_EMPTY_MODULUS_A          11u
#define EGG_CA_EMPTY_MODULUS_B          17u

/* Hand-placed tunnels and exit used by the demo route. */
#define EGG_CA_TUNNEL1_ROW      6u
#define EGG_CA_TUNNEL1_MIN_X    4u
#define EGG_CA_TUNNEL1_MAX_X   36u
#define EGG_CA_TUNNEL2_ROW     18u
#define EGG_CA_TUNNEL2_MIN_X   22u
#define EGG_CA_TUNNEL2_MAX_X   61u
#define EGG_CA_TUNNEL3_COLUMN  35u
#define EGG_CA_TUNNEL3_MIN_Y    5u
#define EGG_CA_TUNNEL3_MAX_Y   26u
#define EGG_CA_TUNNEL4_COLUMN  58u
#define EGG_CA_TUNNEL4_MIN_Y   18u
#define EGG_CA_TUNNEL4_MAX_Y   43u
#define EGG_CA_EXIT_OBJECT_X   70u
#define EGG_CA_EXIT_OBJECT_Y   42u

/* ------------------------------------------------------------------------- */
/* Fixed background-2DPT slot assignment                                     */
/* ------------------------------------------------------------------------- */

#define EGG_CA_SLOT_SET_MASK    0u
#define EGG_CA_SLOT_BACK_PANEL  1u
#define EGG_CA_SLOT_TILEMAP     2u
#define EGG_CA_SLOT_MASK_TOP    3u
#define EGG_CA_SLOT_MASK_LEFT   4u
#define EGG_CA_SLOT_MASK_RIGHT  5u
#define EGG_CA_SLOT_MASK_BOTTOM 6u
#define EGG_CA_SLOT_SET_FRAME   7u
#define EGG_CA_SLOT_FRAME       8u
#define EGG_CA_SLOT_RET         9u

/* ------------------------------------------------------------------------- */
/* Camera phase state                                                        */
/* ------------------------------------------------------------------------- */

typedef enum {
    EGG_CA_PHASE_PAUSE = 0,
    EGG_CA_PHASE_SCROLL
} egg_CA_phase_t;

void egg_CA_boulder_start(void);
void egg_CA_boulder_tick(void);

#endif /* EGG_CA_BOULDER_H */
