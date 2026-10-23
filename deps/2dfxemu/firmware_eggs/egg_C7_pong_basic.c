#include "egg_C7_pong_basic.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "twodfx_api.h"
#include "twodfx_egg.h"

typedef struct {
    uint16_t field_x;
    uint16_t field_y;
    uint16_t field_width;
    uint16_t field_height;
} egg_C7_geometry_t;

static egg_C7_geometry_t g_geometry;
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
static uint16_t g_tick;

static uint8_t g_left_paddle_pixels[
    (EGG_C7_PADDLE_WIDTH * EGG_C7_PADDLE_HEIGHT) / 2u];
static uint8_t g_right_paddle_pixels[
    (EGG_C7_PADDLE_WIDTH * EGG_C7_PADDLE_HEIGHT) / 2u];
static uint8_t g_ball_pixels[
    (EGG_C7_BALL_WIDTH * EGG_C7_BALL_HEIGHT) / 2u];

static const uint8_t g_digit_masks[10] = EGG_C7_DIGIT_MASKS;
static const int8_t g_shake_x[8] = EGG_C7_SHAKE_X_VALUES;
static const int8_t g_shake_y[8] = EGG_C7_SHAKE_Y_VALUES;

static uint16_t field_center_x(void)
{
    return (uint16_t)(g_geometry.field_x + g_geometry.field_width / 2u);
}

static uint16_t left_paddle_x(void)
{
    return (uint16_t)(g_geometry.field_x + EGG_C7_PADDLE_X_INSET);
}

static uint16_t right_paddle_x(void)
{
    return (uint16_t)(g_geometry.field_x + g_geometry.field_width
                      - EGG_C7_PADDLE_WIDTH - EGG_C7_PADDLE_X_INSET);
}

static uint16_t paddle_min_y(void)
{
    return (uint16_t)(g_geometry.field_y + EGG_C7_PADDLE_Y_INSET);
}

static uint16_t paddle_max_y(void)
{
    return (uint16_t)(g_geometry.field_y + g_geometry.field_height
                      - EGG_C7_PADDLE_Y_INSET - EGG_C7_PADDLE_HEIGHT);
}

static uint16_t ball_height(void)
{
    return EGG_C7_BALL_HEIGHT;
}

static uint16_t digit_h_bar_height(void)
{
    return EGG_C7_DIGIT_H_BAR_HEIGHT;
}

static uint16_t digit_v_bar_height(void)
{
    return EGG_C7_DIGIT_V_BAR_HEIGHT;
}

static void select_geometry(void)
{
    g_is_tvc = twodfx_get_machine() == TWODFX_MACHINE_TVC;

    if (g_is_tvc) {
        g_geometry.field_x = EGG_C7_TVC_FIELD_X;
        g_geometry.field_y = EGG_C7_TVC_FIELD_Y;
        g_geometry.field_width = EGG_C7_TVC_FIELD_WIDTH;
        g_geometry.field_height = EGG_C7_TVC_FIELD_HEIGHT;
    } else {
        g_geometry.field_x = EGG_C7_EP_FIELD_X;
        g_geometry.field_y = EGG_C7_EP_FIELD_Y;
        g_geometry.field_width = EGG_C7_EP_FIELD_WIDTH;
        g_geometry.field_height = EGG_C7_EP_FIELD_HEIGHT;
    }
}

static uint8_t pack_pixels(uint8_t left, uint8_t right)
{
    return (uint8_t)((left & 0x0Fu) | ((right & 0x0Fu) << 4));
}

static void build_and_upload_sprite_assets(void)
{
    const uint8_t colour =
        g_is_tvc
            ? EGG_C7_TVC_SPRITE_COLOUR
            : EGG_C7_EP_SPRITE_COLOUR;
    const uint8_t packed = pack_pixels(colour, colour);

    memset(g_left_paddle_pixels, packed, sizeof(g_left_paddle_pixels));
    memset(g_right_paddle_pixels, packed, sizeof(g_right_paddle_pixels));
    memset(g_ball_pixels, packed, sizeof(g_ball_pixels));

    twodfx_upload_raw(EGG_C7_LEFT_PADDLE_ADDRESS,
                      g_left_paddle_pixels,
                      sizeof(g_left_paddle_pixels));
    twodfx_upload_raw(EGG_C7_RIGHT_PADDLE_ADDRESS,
                      g_right_paddle_pixels,
                      sizeof(g_right_paddle_pixels));
    twodfx_upload_raw(EGG_C7_BALL_ADDRESS,
                      g_ball_pixels,
                      sizeof(g_ball_pixels));
}

