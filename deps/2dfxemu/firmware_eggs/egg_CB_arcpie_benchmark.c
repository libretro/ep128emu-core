#include "egg_CB_arcpie_benchmark.h"

#include <stdbool.h>
#include <stdint.h>

/* ------------------------------------------------------------------------- */
/* Active profile geometry                                                   */
/* ------------------------------------------------------------------------- */

static bool g_is_tvc;
static uint16_t g_raw_x;
static uint16_t g_raw_y;
static uint16_t g_raw_width;
static uint16_t g_raw_height;

static const uint8_t g_ep_pie_colours[EGG_CB_PIE_COLOUR_COUNT] =
    EGG_CB_EP_PIE_COLOURS;
static const uint8_t g_tvc_pie_colours[EGG_CB_PIE_COLOUR_COUNT] =
    EGG_CB_TVC_PIE_COLOURS;
static const uint8_t g_ep_arc_colours[EGG_CB_ARC_COLOUR_COUNT] =
    EGG_CB_EP_ARC_COLOURS;
static const uint8_t g_tvc_arc_colours[EGG_CB_ARC_COLOUR_COUNT] =
    EGG_CB_TVC_ARC_COLOURS;

static uint8_t background_colour(void)
{
    return g_is_tvc
        ? EGG_CB_TVC_BACKGROUND_COLOUR
        : EGG_CB_EP_BACKGROUND_COLOUR;
}

static uint8_t pie_colour(uint8_t index)
{
    return g_is_tvc ? g_tvc_pie_colours[index] : g_ep_pie_colours[index];
}

static uint8_t arc_colour(uint8_t index)
{
    return g_is_tvc ? g_tvc_arc_colours[index] : g_ep_arc_colours[index];
}

static void select_profile_geometry(void)
{
    g_is_tvc = twodfx_get_machine() == TWODFX_MACHINE_TVC;

    if (g_is_tvc) {
        g_raw_x = EGG_CB_TVC_RAW_X;
        g_raw_y = EGG_CB_TVC_RAW_Y;
        g_raw_width = EGG_CB_TVC_RAW_WIDTH;
        g_raw_height = EGG_CB_TVC_RAW_HEIGHT;
    } else {
        g_raw_x = EGG_CB_EP_RAW_X;
        g_raw_y = EGG_CB_EP_RAW_Y;
        g_raw_width = EGG_CB_EP_RAW_WIDTH;
        g_raw_height = EGG_CB_EP_RAW_HEIGHT;
    }
}

/* Convert the original bounding-box layout to the public centre/radius API. */
#define EGG_CB_CENTER_X(offset_x, width) \
    ((uint16_t)(g_raw_x + (offset_x) + (width) / 2u))
#define EGG_CB_CENTER_Y(offset_y, height) \
    ((uint16_t)(g_raw_y + (offset_y) + (height) / 2u))
#define EGG_CB_RADIUS_X(width)  ((uint16_t)((width) / 2u))
#define EGG_CB_RADIUS_Y(height) ((uint16_t)((height) / 2u))

/* ------------------------------------------------------------------------- */
/* Static benchmark playlist                                                 */
/* ------------------------------------------------------------------------- */

