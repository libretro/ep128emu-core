#include "egg_D1_ruler.h"

#include <stdbool.h>
#include <stdint.h>

static egg_D1_mode_t g_mode;
static uint16_t g_coordinate;
static uint8_t g_frame_hold;
static uint16_t g_pause_ticks;
static bool g_apply_default_transparency_at_next_update;
static bool g_is_tvc;

static const uint8_t g_digit_segments[10] =
    EGG_D1_DIGIT_SEGMENT_VALUES;

/* ------------------------------------------------------------------------- */
/* Active boot-profile geometry                                              */
/* ------------------------------------------------------------------------- */

static uint16_t active_framebuffer_width(void)
{
    return g_is_tvc
        ? EGG_D1_TVC_FRAMEBUFFER_WIDTH
        : EGG_D1_ENTERPRISE_FRAMEBUFFER_WIDTH;
}

static uint16_t active_max_y(void)
{
    return g_is_tvc
        ? EGG_D1_TVC_MAX_Y
        : EGG_D1_ENTERPRISE_MAX_Y;
}

static uint16_t active_first_visible_y(void)
{
    return g_is_tvc
        ? EGG_D1_TVC_FIRST_VISIBLE_Y
        : EGG_D1_ENTERPRISE_FIRST_VISIBLE_Y;
}

static uint16_t active_visible_lines(void)
{
    return g_is_tvc
        ? EGG_D1_TVC_VISIBLE_LINES
        : EGG_D1_ENTERPRISE_VISIBLE_LINES;
}

static uint8_t active_box_border_colour(bool x_sweep)
{
    if (g_is_tvc) {
        return x_sweep
            ? EGG_D1_TVC_BOX_BORDER_X_COLOUR
            : EGG_D1_TVC_BOX_BORDER_Y_COLOUR;
    }

    return x_sweep
        ? EGG_D1_EP_BOX_BORDER_X_COLOUR
        : EGG_D1_EP_BOX_BORDER_Y_COLOUR;
}

static uint8_t active_digit_colour(void)
{
    return g_is_tvc
        ? EGG_D1_TVC_DIGIT_COLOUR
        : EGG_D1_EP_DIGIT_COLOUR;
}

static uint8_t active_neighbour_line_colour(void)
{
    return g_is_tvc
        ? EGG_D1_TVC_NEIGHBOUR_LINE_COLOUR
        : EGG_D1_EP_NEIGHBOUR_LINE_COLOUR;
}

static uint8_t active_core_line_colour(void)
{
    return g_is_tvc
        ? EGG_D1_TVC_CORE_LINE_COLOUR
        : EGG_D1_EP_CORE_LINE_COLOUR;
}

static uint16_t probe_box_x(void)
{
    uint16_t framebuffer_width = active_framebuffer_width();

    if (framebuffer_width <= EGG_D1_BOX_WIDTH) {
        return 0u;
    }

    return (uint16_t)((framebuffer_width - EGG_D1_BOX_WIDTH) / 2u);
}

static uint16_t probe_box_y(void)
{
    uint16_t first_visible_y = active_first_visible_y();
    uint16_t visible_lines = active_visible_lines();

    if (visible_lines <= EGG_D1_BOX_HEIGHT) {
        return first_visible_y;
    }

    return (uint16_t)(first_visible_y +
                      ((visible_lines - EGG_D1_BOX_HEIGHT) / 2u));
}

/* ------------------------------------------------------------------------- */
/* Seven-segment coordinate display                                          */
/* ------------------------------------------------------------------------- */

