#include "egg_CA_boulder.h"

#include <stdbool.h>
#include <stdint.h>

#include "egg_CA_boulder_tiles.h"

/* ------------------------------------------------------------------------- */
/* Profile geometry and camera state                                         */
/* ------------------------------------------------------------------------- */

static bool g_is_tvc;
static uint8_t g_view_columns;
static uint8_t g_view_rows;
static uint16_t g_screen_x;
static uint16_t g_screen_y;
static uint16_t g_player_x;
static uint16_t g_player_y;

static egg_CA_phase_t g_phase;
static uint16_t g_phase_tick;
static uint8_t g_route_index;

/* Camera and target positions are stored in raw 2dfx pixels. */
static uint16_t g_camera_x;
static uint16_t g_camera_y;
static uint16_t g_target_x;
static uint16_t g_target_y;

/* Ordinary MCU-side build buffers; all RRAM access goes through the API. */
static uint8_t g_tile_buffer[EGG_CA_TILE_BYTES];
static uint8_t g_level_row_pair[EGG_CA_OBJECT_ROWS][EGG_CA_LEVEL_COLUMNS];
static uint8_t g_player_sprite[EGG_CA_PLAYER_BYTES];

static const egg_CA_view_target_t g_route[] = EGG_CA_ROUTE_VALUES;
static const uint8_t g_tvc_colour_lut[8] = EGG_CA_TVC_COLOUR_LUT;

static const egg_CA_view_target_t *active_route(void)
{
    return g_route;
}

static uint8_t active_route_count(void)
{
    return EGG_CA_ROUTE_COUNT;
}

static uint16_t screen_width(void)
{
    return (uint16_t)g_view_columns * EGG_CA_TILE_WIDTH;
}

static uint16_t screen_height(void)
{
    return (uint16_t)g_view_rows * EGG_CA_TILE_HEIGHT;
}

static uint8_t active_mask_colour(void)
{
    return g_is_tvc ? EGG_CA_TVC_MASK_COLOUR : EGG_CA_EP_MASK_COLOUR;
}

static uint8_t active_frame_colour(void)
{
    return g_is_tvc ? EGG_CA_TVC_FRAME_COLOUR : EGG_CA_EP_FRAME_COLOUR;
}

static void select_profile_geometry(void)
{
    if (g_is_tvc) {
        g_view_columns = EGG_CA_TVC_VIEW_COLUMNS;
        g_view_rows = EGG_CA_TVC_VIEW_ROWS;
        g_screen_x = EGG_CA_TVC_SCREEN_X;
        g_screen_y = EGG_CA_TVC_SCREEN_Y;
        g_player_x = (uint16_t)(
            g_screen_x + screen_width() / 2u - EGG_CA_PLAYER_WIDTH / 2u +
            EGG_CA_TVC_PLAYER_X_CORRECTION);
        g_player_y = (uint16_t)(
            g_screen_y + screen_height() / 2u - EGG_CA_PLAYER_HEIGHT / 2u +
            EGG_CA_TVC_PLAYER_Y_CORRECTION);
    } else {
        g_view_columns = EGG_CA_EP_VIEW_COLUMNS;
        g_view_rows = EGG_CA_EP_VIEW_ROWS;
        g_screen_x = EGG_CA_EP_SCREEN_X;
        g_screen_y = EGG_CA_EP_SCREEN_Y;
        g_player_x = (uint16_t)(
            g_screen_x + screen_width() / 2u - EGG_CA_PLAYER_WIDTH / 2u +
            EGG_CA_EP_PLAYER_X_CORRECTION);
        g_player_y = (uint16_t)(
            g_screen_y + screen_height() / 2u - EGG_CA_PLAYER_HEIGHT / 2u +
            EGG_CA_EP_PLAYER_Y_CORRECTION);
    }
}

/* ------------------------------------------------------------------------- */
/* Editable-artwork conversion                                               */
/* ------------------------------------------------------------------------- */

