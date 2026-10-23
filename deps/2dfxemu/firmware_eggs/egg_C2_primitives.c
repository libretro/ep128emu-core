#include "egg_C2_primitives.h"

#include <stdbool.h>
#include <stdint.h>

static bool g_is_tvc;
static uint16_t g_tick;

static const uint16_t g_grid_y[EGG_C2_GRID_LINE_COUNT] =
    EGG_C2_GRID_Y_VALUES;

static uint8_t profile_colour(uint8_t enterprise_colour, uint8_t tvc_colour)
{
    return g_is_tvc ? tvc_colour : enterprise_colour;
}

/* ------------------------------------------------------------------------- */
/* Cross-profile design-coordinate conversion                                */
/* ------------------------------------------------------------------------- */

static uint16_t scene_x(void)
{
    return g_is_tvc ? EGG_C2_TVC_X : EGG_C2_ENTERPRISE_X;
}

static uint16_t scene_y(void)
{
    return g_is_tvc ? EGG_C2_TVC_Y : EGG_C2_ENTERPRISE_Y;
}

static uint16_t scene_width(void)
{
    return g_is_tvc ? EGG_C2_TVC_WIDTH : EGG_C2_ENTERPRISE_WIDTH;
}

static uint16_t scene_height(void)
{
    return g_is_tvc ? EGG_C2_TVC_HEIGHT : EGG_C2_ENTERPRISE_HEIGHT;
}

static uint16_t design_x(uint16_t value)
{
    return (uint16_t)(scene_x() +
        ((uint32_t)value * (scene_width() - 1u) +
         ((EGG_C2_DESIGN_WIDTH - 1u) / 2u)) /
        (EGG_C2_DESIGN_WIDTH - 1u));
}

static uint16_t design_y(uint16_t value)
{
    return (uint16_t)(scene_y() +
        ((uint32_t)value * (scene_height() - 1u) +
         ((EGG_C2_DESIGN_HEIGHT - 1u) / 2u)) /
        (EGG_C2_DESIGN_HEIGHT - 1u));
}

static uint16_t design_width(uint16_t value)
{
    uint16_t scaled = (uint16_t)(
        ((uint32_t)value * scene_width() +
         (EGG_C2_DESIGN_WIDTH / 2u)) /
        EGG_C2_DESIGN_WIDTH);

    return scaled ? scaled : 1u;
}

static uint16_t design_height(uint16_t value)
{
    uint16_t scaled = (uint16_t)(
        ((uint32_t)value * scene_height() +
         (EGG_C2_DESIGN_HEIGHT / 2u)) /
        EGG_C2_DESIGN_HEIGHT);

    return scaled ? scaled : 1u;
}

static uint8_t rounded_panel_colour(void)
{
    return g_is_tvc
        ? EGG_C2_TVC_ROUNDED_PANEL_COLOUR
        : EGG_C2_EP_ROUNDED_PANEL_COLOUR;
}

static uint8_t bucket_fill_colour(void)
{
    return g_is_tvc
        ? EGG_C2_TVC_BUCKET_FILL_COLOUR
        : EGG_C2_EP_BUCKET_FILL_COLOUR;
}

static uint8_t small_frame_colour(void)
{
    return g_is_tvc
        ? EGG_C2_TVC_SMALL_FRAME_COLOUR
        : EGG_C2_EP_SMALL_FRAME_COLOUR;
}

void egg_C2_primitives_start(void)
{
    g_is_tvc = twodfx_get_machine() == TWODFX_MACHINE_TVC;
    g_tick = 0u;

    /* C2 is deliberately 2DPT-only. */
    twodfx_reset_sprites();
    twodfx_reset_2dpt_all();
}

