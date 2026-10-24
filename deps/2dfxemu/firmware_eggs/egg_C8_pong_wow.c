#include "egg_C8_pong_wow.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "twodfx_api.h"
#include "twodfx_egg.h"

#include "egg_C8_pong_wow_assets.h"

typedef struct {
    uint16_t field_x;
    uint16_t field_y;
    uint16_t field_width;
    uint16_t field_height;
    uint16_t tilemap_x;
    uint16_t tilemap_y;
    uint8_t tilemap_columns;
    uint8_t tilemap_rows;
} egg_C8_geometry_t;

static egg_C8_geometry_t g_geometry;
static bool g_is_tvc;

static int16_t g_left_paddle_y;
static int16_t g_right_paddle_y;
static int16_t g_ball_x;
static int16_t g_ball_y;
static int16_t g_ball_dx;
static int16_t g_ball_dy;

static uint8_t g_left_score;
static uint8_t g_right_score;
static uint8_t g_previous_input;
static uint8_t g_current_input;
static uint16_t g_serve_delay;
static uint16_t g_flash_timer;
static uint16_t g_hit_timer;
static uint16_t g_tick;


static const int8_t g_scene_shake_x[8] = EGG_C8_SCENE_SHAKE_X_VALUES;
static const int8_t g_scene_shake_y[8] = EGG_C8_SCENE_SHAKE_Y_VALUES;
static const int8_t g_sprite_shake_x[8] = EGG_C8_SPRITE_SHAKE_X_VALUES;
static const int8_t g_sprite_shake_y[8] = EGG_C8_SPRITE_SHAKE_Y_VALUES;

static uint16_t field_center_x(void)
{
    return (uint16_t)(g_geometry.field_x + g_geometry.field_width / 2u);
}

static uint16_t left_paddle_x(void)
{
    return (uint16_t)(g_geometry.field_x + EGG_C8_PADDLE_X_INSET);
}

static uint16_t right_paddle_x(void)
{
    return (uint16_t)(g_geometry.field_x + g_geometry.field_width
                      - EGG_C8_PADDLE_WIDTH - EGG_C8_PADDLE_X_INSET);
}

static uint16_t paddle_min_y(void)
{
    return (uint16_t)(g_geometry.field_y + EGG_C8_PADDLE_Y_INSET);
}

static uint16_t paddle_max_y(void)
{
    return (uint16_t)(g_geometry.field_y + g_geometry.field_height
                      - EGG_C8_PADDLE_Y_INSET - EGG_C8_PADDLE_HEIGHT);
}

static uint16_t ball_height(void)
{
    return EGG_C8_BALL_HEIGHT;
}

static uint16_t font_cell_width(void)
{
    return (uint16_t)(EGG_C8_FONT_WIDTH_UNITS * 8u);
}

static uint16_t text_width(uint16_t characters)
{
    return (uint16_t)(characters * font_cell_width());
}

static void select_geometry(void)
{
    g_is_tvc = twodfx_get_machine() == TWODFX_MACHINE_TVC;

    if (g_is_tvc) {
        g_geometry.field_x = EGG_C8_TVC_FIELD_X;
        g_geometry.field_y = EGG_C8_TVC_FIELD_Y;
        g_geometry.field_width = EGG_C8_TVC_FIELD_WIDTH;
        g_geometry.field_height = EGG_C8_TVC_FIELD_HEIGHT;
        g_geometry.tilemap_columns = EGG_C8_TVC_TILEMAP_COLUMNS;
        g_geometry.tilemap_rows = EGG_C8_TVC_TILEMAP_ROWS;
        g_geometry.tilemap_x = (uint16_t)(
            g_geometry.field_x + EGG_C8_TVC_TILEMAP_X_INSET);
        g_geometry.tilemap_y = (uint16_t)(
            g_geometry.field_y + EGG_C8_TVC_TILEMAP_Y_INSET);
    } else {
        g_geometry.field_x = EGG_C8_EP_FIELD_X;
        g_geometry.field_y = EGG_C8_EP_FIELD_Y;
        g_geometry.field_width = EGG_C8_EP_FIELD_WIDTH;
        g_geometry.field_height = EGG_C8_EP_FIELD_HEIGHT;
        g_geometry.tilemap_columns = EGG_C8_EP_TILEMAP_COLUMNS;
        g_geometry.tilemap_rows = EGG_C8_EP_TILEMAP_ROWS;
        g_geometry.tilemap_x = (uint16_t)(
            g_geometry.field_x + EGG_C8_EP_TILEMAP_X_INSET);
        g_geometry.tilemap_y = (uint16_t)(
            g_geometry.field_y + EGG_C8_EP_TILEMAP_Y_INSET);
    }
}


