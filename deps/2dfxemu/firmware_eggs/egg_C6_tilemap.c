#include "egg_C6_tilemap.h"

#include <stdint.h>
#include <string.h>

#include "twodfx_api.h"

static uint16_t g_tick;
static uint8_t g_tiles[EGG_C6_TILE_COUNT][TWODFX_TILE_BYTES];
static uint8_t g_map[EGG_C6_MAP_ROWS][EGG_C6_MAP_COLUMNS];

/* Pack two internal 4-bit pixels: left in low nibble, right in high nibble. */
static uint8_t pack_pixels(uint8_t left, uint8_t right)
{
    return (uint8_t)((left & 0x0Fu) | ((right & 0x0Fu) << 4));
}

/* Write one pixel into one generated 16x8 packed tile. */
static void write_tile_pixel(uint8_t tile,
                             uint8_t x,
                             uint8_t y,
                             uint8_t colour)
{
    const uint32_t offset = (uint32_t)y * 8u + (x >> 1);
    uint8_t packed = g_tiles[tile][offset];

    if ((x & 1u) == 0u) {
        packed = (uint8_t)((packed & 0xF0u) | (colour & 0x0Fu));
    } else {
        packed = (uint8_t)((packed & 0x0Fu) | ((colour & 0x0Fu) << 4));
    }

    g_tiles[tile][offset] = packed;
}

/* Fill one generated tile with a single colour before drawing its details. */
static void clear_tile(uint8_t tile, uint8_t colour)
{
    memset(g_tiles[tile],
           pack_pixels(colour, colour),
           TWODFX_TILE_BYTES);
}

/* Generate all tiles once and upload the complete contiguous tileset bank. */
static void build_and_upload_tileset(void)
{
    const bool tvc = twodfx_get_machine() == TWODFX_MACHINE_TVC;

    for (uint8_t tile = 0u; tile < EGG_C6_TILE_COUNT; ++tile) {
        clear_tile(tile, EGG_C6_TRANSPARENT_COLOUR);
    }

    for (uint8_t y = 0u; y < TWODFX_TILE_HEIGHT; ++y) {
        for (uint8_t x = 0u; x < TWODFX_TILE_WIDTH; ++x) {
            const uint8_t checker = (((x + y) & 3u) != 0u)
                ? (tvc ? EGG_C6_TVC_TILE0_BASE : EGG_C6_EP_TILE0_BASE)
                : (tvc ? EGG_C6_TVC_TILE0_ACCENT : EGG_C6_EP_TILE0_ACCENT);
            write_tile_pixel(EGG_C6_TILE_CHECKER, x, y, checker);

            const bool wave_accent =
                y == 0u || y == 7u || ((x + y) & 7u) < 3u;
            const uint8_t waves = wave_accent
                ? (tvc ? EGG_C6_TVC_TILE1_ACCENT : EGG_C6_EP_TILE1_ACCENT)
                : (tvc ? EGG_C6_TVC_TILE1_BASE : EGG_C6_EP_TILE1_BASE);
            write_tile_pixel(EGG_C6_TILE_WAVES, x, y, waves);

            uint8_t panel =
                (x == 0u || x == 15u || y == 0u || y == 7u)
                    ? (tvc ? EGG_C6_TVC_TILE2_BORDER : EGG_C6_EP_TILE2_BORDER)
                    : (tvc ? EGG_C6_TVC_TILE2_BODY : EGG_C6_EP_TILE2_BODY);
            if (((x ^ y) & 3u) == 0u) {
                panel = tvc ? EGG_C6_TVC_TILE2_ACCENT
                            : EGG_C6_EP_TILE2_ACCENT;
            }
            write_tile_pixel(EGG_C6_TILE_PANEL, x, y, panel);

            uint8_t sparkles = tvc ? EGG_C6_TVC_TILE3_BASE
                                   : EGG_C6_EP_TILE3_BASE;
            if ((x == 4u || x == 11u) && (y == 2u || y == 5u)) {
                sparkles = tvc ? EGG_C6_TVC_TILE3_FEATURE
                               : EGG_C6_EP_TILE3_FEATURE;
            }
            if ((x + y) % 9u == 0u) {
                sparkles = tvc ? EGG_C6_TVC_TILE3_SPARKLE
                               : EGG_C6_EP_TILE3_SPARKLE;
            }
            write_tile_pixel(EGG_C6_TILE_SPARKLES, x, y, sparkles);

            uint8_t border =
                (x == 0u || x == 15u || y == 0u || y == 7u)
                    ? (tvc ? EGG_C6_TVC_TILE4_BORDER
                           : EGG_C6_EP_TILE4_BORDER)
                    : EGG_C6_TILE4_INTERIOR;
            if (x == y * 2u || x == 15u - y * 2u) {
                border = tvc ? EGG_C6_TVC_TILE4_DIAGONAL
                             : EGG_C6_EP_TILE4_DIAGONAL;
            }
            write_tile_pixel(EGG_C6_TILE_BORDER, x, y, border);

            uint8_t marker =
                (x == 0u || x == 15u || y == 0u || y == 7u)
                    ? (tvc ? EGG_C6_TVC_TILE5_BORDER
                           : EGG_C6_EP_TILE5_BORDER)
                    : EGG_C6_TILE5_INTERIOR;
            if (x >= 5u && x <= 10u && y >= 2u && y <= 5u) {
                marker = tvc ? EGG_C6_TVC_TILE5_BLOCK
                             : EGG_C6_EP_TILE5_BLOCK;
            }
            write_tile_pixel(EGG_C6_TILE_MOVING_MARKER, x, y, marker);
        }
    }

    twodfx_upload_raw(EGG_C6_TILESET_ADDRESS,
                      g_tiles,
                      sizeof(g_tiles));
}