static uint8_t convert_source_colour(uint8_t colour)
{
    return g_is_tvc && colour < 8u ? g_tvc_colour_lut[colour] : colour;
}

static uint8_t convert_readable_pair(uint8_t packed)
{
    uint8_t left = convert_source_colour((uint8_t)(packed >> 4));
    uint8_t right = convert_source_colour((uint8_t)(packed & 0x0Fu));

    /* Internal 2dfx packing stores the left/even pixel in the low nibble. */
    return (uint8_t)(left | (uint8_t)(right << 4));
}

static void build_and_upload_tileset(void)
{
    /* Convert and upload one tile at a time; no full tileset copy is kept. */
    for (uint8_t tile = 0u; tile < BOULDER_TILE_COUNT; ++tile) {
        for (uint8_t i = 0u; i < EGG_CA_TILE_BYTES; ++i) {
            g_tile_buffer[i] =
                convert_readable_pair(boulder_tiles[tile][i]);
        }

        twodfx_upload_raw(EGG_CA_TILESET_ADDRESS +
                              (uint32_t)tile * EGG_CA_TILE_BYTES,
                          g_tile_buffer,
                          sizeof(g_tile_buffer));
    }
}

/* ------------------------------------------------------------------------- */
/* Deterministic resident level                                              */
/* ------------------------------------------------------------------------- */

static bdr_object_t generated_object_at(uint8_t object_x, uint8_t object_y)
{
    if (object_x == 0u || object_y == 0u ||
        object_x == EGG_CA_LEVEL_OBJECT_COLUMNS - 1u ||
        object_y == EGG_CA_LEVEL_OBJECT_ROWS - 1u) {
        return BDR_OBJ_WALL;
    }

    if (((object_x % EGG_CA_VERTICAL_BRICK_PERIOD) == 0u &&
         object_y > 3u) ||
        ((object_y % EGG_CA_HORIZONTAL_BRICK_PERIOD) == 0u &&
         object_x > 4u)) {
        return BDR_OBJ_BRICK;
    }

    if (((uint16_t)object_x * 7u + (uint16_t)object_y * 11u) %
            EGG_CA_DIAMOND_MODULUS ==
        0u) {
        return BDR_OBJ_DIAMOND;
    }

    if (((uint16_t)object_x * 5u + (uint16_t)object_y * 3u) %
            EGG_CA_ROCK_MODULUS ==
        0u) {
        return BDR_OBJ_ROCK;
    }

    if (((object_x + object_y) % EGG_CA_EMPTY_MODULUS_A) == 0u ||
        ((object_x * 3u + object_y) % EGG_CA_EMPTY_MODULUS_B) == 0u) {
        return BDR_OBJ_EMPTY;
    }

    return BDR_OBJ_DIRT;
}

static void write_object_to_row_pair(uint8_t object_x,
                                     bdr_object_t object)
{
    const uint16_t tile_x =
        (uint16_t)object_x * EGG_CA_OBJECT_COLUMNS;
    const uint8_t *tiles = boulder_object_tiles[object];

    g_level_row_pair[0][tile_x + 0u] = tiles[0];
    g_level_row_pair[0][tile_x + 1u] = tiles[1];
    g_level_row_pair[1][tile_x + 0u] = tiles[2];
    g_level_row_pair[1][tile_x + 1u] = tiles[3];
}