static void upload_fixed_assets(void)
{
    /* All deterministic artwork lives as const data in flash. */
    const uint8_t *left_paddle = g_is_tvc
        ? egg_C8_tvc_left_paddle_raw : egg_C8_ep_left_paddle_raw;
    const uint8_t *right_paddle = g_is_tvc
        ? egg_C8_tvc_right_paddle_raw : egg_C8_ep_right_paddle_raw;
    const uint8_t *ball = g_is_tvc
        ? egg_C8_tvc_ball_raw : egg_C8_ep_ball_raw;
    const uint8_t *font = egg_C8_font_16x8_raw;
    const uint8_t *patterns = g_is_tvc
        ? egg_C8_tvc_patterns_raw : egg_C8_ep_patterns_raw;
    const uint8_t *tileset = g_is_tvc
        ? egg_C8_tvc_tileset_raw : egg_C8_ep_tileset_raw;
    const uint32_t ball_bytes = g_is_tvc
        ? sizeof(egg_C8_tvc_ball_raw) : sizeof(egg_C8_ep_ball_raw);
    const uint32_t font_bytes = sizeof(egg_C8_font_16x8_raw);

    twodfx_upload_raw(EGG_C8_LEFT_PADDLE_ADDRESS,
                      left_paddle,
                      sizeof(egg_C8_ep_left_paddle_raw));
    twodfx_upload_raw(EGG_C8_RIGHT_PADDLE_ADDRESS,
                      right_paddle,
                      sizeof(egg_C8_ep_right_paddle_raw));
    twodfx_upload_raw(EGG_C8_BALL_ADDRESS,
                      ball,
                      ball_bytes);

    twodfx_upload_raw(EGG_C8_FONT_ADDRESS,
                      font,
                      font_bytes);
    twodfx_upload_raw(EGG_C8_PATTERN_ADDRESS,
                      patterns,
                      sizeof(egg_C8_ep_patterns_raw));
    twodfx_upload_raw(EGG_C8_TILESET_ADDRESS,
                      tileset,
                      sizeof(egg_C8_ep_tileset_raw));

    twodfx_upload_raw(EGG_C8_TITLE_STRING_ADDRESS,
                      egg_C8_title_string_raw,
                      sizeof(egg_C8_title_string_raw));
    twodfx_upload_raw(EGG_C8_HINT_STRING_ADDRESS,
                      egg_C8_hint_string_raw,
                      sizeof(egg_C8_hint_string_raw));
}

static uint8_t active_green(void)
{
    return g_is_tvc ? EGG_C8_TVC_GREEN_COLOUR : EGG_C8_EP_GREEN_COLOUR;
}

static uint8_t active_cyan(void)
{
    return g_is_tvc ? EGG_C8_TVC_CYAN_COLOUR : EGG_C8_EP_CYAN_COLOUR;
}

static uint8_t active_yellow(void)
{
    return g_is_tvc ? EGG_C8_TVC_YELLOW_COLOUR : EGG_C8_EP_YELLOW_COLOUR;
}

static uint8_t active_white(void)
{
    return g_is_tvc ? EGG_C8_TVC_WHITE_COLOUR : EGG_C8_EP_WHITE_COLOUR;
}

static uint8_t active_red(void)
{
    return g_is_tvc ? EGG_C8_TVC_RED_COLOUR : EGG_C8_EP_RED_COLOUR;
}

static uint8_t active_dark(void)
{
    return g_is_tvc ? EGG_C8_TVC_DARK_COLOUR : EGG_C8_EP_DARK_COLOUR;
}

static uint8_t active_dark_accent(void)
{
    return g_is_tvc ? EGG_C8_TVC_DARK_ACCENT : EGG_C8_EP_DARK_ACCENT;
}

static uint8_t trail_colour(void)
{
    return g_is_tvc ? EGG_C8_TVC_GREY_COLOUR : EGG_C8_EP_GREY_COLOUR;
}