/*
 * Rebuild and upload only the map cells. Descriptors, tiles and the 2DPT
 * playlist remain unchanged while the marker moves.
 */
static void build_and_upload_tilemap(void)
{
    for (uint8_t y = 0u; y < EGG_C6_MAP_ROWS; ++y) {
        for (uint8_t x = 0u; x < EGG_C6_MAP_COLUMNS; ++x) {
            uint8_t tile;

            if (x == 0u || x == EGG_C6_MAP_COLUMNS - 1u ||
                y == 0u || y == EGG_C6_MAP_ROWS - 1u) {
                tile = EGG_C6_TILE_BORDER;
            } else if (((x + y) & EGG_C6_WAVE_CELL_MASK) == 0u) {
                tile = EGG_C6_TILE_WAVES;
            } else if (((x * 3u + y) & EGG_C6_SPARKLE_CELL_MASK) == 0u) {
                tile = EGG_C6_TILE_SPARKLES;
            } else {
                tile = EGG_C6_TILE_CHECKER;
            }

            g_map[y][x] = tile;
        }
    }

    const uint8_t marker_x = (uint8_t)(
        1u + ((g_tick >> EGG_C6_MARKER_X_TICK_SHIFT) %
              (EGG_C6_MAP_COLUMNS - 2u)));
    const uint8_t marker_y = (uint8_t)(
        1u + ((g_tick >> EGG_C6_MARKER_Y_TICK_SHIFT) %
              (EGG_C6_MAP_ROWS - 2u)));
    g_map[marker_y][marker_x] = EGG_C6_TILE_MOVING_MARKER;

    twodfx_upload_raw(EGG_C6_TILEMAP_ADDRESS,
                      g_map,
                      sizeof(g_map));
}

