#include "egg_C5_pattern.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "twodfx_api.h"

static uint16_t g_tick;
static bool g_is_tvc;
static uint8_t g_patterns[EGG_C5_PATTERN_COUNT][TWODFX_PATTERN_BYTES];

/* Pack two internal 4-bit pixels: left in low nibble, right in high nibble. */
static uint8_t pack_pixels(uint8_t left, uint8_t right)
{
    return (uint8_t)((left & 0x0Fu) | ((right & 0x0Fu) << 4));
}

/* Write one pixel into the generated 16x8 packed pattern bank. */
static void write_pattern_pixel(uint8_t pattern,
                                uint8_t x,
                                uint8_t y,
                                uint8_t colour)
{
    const uint32_t offset = (uint32_t)y * 8u + (x >> 1);
    uint8_t packed = g_patterns[pattern][offset];

    if ((x & 1u) == 0u) {
        packed = (uint8_t)((packed & 0xF0u) | (colour & 0x0Fu));
    } else {
        packed = (uint8_t)((packed & 0x0Fu) | ((colour & 0x0Fu) << 4));
    }

    g_patterns[pattern][offset] = packed;
}

/* Fill one generated pattern with a single colour before drawing details. */
static void clear_pattern(uint8_t pattern, uint8_t colour)
{
    memset(g_patterns[pattern],
           pack_pixels(colour, colour),
           TWODFX_PATTERN_BYTES);
}

/* Generate the eight demonstration patterns and upload the complete bank. */
static void build_patterns(void)
{
    for (uint8_t pattern = 0u; pattern < EGG_C5_PATTERN_COUNT; ++pattern) {
        clear_pattern(pattern, 0u);
    }

    for (uint8_t y = 0u; y < TWODFX_PATTERN_HEIGHT; ++y) {
        for (uint8_t x = 0u; x < TWODFX_PATTERN_WIDTH; ++x) {
            const uint8_t diagonal_bands = g_is_tvc
                ? ((((x + y * 2u) & 7u) < 3u) ? 13u : 3u)
                : ((((x + y * 2u) & 7u) < 3u) ? 4u : 3u);
            write_pattern_pixel(EGG_C5_PATTERN_DIAGONAL_BANDS,
                                x, y, diagonal_bands);

            const uint8_t block_checker = g_is_tvc
                ? (((((x >> 2) ^ (y >> 1)) & 1u) != 0u) ? 7u : 5u)
                : (((((x >> 2) ^ (y >> 1)) & 1u) != 0u) ? 5u : 2u);
            write_pattern_pixel(EGG_C5_PATTERN_BLOCK_CHECKER,
                                x, y, block_checker);

            uint8_t dots;
            if (g_is_tvc) {
                /* Keep the hardware-approved TVC pattern byte-for-byte. */
                dots = 9u;
                if (((x == 3u || x == 11u) && (y == 2u || y == 6u)) ||
                    ((x == 7u || x == 15u) && y == 4u)) {
                    dots = 15u;
                }
            } else {
                /*
                 * Enterprise/NICK normal mode is deliberately shown a bold
                 * multi-pixel weave rather than isolated one-pixel details.
                 * This avoids a misleading pattern whose fine dots can be
                 * lost by the host's single-pixel/transparency behaviour.
                 */
                dots = ((((x >> 2) + (y >> 1)) & 1u) != 0u) ? 5u : 8u;
            }
            write_pattern_pixel(EGG_C5_PATTERN_DOTS, x, y, dots);

            uint8_t bricks = g_is_tvc ? 3u : 2u;
            if (y == 0u || y == 7u) {
                bricks = 7u;
            } else if (((y < 4u) && (x == 0u || x == 8u)) ||
                       ((y >= 4u) && (x == 4u || x == 12u))) {
                bricks = 7u;
            }
            write_pattern_pixel(EGG_C5_PATTERN_BRICKS, x, y, bricks);

            const uint8_t vertical_rails = g_is_tvc
                ? (((x & 3u) == 0u) ? 13u
                   : ((((x + y) & 1u) != 0u) ? 11u : 9u))
                : (((x & 3u) == 0u) ? 7u
                   : ((((x + y) & 1u) != 0u) ? 3u : 1u));
            write_pattern_pixel(EGG_C5_PATTERN_VERTICAL_RAILS,
                                x, y, vertical_rails);

            const uint8_t crosshatch =
                ((((x + y) & 7u) == 0u) ||
                 (((x + 15u - y) & 7u) == 0u))
                    ? 7u : (g_is_tvc ? 11u : 5u);
            write_pattern_pixel(EGG_C5_PATTERN_CROSSHATCH,
                                x, y, crosshatch);

            const uint8_t texture = g_is_tvc
                ? ((((x * 3u + y * 5u) & 7u) < 4u) ? 13u : 5u)
                : ((((x * 3u + y * 5u) & 7u) < 4u) ? 6u : 4u);
            write_pattern_pixel(EGG_C5_PATTERN_TEXTURE, x, y, texture);

            uint8_t border =
                (x == 0u || x == 15u || y == 0u || y == 7u)
                    ? (g_is_tvc ? 15u : 7u) : 0u;
            if (((x + y) & 7u) == 3u) {
                border = g_is_tvc ? 9u : 4u;
            }
            write_pattern_pixel(EGG_C5_PATTERN_BORDER, x, y, border);
        }
    }

    twodfx_upload_raw(EGG_C5_PATTERN_BANK_ADDRESS,
                      g_patterns,
                      sizeof(g_patterns));
}

