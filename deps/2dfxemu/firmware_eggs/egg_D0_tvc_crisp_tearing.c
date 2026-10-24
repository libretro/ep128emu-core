#include "egg_D0_tvc_crisp_tearing.h"

#include <stdbool.h>
#include <stdint.h>

/* ------------------------------------------------------------------------- */
/* Runtime state                                                             */
/* ------------------------------------------------------------------------- */

static egg_D0_mode_t g_mode;
static uint16_t g_mode_tick;
static uint8_t g_flash_phase;
static uint16_t g_boundary_x;
static int16_t g_boundary_step;
static bool g_apply_test_transparency_at_next_update;

/* ------------------------------------------------------------------------- */
/* Public-API frame playlists                                                */
/* ------------------------------------------------------------------------- */

static void build_full_colour_frame(uint8_t colour)
{
    uint8_t slot = 0u;

    twodfx_2d_set_color(TWODFX_LAYER_BACKGROUND, slot++,
                        colour, EGG_D0_COLOUR_BLACK);
    twodfx_2d_fill_rect(TWODFX_LAYER_BACKGROUND, slot++,
                        EGG_D0_FRAMEBUFFER_X,
                        EGG_D0_FRAMEBUFFER_Y,
                        EGG_D0_FRAMEBUFFER_WIDTH,
                        EGG_D0_FRAMEBUFFER_HEIGHT);
    twodfx_2d_ret(TWODFX_LAYER_BACKGROUND, slot);
}

static void build_boundary_frame(uint16_t boundary_x)
{
    uint8_t slot = 0u;

    /* Black base guarantees a complete stored-frame update every time. */
    twodfx_2d_set_color(TWODFX_LAYER_BACKGROUND, slot++,
                        EGG_D0_COLOUR_BLACK,
                        EGG_D0_COLOUR_BLACK);
    twodfx_2d_fill_rect(TWODFX_LAYER_BACKGROUND, slot++,
                        EGG_D0_FRAMEBUFFER_X,
                        EGG_D0_FRAMEBUFFER_Y,
                        EGG_D0_FRAMEBUFFER_WIDTH,
                        EGG_D0_FRAMEBUFFER_HEIGHT);

    /* Overlay white from the moving boundary to the stored right edge. */
    if (boundary_x < EGG_D0_FRAMEBUFFER_WIDTH) {
        twodfx_2d_set_color(TWODFX_LAYER_BACKGROUND, slot++,
                            EGG_D0_COLOUR_WHITE,
                            EGG_D0_COLOUR_BLACK);
        twodfx_2d_fill_rect(
            TWODFX_LAYER_BACKGROUND, slot++,
            boundary_x,
            EGG_D0_FRAMEBUFFER_Y,
            (uint16_t)(EGG_D0_FRAMEBUFFER_WIDTH - boundary_x),
            EGG_D0_FRAMEBUFFER_HEIGHT);
    }

    twodfx_2d_ret(TWODFX_LAYER_BACKGROUND, slot);
}

/* ------------------------------------------------------------------------- */
/* Animation state                                                           */
/* ------------------------------------------------------------------------- */

static void advance_boundary(void)
{
    int32_t next = (int32_t)g_boundary_x + (int32_t)g_boundary_step;

    if (next >= (int32_t)EGG_D0_BOUNDARY_MAX_X) {
        g_boundary_x = EGG_D0_BOUNDARY_MAX_X;
        g_boundary_step = -(int16_t)EGG_D0_BOUNDARY_STEP_PIXELS;
    } else if (next <= (int32_t)EGG_D0_BOUNDARY_MIN_X) {
        g_boundary_x = EGG_D0_BOUNDARY_MIN_X;
        g_boundary_step = (int16_t)EGG_D0_BOUNDARY_STEP_PIXELS;
    } else {
        g_boundary_x = (uint16_t)next;
    }
}

static void select_next_mode(void)
{
    g_mode_tick = 0u;

    if (g_mode == EGG_D0_MODE_FLASH) {
        g_mode = EGG_D0_MODE_MOVING_BOUNDARY;
        g_boundary_x = EGG_D0_BOUNDARY_MIN_X;
        g_boundary_step = (int16_t)EGG_D0_BOUNDARY_STEP_PIXELS;
    } else {
        g_mode = EGG_D0_MODE_FLASH;
        g_flash_phase = 0u;
    }
}

void egg_D0_tvc_crisp_tearing_start(void)
{
    twodfx_set_fps(TWODFX_FPS_50);
    twodfx_reset_sprites();
    twodfx_reset_2dpt_all();

    g_mode = EGG_D0_MODE_FLASH;
    g_mode_tick = 0u;
    g_flash_phase = 0u;
    g_boundary_x = EGG_D0_BOUNDARY_MIN_X;
    g_boundary_step = (int16_t)EGG_D0_BOUNDARY_STEP_PIXELS;

    /*
     * Preserve the established behavior: select the test transparency from
     * the first tick rather than from the activation callback itself.
     */
    g_apply_test_transparency_at_next_update = true;

    build_full_colour_frame(EGG_D0_COLOUR_DARK_RED);
}

void egg_D0_tvc_crisp_tearing_tick(void)
{
    if (g_apply_test_transparency_at_next_update) {
        twodfx_set_transparent_color(EGG_D0_TRANSPARENT_COLOUR);
        g_apply_test_transparency_at_next_update = false;
    }

    if (g_mode == EGG_D0_MODE_FLASH) {
        uint8_t colour = (g_flash_phase == 0u)
            ? EGG_D0_COLOUR_DARK_RED
            : EGG_D0_COLOUR_CYAN;

        build_full_colour_frame(colour);
        g_flash_phase ^= 1u;
    } else {
        build_boundary_frame(g_boundary_x);
        advance_boundary();
    }

    ++g_mode_tick;
    if (g_mode_tick >= EGG_D0_MODE_TICKS) {
        select_next_mode();
    }
}