/* Build the static background 2DPT playlist once when the egg starts. */
static void build_scene(void)
{
    uint8_t slot = 0u;
    const bool tvc = twodfx_get_machine() == TWODFX_MACHINE_TVC;

    const uint16_t map_x = tvc ? EGG_C6_TVC_MAP_X : EGG_C6_EP_MAP_X;
    const uint16_t map_y = tvc ? EGG_C6_TVC_MAP_Y : EGG_C6_EP_MAP_Y;
    const uint16_t frame_x = tvc ? EGG_C6_TVC_FRAME_X : EGG_C6_EP_FRAME_X;
    const uint16_t frame_y = tvc ? EGG_C6_TVC_FRAME_Y : EGG_C6_EP_FRAME_Y;
    const uint16_t bar_x = tvc ? EGG_C6_TVC_BAR_X : EGG_C6_EP_BAR_X;
    const uint16_t bar_y = tvc ? EGG_C6_TVC_BAR_Y : EGG_C6_EP_BAR_Y;

    twodfx_reset_2dpt_all();
    twodfx_define_tileset(EGG_C6_TILESET_DESCRIPTOR,
                          EGG_C6_TILESET_ADDRESS);
    twodfx_define_tilemap(EGG_C6_TILEMAP_DESCRIPTOR,
                          EGG_C6_TILEMAP_ADDRESS,
                          EGG_C6_MAP_COLUMNS,
                          EGG_C6_MAP_ROWS,
                          EGG_C6_TILESET_DESCRIPTOR);

    if (!tvc) {
        twodfx_2d_set_color(TWODFX_LAYER_BACKGROUND,
                            slot++,
                            EGG_C6_EP_BACKING_COLOUR,
                            EGG_C6_TRANSPARENT_COLOUR);
        twodfx_2d_fill_rect(TWODFX_LAYER_BACKGROUND,
                            slot++,
                            EGG_C6_EP_BACKING_X,
                            EGG_C6_EP_BACKING_Y,
                            EGG_C6_EP_BACKING_WIDTH,
                            EGG_C6_EP_BACKING_HEIGHT);
    }

    twodfx_2d_tilemap(TWODFX_LAYER_BACKGROUND,
                      slot++,
                      map_x,
                      map_y,
                      EGG_C6_TILEMAP_DESCRIPTOR,
                      EGG_C6_SOURCE_TILE_X,
                      EGG_C6_SOURCE_TILE_Y,
                      EGG_C6_VIEW_COLUMNS,
                      EGG_C6_VIEW_ROWS);

    twodfx_2d_set_color(TWODFX_LAYER_BACKGROUND,
                        slot++,
                        tvc ? EGG_C6_TVC_FRAME_COLOUR
                            : EGG_C6_EP_FRAME_COLOUR,
                        EGG_C6_TRANSPARENT_COLOUR);
    twodfx_2d_rect(TWODFX_LAYER_BACKGROUND,
                   slot++,
                   frame_x,
                   frame_y,
                   EGG_C6_FRAME_WIDTH,
                   EGG_C6_FRAME_HEIGHT,
                   EGG_C6_FRAME_STROKE);

    twodfx_2d_set_color(TWODFX_LAYER_BACKGROUND,
                        slot++,
                        tvc ? EGG_C6_TVC_BAR_COLOUR
                            : EGG_C6_EP_BAR_COLOUR,
                        EGG_C6_TRANSPARENT_COLOUR);
    twodfx_2d_fill_rect(TWODFX_LAYER_BACKGROUND,
                        slot++,
                        bar_x,
                        bar_y,
                        EGG_C6_BAR_WIDTH,
                        EGG_C6_BAR_HEIGHT);

    twodfx_2d_ret(TWODFX_LAYER_BACKGROUND, slot);
}

void egg_C6_tilemap_start(void)
{
    twodfx_reset_sprites();
    g_tick = 0u;

    build_and_upload_tileset();
    build_and_upload_tilemap();
    build_scene();
}

void egg_C6_tilemap_tick(void)
{
    ++g_tick;
    build_and_upload_tilemap();
}