/* Build the complete background 2DPT playlist for the current update tick. */
static void build_scene(void)
{
    uint8_t slot = 0u;
    const bool tvc = twodfx_get_machine() == TWODFX_MACHINE_TVC;

    const uint16_t x = tvc ? EGG_C5_TVC_X : EGG_C5_EP_X;
    const uint16_t y = tvc ? EGG_C5_TVC_Y : EGG_C5_EP_Y;
    const uint16_t width = tvc ? EGG_C5_TVC_WIDTH : EGG_C5_EP_WIDTH;
    const uint16_t height = tvc ? EGG_C5_TVC_HEIGHT : EGG_C5_EP_HEIGHT;

    twodfx_reset_2dpt_all();
    twodfx_define_pattern(EGG_C5_PATTERN_DESCRIPTOR,
                          EGG_C5_PATTERN_BANK_ADDRESS);

    twodfx_2d_pattern_fill(
        TWODFX_LAYER_BACKGROUND,
        slot++,
        x, y, width, height,
        EGG_C5_PATTERN_DESCRIPTOR,
        (uint8_t)((g_tick >> EGG_C5_BACKGROUND_PATTERN_SHIFT) &
                  EGG_C5_PATTERN_NUMBER_MASK));

    if (tvc) {
        twodfx_2d_pattern_fill(
            TWODFX_LAYER_BACKGROUND,
            slot++,
            x + EGG_C5_TVC_TOP_LEFT_X_OFFSET,
            y + EGG_C5_TVC_TOP_Y_OFFSET,
            EGG_C5_TVC_TOP_PANEL_WIDTH,
            EGG_C5_TOP_PANEL_HEIGHT,
            EGG_C5_PATTERN_DESCRIPTOR,
            EGG_C5_PATTERN_DIAGONAL_BANDS);

        twodfx_2d_pattern_fill(
            TWODFX_LAYER_BACKGROUND,
            slot++,
            x + EGG_C5_TVC_TOP_RIGHT_X_OFFSET,
            y + EGG_C5_TVC_TOP_Y_OFFSET,
            EGG_C5_TVC_TOP_PANEL_WIDTH,
            EGG_C5_TOP_PANEL_HEIGHT,
            EGG_C5_PATTERN_DESCRIPTOR,
            EGG_C5_PATTERN_BLOCK_CHECKER);

        twodfx_2d_pattern_fill(
            TWODFX_LAYER_BACKGROUND,
            slot++,
            x + EGG_C5_TVC_MIDDLE_X0_OFFSET,
            y + EGG_C5_TVC_MIDDLE_Y_OFFSET,
            EGG_C5_TVC_MIDDLE_PANEL_WIDTH,
            EGG_C5_MIDDLE_PANEL_HEIGHT,
            EGG_C5_PATTERN_DESCRIPTOR,
            EGG_C5_PATTERN_DOTS);

        twodfx_2d_pattern_fill(
            TWODFX_LAYER_BACKGROUND,
            slot++,
            x + EGG_C5_TVC_MIDDLE_X1_OFFSET,
            y + EGG_C5_TVC_MIDDLE_Y_OFFSET,
            EGG_C5_TVC_MIDDLE_PANEL_WIDTH,
            EGG_C5_MIDDLE_PANEL_HEIGHT,
            EGG_C5_PATTERN_DESCRIPTOR,
            EGG_C5_PATTERN_BRICKS);

        twodfx_2d_pattern_fill(
            TWODFX_LAYER_BACKGROUND,
            slot++,
            x + EGG_C5_TVC_MIDDLE_X2_OFFSET,
            y + EGG_C5_TVC_MIDDLE_Y_OFFSET,
            EGG_C5_TVC_MIDDLE_PANEL_WIDTH,
            EGG_C5_MIDDLE_PANEL_HEIGHT,
            EGG_C5_PATTERN_DESCRIPTOR,
            EGG_C5_PATTERN_VERTICAL_RAILS);

        twodfx_2d_pattern_fill(
            TWODFX_LAYER_BACKGROUND,
            slot++,
            x + EGG_C5_TVC_MIDDLE_X3_OFFSET,
            y + EGG_C5_TVC_MIDDLE_Y_OFFSET,
            EGG_C5_TVC_MIDDLE_PANEL_WIDTH,
            EGG_C5_MIDDLE_PANEL_HEIGHT,
            EGG_C5_PATTERN_DESCRIPTOR,
            EGG_C5_PATTERN_CROSSHATCH);

        twodfx_2d_pattern_fill(
            TWODFX_LAYER_BACKGROUND,
            slot++,
            x + EGG_C5_PANEL_INSET_X,
            y + EGG_C5_TVC_TICKER_Y_OFFSET,
            EGG_C5_TVC_TICKER_WIDTH,
            EGG_C5_TICKER_HEIGHT,
            EGG_C5_PATTERN_DESCRIPTOR,
            (uint8_t)((g_tick >> EGG_C5_TICKER_PATTERN_SHIFT) &
                      EGG_C5_PATTERN_NUMBER_MASK));

        const uint16_t highlight_x = (uint16_t)(
            x + EGG_C5_PANEL_INSET_X +
            ((g_tick * EGG_C5_HIGHLIGHT_STEP_X) %
             EGG_C5_TVC_HIGHLIGHT_TRAVEL));

        twodfx_2d_set_color(TWODFX_LAYER_BACKGROUND,
                            slot++,
                            EGG_C5_TVC_FRAME_COLOUR,
                            EGG_C5_FRAME_BACKGROUND_COLOUR);
        twodfx_2d_fill_rect(TWODFX_LAYER_BACKGROUND,
                            slot++,
                            highlight_x,
                            y + EGG_C5_TVC_TICKER_Y_OFFSET,
                            EGG_C5_HIGHLIGHT_WIDTH,
                            EGG_C5_HIGHLIGHT_HEIGHT);

        twodfx_2d_set_color(TWODFX_LAYER_BACKGROUND,
                            slot++,
                            EGG_C5_TVC_FRAME_COLOUR,
                            EGG_C5_FRAME_BACKGROUND_COLOUR);
        twodfx_2d_rect(TWODFX_LAYER_BACKGROUND,
                       slot++,
                       x, y, width, height,
                       EGG_C5_OUTER_FRAME_STROKE);

        twodfx_2d_set_color(TWODFX_LAYER_BACKGROUND,
                            slot++,
                            EGG_C5_TVC_PANEL_FRAME_COLOUR,
                            EGG_C5_FRAME_BACKGROUND_COLOUR);
        twodfx_2d_rect(TWODFX_LAYER_BACKGROUND,
                       slot++,
                       x + EGG_C5_TVC_TOP_LEFT_X_OFFSET,
                       y + EGG_C5_TVC_TOP_Y_OFFSET,
                       EGG_C5_TVC_TOP_PANEL_WIDTH,
                       EGG_C5_TOP_PANEL_HEIGHT,
                       EGG_C5_PANEL_FRAME_STROKE);
        twodfx_2d_rect(TWODFX_LAYER_BACKGROUND,
                       slot++,
                       x + EGG_C5_TVC_TOP_RIGHT_X_OFFSET,
                       y + EGG_C5_TVC_TOP_Y_OFFSET,
                       EGG_C5_TVC_TOP_PANEL_WIDTH,
                       EGG_C5_TOP_PANEL_HEIGHT,
                       EGG_C5_PANEL_FRAME_STROKE);
        twodfx_2d_rect(TWODFX_LAYER_BACKGROUND,
                       slot++,
                       x + EGG_C5_TVC_MIDDLE_X0_OFFSET,
                       y + EGG_C5_TVC_MIDDLE_Y_OFFSET,
                       EGG_C5_TVC_MIDDLE_PANEL_WIDTH,
                       EGG_C5_MIDDLE_PANEL_HEIGHT,
                       EGG_C5_PANEL_FRAME_STROKE);
        twodfx_2d_rect(TWODFX_LAYER_BACKGROUND,
                       slot++,
                       x + EGG_C5_TVC_MIDDLE_X1_OFFSET,
                       y + EGG_C5_TVC_MIDDLE_Y_OFFSET,
                       EGG_C5_TVC_MIDDLE_PANEL_WIDTH,
                       EGG_C5_MIDDLE_PANEL_HEIGHT,
                       EGG_C5_PANEL_FRAME_STROKE);
        twodfx_2d_rect(TWODFX_LAYER_BACKGROUND,
                       slot++,
                       x + EGG_C5_TVC_MIDDLE_X2_OFFSET,
                       y + EGG_C5_TVC_MIDDLE_Y_OFFSET,
                       EGG_C5_TVC_MIDDLE_PANEL_WIDTH,
                       EGG_C5_MIDDLE_PANEL_HEIGHT,
                       EGG_C5_PANEL_FRAME_STROKE);
        twodfx_2d_rect(TWODFX_LAYER_BACKGROUND,
                       slot++,
                       x + EGG_C5_TVC_MIDDLE_X3_OFFSET,
                       y + EGG_C5_TVC_MIDDLE_Y_OFFSET,
                       EGG_C5_TVC_MIDDLE_PANEL_WIDTH,
                       EGG_C5_MIDDLE_PANEL_HEIGHT,
                       EGG_C5_PANEL_FRAME_STROKE);
    } else {
        twodfx_2d_pattern_fill(
            TWODFX_LAYER_BACKGROUND,
            slot++,
            x + EGG_C5_EP_TOP_LEFT_X_OFFSET,
            y + EGG_C5_TOP_Y_OFFSET,
            EGG_C5_EP_TOP_PANEL_WIDTH,
            EGG_C5_TOP_PANEL_HEIGHT,
            EGG_C5_PATTERN_DESCRIPTOR,
            EGG_C5_PATTERN_DIAGONAL_BANDS);

        twodfx_2d_pattern_fill(
            TWODFX_LAYER_BACKGROUND,
            slot++,
            x + EGG_C5_EP_TOP_RIGHT_X_OFFSET,
            y + EGG_C5_TOP_Y_OFFSET,
            EGG_C5_EP_TOP_PANEL_WIDTH,
            EGG_C5_TOP_PANEL_HEIGHT,
            EGG_C5_PATTERN_DESCRIPTOR,
            EGG_C5_PATTERN_BLOCK_CHECKER);

        twodfx_2d_pattern_fill(
            TWODFX_LAYER_BACKGROUND,
            slot++,
            x + EGG_C5_EP_MIDDLE_X0_OFFSET,
            y + EGG_C5_MIDDLE_Y_OFFSET,
            EGG_C5_EP_MIDDLE_PANEL_WIDTH,
            EGG_C5_MIDDLE_PANEL_HEIGHT,
            EGG_C5_PATTERN_DESCRIPTOR,
            EGG_C5_PATTERN_DOTS);

        twodfx_2d_pattern_fill(
            TWODFX_LAYER_BACKGROUND,
            slot++,
            x + EGG_C5_EP_MIDDLE_X1_OFFSET,
            y + EGG_C5_MIDDLE_Y_OFFSET,
            EGG_C5_EP_MIDDLE_PANEL_WIDTH,
            EGG_C5_MIDDLE_PANEL_HEIGHT,
            EGG_C5_PATTERN_DESCRIPTOR,
            EGG_C5_PATTERN_BRICKS);

        twodfx_2d_pattern_fill(
            TWODFX_LAYER_BACKGROUND,
            slot++,
            x + EGG_C5_EP_MIDDLE_X2_OFFSET,
            y + EGG_C5_MIDDLE_Y_OFFSET,
            EGG_C5_EP_MIDDLE_PANEL_WIDTH,
            EGG_C5_MIDDLE_PANEL_HEIGHT,
            EGG_C5_PATTERN_DESCRIPTOR,
            EGG_C5_PATTERN_TEXTURE);

        twodfx_2d_pattern_fill(
            TWODFX_LAYER_BACKGROUND,
            slot++,
            x + EGG_C5_PANEL_INSET_X,
            y + EGG_C5_TICKER_Y_OFFSET,
            EGG_C5_EP_TICKER_WIDTH,
            EGG_C5_TICKER_HEIGHT,
            EGG_C5_PATTERN_DESCRIPTOR,
            (uint8_t)((g_tick >> EGG_C5_TICKER_PATTERN_SHIFT) &
                      EGG_C5_PATTERN_NUMBER_MASK));

        const uint16_t highlight_x = (uint16_t)(
            x + EGG_C5_PANEL_INSET_X +
            ((g_tick * EGG_C5_HIGHLIGHT_STEP_X) %
             EGG_C5_EP_HIGHLIGHT_TRAVEL));

        twodfx_2d_set_color(TWODFX_LAYER_BACKGROUND,
                            slot++,
                            EGG_C5_FRAME_COLOUR,
                            EGG_C5_FRAME_BACKGROUND_COLOUR);
        twodfx_2d_fill_rect(TWODFX_LAYER_BACKGROUND,
                            slot++,
                            highlight_x,
                            y + EGG_C5_TICKER_Y_OFFSET,
                            EGG_C5_HIGHLIGHT_WIDTH,
                            EGG_C5_HIGHLIGHT_HEIGHT);

        twodfx_2d_set_color(TWODFX_LAYER_BACKGROUND,
                            slot++,
                            EGG_C5_FRAME_COLOUR,
                            EGG_C5_FRAME_BACKGROUND_COLOUR);
        twodfx_2d_rect(TWODFX_LAYER_BACKGROUND,
                       slot++,
                       x, y, width, height,
                       EGG_C5_OUTER_FRAME_STROKE);

        twodfx_2d_set_color(TWODFX_LAYER_BACKGROUND,
                            slot++,
                            EGG_C5_PANEL_FRAME_COLOUR,
                            EGG_C5_FRAME_BACKGROUND_COLOUR);
        twodfx_2d_rect(TWODFX_LAYER_BACKGROUND,
                       slot++,
                       x + EGG_C5_EP_TOP_LEFT_X_OFFSET,
                       y + EGG_C5_TOP_Y_OFFSET,
                       EGG_C5_EP_TOP_PANEL_WIDTH,
                       EGG_C5_TOP_PANEL_HEIGHT,
                       EGG_C5_PANEL_FRAME_STROKE);
        twodfx_2d_rect(TWODFX_LAYER_BACKGROUND,
                       slot++,
                       x + EGG_C5_EP_TOP_RIGHT_X_OFFSET,
                       y + EGG_C5_TOP_Y_OFFSET,
                       EGG_C5_EP_TOP_PANEL_WIDTH,
                       EGG_C5_TOP_PANEL_HEIGHT,
                       EGG_C5_PANEL_FRAME_STROKE);
        twodfx_2d_rect(TWODFX_LAYER_BACKGROUND,
                       slot++,
                       x + EGG_C5_EP_MIDDLE_X0_OFFSET,
                       y + EGG_C5_MIDDLE_Y_OFFSET,
                       EGG_C5_EP_MIDDLE_PANEL_WIDTH,
                       EGG_C5_MIDDLE_PANEL_HEIGHT,
                       EGG_C5_PANEL_FRAME_STROKE);
        twodfx_2d_rect(TWODFX_LAYER_BACKGROUND,
                       slot++,
                       x + EGG_C5_EP_MIDDLE_X1_OFFSET,
                       y + EGG_C5_MIDDLE_Y_OFFSET,
                       EGG_C5_EP_MIDDLE_PANEL_WIDTH,
                       EGG_C5_MIDDLE_PANEL_HEIGHT,
                       EGG_C5_PANEL_FRAME_STROKE);
        twodfx_2d_rect(TWODFX_LAYER_BACKGROUND,
                       slot++,
                       x + EGG_C5_EP_MIDDLE_X2_OFFSET,
                       y + EGG_C5_MIDDLE_Y_OFFSET,
                       EGG_C5_EP_MIDDLE_PANEL_WIDTH,
                       EGG_C5_MIDDLE_PANEL_HEIGHT,
                       EGG_C5_PANEL_FRAME_STROKE);
    }

    twodfx_2d_ret(TWODFX_LAYER_BACKGROUND, slot);
}

void egg_C5_pattern_start(void)
{
    twodfx_reset_sprites();
    g_tick = 0u;
    g_is_tvc = twodfx_get_machine() == TWODFX_MACHINE_TVC;

    build_patterns();
    build_scene();
}

void egg_C5_pattern_tick(void)
{
    ++g_tick;
    build_scene();
}