static void reset_round(int16_t direction)
{
    g_ball_x = (int16_t)(g_geometry.field_x + g_geometry.field_width / 2u
                         - EGG_C7_BALL_WIDTH / 2u);
    g_ball_y = (int16_t)(g_geometry.field_y + g_geometry.field_height / 2u
                         - (int16_t)ball_height() / 2);
    g_ball_dx = direction >= 0
        ? EGG_C7_BALL_SPEED_X
        : -EGG_C7_BALL_SPEED_X;
    g_ball_dy = EGG_C7_BALL_INITIAL_DY;
    g_serve_delay = EGG_C7_SERVE_DELAY_TICKS;
}

static void reset_game(void)
{
    g_left_paddle_y = (int16_t)(
        g_geometry.field_y + g_geometry.field_height / 2u
        - EGG_C7_PADDLE_HEIGHT / 2u);
    g_right_paddle_y = g_left_paddle_y;
    g_left_score = 0u;
    g_right_score = 0u;
    g_flash_timer = 0u;
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

    if ((g_current_input & EGG_C7_INPUT_NEW_GAME) != 0u &&
        (g_previous_input & EGG_C7_INPUT_NEW_GAME) == 0u) {
        reset_game();
    }
    g_previous_input = g_current_input;

    if ((g_current_input & EGG_C7_INPUT_LEFT_PADDLE_UP) != 0u) {
        g_left_paddle_y -= EGG_C7_PADDLE_SPEED;
    }
    if ((g_current_input & EGG_C7_INPUT_LEFT_PADDLE_DOWN) != 0u) {
        g_left_paddle_y += EGG_C7_PADDLE_SPEED;
    }
    if ((g_current_input & EGG_C7_INPUT_RIGHT_PADDLE_UP) != 0u) {
        g_right_paddle_y -= EGG_C7_PADDLE_SPEED;
    }
    if ((g_current_input & EGG_C7_INPUT_RIGHT_PADDLE_DOWN) != 0u) {
        g_right_paddle_y += EGG_C7_PADDLE_SPEED;
    }
    clamp_paddles();

    if (g_flash_timer > 0u) {
        --g_flash_timer;
    }

    if (g_serve_delay > 0u) {
        --g_serve_delay;
        return;
    }

    g_ball_x += g_ball_dx;
    g_ball_y += g_ball_dy;

    const int16_t top_wall =
        (int16_t)(g_geometry.field_y + EGG_C7_BALL_WALL_INSET);
    const int16_t bottom_wall =
        (int16_t)(g_geometry.field_y + g_geometry.field_height
                  - EGG_C7_BALL_WALL_INSET - ball_height());

    if (g_ball_y <= top_wall) {
        g_ball_y = top_wall;
        g_ball_dy = (int16_t)-g_ball_dy;
    }

    if (g_ball_y >= bottom_wall) {
        g_ball_y = bottom_wall;
        g_ball_dy = (int16_t)-g_ball_dy;
    }

    if (g_ball_dx < 0 &&
        rectangles_overlap(g_ball_x,
                           g_ball_y,
                           EGG_C7_BALL_WIDTH,
                           (int16_t)ball_height(),
                           (int16_t)left_paddle_x(),
                           g_left_paddle_y,
                           EGG_C7_PADDLE_WIDTH,
                           EGG_C7_PADDLE_HEIGHT)) {
        g_ball_x = (int16_t)(left_paddle_x() + EGG_C7_PADDLE_WIDTH);
        g_ball_dx = EGG_C7_BALL_SPEED_X;

        const int16_t relative_hit = (int16_t)(
            g_ball_y + (int16_t)ball_height() / 2
            - (g_left_paddle_y + EGG_C7_PADDLE_HEIGHT / 2u));
        g_ball_dy = (int16_t)(relative_hit / EGG_C7_PADDLE_DEFLECT_DIV);
        if (g_ball_dy == 0) {
            g_ball_dy = relative_hit < 0
                ? -EGG_C7_BALL_MIN_DY
                : EGG_C7_BALL_MIN_DY;
        }
    }

    if (g_ball_dx > 0 &&
        rectangles_overlap(g_ball_x,
                           g_ball_y,
                           EGG_C7_BALL_WIDTH,
                           (int16_t)ball_height(),
                           (int16_t)right_paddle_x(),
                           g_right_paddle_y,
                           EGG_C7_PADDLE_WIDTH,
                           EGG_C7_PADDLE_HEIGHT)) {
        g_ball_x = (int16_t)(right_paddle_x() - EGG_C7_BALL_WIDTH);
        g_ball_dx = -EGG_C7_BALL_SPEED_X;

        const int16_t relative_hit = (int16_t)(
            g_ball_y + (int16_t)ball_height() / 2
            - (g_right_paddle_y + EGG_C7_PADDLE_HEIGHT / 2u));
        g_ball_dy = (int16_t)(relative_hit / EGG_C7_PADDLE_DEFLECT_DIV);
        if (g_ball_dy == 0) {
            g_ball_dy = relative_hit < 0
                ? -EGG_C7_BALL_MIN_DY
                : EGG_C7_BALL_MIN_DY;
        }
    }

    if (g_ball_x + (int16_t)EGG_C7_BALL_WIDTH < (int16_t)g_geometry.field_x) {
        g_right_score = (uint8_t)((g_right_score + 1u)
                                  % EGG_C7_SCORE_MODULUS);
        g_flash_timer = EGG_C7_SCORE_FLASH_TICKS;
        reset_round(-1);
    }

    if (g_ball_x > (int16_t)(g_geometry.field_x + g_geometry.field_width)) {
        g_left_score = (uint8_t)((g_left_score + 1u)
                                 % EGG_C7_SCORE_MODULUS);
        g_flash_timer = EGG_C7_SCORE_FLASH_TICKS;
        reset_round(1);
    }
}