static void append_seven_segment_digit(uint8_t *slot,
                                       uint16_t x,
                                       uint16_t y,
                                       uint8_t digit)
{
    uint8_t segments = g_digit_segments[digit];
    uint16_t horizontal_width =
        (uint16_t)(EGG_D1_DIGIT_WIDTH -
                   2u * EGG_D1_SEGMENT_THICKNESS);
    uint16_t vertical_height =
        (uint16_t)(EGG_D1_DIGIT_HEIGHT / 2u -
                   EGG_D1_SEGMENT_THICKNESS);

    if (segments & EGG_D1_SEGMENT_A) {
        twodfx_2d_set_color(TWODFX_LAYER_BACKGROUND, (*slot)++,
                            active_digit_colour(),
                            EGG_D1_COLOUR_STATE_BACKGROUND);
        twodfx_2d_fill_rect(
            TWODFX_LAYER_BACKGROUND, (*slot)++,
            (uint16_t)(x + EGG_D1_SEGMENT_THICKNESS), y,
            horizontal_width, EGG_D1_SEGMENT_THICKNESS);
    }

    if (segments & EGG_D1_SEGMENT_B) {
        twodfx_2d_set_color(TWODFX_LAYER_BACKGROUND, (*slot)++,
                            active_digit_colour(),
                            EGG_D1_COLOUR_STATE_BACKGROUND);
        twodfx_2d_fill_rect(
            TWODFX_LAYER_BACKGROUND, (*slot)++,
            (uint16_t)(x + EGG_D1_DIGIT_WIDTH -
                       EGG_D1_SEGMENT_THICKNESS),
            (uint16_t)(y + EGG_D1_SEGMENT_THICKNESS),
            EGG_D1_SEGMENT_THICKNESS, vertical_height);
    }

    if (segments & EGG_D1_SEGMENT_C) {
        twodfx_2d_set_color(TWODFX_LAYER_BACKGROUND, (*slot)++,
                            active_digit_colour(),
                            EGG_D1_COLOUR_STATE_BACKGROUND);
        twodfx_2d_fill_rect(
            TWODFX_LAYER_BACKGROUND, (*slot)++,
            (uint16_t)(x + EGG_D1_DIGIT_WIDTH -
                       EGG_D1_SEGMENT_THICKNESS),
            (uint16_t)(y + EGG_D1_DIGIT_HEIGHT / 2u),
            EGG_D1_SEGMENT_THICKNESS, vertical_height);
    }

    if (segments & EGG_D1_SEGMENT_D) {
        twodfx_2d_set_color(TWODFX_LAYER_BACKGROUND, (*slot)++,
                            active_digit_colour(),
                            EGG_D1_COLOUR_STATE_BACKGROUND);
        twodfx_2d_fill_rect(
            TWODFX_LAYER_BACKGROUND, (*slot)++,
            (uint16_t)(x + EGG_D1_SEGMENT_THICKNESS),
            (uint16_t)(y + EGG_D1_DIGIT_HEIGHT -
                       EGG_D1_SEGMENT_THICKNESS),
            horizontal_width, EGG_D1_SEGMENT_THICKNESS);
    }

    if (segments & EGG_D1_SEGMENT_E) {
        twodfx_2d_set_color(TWODFX_LAYER_BACKGROUND, (*slot)++,
                            active_digit_colour(),
                            EGG_D1_COLOUR_STATE_BACKGROUND);
        twodfx_2d_fill_rect(
            TWODFX_LAYER_BACKGROUND, (*slot)++,
            x, (uint16_t)(y + EGG_D1_DIGIT_HEIGHT / 2u),
            EGG_D1_SEGMENT_THICKNESS, vertical_height);
    }

    if (segments & EGG_D1_SEGMENT_F) {
        twodfx_2d_set_color(TWODFX_LAYER_BACKGROUND, (*slot)++,
                            active_digit_colour(),
                            EGG_D1_COLOUR_STATE_BACKGROUND);
        twodfx_2d_fill_rect(
            TWODFX_LAYER_BACKGROUND, (*slot)++,
            x, (uint16_t)(y + EGG_D1_SEGMENT_THICKNESS),
            EGG_D1_SEGMENT_THICKNESS, vertical_height);
    }

    if (segments & EGG_D1_SEGMENT_G) {
        twodfx_2d_set_color(TWODFX_LAYER_BACKGROUND, (*slot)++,
                            active_digit_colour(),
                            EGG_D1_COLOUR_STATE_BACKGROUND);
        twodfx_2d_fill_rect(
            TWODFX_LAYER_BACKGROUND, (*slot)++,
            (uint16_t)(x + EGG_D1_SEGMENT_THICKNESS),
            (uint16_t)(y + EGG_D1_DIGIT_HEIGHT / 2u -
                       EGG_D1_SEGMENT_THICKNESS / 2u),
            horizontal_width, EGG_D1_SEGMENT_THICKNESS);
    }
}