static void build_and_upload_level(void)
{
    /*
     * Generate one 2-tile-high object row at a time and upload its 320 map
     * bytes immediately. The complete 16 KiB level therefore lives only in
     * RRAM, not in permanent MCU BSS.
     */
    for (uint8_t object_y = 0u;
         object_y < EGG_CA_LEVEL_OBJECT_ROWS;
         ++object_y) {
        for (uint8_t object_x = 0u;
             object_x < EGG_CA_LEVEL_OBJECT_COLUMNS;
             ++object_x) {
            bdr_object_t object = generated_object_at(object_x, object_y);

            if ((object_y == EGG_CA_TUNNEL1_ROW &&
                 object_x > EGG_CA_TUNNEL1_MIN_X &&
                 object_x < EGG_CA_TUNNEL1_MAX_X) ||
                (object_y == EGG_CA_TUNNEL2_ROW &&
                 object_x > EGG_CA_TUNNEL2_MIN_X &&
                 object_x < EGG_CA_TUNNEL2_MAX_X) ||
                (object_x == EGG_CA_TUNNEL3_COLUMN &&
                 object_y > EGG_CA_TUNNEL3_MIN_Y &&
                 object_y < EGG_CA_TUNNEL3_MAX_Y) ||
                (object_x == EGG_CA_TUNNEL4_COLUMN &&
                 object_y > EGG_CA_TUNNEL4_MIN_Y &&
                 object_y < EGG_CA_TUNNEL4_MAX_Y)) {
                object = BDR_OBJ_EMPTY;
            }

            if (object_x == EGG_CA_EXIT_OBJECT_X &&
                object_y == EGG_CA_EXIT_OBJECT_Y) {
                object = BDR_OBJ_EXIT;
            }

            write_object_to_row_pair(object_x, object);
        }

        twodfx_upload_raw(
            EGG_CA_LEVEL_ADDRESS +
                (uint32_t)object_y * EGG_CA_OBJECT_ROWS *
                    EGG_CA_LEVEL_COLUMNS,
            g_level_row_pair,
            sizeof(g_level_row_pair));
    }
}

/* ------------------------------------------------------------------------- */
/* Fixed centre player sprite                                                */
/* ------------------------------------------------------------------------- */

static void copy_player_tile_row(uint8_t destination_row,
                                 uint8_t destination_byte,
                                 uint8_t tile,
                                 uint8_t source_row)
{
    for (uint8_t i = 0u; i < EGG_CA_TILE_WIDTH / 2u; ++i) {
        g_player_sprite[(uint32_t)destination_row *
                            (EGG_CA_PLAYER_WIDTH / 2u) +
                        destination_byte + i] =
            convert_readable_pair(
                boulder_tiles[tile]
                             [(uint32_t)source_row *
                                  (EGG_CA_TILE_WIDTH / 2u) +
                              i]);
    }
}

static void build_upload_and_place_player(void)
{
    const uint8_t *player_tiles = boulder_object_tiles[BDR_OBJ_PLAYER];

    for (uint8_t row = 0u; row < EGG_CA_TILE_HEIGHT; ++row) {
        copy_player_tile_row(row,
                             0u,
                             player_tiles[0],
                             row);
        copy_player_tile_row(row,
                             EGG_CA_TILE_WIDTH / 2u,
                             player_tiles[1],
                             row);
        copy_player_tile_row((uint8_t)(row + EGG_CA_TILE_HEIGHT),
                             0u,
                             player_tiles[2],
                             row);
        copy_player_tile_row((uint8_t)(row + EGG_CA_TILE_HEIGHT),
                             EGG_CA_TILE_WIDTH / 2u,
                             player_tiles[3],
                             row);
    }

    twodfx_upload_raw(EGG_CA_PLAYER_SPRITE_ADDRESS,
                      g_player_sprite,
                      sizeof(g_player_sprite));

    twodfx_sprite_set(EGG_CA_PLAYER_SPRITE,
                      EGG_CA_PLAYER_SPRITE_ADDRESS,
                      g_player_x,
                      g_player_y,
                      EGG_CA_PLAYER_WIDTH,
                      EGG_CA_PLAYER_HEIGHT,
                      true,
                      false,
                      0u);
}

/* ------------------------------------------------------------------------- */
/* Public TILEMAP scene                                                      */
/* ------------------------------------------------------------------------- */