static uint8_t tile_for_cell(uint8_t x, uint8_t y,
                             uint8_t columns, uint8_t rows)
{
    if (x == 0u || x == columns - 1u || y == 0u || y == rows - 1u) {
        return EGG_C8_TILE_BORDER;
    }
    if (y == EGG_C8_TILEMAP_H_BAR_ROW_A ||
        y == EGG_C8_TILEMAP_H_BAR_ROW_B) {
        return EGG_C8_TILE_H_BAR;
    }
    if (x == EGG_C8_TILEMAP_V_BAR_COL_A ||
        x == EGG_C8_TILEMAP_V_BAR_COL_B ||
        x == EGG_C8_TILEMAP_V_BAR_COL_C) {
        return EGG_C8_TILE_V_BAR;
    }
    if (((x + y + (g_tick >> EGG_C8_TILEMAP_ANIMATION_SHIFT))
         & EGG_C8_TILEMAP_SPARKLE_MASK) == 0u) {
        return EGG_C8_TILE_SPARKLE;
    }
    if (((x + y) & EGG_C8_TILEMAP_WAVE_MASK) == 0u) {
        return EGG_C8_TILE_WAVE;
    }
    return EGG_C8_TILE_FLOOR;
}

static void build_and_upload_tilemap(void)
{
    const uint8_t columns = g_geometry.tilemap_columns;
    const uint8_t rows = g_geometry.tilemap_rows;
    uint8_t row[EGG_C8_MAX_TILEMAP_COLUMNS];

    /* One short row is enough; the complete animated map never occupies BSS. */
    for (uint8_t y = 0u; y < rows; ++y) {
        for (uint8_t x = 0u; x < columns; ++x) {
            row[x] = tile_for_cell(x, y, columns, rows);
        }
        twodfx_upload_raw(EGG_C8_TILEMAP_ADDRESS + (uint32_t)y * columns,
                          row,
                          columns);
    }
}

static void upload_score_string(uint32_t address,
                                const uint8_t *template_string,
                                uint8_t score)
{
    uint8_t string[sizeof(egg_C8_p1_string_template_raw)];

    memcpy(string, template_string, sizeof(string));
    string[EGG_C8_SCORE_DIGIT_OFFSET] =
        (uint8_t)('0' + (score % EGG_C8_SCORE_MODULUS));
    twodfx_upload_raw(address, string, sizeof(string));
}

static void upload_score_strings(void)
{
    upload_score_string(EGG_C8_P1_STRING_ADDRESS,
                        egg_C8_p1_string_template_raw,
                        g_left_score);
    upload_score_string(EGG_C8_P2_STRING_ADDRESS,
                        egg_C8_p2_string_template_raw,
                        g_right_score);
}

static void define_resources(void)
{
    twodfx_define_font(EGG_C8_FONT_DESCRIPTOR,
                       EGG_C8_FONT_ADDRESS,
                       EGG_C8_FONT_WIDTH_UNITS,
                       EGG_C8_FONT_HEIGHT_UNITS);
    twodfx_define_pattern(EGG_C8_PATTERN_DESCRIPTOR,
                          EGG_C8_PATTERN_ADDRESS);
    twodfx_define_tileset(EGG_C8_TILESET_DESCRIPTOR,
                          EGG_C8_TILESET_ADDRESS);
    twodfx_define_tilemap(EGG_C8_TILEMAP_DESCRIPTOR,
                          EGG_C8_TILEMAP_ADDRESS,
                          g_geometry.tilemap_columns,
                          g_geometry.tilemap_rows,
                          EGG_C8_TILESET_DESCRIPTOR);
}

static void reset_round(int16_t direction)
{
    g_ball_x = (int16_t)(g_geometry.field_x + g_geometry.field_width / 2u
                         - EGG_C8_BALL_WIDTH / 2u);
    g_ball_y = (int16_t)(g_geometry.field_y + g_geometry.field_height / 2u
                         - (int16_t)ball_height() / 2);
    g_ball_dx = direction >= 0
        ? EGG_C8_BALL_SPEED_X
        : -EGG_C8_BALL_SPEED_X;
    g_ball_dy = EGG_C8_BALL_INITIAL_DY;
    g_serve_delay = EGG_C8_SERVE_DELAY_TICKS;
}