static uint8_t draw_digit(uint8_t slot,
                          uint8_t digit,
                          uint16_t x,
                          uint16_t y)
{
    const uint8_t mask = g_digit_masks[digit % 10u];
    const uint16_t vertical_width = EGG_C7_DIGIT_V_BAR_WIDTH;
    const uint16_t horizontal_height = digit_h_bar_height();
    const uint16_t horizontal_length = EGG_C7_DIGIT_H_BAR_LENGTH;
    const uint16_t vertical_height = digit_v_bar_height();

    if ((mask & EGG_C7_SEGMENT_A) != 0u) {
        twodfx_2d_fill_rect(TWODFX_LAYER_BACKGROUND,
                            slot++,
                            (uint16_t)(x + vertical_width),
                            y,
                            horizontal_length,
                            horizontal_height);
    }
    if ((mask & EGG_C7_SEGMENT_B) != 0u) {
        twodfx_2d_fill_rect(TWODFX_LAYER_BACKGROUND,
                            slot++,
                            (uint16_t)(x + vertical_width + horizontal_length),
                            (uint16_t)(y + horizontal_height),
                            vertical_width,
                            vertical_height);
    }
    if ((mask & EGG_C7_SEGMENT_C) != 0u) {
        twodfx_2d_fill_rect(TWODFX_LAYER_BACKGROUND,
                            slot++,
                            (uint16_t)(x + vertical_width + horizontal_length),
                            (uint16_t)(y + horizontal_height + vertical_height + horizontal_height),
                            vertical_width,
                            vertical_height);
    }
    if ((mask & EGG_C7_SEGMENT_D) != 0u) {
        twodfx_2d_fill_rect(TWODFX_LAYER_BACKGROUND,
                            slot++,
                            (uint16_t)(x + vertical_width),
                            (uint16_t)(y + 2u * (horizontal_height + vertical_height)),
                            horizontal_length,
                            horizontal_height);
    }
    if ((mask & EGG_C7_SEGMENT_E) != 0u) {
        twodfx_2d_fill_rect(TWODFX_LAYER_BACKGROUND,
                            slot++,
                            x,
                            (uint16_t)(y + horizontal_height + vertical_height + horizontal_height),
                            vertical_width,
                            vertical_height);
    }
    if ((mask & EGG_C7_SEGMENT_F) != 0u) {
        twodfx_2d_fill_rect(TWODFX_LAYER_BACKGROUND,
                            slot++,
                            x,
                            (uint16_t)(y + horizontal_height),
                            vertical_width,
                            vertical_height);
    }
    if ((mask & EGG_C7_SEGMENT_G) != 0u) {
        twodfx_2d_fill_rect(TWODFX_LAYER_BACKGROUND,
                            slot++,
                            (uint16_t)(x + vertical_width),
                            (uint16_t)(y + horizontal_height + vertical_height),
                            horizontal_length,
                            horizontal_height);
    }

    return slot;
}