static void build_scene(void)
{
    const uint8_t source_tile_x =
        (uint8_t)(g_camera_x / EGG_CA_TILE_WIDTH);
    const uint8_t source_tile_y =
        (uint8_t)(g_camera_y / EGG_CA_TILE_HEIGHT);
    const uint8_t sub_tile_x =
        (uint8_t)(g_camera_x & (EGG_CA_TILE_WIDTH - 1u));
    const uint8_t sub_tile_y =
        (uint8_t)(g_camera_y & (EGG_CA_TILE_HEIGHT - 1u));

    const uint16_t width = screen_width();
    const uint16_t height = screen_height();
    const uint16_t draw_x = (uint16_t)(g_screen_x - sub_tile_x);
    const uint16_t draw_y = (uint16_t)(g_screen_y - sub_tile_y);

    twodfx_reset_2dpt_all();

    /* Preserve the original behavior: descriptors are refreshed every tick. */
    twodfx_define_tileset(EGG_CA_TILESET_DESCRIPTOR,
                          EGG_CA_TILESET_ADDRESS);
    twodfx_define_tilemap(EGG_CA_TILEMAP_DESCRIPTOR,
                          EGG_CA_LEVEL_ADDRESS,
                          EGG_CA_LEVEL_COLUMNS,
                          EGG_CA_LEVEL_ROWS,
                          EGG_CA_TILESET_DESCRIPTOR);

    twodfx_2d_set_color(TWODFX_LAYER_BACKGROUND,
                        EGG_CA_SLOT_SET_MASK,
                        active_mask_colour(),
                        EGG_CA_TRANSPARENT_COLOUR);

    twodfx_2d_fill_rect(TWODFX_LAYER_BACKGROUND,
                        EGG_CA_SLOT_BACK_PANEL,
                        g_screen_x,
                        g_screen_y,
                        width,
                        height);

    twodfx_2d_tilemap(TWODFX_LAYER_BACKGROUND,
                      EGG_CA_SLOT_TILEMAP,
                      draw_x,
                      draw_y,
                      EGG_CA_TILEMAP_DESCRIPTOR,
                      source_tile_x,
                      source_tile_y,
                      (uint8_t)(g_view_columns +
                                EGG_CA_EXTRA_DRAW_COLUMNS),
                      (uint8_t)(g_view_rows + EGG_CA_EXTRA_DRAW_ROWS));

    twodfx_2d_fill_rect(TWODFX_LAYER_BACKGROUND,
                        EGG_CA_SLOT_MASK_TOP,
                        (uint16_t)(g_screen_x -
                                   EGG_CA_MASK_HORIZONTAL),
                        (uint16_t)(g_screen_y - EGG_CA_MASK_VERTICAL),
                        (uint16_t)(width +
                                   2u * EGG_CA_MASK_HORIZONTAL),
                        EGG_CA_MASK_VERTICAL);

    twodfx_2d_fill_rect(TWODFX_LAYER_BACKGROUND,
                        EGG_CA_SLOT_MASK_LEFT,
                        (uint16_t)(g_screen_x -
                                   EGG_CA_MASK_HORIZONTAL),
                        g_screen_y,
                        EGG_CA_MASK_HORIZONTAL,
                        height);

    twodfx_2d_fill_rect(TWODFX_LAYER_BACKGROUND,
                        EGG_CA_SLOT_MASK_RIGHT,
                        (uint16_t)(g_screen_x + width),
                        g_screen_y,
                        EGG_CA_MASK_HORIZONTAL,
                        height);

    twodfx_2d_fill_rect(TWODFX_LAYER_BACKGROUND,
                        EGG_CA_SLOT_MASK_BOTTOM,
                        (uint16_t)(g_screen_x -
                                   EGG_CA_MASK_HORIZONTAL),
                        (uint16_t)(g_screen_y + height),
                        (uint16_t)(width +
                                   2u * EGG_CA_MASK_HORIZONTAL),
                        EGG_CA_MASK_VERTICAL);

    twodfx_2d_set_color(TWODFX_LAYER_BACKGROUND,
                        EGG_CA_SLOT_SET_FRAME,
                        active_frame_colour(),
                        EGG_CA_TRANSPARENT_COLOUR);

    twodfx_2d_rect(TWODFX_LAYER_BACKGROUND,
                   EGG_CA_SLOT_FRAME,
                   (uint16_t)(g_screen_x - EGG_CA_FRAME_INSET),
                   (uint16_t)(g_screen_y - EGG_CA_FRAME_INSET),
                   (uint16_t)(width + 2u * EGG_CA_FRAME_INSET),
                   (uint16_t)(height + 2u * EGG_CA_FRAME_INSET),
                   EGG_CA_FRAME_STROKE);

    twodfx_2d_ret(TWODFX_LAYER_BACKGROUND, EGG_CA_SLOT_RET);
}