static bool mode_is_x_sweep(egg_D1_mode_t mode)
{
    return mode == EGG_D1_MODE_X_FORWARD ||
           mode == EGG_D1_MODE_X_REVERSE ||
           mode == EGG_D1_MODE_PAUSE_AFTER_X_FORWARD ||
           mode == EGG_D1_MODE_PAUSE_AFTER_X_REVERSE;
}

static bool mode_is_pause(egg_D1_mode_t mode)
{
    return mode == EGG_D1_MODE_PAUSE_AFTER_X_FORWARD ||
           mode == EGG_D1_MODE_PAUSE_AFTER_X_REVERSE ||
           mode == EGG_D1_MODE_PAUSE_AFTER_Y_FORWARD ||
           mode == EGG_D1_MODE_PAUSE_AFTER_Y_REVERSE;
}

static void append_probe_box(uint8_t *slot,
                             bool x_sweep,
                             uint16_t coordinate)
{
    uint16_t box_x = probe_box_x();
    uint16_t box_y = probe_box_y();
    uint16_t inner_x = (uint16_t)(box_x + EGG_D1_BOX_BORDER);
    uint16_t inner_y = (uint16_t)(box_y + EGG_D1_BOX_BORDER);
    uint16_t inner_width =
        (uint16_t)(EGG_D1_BOX_WIDTH - 2u * EGG_D1_BOX_BORDER);
    uint16_t inner_height =
        (uint16_t)(EGG_D1_BOX_HEIGHT - 2u * EGG_D1_BOX_BORDER);
    uint8_t border_colour = active_box_border_colour(x_sweep);

    twodfx_2d_set_color(TWODFX_LAYER_BACKGROUND, (*slot)++,
                        border_colour,
                        EGG_D1_COLOUR_STATE_BACKGROUND);
    twodfx_2d_fill_rect(TWODFX_LAYER_BACKGROUND, (*slot)++,
                        box_x, box_y,
                        EGG_D1_BOX_WIDTH, EGG_D1_BOX_HEIGHT);

    twodfx_2d_set_color(TWODFX_LAYER_BACKGROUND, (*slot)++,
                        EGG_D1_BOX_FILL_COLOUR,
                        EGG_D1_COLOUR_STATE_BACKGROUND);
    twodfx_2d_fill_rect(TWODFX_LAYER_BACKGROUND, (*slot)++,
                        inner_x, inner_y, inner_width, inner_height);

    twodfx_2d_set_color(TWODFX_LAYER_BACKGROUND, (*slot)++,
                        border_colour,
                        EGG_D1_COLOUR_STATE_BACKGROUND);
    twodfx_2d_fill_rect(
        TWODFX_LAYER_BACKGROUND, (*slot)++,
        inner_x, inner_y,
        EGG_D1_BOX_AXIS_MARKER_W, inner_height);

    uint16_t digit_base_x =
        (uint16_t)(box_x + EGG_D1_BOX_PADDING_X +
                   EGG_D1_BOX_AXIS_MARKER_W + EGG_D1_BOX_DIGIT_GAP);
    uint16_t digit_y = (uint16_t)(box_y + EGG_D1_BOX_PADDING_Y);
    uint8_t digits[EGG_D1_DIGIT_COUNT];

    digits[0] = (uint8_t)((coordinate / 1000u) % 10u);
    digits[1] = (uint8_t)((coordinate / 100u) % 10u);
    digits[2] = (uint8_t)((coordinate / 10u) % 10u);
    digits[3] = (uint8_t)(coordinate % 10u);

    for (uint8_t i = 0u; i < EGG_D1_DIGIT_COUNT; ++i) {
        append_seven_segment_digit(
            slot,
            (uint16_t)(digit_base_x +
                       (uint16_t)i * EGG_D1_DIGIT_ADVANCE),
            digit_y,
            digits[i]);
    }
}