static void build_scene(void)
{
    uint8_t slot = 0u;
    int8_t shake_x = 0;
    int8_t shake_y = 0;

    twodfx_reset_2dpt_all();

    if (g_flash_timer > EGG_C7_SHAKE_ACTIVE_ABOVE) {
        const uint8_t phase = (uint8_t)(g_tick & EGG_C7_SHAKE_PHASE_MASK);
        shake_x = g_shake_x[phase];
        shake_y = g_shake_y[phase];
    }

    twodfx_2d_set_xy_offset(TWODFX_LAYER_BACKGROUND,
                       slot++,
                       shake_x,
                       shake_y);

    const bool tvc = g_is_tvc;
    const bool flashing = g_flash_timer > EGG_C7_SHAKE_ACTIVE_ABOVE;
    const uint8_t field_colour = tvc
        ? (flashing ? EGG_C7_TVC_FLASH_COLOUR : EGG_C7_TVC_FIELD_COLOUR)
        : (flashing ? EGG_C7_EP_FLASH_COLOUR : EGG_C7_EP_FIELD_COLOUR);
    const uint8_t score_colour = tvc
        ? (flashing ? EGG_C7_TVC_FLASH_COLOUR : EGG_C7_TVC_SCORE_COLOUR)
        : (flashing ? EGG_C7_EP_FLASH_COLOUR : EGG_C7_EP_SCORE_COLOUR);

    twodfx_2d_set_color(TWODFX_LAYER_BACKGROUND,
                        slot++,
                        field_colour,
                        EGG_C7_TRANSPARENT_COLOUR);
    twodfx_2d_rect(TWODFX_LAYER_BACKGROUND,
                   slot++,
                   g_geometry.field_x,
                   g_geometry.field_y,
                   g_geometry.field_width,
                   g_geometry.field_height,
                   EGG_C7_FIELD_STROKE);

    for (uint16_t y = (uint16_t)(g_geometry.field_y
                                 + EGG_C7_CENTER_DASH_TOP);
         y < (uint16_t)(g_geometry.field_y + g_geometry.field_height
                        - EGG_C7_CENTER_DASH_BOTTOM);
         y = (uint16_t)(y + EGG_C7_CENTER_DASH_STEP)) {
        twodfx_2d_fill_rect(TWODFX_LAYER_BACKGROUND,
                            slot++,
                            (uint16_t)(field_center_x()
                                       - EGG_C7_CENTER_DASH_X_OFFSET),
                            y,
                            EGG_C7_CENTER_DASH_WIDTH,
                            EGG_C7_CENTER_DASH_HEIGHT);
    }

    twodfx_2d_set_color(TWODFX_LAYER_BACKGROUND,
                        slot++,
                        score_colour,
                        EGG_C7_TRANSPARENT_COLOUR);

    slot = draw_digit(slot,
                      g_left_score,
                      (uint16_t)(field_center_x()
                                 - EGG_C7_LEFT_SCORE_X_OFFSET),
                      (uint16_t)(g_geometry.field_y
                                 + EGG_C7_SCORE_Y_OFFSET));
    slot = draw_digit(slot,
                      g_right_score,
                      (uint16_t)(field_center_x()
                                 + EGG_C7_RIGHT_SCORE_X_OFFSET),
                      (uint16_t)(g_geometry.field_y
                                 + EGG_C7_SCORE_Y_OFFSET));

    twodfx_2d_ret(TWODFX_LAYER_BACKGROUND, slot);
}

static void update_sprites(void)
{
    twodfx_reset_sprites();

    twodfx_sprite_set(EGG_C7_LEFT_PADDLE_SPRITE,
                      EGG_C7_LEFT_PADDLE_ADDRESS,
                      left_paddle_x(),
                      (uint16_t)g_left_paddle_y,
                      EGG_C7_PADDLE_WIDTH,
                      EGG_C7_PADDLE_HEIGHT,
                      true,
                      false,
                      0u);

    twodfx_sprite_set(EGG_C7_RIGHT_PADDLE_SPRITE,
                      EGG_C7_RIGHT_PADDLE_ADDRESS,
                      right_paddle_x(),
                      (uint16_t)g_right_paddle_y,
                      EGG_C7_PADDLE_WIDTH,
                      EGG_C7_PADDLE_HEIGHT,
                      true,
                      false,
                      0u);

    twodfx_sprite_set(EGG_C7_BALL_SPRITE,
                      EGG_C7_BALL_ADDRESS,
                      (uint16_t)g_ball_x,
                      (uint16_t)g_ball_y,
                      EGG_C7_BALL_WIDTH,
                      ball_height(),
                      true,
                      false,
                      0u);
}

void egg_C7_pong_basic_start(void)
{
    select_geometry();

    g_tick = 0u;
    g_previous_input = 0u;
    g_current_input = 0u;

    build_and_upload_sprite_assets();
    reset_game();
    build_scene();
    update_sprites();
}

void egg_C7_pong_basic_tick(void)
{
    ++g_tick;
    update_game();
    build_scene();
    update_sprites();
}