/* ------------------------------------------------------------------------- */
/* Camera route                                                              */
/* ------------------------------------------------------------------------- */

static void select_next_target(void)
{
    const egg_CA_view_target_t *route = active_route();
    const uint8_t route_count = active_route_count();

    ++g_route_index;
    if (g_route_index >= route_count) {
        /* Route point zero is repeated at the end; continue from point one. */
        g_route_index = 1u;
    }

    g_target_x =
        (uint16_t)route[g_route_index].x * EGG_CA_TILE_WIDTH;
    g_target_y =
        (uint16_t)route[g_route_index].y * EGG_CA_TILE_HEIGHT;
}

static void update_camera_route(void)
{
    ++g_phase_tick;

    if (g_phase == EGG_CA_PHASE_PAUSE) {
        if (g_phase_tick >= EGG_CA_PAUSE_FRAMES) {
            select_next_target();
            g_phase = EGG_CA_PHASE_SCROLL;
            g_phase_tick = 0u;
        }
        return;
    }

    if (g_phase_tick < EGG_CA_SCROLL_HOLD_FRAMES) {
        return;
    }
    g_phase_tick = 0u;

    if (g_camera_x < g_target_x) {
        const uint16_t next =
            (uint16_t)(g_camera_x + EGG_CA_CAMERA_STEP_X);
        g_camera_x = next > g_target_x ? g_target_x : next;
    } else if (g_camera_x > g_target_x) {
        g_camera_x =
            g_camera_x < (uint16_t)(g_target_x + EGG_CA_CAMERA_STEP_X)
                ? g_target_x
                : (uint16_t)(g_camera_x - EGG_CA_CAMERA_STEP_X);
    } else if (g_camera_y < g_target_y) {
        g_camera_y =
            (uint16_t)(g_camera_y + EGG_CA_CAMERA_STEP_Y);
    } else if (g_camera_y > g_target_y) {
        g_camera_y =
            (uint16_t)(g_camera_y - EGG_CA_CAMERA_STEP_Y);
    }

    if (g_camera_x == g_target_x && g_camera_y == g_target_y) {
        g_phase = EGG_CA_PHASE_PAUSE;
        g_phase_tick = 0u;
    }
}

/* ------------------------------------------------------------------------- */
/* Public egg entry points                                                   */
/* ------------------------------------------------------------------------- */

void egg_CA_boulder_start(void)
{
    g_is_tvc = twodfx_get_machine() == TWODFX_MACHINE_TVC;
    select_profile_geometry();
    twodfx_reset_sprites();
    twodfx_reset_2dpt_all();

    g_phase = EGG_CA_PHASE_PAUSE;
    g_phase_tick = 0u;
    g_route_index = 0u;

    const egg_CA_view_target_t *route = active_route();
    g_camera_x = (uint16_t)route[0].x * EGG_CA_TILE_WIDTH;
    g_camera_y = (uint16_t)route[0].y * EGG_CA_TILE_HEIGHT;
    g_target_x = g_camera_x;
    g_target_y = g_camera_y;

    build_and_upload_tileset();
    build_and_upload_level();
    build_upload_and_place_player();
    build_scene();
}

void egg_CA_boulder_tick(void)
{
    update_camera_route();
    build_scene();
}
