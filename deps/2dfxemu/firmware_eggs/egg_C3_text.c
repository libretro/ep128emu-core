#include "egg_C3_text.h"
#include "egg_C3_text_assets.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

static bool g_is_tvc;
static egg_C3_font_load_state_t g_font_load_state;
static uint16_t g_frame_counter;

static uint8_t profile_colour(uint8_t enterprise_colour, uint8_t tvc_colour)
{
    return g_is_tvc ? tvc_colour : enterprise_colour;
}

static uint8_t wide_text_colour(void)
{
    return g_is_tvc ? EGG_C3_TVC_WIDE_COLOR : EGG_C3_EP_WIDE_COLOR;
}

/* ------------------------------------------------------------------------- */
/* Static strings and separately named offline font assets                   */
/* ------------------------------------------------------------------------- */

static void upload_static_strings(void)
{
    twodfx_upload_raw(EGG_C3_STRING_TITLE_ADDRESS,
                      egg_C3_string_title_raw,
                      sizeof(egg_C3_string_title_raw));
    twodfx_upload_raw(EGG_C3_STRING_NORMAL_ADDRESS,
                      egg_C3_string_normal_raw,
                      sizeof(egg_C3_string_normal_raw));
    twodfx_upload_raw(EGG_C3_STRING_NARROW_ADDRESS,
                      egg_C3_string_narrow_raw,
                      sizeof(egg_C3_string_narrow_raw));
    twodfx_upload_raw(EGG_C3_STRING_WIDE_ADDRESS,
                      egg_C3_string_wide_raw,
                      sizeof(egg_C3_string_wide_raw));
    twodfx_upload_raw(EGG_C3_STRING_TALL_ADDRESS,
                      egg_C3_string_tall_raw,
                      sizeof(egg_C3_string_tall_raw));
    twodfx_upload_raw(EGG_C3_STRING_BIG_ADDRESS,
                      egg_C3_string_big_raw,
                      sizeof(egg_C3_string_big_raw));
    twodfx_upload_raw(EGG_C3_STRING_TRANS_ADDRESS,
                      egg_C3_string_transparent_raw,
                      sizeof(egg_C3_string_transparent_raw));
    twodfx_upload_raw(EGG_C3_STRING_FILLED_ADDRESS,
                      egg_C3_string_filled_raw,
                      sizeof(egg_C3_string_filled_raw));
    twodfx_upload_raw(EGG_C3_STRING_COUNTER_ADDRESS,
                      egg_C3_string_counter_initial_raw,
                      sizeof(egg_C3_string_counter_initial_raw));
}

static bool queue_font_decompression(egg_C3_font_load_state_t state)
{
    const uint8_t *stream;
    uint32_t stream_size;
    uint32_t destination;

    switch (state) {
        case EGG_C3_FONT_LOAD_NARROW:
            stream = egg_C3_font_narrow_zx1;
            stream_size = sizeof(egg_C3_font_narrow_zx1);
            destination = EGG_C3_FONT_NARROW_ADDRESS;
            break;

        case EGG_C3_FONT_LOAD_NORMAL:
            stream = egg_C3_font_normal_zx1;
            stream_size = sizeof(egg_C3_font_normal_zx1);
            destination = EGG_C3_FONT_NORMAL_ADDRESS;
            break;

        case EGG_C3_FONT_LOAD_WIDE:
            stream = egg_C3_font_wide_zx1;
            stream_size = sizeof(egg_C3_font_wide_zx1);
            destination = EGG_C3_FONT_WIDE_ADDRESS;
            break;

        case EGG_C3_FONT_LOAD_TALL:
            stream = egg_C3_font_tall_zx1;
            stream_size = sizeof(egg_C3_font_tall_zx1);
            destination = EGG_C3_FONT_TALL_ADDRESS;
            break;

        case EGG_C3_FONT_LOAD_BIG:
            stream = egg_C3_font_big_zx1;
            stream_size = sizeof(egg_C3_font_big_zx1);
            destination = EGG_C3_FONT_BIG_ADDRESS;
            break;

        default:
            return false;
    }

    twodfx_upload_raw(EGG_C3_FONT_ZX1_STAGING_ADDRESS,
                      stream,
                      stream_size);

    return twodfx_unzx1_raw(EGG_C3_FONT_ZX1_STAGING_ADDRESS,
                            destination);
}