static void reset_game(void)
{
    g_left_paddle_y = (int16_t)(
        g_geometry.field_y + g_geometry.field_height / 2u
        - EGG_C8_PADDLE_HEIGHT / 2u);
    g_right_paddle_y = g_left_paddle_y;
    g_left_score = 0u;
    g_right_score = 0u;
    g_flash_timer = 0u;
    g_hit_timer = 0u;
    reset_round(1);
}

static void clamp_paddles(void)
{
    const int16_t minimum_y = (int16_t)paddle_min_y();
    const int16_t maximum_y = (int16_t)paddle_max_y();

    if (g_left_paddle_y < minimum_y) {
        g_left_paddle_y = minimum_y;
    }
    if (g_left_paddle_y > maximum_y) {
        g_left_paddle_y = maximum_y;
    }
    if (g_right_paddle_y < minimum_y) {
        g_right_paddle_y = minimum_y;
    }
    if (g_right_paddle_y > maximum_y) {
        g_right_paddle_y = maximum_y;
    }
}

static bool rectangles_overlap(int16_t ax,
                               int16_t ay,
                               int16_t aw,
                               int16_t ah,
                               int16_t bx,
                               int16_t by,
                               int16_t bw,
                               int16_t bh)
{
    return ax < bx + bw && ax + aw > bx &&
           ay < by + bh && ay + ah > by;
}

static void read_input(void)
{
    uint8_t next_input;

    while (twodfx_egg_read_f9(&next_input)) {
        g_current_input = next_input;
    }
}

static void update_game(void)
{
    read_input();

    if ((g_current_input & EGG_C8_INPUT_NEW_GAME) != 0u &&
        (g_previous_input & EGG_C8_INPUT_NEW_GAME) == 0u) {
        reset_game();
    }
    g_previous_input = g_current_input;

    if ((g_current_input & EGG_C8_INPUT_LEFT_PADDLE_UP) != 0u) {
        g_left_paddle_y -= EGG_C8_PADDLE_SPEED;
    }
    if ((g_current_input & EGG_C8_INPUT_LEFT_PADDLE_DOWN) != 0u) {
        g_left_paddle_y += EGG_C8_PADDLE_SPEED;
    }
    if ((g_current_input & EGG_C8_INPUT_RIGHT_PADDLE_UP) != 0u) {
        g_right_paddle_y -= EGG_C8_PADDLE_SPEED;
    }
    if ((g_current_input & EGG_C8_INPUT_RIGHT_PADDLE_DOWN) != 0u) {
        g_right_paddle_y += EGG_C8_PADDLE_SPEED;
    }
    clamp_paddles();

    if (g_flash_timer > 0u) {
        --g_flash_timer;
    }
    if (g_hit_timer > 0u) {
        --g_hit_timer;
    }

    if (g_serve_delay > 0u) {
        --g_serve_delay;
        return;
    }

    g_ball_x += g_ball_dx;
    g_ball_y += g_ball_dy;

    const int16_t top_wall =
        (int16_t)(g_geometry.field_y + EGG_C8_BALL_WALL_INSET);
    const int16_t bottom_wall =
        (int16_t)(g_geometry.field_y + g_geometry.field_height
                  - EGG_C8_BALL_WALL_INSET - ball_height());

    if (g_ball_y <= top_wall) {
        g_ball_y = top_wall;
        g_ball_dy = (int16_t)-g_ball_dy;
        g_hit_timer = EGG_C8_WALL_HIT_TICKS;
    }

    if (g_ball_y >= bottom_wall) {
        g_ball_y = bottom_wall;
        g_ball_dy = (int16_t)-g_ball_dy;
        g_hit_timer = EGG_C8_WALL_HIT_TICKS;
    }

    if (g_ball_dx < 0 &&
        rectangles_overlap(g_ball_x,
                           g_ball_y,
                           EGG_C8_BALL_WIDTH,
                           (int16_t)ball_height(),
                           (int16_t)left_paddle_x(),
                           g_left_paddle_y,
                           EGG_C8_PADDLE_WIDTH,
                           EGG_C8_PADDLE_HEIGHT)) {
        g_ball_x = (int16_t)(left_paddle_x() + EGG_C8_PADDLE_WIDTH);
        g_ball_dx = EGG_C8_BALL_SPEED_X;

        const int16_t relative_hit = (int16_t)(
            g_ball_y + (int16_t)ball_height() / 2
            - (g_left_paddle_y + EGG_C8_PADDLE_HEIGHT / 2u));
        g_ball_dy = (int16_t)(relative_hit / EGG_C8_PADDLE_DEFLECT_DIV);
        if (g_ball_dy == 0) {
            g_ball_dy = relative_hit < 0
                ? -EGG_C8_BALL_MIN_DY
                : EGG_C8_BALL_MIN_DY;
        }
        g_hit_timer = EGG_C8_PADDLE_HIT_TICKS;
    }

    if (g_ball_dx > 0 &&
        rectangles_overlap(g_ball_x,
                           g_ball_y,
                           EGG_C8_BALL_WIDTH,
                           (int16_t)ball_height(),
                           (int16_t)right_paddle_x(),
                           g_right_paddle_y,
                           EGG_C8_PADDLE_WIDTH,
                           EGG_C8_PADDLE_HEIGHT)) {
        g_ball_x = (int16_t)(right_paddle_x() - EGG_C8_BALL_WIDTH);
        g_ball_dx = -EGG_C8_BALL_SPEED_X;

        const int16_t relative_hit = (int16_t)(
            g_ball_y + (int16_t)ball_height() / 2
            - (g_right_paddle_y + EGG_C8_PADDLE_HEIGHT / 2u));
        g_ball_dy = (int16_t)(relative_hit / EGG_C8_PADDLE_DEFLECT_DIV);
        if (g_ball_dy == 0) {
            g_ball_dy = relative_hit < 0
                ? -EGG_C8_BALL_MIN_DY
                : EGG_C8_BALL_MIN_DY;
        }
        g_hit_timer = EGG_C8_PADDLE_HIT_TICKS;
    }

    if (g_ball_x + (int16_t)EGG_C8_BALL_WIDTH < (int16_t)g_geometry.field_x) {
        g_right_score = (uint8_t)((g_right_score + 1u)
                                  % EGG_C8_SCORE_MODULUS);
        g_flash_timer = EGG_C8_SCORE_FLASH_TICKS;
        reset_round(-1);
    }

    if (g_ball_x > (int16_t)(g_geometry.field_x + g_geometry.field_width)) {
        g_left_score = (uint8_t)((g_left_score + 1u)
                                 % EGG_C8_SCORE_MODULUS);
        g_flash_timer = EGG_C8_SCORE_FLASH_TICKS;
        reset_round(1);
    }
}