/* ------------------------------------------------------------------------- */
/* Public-API frame construction                                             */
/* ------------------------------------------------------------------------- */

static void build_frame(void)
{
    uint8_t slot = 0u;
    uint16_t framebuffer_width = active_framebuffer_width();
    uint16_t total_height = (uint16_t)(active_max_y() + 1u);
    bool x_sweep = mode_is_x_sweep(g_mode);

    twodfx_2d_set_color(TWODFX_LAYER_BACKGROUND, slot++,
                        EGG_D1_BACKGROUND_COLOUR,
                        EGG_D1_COLOUR_STATE_BACKGROUND);
    twodfx_2d_fill_rect(TWODFX_LAYER_BACKGROUND, slot++,
                        0u, 0u, framebuffer_width, total_height);

    if (x_sweep) {
        if (g_coordinate > 0u) {
            twodfx_2d_set_color(TWODFX_LAYER_BACKGROUND, slot++,
                                active_neighbour_line_colour(),
                                EGG_D1_COLOUR_STATE_BACKGROUND);
            twodfx_2d_fill_rect(
                TWODFX_LAYER_BACKGROUND, slot++,
                (uint16_t)(g_coordinate - 1u), 0u,
                1u, total_height);
        }

        twodfx_2d_set_color(TWODFX_LAYER_BACKGROUND, slot++,
                            active_core_line_colour(),
                            EGG_D1_COLOUR_STATE_BACKGROUND);
        twodfx_2d_fill_rect(TWODFX_LAYER_BACKGROUND, slot++,
                            g_coordinate, 0u, 1u, total_height);

        if ((uint32_t)g_coordinate + 1u < framebuffer_width) {
            twodfx_2d_set_color(TWODFX_LAYER_BACKGROUND, slot++,
                                active_neighbour_line_colour(),
                                EGG_D1_COLOUR_STATE_BACKGROUND);
            twodfx_2d_fill_rect(
                TWODFX_LAYER_BACKGROUND, slot++,
                (uint16_t)(g_coordinate + 1u), 0u,
                1u, total_height);
        }
    } else {
        if (g_coordinate > 0u) {
            twodfx_2d_set_color(TWODFX_LAYER_BACKGROUND, slot++,
                                active_neighbour_line_colour(),
                                EGG_D1_COLOUR_STATE_BACKGROUND);
            twodfx_2d_fill_rect(
                TWODFX_LAYER_BACKGROUND, slot++,
                0u, (uint16_t)(g_coordinate - 1u),
                framebuffer_width, 1u);
        }

        twodfx_2d_set_color(TWODFX_LAYER_BACKGROUND, slot++,
                            active_core_line_colour(),
                            EGG_D1_COLOUR_STATE_BACKGROUND);
        twodfx_2d_fill_rect(TWODFX_LAYER_BACKGROUND, slot++,
                            0u, g_coordinate,
                            framebuffer_width, 1u);

        if ((uint32_t)g_coordinate + 1u < total_height) {
            twodfx_2d_set_color(TWODFX_LAYER_BACKGROUND, slot++,
                                active_neighbour_line_colour(),
                                EGG_D1_COLOUR_STATE_BACKGROUND);
            twodfx_2d_fill_rect(
                TWODFX_LAYER_BACKGROUND, slot++,
                0u, (uint16_t)(g_coordinate + 1u),
                framebuffer_width, 1u);
        }
    }

    append_probe_box(&slot, x_sweep, g_coordinate);
    twodfx_2d_ret(TWODFX_LAYER_BACKGROUND, slot);
}

/* ------------------------------------------------------------------------- */
/* Sweep state machine                                                       */
/* ------------------------------------------------------------------------- */