/* ------------------------------------------------------------------------- */
/* Descriptor installation and visible scene                                */
/* ------------------------------------------------------------------------- */

static void define_fonts(void)
{
    twodfx_define_font(EGG_C3_FONT_NARROW_ID,
                       EGG_C3_FONT_NARROW_ADDRESS,
                       EGG_C3_FONT_NARROW_WIDTH_UNITS,
                       EGG_C3_FONT_NARROW_HEIGHT_UNITS);
    twodfx_define_font(EGG_C3_FONT_NORMAL_ID,
                       EGG_C3_FONT_NORMAL_ADDRESS,
                       EGG_C3_FONT_NORMAL_WIDTH_UNITS,
                       EGG_C3_FONT_NORMAL_HEIGHT_UNITS);
    twodfx_define_font(EGG_C3_FONT_WIDE_ID,
                       EGG_C3_FONT_WIDE_ADDRESS,
                       EGG_C3_FONT_WIDE_WIDTH_UNITS,
                       EGG_C3_FONT_WIDE_HEIGHT_UNITS);
    twodfx_define_font(EGG_C3_FONT_TALL_ID,
                       EGG_C3_FONT_TALL_ADDRESS,
                       EGG_C3_FONT_TALL_WIDTH_UNITS,
                       EGG_C3_FONT_TALL_HEIGHT_UNITS);
    twodfx_define_font(EGG_C3_FONT_BIG_ID,
                       EGG_C3_FONT_BIG_ADDRESS,
                       EGG_C3_FONT_BIG_WIDTH_UNITS,
                       EGG_C3_FONT_BIG_HEIGHT_UNITS);
}