static void build_scene(void)
{
    uint8_t slot = 0u;

    twodfx_2d_set_color(TWODFX_LAYER_BACKGROUND, slot++,
                        background_colour(),
                        EGG_CB_TRANSPARENT_COLOUR);
    twodfx_2d_fill_rect(TWODFX_LAYER_BACKGROUND, slot++,
                        g_raw_x, g_raw_y, g_raw_width, g_raw_height);

    /* Two ordinary ellipses provide visual reference rings. */
    twodfx_2d_set_color(TWODFX_LAYER_BACKGROUND, slot++,
                        EGG_CB_REFERENCE_OUTER_COLOUR,
                        EGG_CB_TRANSPARENT_COLOUR);
    twodfx_2d_ellipse(
        TWODFX_LAYER_BACKGROUND, slot++,
        EGG_CB_CENTER_X(EGG_CB_REFERENCE_OUTER_X, EGG_CB_REFERENCE_OUTER_W),
        EGG_CB_CENTER_Y(EGG_CB_REFERENCE_OUTER_Y, EGG_CB_REFERENCE_OUTER_H),
        EGG_CB_RADIUS_X(EGG_CB_REFERENCE_OUTER_W),
        EGG_CB_RADIUS_Y(EGG_CB_REFERENCE_OUTER_H),
        EGG_CB_REFERENCE_OUTER_STROKE);

    twodfx_2d_set_color(TWODFX_LAYER_BACKGROUND, slot++,
                        EGG_CB_REFERENCE_INNER_COLOUR,
                        EGG_CB_TRANSPARENT_COLOUR);
    twodfx_2d_ellipse(
        TWODFX_LAYER_BACKGROUND, slot++,
        EGG_CB_CENTER_X(EGG_CB_REFERENCE_INNER_X, EGG_CB_REFERENCE_INNER_W),
        EGG_CB_CENTER_Y(EGG_CB_REFERENCE_INNER_Y, EGG_CB_REFERENCE_INNER_H),
        EGG_CB_RADIUS_X(EGG_CB_REFERENCE_INNER_W),
        EGG_CB_RADIUS_Y(EGG_CB_REFERENCE_INNER_H),
        EGG_CB_REFERENCE_INNER_STROKE);

    /* Large PIEs, including wrapping sweeps, stress filled-sector math. */
    twodfx_2d_set_color(TWODFX_LAYER_BACKGROUND, slot++, pie_colour(0u), 0u);
    twodfx_2d_pie(TWODFX_LAYER_BACKGROUND, slot++,
                  EGG_CB_CENTER_X(EGG_CB_PIE0_X, EGG_CB_PIE0_W),
                  EGG_CB_CENTER_Y(EGG_CB_PIE0_Y, EGG_CB_PIE0_H),
                  EGG_CB_RADIUS_X(EGG_CB_PIE0_W), EGG_CB_RADIUS_Y(EGG_CB_PIE0_H),
                  EGG_CB_PIE0_START, EGG_CB_PIE0_END);

    twodfx_2d_set_color(TWODFX_LAYER_BACKGROUND, slot++, pie_colour(1u), 0u);
    twodfx_2d_pie(TWODFX_LAYER_BACKGROUND, slot++,
                  EGG_CB_CENTER_X(EGG_CB_PIE1_X, EGG_CB_PIE1_W),
                  EGG_CB_CENTER_Y(EGG_CB_PIE1_Y, EGG_CB_PIE1_H),
                  EGG_CB_RADIUS_X(EGG_CB_PIE1_W), EGG_CB_RADIUS_Y(EGG_CB_PIE1_H),
                  EGG_CB_PIE1_START, EGG_CB_PIE1_END);

    twodfx_2d_set_color(TWODFX_LAYER_BACKGROUND, slot++, pie_colour(2u), 0u);
    twodfx_2d_pie(TWODFX_LAYER_BACKGROUND, slot++,
                  EGG_CB_CENTER_X(EGG_CB_PIE2_X, EGG_CB_PIE2_W),
                  EGG_CB_CENTER_Y(EGG_CB_PIE2_Y, EGG_CB_PIE2_H),
                  EGG_CB_RADIUS_X(EGG_CB_PIE2_W), EGG_CB_RADIUS_Y(EGG_CB_PIE2_H),
                  EGG_CB_PIE2_START, EGG_CB_PIE2_END);

    twodfx_2d_set_color(TWODFX_LAYER_BACKGROUND, slot++, pie_colour(3u), 0u);
    twodfx_2d_pie(TWODFX_LAYER_BACKGROUND, slot++,
                  EGG_CB_CENTER_X(EGG_CB_PIE3_X, EGG_CB_PIE3_W),
                  EGG_CB_CENTER_Y(EGG_CB_PIE3_Y, EGG_CB_PIE3_H),
                  EGG_CB_RADIUS_X(EGG_CB_PIE3_W), EGG_CB_RADIUS_Y(EGG_CB_PIE3_H),
                  EGG_CB_PIE3_START, EGG_CB_PIE3_END);

    twodfx_2d_set_color(TWODFX_LAYER_BACKGROUND, slot++, pie_colour(4u), 0u);
    twodfx_2d_pie(TWODFX_LAYER_BACKGROUND, slot++,
                  EGG_CB_CENTER_X(EGG_CB_PIE4_X, EGG_CB_PIE4_W),
                  EGG_CB_CENTER_Y(EGG_CB_PIE4_Y, EGG_CB_PIE4_H),
                  EGG_CB_RADIUS_X(EGG_CB_PIE4_W), EGG_CB_RADIUS_Y(EGG_CB_PIE4_H),
                  EGG_CB_PIE4_START, EGG_CB_PIE4_END);

    twodfx_2d_set_color(TWODFX_LAYER_BACKGROUND, slot++, pie_colour(5u), 0u);
    twodfx_2d_pie(TWODFX_LAYER_BACKGROUND, slot++,
                  EGG_CB_CENTER_X(EGG_CB_PIE5_X, EGG_CB_PIE5_W),
                  EGG_CB_CENTER_Y(EGG_CB_PIE5_Y, EGG_CB_PIE5_H),
                  EGG_CB_RADIUS_X(EGG_CB_PIE5_W), EGG_CB_RADIUS_Y(EGG_CB_PIE5_H),
                  EGG_CB_PIE5_START, EGG_CB_PIE5_END);

    /* Thick ARCs exercise the optimized inner-ring path. */
    twodfx_2d_set_color(TWODFX_LAYER_BACKGROUND, slot++, arc_colour(0u), 0u);
    twodfx_2d_arc(TWODFX_LAYER_BACKGROUND, slot++,
                  EGG_CB_CENTER_X(EGG_CB_ARC0_X, EGG_CB_ARC0_W),
                  EGG_CB_CENTER_Y(EGG_CB_ARC0_Y, EGG_CB_ARC0_H),
                  EGG_CB_RADIUS_X(EGG_CB_ARC0_W), EGG_CB_RADIUS_Y(EGG_CB_ARC0_H),
                  EGG_CB_ARC0_START, EGG_CB_ARC0_END, EGG_CB_ARC0_STROKE);

    twodfx_2d_set_color(TWODFX_LAYER_BACKGROUND, slot++, arc_colour(1u), 0u);
    twodfx_2d_arc(TWODFX_LAYER_BACKGROUND, slot++,
                  EGG_CB_CENTER_X(EGG_CB_ARC1_X, EGG_CB_ARC1_W),
                  EGG_CB_CENTER_Y(EGG_CB_ARC1_Y, EGG_CB_ARC1_H),
                  EGG_CB_RADIUS_X(EGG_CB_ARC1_W), EGG_CB_RADIUS_Y(EGG_CB_ARC1_H),
                  EGG_CB_ARC1_START, EGG_CB_ARC1_END, EGG_CB_ARC1_STROKE);

    twodfx_2d_set_color(TWODFX_LAYER_BACKGROUND, slot++, arc_colour(2u), 0u);
    twodfx_2d_arc(TWODFX_LAYER_BACKGROUND, slot++,
                  EGG_CB_CENTER_X(EGG_CB_ARC2_X, EGG_CB_ARC2_W),
                  EGG_CB_CENTER_Y(EGG_CB_ARC2_Y, EGG_CB_ARC2_H),
                  EGG_CB_RADIUS_X(EGG_CB_ARC2_W), EGG_CB_RADIUS_Y(EGG_CB_ARC2_H),
                  EGG_CB_ARC2_START, EGG_CB_ARC2_END, EGG_CB_ARC2_STROKE);

    twodfx_2d_set_color(TWODFX_LAYER_BACKGROUND, slot++, arc_colour(3u), 0u);
    twodfx_2d_arc(TWODFX_LAYER_BACKGROUND, slot++,
                  EGG_CB_CENTER_X(EGG_CB_ARC3_X, EGG_CB_ARC3_W),
                  EGG_CB_CENTER_Y(EGG_CB_ARC3_Y, EGG_CB_ARC3_H),
                  EGG_CB_RADIUS_X(EGG_CB_ARC3_W), EGG_CB_RADIUS_Y(EGG_CB_ARC3_H),
                  EGG_CB_ARC3_START, EGG_CB_ARC3_END, EGG_CB_ARC3_STROKE);

    twodfx_2d_set_color(TWODFX_LAYER_BACKGROUND, slot++, arc_colour(4u), 0u);
    twodfx_2d_arc(TWODFX_LAYER_BACKGROUND, slot++,
                  EGG_CB_CENTER_X(EGG_CB_ARC4_X, EGG_CB_ARC4_W),
                  EGG_CB_CENTER_Y(EGG_CB_ARC4_Y, EGG_CB_ARC4_H),
                  EGG_CB_RADIUS_X(EGG_CB_ARC4_W), EGG_CB_RADIUS_Y(EGG_CB_ARC4_H),
                  EGG_CB_ARC4_START, EGG_CB_ARC4_END, EGG_CB_ARC4_STROKE);

    twodfx_2d_set_color(TWODFX_LAYER_BACKGROUND, slot++, arc_colour(5u), 0u);
    twodfx_2d_arc(TWODFX_LAYER_BACKGROUND, slot++,
                  EGG_CB_CENTER_X(EGG_CB_ARC5_X, EGG_CB_ARC5_W),
                  EGG_CB_CENTER_Y(EGG_CB_ARC5_Y, EGG_CB_ARC5_H),
                  EGG_CB_RADIUS_X(EGG_CB_ARC5_W), EGG_CB_RADIUS_Y(EGG_CB_ARC5_H),
                  EGG_CB_ARC5_START, EGG_CB_ARC5_END, EGG_CB_ARC5_STROKE);

    twodfx_2d_set_color(TWODFX_LAYER_BACKGROUND, slot++, arc_colour(6u), 0u);
    twodfx_2d_arc(TWODFX_LAYER_BACKGROUND, slot++,
                  EGG_CB_CENTER_X(EGG_CB_ARC6_X, EGG_CB_ARC6_W),
                  EGG_CB_CENTER_Y(EGG_CB_ARC6_Y, EGG_CB_ARC6_H),
                  EGG_CB_RADIUS_X(EGG_CB_ARC6_W), EGG_CB_RADIUS_Y(EGG_CB_ARC6_H),
                  EGG_CB_ARC6_START, EGG_CB_ARC6_END, EGG_CB_ARC6_STROKE);

    twodfx_2d_set_color(TWODFX_LAYER_BACKGROUND, slot++, arc_colour(7u), 0u);
    twodfx_2d_arc(TWODFX_LAYER_BACKGROUND, slot++,
                  EGG_CB_CENTER_X(EGG_CB_ARC7_X, EGG_CB_ARC7_W),
                  EGG_CB_CENTER_Y(EGG_CB_ARC7_Y, EGG_CB_ARC7_H),
                  EGG_CB_RADIUS_X(EGG_CB_ARC7_W), EGG_CB_RADIUS_Y(EGG_CB_ARC7_H),
                  EGG_CB_ARC7_START, EGG_CB_ARC7_END, EGG_CB_ARC7_STROKE);

    twodfx_2d_ret(TWODFX_LAYER_BACKGROUND, slot);
}

void egg_CB_arcpie_benchmark_start(void)
{
    select_profile_geometry();
    twodfx_reset_sprites();
    twodfx_reset_2dpt_all();
    build_scene();
}

void egg_CB_arcpie_benchmark_tick(void)
{
    /* Static scene: no per-frame table rebuild, preserving stable timing. */
}
