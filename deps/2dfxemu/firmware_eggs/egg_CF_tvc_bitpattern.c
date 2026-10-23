#include "egg_CF_tvc_bitpattern.h"

#include <stdint.h>

/* One packed scanline is sufficient to generate the complete bitmap. */
static uint8_t g_timing_row[EGG_CF_BITMAP_ROW_BYTES];

static uint8_t pack_pixels(uint8_t left, uint8_t right)
{
    return (uint8_t)((left & 0x0Fu) | ((right & 0x0Fu) << 4));
}

/* ------------------------------------------------------------------------- */
/* Procedural timing bitmap                                                  */
/* ------------------------------------------------------------------------- */

static uint8_t pattern_pixel_colour(uint8_t pattern, uint16_t x)
{
    uint8_t bit_position =
        (uint8_t)(x & (EGG_CF_PATTERN_PIXEL_PERIOD - 1u));
    uint8_t mask =
        (uint8_t)(1u << (EGG_CF_PATTERN_MSB - bit_position));

    return (pattern & mask) ? EGG_CF_ONE_COLOUR : EGG_CF_ZERO_COLOUR;
}

static void generate_pattern_row(uint8_t pattern)
{
    for (uint16_t byte_x = 0u;
         byte_x < EGG_CF_BITMAP_ROW_BYTES;
         ++byte_x) {
        uint16_t left_x = (uint16_t)(byte_x * 2u);
        uint8_t left = pattern_pixel_colour(pattern, left_x);
        uint8_t right =
            pattern_pixel_colour(pattern, (uint16_t)(left_x + 1u));

        g_timing_row[byte_x] = pack_pixels(left, right);
    }
}

static void upload_timing_bitmap(void)
{
    for (uint8_t pattern = 0u; pattern < EGG_CF_BAR_COUNT; ++pattern) {
        generate_pattern_row(pattern);

        for (uint8_t row = 0u; row < EGG_CF_BAR_HEIGHT; ++row) {
            uint32_t bitmap_row =
                (uint32_t)pattern * EGG_CF_BAR_HEIGHT + row;

            twodfx_upload_raw(
                EGG_CF_BITMAP_ADDRESS +
                    bitmap_row * EGG_CF_BITMAP_ROW_BYTES,
                g_timing_row,
                sizeof(g_timing_row));
        }
    }
}

/* ------------------------------------------------------------------------- */
/* Public-API scene playlist                                                 */
/* ------------------------------------------------------------------------- */

static void build_scene(void)
{
    uint8_t slot = 0u;

    /*
     * The original diagnostic deliberately reasserted a completely clean
     * static playlist and bitmap descriptor on every update opportunity.
     */
    twodfx_reset_2dpt_all();
    twodfx_define_bitmap(EGG_CF_BITMAP_ID,
                         EGG_CF_BITMAP_ADDRESS,
                         EGG_CF_BITMAP_WIDTH,
                         EGG_CF_BITMAP_HEIGHT);

    twodfx_2d_set_color(TWODFX_LAYER_BACKGROUND, slot++,
                        EGG_CF_ZERO_COLOUR,
                        EGG_CF_ZERO_COLOUR);
    twodfx_2d_fill_rect(TWODFX_LAYER_BACKGROUND, slot++,
                        EGG_CF_VISIBLE_X,
                        EGG_CF_VISIBLE_Y,
                        EGG_CF_VISIBLE_WIDTH,
                        EGG_CF_VISIBLE_HEIGHT);

    twodfx_2d_blit(TWODFX_LAYER_BACKGROUND, slot++,
                   EGG_CF_BITMAP_X,
                   EGG_CF_BITMAP_Y,
                   EGG_CF_BITMAP_ID,
                   TWODFX_BLIT_EXACT);

    twodfx_2d_ret(TWODFX_LAYER_BACKGROUND, slot);
}

void egg_CF_tvc_bitpattern_start(void)
{
    twodfx_set_fps(TWODFX_FPS_50);
    twodfx_set_transparent_color(EGG_CF_TRANSPARENT_COLOUR);

    twodfx_reset_sprites();
    upload_timing_bitmap();
    build_scene();
}

void egg_CF_tvc_bitpattern_tick(void)
{
    /* Keep the diagnostic independent of stale scene changes from the host. */
    build_scene();
}