static void build_scene(void)
{
    const uint16_t panel_x = g_is_tvc
        ? EGG_C3_TVC_PANEL_X
        : EGG_C3_ENTERPRISE_PANEL_X;
    const uint16_t panel_y = g_is_tvc
        ? EGG_C3_TVC_PANEL_Y
        : EGG_C3_ENTERPRISE_PANEL_Y;
    const uint16_t panel_w = g_is_tvc
        ? EGG_C3_TVC_PANEL_W
        : EGG_C3_ENTERPRISE_PANEL_W;
    const uint16_t panel_h = g_is_tvc
        ? EGG_C3_TVC_PANEL_H
        : EGG_C3_ENTERPRISE_PANEL_H;
    const uint16_t body_x = g_is_tvc
        ? EGG_C3_TVC_BODY_X
        : EGG_C3_ENTERPRISE_BODY_X;
    const uint16_t title_x = g_is_tvc
        ? EGG_C3_TVC_TITLE_X
        : EGG_C3_ENTERPRISE_TITLE_X;
    const uint16_t right_x = g_is_tvc
        ? EGG_C3_TVC_RIGHT_X
        : EGG_C3_ENTERPRISE_RIGHT_X;

    twodfx_reset_2dpt_all();

    twodfx_2d_set_color(TWODFX_LAYER_BACKGROUND, 0u,
                        profile_colour(EGG_C3_PANEL_FILL_COLOR, EGG_C3_TVC_PANEL_FILL_COLOR),
                        EGG_C3_TRANSPARENT_BACKGROUND);
    twodfx_2d_fill_rect(TWODFX_LAYER_BACKGROUND, 1u,
                        panel_x, panel_y, panel_w, panel_h);

    twodfx_2d_set_color(TWODFX_LAYER_BACKGROUND, 2u,
                        profile_colour(EGG_C3_PANEL_BORDER_COLOR, EGG_C3_TVC_PANEL_BORDER_COLOR),
                        EGG_C3_TRANSPARENT_BACKGROUND);
    twodfx_2d_rect(TWODFX_LAYER_BACKGROUND, 3u,
                   panel_x, panel_y, panel_w, panel_h,
                   EGG_C3_PANEL_BORDER_STROKE);
    twodfx_2d_line(TWODFX_LAYER_BACKGROUND, 4u,
                   panel_x, EGG_C3_SEPARATOR_Y,
                   right_x, EGG_C3_SEPARATOR_Y,
                   EGG_C3_SEPARATOR_STROKE);

    twodfx_2d_set_color(TWODFX_LAYER_BACKGROUND, 5u,
                        profile_colour(EGG_C3_TITLE_COLOR, EGG_C3_TVC_TITLE_COLOR),
                        EGG_C3_TRANSPARENT_BACKGROUND);
    twodfx_2d_draw_text(TWODFX_LAYER_BACKGROUND, 6u,
                        title_x, EGG_C3_TITLE_Y,
                        EGG_C3_FONT_BIG_ID,
                        EGG_C3_STRING_TITLE_ADDRESS);

    twodfx_2d_set_color(TWODFX_LAYER_BACKGROUND, 7u,
                        profile_colour(EGG_C3_NORMAL_COLOR, EGG_C3_TVC_NORMAL_COLOR),
                        EGG_C3_TRANSPARENT_BACKGROUND);
    twodfx_2d_draw_text(TWODFX_LAYER_BACKGROUND, 8u,
                        body_x, EGG_C3_NORMAL_Y,
                        EGG_C3_FONT_NORMAL_ID,
                        EGG_C3_STRING_NORMAL_ADDRESS);

    twodfx_2d_set_color(TWODFX_LAYER_BACKGROUND, 9u,
                        profile_colour(EGG_C3_NARROW_COLOR, EGG_C3_TVC_NARROW_COLOR),
                        EGG_C3_TRANSPARENT_BACKGROUND);
    twodfx_2d_draw_text(TWODFX_LAYER_BACKGROUND, 10u,
                        body_x, EGG_C3_NARROW_Y,
                        EGG_C3_FONT_NARROW_ID,
                        EGG_C3_STRING_NARROW_ADDRESS);

    twodfx_2d_set_color(TWODFX_LAYER_BACKGROUND, 11u,
                        wide_text_colour(),
                        EGG_C3_TRANSPARENT_BACKGROUND);
    twodfx_2d_draw_text(TWODFX_LAYER_BACKGROUND, 12u,
                        body_x, EGG_C3_WIDE_Y,
                        EGG_C3_FONT_WIDE_ID,
                        EGG_C3_STRING_WIDE_ADDRESS);

    twodfx_2d_set_color(TWODFX_LAYER_BACKGROUND, 13u,
                        profile_colour(EGG_C3_TALL_COLOR, EGG_C3_TVC_TALL_COLOR),
                        EGG_C3_TRANSPARENT_BACKGROUND);
    twodfx_2d_draw_text(TWODFX_LAYER_BACKGROUND, 14u,
                        body_x, EGG_C3_TALL_Y,
                        EGG_C3_FONT_TALL_ID,
                        EGG_C3_STRING_TALL_ADDRESS);

    twodfx_2d_set_color(TWODFX_LAYER_BACKGROUND, 15u,
                        profile_colour(EGG_C3_BIG_COLOR, EGG_C3_TVC_BIG_COLOR),
                        EGG_C3_TRANSPARENT_BACKGROUND);
    twodfx_2d_draw_text(TWODFX_LAYER_BACKGROUND, 16u,
                        body_x, EGG_C3_BIG_Y,
                        EGG_C3_FONT_BIG_ID,
                        EGG_C3_STRING_BIG_ADDRESS);

    twodfx_2d_set_color(TWODFX_LAYER_BACKGROUND, 17u,
                        profile_colour(EGG_C3_TRANSPARENT_TEXT_COLOR, EGG_C3_TVC_TRANSPARENT_TEXT_COLOR),
                        EGG_C3_TRANSPARENT_BACKGROUND);
    twodfx_2d_draw_text(TWODFX_LAYER_BACKGROUND, 18u,
                        body_x, EGG_C3_TRANS_Y,
                        EGG_C3_FONT_NORMAL_ID,
                        EGG_C3_STRING_TRANS_ADDRESS);

    twodfx_2d_set_color(TWODFX_LAYER_BACKGROUND, 19u,
                        profile_colour(EGG_C3_FILLED_TEXT_COLOR, EGG_C3_TVC_FILLED_TEXT_COLOR),
                        profile_colour(EGG_C3_FILLED_BACKGROUND, EGG_C3_TVC_FILLED_BACKGROUND));
    twodfx_2d_draw_text(TWODFX_LAYER_BACKGROUND, 20u,
                        body_x, EGG_C3_FILLED_Y,
                        EGG_C3_FONT_NORMAL_ID,
                        EGG_C3_STRING_FILLED_ADDRESS);

    twodfx_2d_set_color(TWODFX_LAYER_BACKGROUND, 21u,
                        profile_colour(EGG_C3_COUNTER_COLOR, EGG_C3_TVC_COUNTER_COLOR),
                        EGG_C3_TRANSPARENT_BACKGROUND);
    twodfx_2d_draw_text(TWODFX_LAYER_BACKGROUND, 22u,
                        body_x, EGG_C3_COUNTER_Y,
                        EGG_C3_FONT_NORMAL_ID,
                        EGG_C3_STRING_COUNTER_ADDRESS);

    twodfx_2d_ret(TWODFX_LAYER_BACKGROUND, 23u);
}