static void build_scene(void)
{
    uint8_t slot = 0u;
    int16_t shake_x = 0;
    int16_t shake_y = 0;

    twodfx_reset_2dpt_all();

    if (g_flash_timer > EGG_C8_SCENE_SHAKE_ACTIVE_ABOVE) {
        const uint8_t phase = (uint8_t)(g_tick & EGG_C8_SHAKE_PHASE_MASK);
        shake_x = g_scene_shake_x[phase];
        shake_y = g_scene_shake_y[phase];
    }

#define EGG_C8_SCENE_X(value) \
    ((uint16_t)((int16_t)(value) + shake_x))
#define EGG_C8_SCENE_Y(value) \
    ((uint16_t)((int16_t)(value) + shake_y))

    twodfx_2d_tilemap(TWODFX_LAYER_BACKGROUND,
                      slot++,
                      EGG_C8_SCENE_X(g_geometry.tilemap_x),
                      EGG_C8_SCENE_Y(g_geometry.tilemap_y),
                      EGG_C8_TILEMAP_DESCRIPTOR,
                      0u,
                      0u,
                      g_geometry.tilemap_columns,
                      g_geometry.tilemap_rows);

    twodfx_2d_pattern_fill(
        TWODFX_LAYER_BACKGROUND,
        slot++,
        EGG_C8_SCENE_X(g_geometry.field_x + EGG_C8_ENERGY_BAR_X_INSET),
        EGG_C8_SCENE_Y(g_geometry.field_y + EGG_C8_ENERGY_BAR_Y_INSET),
        (uint16_t)(g_geometry.field_width - 2u * EGG_C8_ENERGY_BAR_X_INSET),
        EGG_C8_ENERGY_BAR_HEIGHT,
        EGG_C8_PATTERN_DESCRIPTOR,
        (uint8_t)((g_tick >> EGG_C8_PATTERN_PHASE_SHIFT)
                  & EGG_C8_PATTERN_PHASE_MASK));

    twodfx_2d_pattern_fill(
        TWODFX_LAYER_BACKGROUND,
        slot++,
        EGG_C8_SCENE_X(g_geometry.field_x + EGG_C8_ENERGY_BAR_X_INSET),
        EGG_C8_SCENE_Y(g_geometry.field_y + g_geometry.field_height
                       - EGG_C8_ENERGY_BAR_BOTTOM_INSET),
        (uint16_t)(g_geometry.field_width - 2u * EGG_C8_ENERGY_BAR_X_INSET),
        EGG_C8_ENERGY_BAR_HEIGHT,
        EGG_C8_PATTERN_DESCRIPTOR,
        (uint8_t)(((g_tick >> EGG_C8_PATTERN_PHASE_SHIFT) + 1u)
                  & EGG_C8_PATTERN_PHASE_MASK));

    twodfx_2d_pattern_fill(
        TWODFX_LAYER_BACKGROUND,
        slot++,
        EGG_C8_SCENE_X(g_geometry.field_x + EGG_C8_LEFT_PANEL_X_INSET),
        EGG_C8_SCENE_Y(g_geometry.field_y + EGG_C8_PANEL_Y_INSET),
        EGG_C8_SIDE_PANEL_WIDTH,
        EGG_C8_PANEL_HEIGHT,
        EGG_C8_PATTERN_DESCRIPTOR,
        EGG_C8_PATTERN_PANEL);

    twodfx_2d_pattern_fill(
        TWODFX_LAYER_BACKGROUND,
        slot++,
        EGG_C8_SCENE_X(g_geometry.field_x + g_geometry.field_width
                       - EGG_C8_RIGHT_PANEL_X_INSET),
        EGG_C8_SCENE_Y(g_geometry.field_y + EGG_C8_PANEL_Y_INSET),
        EGG_C8_SIDE_PANEL_WIDTH,
        EGG_C8_PANEL_HEIGHT,
        EGG_C8_PATTERN_DESCRIPTOR,
        EGG_C8_PATTERN_PANEL);

    twodfx_2d_pattern_fill(
        TWODFX_LAYER_BACKGROUND,
        slot++,
        EGG_C8_SCENE_X(field_center_x() - EGG_C8_CENTRE_PANEL_HALF_WIDTH),
        EGG_C8_SCENE_Y(g_geometry.field_y + EGG_C8_PANEL_Y_INSET),
        EGG_C8_CENTRE_PANEL_WIDTH,
        EGG_C8_PANEL_HEIGHT,
        EGG_C8_PATTERN_DESCRIPTOR,
        EGG_C8_PATTERN_CENTRE);

    twodfx_2d_set_color(TWODFX_LAYER_BACKGROUND,
                        slot++,
                        (g_flash_timer != 0u || g_hit_timer != 0u)
                            ? active_yellow()
                            : active_green(),
                        EGG_C8_TRANSPARENT_COLOUR);
    twodfx_2d_rect(TWODFX_LAYER_BACKGROUND,
                   slot++,
                   EGG_C8_SCENE_X(g_geometry.field_x),
                   EGG_C8_SCENE_Y(g_geometry.field_y),
                   g_geometry.field_width,
                   g_geometry.field_height,
                   EGG_C8_OUTER_BORDER_STROKE);

    twodfx_2d_set_color(TWODFX_LAYER_BACKGROUND,
                        slot++,
                        active_white(),
                        EGG_C8_TRANSPARENT_COLOUR);
    twodfx_2d_rect(
        TWODFX_LAYER_BACKGROUND,
        slot++,
        EGG_C8_SCENE_X(g_geometry.field_x + EGG_C8_INNER_BORDER_INSET),
        EGG_C8_SCENE_Y(g_geometry.field_y + EGG_C8_INNER_BORDER_INSET),
        (uint16_t)(g_geometry.field_width - 2u * EGG_C8_INNER_BORDER_INSET),
        (uint16_t)(g_geometry.field_height - 2u * EGG_C8_INNER_BORDER_INSET),
        EGG_C8_INNER_BORDER_STROKE);

    twodfx_2d_set_color(TWODFX_LAYER_BACKGROUND,
                        slot++,
                        active_cyan(),
                        EGG_C8_TRANSPARENT_COLOUR);
    for (uint16_t y = (uint16_t)(g_geometry.field_y
                                 + EGG_C8_CENTER_DASH_TOP);
         y < (uint16_t)(g_geometry.field_y + g_geometry.field_height
                        - EGG_C8_CENTER_DASH_BOTTOM);
         y = (uint16_t)(y + EGG_C8_CENTER_DASH_STEP)) {
        twodfx_2d_fill_rect(
            TWODFX_LAYER_BACKGROUND,
            slot++,
            EGG_C8_SCENE_X(field_center_x() - EGG_C8_CENTER_DASH_X_HALF),
            EGG_C8_SCENE_Y(y),
            EGG_C8_CENTER_DASH_WIDTH,
            EGG_C8_CENTER_DASH_HEIGHT);
    }

    twodfx_2d_set_color(TWODFX_LAYER_BACKGROUND,
                        slot++,
                        trail_colour(),
                        EGG_C8_TRANSPARENT_COLOUR);
    const int16_t trail_x =
        (int16_t)(g_ball_x - g_ball_dx * EGG_C8_TRAIL_MULTIPLIER);
    const int16_t trail_y =
        (int16_t)(g_ball_y - g_ball_dy * EGG_C8_TRAIL_MULTIPLIER);
    if (trail_x > (int16_t)(g_geometry.field_x + EGG_C8_TRAIL_MIN_INSET) &&
        trail_x < (int16_t)(g_geometry.field_x + g_geometry.field_width
                            - EGG_C8_TRAIL_MAX_INSET) &&
        trail_y > (int16_t)(g_geometry.field_y + EGG_C8_TRAIL_MIN_INSET) &&
        trail_y < (int16_t)(g_geometry.field_y + g_geometry.field_height
                            - EGG_C8_TRAIL_MAX_INSET)) {
        twodfx_2d_fill_rect(TWODFX_LAYER_BACKGROUND,
                            slot++,
                            EGG_C8_SCENE_X(trail_x),
                            EGG_C8_SCENE_Y(trail_y),
                            EGG_C8_BALL_WIDTH,
                            ball_height());
    }

    const uint16_t title_x = (uint16_t)(
        field_center_x() - text_width(EGG_C8_TITLE_CHARACTERS) / 2u);
    const uint16_t p2_x = (uint16_t)(
        g_geometry.field_x + g_geometry.field_width -
        EGG_C8_P2_RIGHT_MARGIN - text_width(EGG_C8_SCORE_CHARACTERS));
    const uint16_t hint_x = (uint16_t)(
        field_center_x() - text_width(EGG_C8_HINT_CHARACTERS) / 2u);

    /*
     * Both profiles use the same opaque 16x8 cells. Besides making WOW Pong
     * visually consistent, this avoids single-pixel/transparency surprises on
     * NICK and keeps the score/hint deliberately loud and readable.
     */
    twodfx_2d_set_color(TWODFX_LAYER_BACKGROUND,
                        slot++,
                        active_white(),
                        active_dark());
    twodfx_2d_draw_text(
        TWODFX_LAYER_BACKGROUND,
        slot++,
        EGG_C8_SCENE_X(title_x),
        EGG_C8_SCENE_Y(g_geometry.field_y + EGG_C8_TITLE_Y_INSET),
        EGG_C8_FONT_DESCRIPTOR,
        EGG_C8_TITLE_STRING_ADDRESS);

    twodfx_2d_set_color(TWODFX_LAYER_BACKGROUND,
                        slot++,
                        active_white(),
                        active_red());
    twodfx_2d_draw_text(
        TWODFX_LAYER_BACKGROUND,
        slot++,
        EGG_C8_SCENE_X(g_geometry.field_x + EGG_C8_P1_X_INSET),
        EGG_C8_SCENE_Y(g_geometry.field_y + EGG_C8_SCORE_Y_INSET),
        EGG_C8_FONT_DESCRIPTOR,
        EGG_C8_P1_STRING_ADDRESS);

    twodfx_2d_set_color(TWODFX_LAYER_BACKGROUND,
                        slot++,
                        active_white(),
                        active_cyan());
    twodfx_2d_draw_text(
        TWODFX_LAYER_BACKGROUND,
        slot++,
        EGG_C8_SCENE_X(p2_x),
        EGG_C8_SCENE_Y(g_geometry.field_y + EGG_C8_SCORE_Y_INSET),
        EGG_C8_FONT_DESCRIPTOR,
        EGG_C8_P2_STRING_ADDRESS);

    twodfx_2d_set_color(TWODFX_LAYER_BACKGROUND,
                        slot++,
                        active_yellow(),
                        active_dark_accent());
    twodfx_2d_draw_text(
        TWODFX_LAYER_BACKGROUND,
        slot++,
        EGG_C8_SCENE_X(hint_x),
        EGG_C8_SCENE_Y(g_geometry.field_y + g_geometry.field_height
                       - EGG_C8_HINT_BOTTOM_INSET),
        EGG_C8_FONT_DESCRIPTOR,
        EGG_C8_HINT_STRING_ADDRESS);

    if (g_flash_timer != 0u || g_hit_timer != 0u) {
        twodfx_2d_set_color(TWODFX_LAYER_BACKGROUND,
                            slot++,
                            active_yellow(),
                            EGG_C8_TRANSPARENT_COLOUR);
        twodfx_2d_fill_rect(
            TWODFX_LAYER_BACKGROUND,
            slot++,
            EGG_C8_SCENE_X(g_geometry.field_x + EGG_C8_FLASH_LINE_X_INSET),
            EGG_C8_SCENE_Y(g_geometry.field_y + EGG_C8_FLASH_LINE_Y_INSET),
            (uint16_t)(g_geometry.field_width
                       - 2u * EGG_C8_FLASH_LINE_X_INSET),
            EGG_C8_FLASH_LINE_HEIGHT);
        twodfx_2d_fill_rect(
            TWODFX_LAYER_BACKGROUND,
            slot++,
            EGG_C8_SCENE_X(g_geometry.field_x + EGG_C8_FLASH_LINE_X_INSET),
            EGG_C8_SCENE_Y(g_geometry.field_y + g_geometry.field_height
                           - EGG_C8_FLASH_LINE_BOTTOM_INSET),
            (uint16_t)(g_geometry.field_width
                       - 2u * EGG_C8_FLASH_LINE_X_INSET),
            EGG_C8_FLASH_LINE_HEIGHT);
    }

    twodfx_2d_ret(TWODFX_LAYER_BACKGROUND, slot);

#undef EGG_C8_SCENE_X
#undef EGG_C8_SCENE_Y
}