void egg_C2_primitives_tick(void)
{
    ++g_tick;

    /*
     * Rebuild the complete background playlist every frame. This keeps all
     * animation coordinates obvious and makes C2 a direct API example rather
     * than a raw-table or partial-update demonstration.
     */
    twodfx_reset_2dpt_all();

    const uint16_t x0 = scene_x();
    const uint16_t y0 = scene_y();
    const uint16_t width = scene_width();
    const uint16_t height = scene_height();
    const uint16_t x1 = (uint16_t)(x0 + width - 1u);
    const uint16_t y1 = (uint16_t)(y0 + height - 1u);

    uint8_t slot = 0u;

    /* Base field and three horizontal colour regions. */
    twodfx_2d_set_color(TWODFX_LAYER_BACKGROUND, slot++,
                        profile_colour(EGG_C2_BACKGROUND_COLOUR, EGG_C2_TVC_BACKGROUND_COLOUR),
                        EGG_C2_COLOUR_STATE_BACKGROUND);
    twodfx_2d_fill_rect(TWODFX_LAYER_BACKGROUND, slot++,
                        x0, y0, width, height);

    twodfx_2d_set_color(TWODFX_LAYER_BACKGROUND, slot++,
                        profile_colour(EGG_C2_TOP_BAND_COLOUR, EGG_C2_TVC_TOP_BAND_COLOUR),
                        EGG_C2_COLOUR_STATE_BACKGROUND);
    twodfx_2d_fill_rect(TWODFX_LAYER_BACKGROUND, slot++,
                        x0, y0,
                        width,
                        design_height(EGG_C2_TOP_BAND_HEIGHT_DESIGN));

    twodfx_2d_set_color(TWODFX_LAYER_BACKGROUND, slot++,
                        profile_colour(EGG_C2_MIDDLE_BAND_COLOUR, EGG_C2_TVC_MIDDLE_BAND_COLOUR),
                        EGG_C2_COLOUR_STATE_BACKGROUND);
    twodfx_2d_fill_rect(TWODFX_LAYER_BACKGROUND, slot++,
                        x0,
                        design_y(EGG_C2_MIDDLE_BAND_Y_DESIGN),
                        width,
                        design_height(EGG_C2_MIDDLE_BAND_HEIGHT_DESIGN));

    twodfx_2d_set_color(TWODFX_LAYER_BACKGROUND, slot++,
                        profile_colour(EGG_C2_BACKGROUND_COLOUR, EGG_C2_TVC_BACKGROUND_COLOUR),
                        EGG_C2_COLOUR_STATE_BACKGROUND);
    twodfx_2d_fill_rect(TWODFX_LAYER_BACKGROUND, slot++,
                        x0,
                        design_y(EGG_C2_LOWER_BACKGROUND_Y_DESIGN),
                        width,
                        design_height(EGG_C2_LOWER_BACKGROUND_H_DESIGN));

    /* Upper filled ellipse, followed by its larger stroked outline. */
    uint16_t ellipse_x = design_x(EGG_C2_FILLED_ELLIPSE_X_DESIGN);
    uint16_t ellipse_y = design_y(EGG_C2_FILLED_ELLIPSE_Y_DESIGN);
    uint16_t ellipse_w = design_width(EGG_C2_FILLED_ELLIPSE_W_DESIGN);
    uint16_t ellipse_h = design_height(EGG_C2_FILLED_ELLIPSE_H_DESIGN);

    twodfx_2d_set_color(TWODFX_LAYER_BACKGROUND, slot++,
                        profile_colour(EGG_C2_FILLED_ELLIPSE_COLOUR, EGG_C2_TVC_FILLED_ELLIPSE_COLOUR),
                        EGG_C2_COLOUR_STATE_BACKGROUND);
    twodfx_2d_fill_ellipse(
        TWODFX_LAYER_BACKGROUND, slot++,
        (uint16_t)(ellipse_x + ellipse_w / 2u),
        (uint16_t)(ellipse_y + ellipse_h / 2u),
        (uint16_t)(ellipse_w / 2u),
        (uint16_t)(ellipse_h / 2u));

    ellipse_x = design_x(EGG_C2_OUTER_ELLIPSE_X_DESIGN);
    ellipse_y = design_y(EGG_C2_OUTER_ELLIPSE_Y_DESIGN);
    ellipse_w = design_width(EGG_C2_OUTER_ELLIPSE_W_DESIGN);
    ellipse_h = design_height(EGG_C2_OUTER_ELLIPSE_H_DESIGN);

    twodfx_2d_set_color(TWODFX_LAYER_BACKGROUND, slot++,
                        profile_colour(EGG_C2_ACCENT_COLOUR, EGG_C2_TVC_ACCENT_COLOUR),
                        EGG_C2_COLOUR_STATE_BACKGROUND);
    twodfx_2d_ellipse(
        TWODFX_LAYER_BACKGROUND, slot++,
        (uint16_t)(ellipse_x + ellipse_w / 2u),
        (uint16_t)(ellipse_y + ellipse_h / 2u),
        (uint16_t)(ellipse_w / 2u),
        (uint16_t)(ellipse_h / 2u),
        EGG_C2_OUTER_ELLIPSE_STROKE);

    /* Horizon and outer frame. */
    twodfx_2d_set_color(TWODFX_LAYER_BACKGROUND, slot++,
                        profile_colour(EGG_C2_FRAME_COLOUR, EGG_C2_TVC_FRAME_COLOUR),
                        EGG_C2_COLOUR_STATE_BACKGROUND);
    twodfx_2d_line(TWODFX_LAYER_BACKGROUND, slot++,
                   x0, design_y(EGG_C2_HORIZON_Y_DESIGN),
                   x1, design_y(EGG_C2_HORIZON_Y_DESIGN),
                   EGG_C2_HORIZON_STROKE);

    twodfx_2d_set_color(TWODFX_LAYER_BACKGROUND, slot++,
                        profile_colour(EGG_C2_FRAME_COLOUR, EGG_C2_TVC_FRAME_COLOUR),
                        EGG_C2_COLOUR_STATE_BACKGROUND);
    twodfx_2d_rect(
        TWODFX_LAYER_BACKGROUND, slot++,
        design_x(EGG_C2_OUTER_FRAME_INSET_X_DESIGN),
        design_y(EGG_C2_OUTER_FRAME_INSET_Y_DESIGN),
        design_width(EGG_C2_DESIGN_WIDTH -
                     2u * EGG_C2_OUTER_FRAME_INSET_X_DESIGN),
        design_height(EGG_C2_DESIGN_HEIGHT -
                      2u * EGG_C2_OUTER_FRAME_INSET_Y_DESIGN),
        EGG_C2_OUTER_FRAME_STROKE);

    /* Alternating horizontal perspective-grid lines. */
    for (uint8_t index = 0u; index < EGG_C2_GRID_LINE_COUNT; ++index) {
        uint16_t grid_y = design_y(g_grid_y[index]);

        twodfx_2d_set_color(TWODFX_LAYER_BACKGROUND, slot++,
                            (index & 1u)
                                ? profile_colour(EGG_C2_GRID_COLOUR_B, EGG_C2_TVC_GRID_COLOUR_B)
                                : profile_colour(EGG_C2_GRID_COLOUR_A, EGG_C2_TVC_GRID_COLOUR_A),
                            EGG_C2_COLOUR_STATE_BACKGROUND);
        twodfx_2d_line(TWODFX_LAYER_BACKGROUND, slot++,
                       x0, grid_y, x1, grid_y,
                       EGG_C2_GRID_LINE_STROKE);
    }

    /* Perspective rays converge near the horizon. */
    const uint16_t vanish_x = design_x(EGG_C2_DESIGN_WIDTH / 2u);
    const uint16_t vanish_y = design_y(EGG_C2_VANISH_Y_DESIGN);

    for (uint8_t index = 0u;
         index <= EGG_C2_PERSPECTIVE_SEGMENTS;
         ++index) {
        uint16_t bottom_x_design = (uint16_t)(
            (uint32_t)index * (EGG_C2_DESIGN_WIDTH - 1u) /
            EGG_C2_PERSPECTIVE_SEGMENTS);

        twodfx_2d_set_color(TWODFX_LAYER_BACKGROUND, slot++,
                            profile_colour(EGG_C2_GRID_COLOUR_A, EGG_C2_TVC_GRID_COLOUR_A),
                            EGG_C2_COLOUR_STATE_BACKGROUND);
        twodfx_2d_line(TWODFX_LAYER_BACKGROUND, slot++,
                       design_x(bottom_x_design), y1,
                       vanish_x, vanish_y,
                       EGG_C2_PERSPECTIVE_LINE_STROKE);
    }

    /* Bright horizontal sweep. */
    uint16_t sweep_y_design = (uint16_t)(
        (g_tick * EGG_C2_SWEEP_SPEED_DESIGN_PIXELS) %
        EGG_C2_DESIGN_HEIGHT);
    uint16_t sweep_y = design_y(sweep_y_design);

    twodfx_2d_set_color(TWODFX_LAYER_BACKGROUND, slot++,
                        profile_colour(EGG_C2_ACCENT_COLOUR, EGG_C2_TVC_ACCENT_COLOUR),
                        EGG_C2_COLOUR_STATE_BACKGROUND);
    twodfx_2d_line(TWODFX_LAYER_BACKGROUND, slot++,
                   x0, sweep_y, x1, sweep_y,
                   EGG_C2_SWEEP_STROKE);

    /* Two nested pulsing ellipses. */
    uint8_t phase = (uint8_t)(g_tick & EGG_C2_PULSE_PHASE_MASK);
    uint8_t triangle = phase < EGG_C2_PULSE_HALF_PERIOD
        ? phase
        : (uint8_t)(EGG_C2_PULSE_PHASE_MASK - phase);

    uint16_t pulse_w_design = (uint16_t)(
        EGG_C2_PULSE_BASE_WIDTH +
        triangle * EGG_C2_PULSE_WIDTH_STEP);
    uint16_t pulse_h_design = (uint16_t)(
        EGG_C2_PULSE_BASE_HEIGHT +
        triangle * EGG_C2_PULSE_HEIGHT_STEP);
    uint16_t pulse_x_design = (uint16_t)(
        (EGG_C2_DESIGN_WIDTH - pulse_w_design) / 2u);
    uint16_t pulse_y_design = (uint16_t)(
        EGG_C2_PULSE_BASE_Y -
        triangle / EGG_C2_PULSE_Y_DIVISOR);

    ellipse_x = design_x(pulse_x_design);
    ellipse_y = design_y(pulse_y_design);
    ellipse_w = design_width(pulse_w_design);
    ellipse_h = design_height(pulse_h_design);

    twodfx_2d_set_color(TWODFX_LAYER_BACKGROUND, slot++,
                        profile_colour(EGG_C2_PULSE_OUTER_COLOUR, EGG_C2_TVC_PULSE_OUTER_COLOUR),
                        EGG_C2_COLOUR_STATE_BACKGROUND);
    twodfx_2d_ellipse(
        TWODFX_LAYER_BACKGROUND, slot++,
        (uint16_t)(ellipse_x + ellipse_w / 2u),
        (uint16_t)(ellipse_y + ellipse_h / 2u),
        (uint16_t)(ellipse_w / 2u),
        (uint16_t)(ellipse_h / 2u),
        EGG_C2_PULSE_OUTER_STROKE);

    uint16_t inner_w_design = pulse_w_design >
        EGG_C2_PULSE_INNER_WIDTH_TRIM
            ? (uint16_t)(pulse_w_design -
                         EGG_C2_PULSE_INNER_WIDTH_TRIM)
            : EGG_C2_PULSE_INNER_MIN_WIDTH;
    uint16_t inner_h_design = pulse_h_design >
        EGG_C2_PULSE_INNER_HEIGHT_TRIM
            ? (uint16_t)(pulse_h_design -
                         EGG_C2_PULSE_INNER_HEIGHT_TRIM)
            : EGG_C2_PULSE_INNER_MIN_HEIGHT;

    ellipse_x = design_x((uint16_t)(pulse_x_design +
                                    EGG_C2_PULSE_INNER_INSET_X));
    ellipse_y = design_y((uint16_t)(pulse_y_design +
                                    EGG_C2_PULSE_INNER_INSET_Y));
    ellipse_w = design_width(inner_w_design);
    ellipse_h = design_height(inner_h_design);

    twodfx_2d_set_color(TWODFX_LAYER_BACKGROUND, slot++,
                        profile_colour(EGG_C2_FRAME_COLOUR, EGG_C2_TVC_FRAME_COLOUR),
                        EGG_C2_COLOUR_STATE_BACKGROUND);
    twodfx_2d_ellipse(
        TWODFX_LAYER_BACKGROUND, slot++,
        (uint16_t)(ellipse_x + ellipse_w / 2u),
        (uint16_t)(ellipse_y + ellipse_h / 2u),
        (uint16_t)(ellipse_w / 2u),
        (uint16_t)(ellipse_h / 2u),
        EGG_C2_PULSE_INNER_STROKE);

    /* Moving rounded block and its filled inner ellipse. */
    uint16_t block_x_design = (uint16_t)(
        EGG_C2_BLOCK_START_X_DESIGN +
        ((g_tick * EGG_C2_BLOCK_SPEED_DESIGN) %
         (EGG_C2_DESIGN_WIDTH - EGG_C2_BLOCK_RIGHT_MARGIN_DESIGN)));
    uint16_t block_x = design_x(block_x_design);

    twodfx_2d_set_color(TWODFX_LAYER_BACKGROUND, slot++,
                        profile_colour(EGG_C2_ACCENT_COLOUR, EGG_C2_TVC_ACCENT_COLOUR),
                        profile_colour(EGG_C2_BLOCK_BACKGROUND_COLOUR, EGG_C2_TVC_BLOCK_BACKGROUND_COLOUR));
    twodfx_2d_round_rect(
        TWODFX_LAYER_BACKGROUND, slot++,
        block_x,
        design_y(EGG_C2_BLOCK_Y_DESIGN),
        design_width(EGG_C2_BLOCK_WIDTH_DESIGN),
        design_height(EGG_C2_BLOCK_HEIGHT_DESIGN),
        EGG_C2_BLOCK_STROKE,
        (uint8_t)design_width(EGG_C2_BLOCK_RADIUS_X_DESIGN),
        (uint8_t)design_height(EGG_C2_BLOCK_RADIUS_Y_DESIGN));

    ellipse_x = design_x((uint16_t)(
        block_x_design + EGG_C2_BLOCK_ELLIPSE_X_OFFSET_DESIGN));
    ellipse_y = design_y(EGG_C2_BLOCK_ELLIPSE_Y_DESIGN);
    ellipse_w = design_width(EGG_C2_BLOCK_ELLIPSE_W_DESIGN);
    ellipse_h = design_height(EGG_C2_BLOCK_ELLIPSE_H_DESIGN);

    twodfx_2d_set_color(TWODFX_LAYER_BACKGROUND, slot++,
                        profile_colour(EGG_C2_FILLED_ELLIPSE_COLOUR, EGG_C2_TVC_FILLED_ELLIPSE_COLOUR),
                        EGG_C2_COLOUR_STATE_BACKGROUND);
    twodfx_2d_fill_ellipse(
        TWODFX_LAYER_BACKGROUND, slot++,
        (uint16_t)(ellipse_x + ellipse_w / 2u),
        (uint16_t)(ellipse_y + ellipse_h / 2u),
        (uint16_t)(ellipse_w / 2u),
        (uint16_t)(ellipse_h / 2u));

    /* Lower-right ROUND_RECT, ERASE_LINE and BUCKET_FILL examples. */
    twodfx_2d_set_color(TWODFX_LAYER_BACKGROUND, slot++,
                        rounded_panel_colour(),
                        EGG_C2_COLOUR_STATE_BACKGROUND);
    twodfx_2d_fill_round_rect(
        TWODFX_LAYER_BACKGROUND, slot++,
        design_x(EGG_C2_FILLED_ROUND_X_DESIGN),
        design_y(EGG_C2_FILLED_ROUND_Y_DESIGN),
        design_width(EGG_C2_FILLED_ROUND_W_DESIGN),
        design_height(EGG_C2_FILLED_ROUND_H_DESIGN),
        (uint8_t)design_width(EGG_C2_FILLED_ROUND_RX_DESIGN),
        (uint8_t)design_height(EGG_C2_FILLED_ROUND_RY_DESIGN));

    twodfx_2d_set_color(TWODFX_LAYER_BACKGROUND, slot++,
                        EGG_C2_COLOUR_STATE_BACKGROUND,
                        EGG_C2_ERASE_LINE_BACKGROUND);
    twodfx_2d_erase_line(
        TWODFX_LAYER_BACKGROUND, slot++,
        design_x(EGG_C2_ERASE_LINE_X0_DESIGN),
        design_y(EGG_C2_ERASE_LINE_Y0_DESIGN),
        design_x(EGG_C2_ERASE_LINE_X1_DESIGN),
        design_y(EGG_C2_ERASE_LINE_Y1_DESIGN),
        EGG_C2_ERASE_LINE_STROKE);

    twodfx_2d_set_color(TWODFX_LAYER_BACKGROUND, slot++,
                        small_frame_colour(),
                        EGG_C2_COLOUR_STATE_BACKGROUND);
    twodfx_2d_rect(
        TWODFX_LAYER_BACKGROUND, slot++,
        design_x(EGG_C2_BUCKET_FRAME_X_DESIGN),
        design_y(EGG_C2_BUCKET_FRAME_Y_DESIGN),
        design_width(EGG_C2_BUCKET_FRAME_W_DESIGN),
        design_height(EGG_C2_BUCKET_FRAME_H_DESIGN),
        EGG_C2_BUCKET_FRAME_STROKE);

    twodfx_2d_set_color(TWODFX_LAYER_BACKGROUND, slot++,
                        bucket_fill_colour(),
                        EGG_C2_COLOUR_STATE_BACKGROUND);
    twodfx_2d_bucket_fill(
        TWODFX_LAYER_BACKGROUND, slot++,
        design_x(EGG_C2_BUCKET_SEED_X_DESIGN),
        design_y(EGG_C2_BUCKET_SEED_Y_DESIGN));

    twodfx_2d_ret(TWODFX_LAYER_BACKGROUND, slot);
}