static void begin_visible_showcase(void)
{
    define_fonts();
    g_frame_counter = 0u;
    build_scene();
}

/* ------------------------------------------------------------------------- */
/* Asynchronous font installation                                            */
/* ------------------------------------------------------------------------- */

static void service_font_loading(void)
{
    twodfx_resource_job_status_t status = twodfx_resource_job_status();

    if (status == TWODFX_RESOURCE_JOB_BUSY) {
        return;
    }

    if (status == TWODFX_RESOURCE_JOB_ERROR) {
        g_font_load_state = EGG_C3_FONT_LOAD_ERROR;
        return;
    }

    if (g_font_load_state < EGG_C3_FONT_LOAD_BIG) {
        g_font_load_state = (egg_C3_font_load_state_t)(g_font_load_state + 1);

        if (!queue_font_decompression(g_font_load_state)) {
            g_font_load_state = EGG_C3_FONT_LOAD_ERROR;
        }
        return;
    }

    if (g_font_load_state == EGG_C3_FONT_LOAD_BIG) {
        g_font_load_state = EGG_C3_FONT_LOAD_READY;
        begin_visible_showcase();
    }
}

/* ------------------------------------------------------------------------- */
/* Live counter                                                              */
/* ------------------------------------------------------------------------- */

static void upload_counter_string(void)
{
    uint8_t text[sizeof(egg_C3_string_counter_initial_raw)];
    uint16_t value = g_frame_counter;

    memcpy(text,
           egg_C3_string_counter_initial_raw,
           sizeof(egg_C3_string_counter_initial_raw));

    text[EGG_C3_COUNTER_THOUSANDS_INDEX] =
        (uint8_t)('0' + ((value / 1000u) % 10u));
    text[EGG_C3_COUNTER_HUNDREDS_INDEX] =
        (uint8_t)('0' + ((value / 100u) % 10u));
    text[EGG_C3_COUNTER_TENS_INDEX] =
        (uint8_t)('0' + ((value / 10u) % 10u));
    text[EGG_C3_COUNTER_ONES_INDEX] =
        (uint8_t)('0' + (value % 10u));

    twodfx_upload_raw(EGG_C3_STRING_COUNTER_ADDRESS,
                      text,
                      sizeof(text));
}

void egg_C3_text_start(void)
{
    g_is_tvc = twodfx_get_machine() == TWODFX_MACHINE_TVC;

    twodfx_reset_sprites();
    twodfx_reset_2dpt_all();

    upload_static_strings();

    g_frame_counter = 0u;
    g_font_load_state = EGG_C3_FONT_LOAD_NARROW;

    if (!queue_font_decompression(g_font_load_state)) {
        g_font_load_state = EGG_C3_FONT_LOAD_ERROR;
    }
}

void egg_C3_text_tick(void)
{
    if (g_font_load_state != EGG_C3_FONT_LOAD_READY) {
        service_font_loading();
        return;
    }

    ++g_frame_counter;
    if (g_frame_counter > EGG_C3_COUNTER_MAXIMUM) {
        g_frame_counter = 0u;
    }

    upload_counter_string();
}