static void update_sprites(void)
{
    int16_t shake_x = 0;
    int16_t shake_y = 0;

    if (g_flash_timer > EGG_C8_SPRITE_SHAKE_ACTIVE_ABOVE) {
        const uint8_t phase = (uint8_t)(g_tick & EGG_C8_SHAKE_PHASE_MASK);
        shake_x = g_sprite_shake_x[phase];
        shake_y = g_sprite_shake_y[phase];
    }

    twodfx_reset_sprites();

    twodfx_sprite_set(EGG_C8_LEFT_PADDLE_SPRITE,
                      EGG_C8_LEFT_PADDLE_ADDRESS,
                      (uint16_t)((int16_t)left_paddle_x() + shake_x),
                      (uint16_t)(g_left_paddle_y + shake_y),
                      EGG_C8_PADDLE_WIDTH,
                      EGG_C8_PADDLE_HEIGHT,
                      true,
                      false,
                      0u);

    twodfx_sprite_set(EGG_C8_RIGHT_PADDLE_SPRITE,
                      EGG_C8_RIGHT_PADDLE_ADDRESS,
                      (uint16_t)((int16_t)right_paddle_x() + shake_x),
                      (uint16_t)(g_right_paddle_y + shake_y),
                      EGG_C8_PADDLE_WIDTH,
                      EGG_C8_PADDLE_HEIGHT,
                      true,
                      false,
                      0u);

    twodfx_sprite_set(EGG_C8_BALL_SPRITE,
                      EGG_C8_BALL_ADDRESS,
                      (uint16_t)(g_ball_x + shake_x),
                      (uint16_t)(g_ball_y + shake_y),
                      EGG_C8_BALL_WIDTH,
                      ball_height(),
                      true,
                      false,
                      0u);
}

void egg_C8_pong_wow_start(void)
{
    select_geometry();

    g_tick = 0u;
    g_previous_input = 0u;
    g_current_input = 0u;

    upload_fixed_assets();
    define_resources();

    reset_game();
    build_and_upload_tilemap();
    upload_score_strings();
    build_scene();
    update_sprites();
}

void egg_C8_pong_wow_tick(void)
{
    ++g_tick;
    update_game();
    build_and_upload_tilemap();
    upload_score_strings();
    build_scene();
    update_sprites();
}