static void enter_pause_after(egg_D1_mode_t completed_mode)
{
    switch (completed_mode) {
        case EGG_D1_MODE_X_FORWARD:
            g_mode = EGG_D1_MODE_PAUSE_AFTER_X_FORWARD;
            break;
        case EGG_D1_MODE_X_REVERSE:
            g_mode = EGG_D1_MODE_PAUSE_AFTER_X_REVERSE;
            break;
        case EGG_D1_MODE_Y_FORWARD:
            g_mode = EGG_D1_MODE_PAUSE_AFTER_Y_FORWARD;
            break;
        case EGG_D1_MODE_Y_REVERSE:
            g_mode = EGG_D1_MODE_PAUSE_AFTER_Y_REVERSE;
            break;
        default:
            break;
    }

    g_pause_ticks = EGG_D1_PAUSE_TICKS;
}

static void advance_state(void)
{
    uint16_t max_x = (uint16_t)(active_framebuffer_width() - 1u);
    uint16_t max_y = active_max_y();

    if (mode_is_pause(g_mode)) {
        if (g_pause_ticks > 0u) {
            --g_pause_ticks;
            return;
        }

        switch (g_mode) {
            case EGG_D1_MODE_PAUSE_AFTER_X_FORWARD:
                g_mode = EGG_D1_MODE_X_REVERSE;
                g_coordinate = max_x;
                break;
            case EGG_D1_MODE_PAUSE_AFTER_X_REVERSE:
                g_mode = EGG_D1_MODE_Y_FORWARD;
                g_coordinate = 0u;
                break;
            case EGG_D1_MODE_PAUSE_AFTER_Y_FORWARD:
                g_mode = EGG_D1_MODE_Y_REVERSE;
                g_coordinate = max_y;
                break;
            case EGG_D1_MODE_PAUSE_AFTER_Y_REVERSE:
            default:
                g_mode = EGG_D1_MODE_X_FORWARD;
                g_coordinate = 0u;
                break;
        }

        g_frame_hold = 0u;
        return;
    }

    ++g_frame_hold;
    if (g_frame_hold < EGG_D1_FRAMES_PER_PIXEL) {
        return;
    }
    g_frame_hold = 0u;

    switch (g_mode) {
        case EGG_D1_MODE_X_FORWARD:
            if (g_coordinate >= max_x) {
                enter_pause_after(EGG_D1_MODE_X_FORWARD);
            } else {
                ++g_coordinate;
            }
            break;

        case EGG_D1_MODE_X_REVERSE:
            if (g_coordinate == 0u) {
                enter_pause_after(EGG_D1_MODE_X_REVERSE);
            } else {
                --g_coordinate;
            }
            break;

        case EGG_D1_MODE_Y_FORWARD:
            if (g_coordinate >= max_y) {
                enter_pause_after(EGG_D1_MODE_Y_FORWARD);
            } else {
                ++g_coordinate;
            }
            break;

        case EGG_D1_MODE_Y_REVERSE:
            if (g_coordinate == 0u) {
                enter_pause_after(EGG_D1_MODE_Y_REVERSE);
            } else {
                --g_coordinate;
            }
            break;

        default:
            g_mode = EGG_D1_MODE_X_FORWARD;
            g_coordinate = 0u;
            g_pause_ticks = 0u;
            break;
    }
}

void egg_D1_ruler_start(void)
{
    g_is_tvc = (twodfx_get_machine() == TWODFX_MACHINE_TVC);
    twodfx_set_fps(TWODFX_FPS_50);
    twodfx_reset_sprites();
    twodfx_reset_2dpt_all();

    g_mode = EGG_D1_MODE_X_FORWARD;
    g_coordinate = 0u;
    g_frame_hold = 0u;
    g_pause_ticks = 0u;
    g_apply_default_transparency_at_next_update = true;

    build_frame();
}

void egg_D1_ruler_tick(void)
{
    if (g_apply_default_transparency_at_next_update) {
        twodfx_set_transparent_color(EGG_D1_TRANSPARENT_COLOUR);
        g_apply_default_transparency_at_next_update = false;
    }

    build_frame();
    advance_state();
}
