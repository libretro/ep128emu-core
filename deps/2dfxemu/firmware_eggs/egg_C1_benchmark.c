#include "egg_C1_benchmark.h"
#include "egg_C1_benchmark_assets.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "pico/rand.h"
#include "pico/stdlib.h"

typedef struct {
    int32_t x;
    int32_t y;
    int16_t velocity_x;
    int16_t velocity_y;
    uint16_t width;
    uint16_t height;
    uint32_t resource_address;
} egg_C1_sprite_state_t;

static bool g_is_tvc;
static egg_C1_asset_load_state_t g_asset_load_state;
static egg_C1_sprite_state_t g_sprite[TWODFX_SPRITE_COUNT];
static egg_C1_stage_t g_stage = EGG_C1_STAGE_INTRO_WAIT;
static egg_C1_phase_t g_phase = EGG_C1_PHASE_NORMAL;
static uint32_t g_frame_counter;
static uint8_t g_visible_small_count;
static uint32_t g_rng = EGG_C1_RNG_INITIAL_STATE;

/* ------------------------------------------------------------------------- */
/* Profile and random-number helpers                                         */
/* ------------------------------------------------------------------------- */

static uint32_t next_random_value(void)
{
    g_rng = g_rng * EGG_C1_RNG_MULTIPLIER + EGG_C1_RNG_INCREMENT;
    return g_rng;
}

static void seed_random_generator(void)
{
    g_rng = get_rand_32() ^ time_us_32() ^ EGG_C1_RNG_SEED_XOR;

    if (g_rng == 0u) {
        g_rng = EGG_C1_RNG_INITIAL_STATE;
    }
}

static int16_t random_small_velocity(void)
{
    return (int16_t)((int32_t)(next_random_value() %
                               EGG_C1_SMALL_VELOCITY_MODULUS) -
                     EGG_C1_SMALL_VELOCITY_BIAS);
}

static int16_t random_large_velocity(void)
{
    return (int16_t)((int32_t)(next_random_value() %
                               EGG_C1_LARGE_VELOCITY_MODULUS) +
                     EGG_C1_LARGE_VELOCITY_MINIMUM);
}

static uint32_t intro_wait_frames(void)
{
    return EGG_C1_INTRO_WAIT_SECONDS * EGG_C1_FRAMES_PER_SECOND;
}

static uint32_t transform_phase_frames(void)
{
    return EGG_C1_PHASE_SECONDS * EGG_C1_FRAMES_PER_SECOND;
}

static int32_t movement_min_x(void)
{
    return g_is_tvc
        ? EGG_C1_TVC_MOVE_MIN_X
        : EGG_C1_ENTERPRISE_MOVE_MIN_X;
}

static int32_t movement_max_x(void)
{
    return g_is_tvc
        ? EGG_C1_TVC_MOVE_MAX_X
        : EGG_C1_ENTERPRISE_MOVE_MAX_X;
}

static int32_t movement_min_y(void)
{
    return g_is_tvc
        ? EGG_C1_TVC_MOVE_MIN_Y
        : EGG_C1_ENTERPRISE_MOVE_MIN_Y;
}

static int32_t movement_max_y(void)
{
    return g_is_tvc
        ? EGG_C1_TVC_MOVE_MAX_Y
        : EGG_C1_ENTERPRISE_MOVE_MAX_Y;
}

static uint16_t clamp_sprite_coordinate(int32_t coordinate)
{
    if (coordinate < 0) {
        return 0u;
    }

    if (coordinate > 1023) {
        return 1023u;
    }

    return (uint16_t)coordinate;
}

/* ------------------------------------------------------------------------- */
/* Offline flash assets and asynchronous RRAM installation                   */
/* ------------------------------------------------------------------------- */

static void upload_selected_assets_to_rram(void)
{
    if (g_is_tvc) {
        twodfx_upload_raw(EGG_C1_SMILEY_ADDRESS,
                          egg_C1_smiley_tvc_raw,
                          sizeof(egg_C1_smiley_tvc_raw));
        twodfx_upload_raw(EGG_C1_PANEL0_ZX1_STAGING_ADDRESS,
                          egg_C1_panel0_tvc_zx1,
                          sizeof(egg_C1_panel0_tvc_zx1));
        twodfx_upload_raw(EGG_C1_PANEL1_ZX1_STAGING_ADDRESS,
                          egg_C1_panel1_tvc_zx1,
                          sizeof(egg_C1_panel1_tvc_zx1));
    } else {
        twodfx_upload_raw(EGG_C1_SMILEY_ADDRESS,
                          egg_C1_smiley_enterprise_raw,
                          sizeof(egg_C1_smiley_enterprise_raw));
        twodfx_upload_raw(EGG_C1_PANEL0_ZX1_STAGING_ADDRESS,
                          egg_C1_panel0_enterprise_zx1,
                          sizeof(egg_C1_panel0_enterprise_zx1));
        twodfx_upload_raw(EGG_C1_PANEL1_ZX1_STAGING_ADDRESS,
                          egg_C1_panel1_enterprise_zx1,
                          sizeof(egg_C1_panel1_enterprise_zx1));
    }
}

static bool queue_panel0_decompression(void)
{
    return twodfx_unzx1_raw(EGG_C1_PANEL0_ZX1_STAGING_ADDRESS,
                            EGG_C1_PANEL0_ADDRESS);
}

static bool queue_panel1_decompression(void)
{
    return twodfx_unzx1_raw(EGG_C1_PANEL1_ZX1_STAGING_ADDRESS,
                            EGG_C1_PANEL1_ADDRESS);
}

/* ------------------------------------------------------------------------- */
/* Transform and motion helpers                                              */
/* ------------------------------------------------------------------------- */

static uint8_t small_transform_flags(egg_C1_phase_t phase)
{
    switch (phase) {
        case EGG_C1_PHASE_SMALL_X_DOUBLE:
            return TWODFX_SPRITE_X_DOUBLE;

        case EGG_C1_PHASE_SMALL_Y_DOUBLE:
            return TWODFX_SPRITE_Y_DOUBLE;

        case EGG_C1_PHASE_SMALL_XY_DOUBLE:
            return (uint8_t)(TWODFX_SPRITE_X_DOUBLE |
                             TWODFX_SPRITE_Y_DOUBLE);

        case EGG_C1_PHASE_SMALL_X_FLIP:
            return TWODFX_SPRITE_X_FLIP;

        case EGG_C1_PHASE_SMALL_Y_FLIP:
            return TWODFX_SPRITE_Y_FLIP;

        case EGG_C1_PHASE_SMALL_XY_FLIP:
            return (uint8_t)(TWODFX_SPRITE_X_FLIP |
                             TWODFX_SPRITE_Y_FLIP);

        case EGG_C1_PHASE_SMALL_X_DOUBLE_X_FLIP:
            return (uint8_t)(TWODFX_SPRITE_X_DOUBLE |
                             TWODFX_SPRITE_X_FLIP);

        case EGG_C1_PHASE_SMALL_Y_DOUBLE_Y_FLIP:
            return (uint8_t)(TWODFX_SPRITE_Y_DOUBLE |
                             TWODFX_SPRITE_Y_FLIP);

        case EGG_C1_PHASE_SMALL_XY_DOUBLE_XY_FLIP:
            return (uint8_t)(TWODFX_SPRITE_X_DOUBLE |
                             TWODFX_SPRITE_Y_DOUBLE |
                             TWODFX_SPRITE_X_FLIP |
                             TWODFX_SPRITE_Y_FLIP);

        default:
            return 0u;
    }
}

static uint8_t large_transform_flags(egg_C1_phase_t phase)
{
    /* Large-panel transform phases are intentionally not enabled. */
    (void)phase;
    return 0u;
}

static uint16_t displayed_width(uint16_t source_width, uint8_t flags)
{
    return (uint16_t)(source_width *
        ((flags & TWODFX_SPRITE_X_DOUBLE) ? 2u : 1u));
}

static uint16_t displayed_height(uint16_t source_height, uint8_t flags)
{
    return (uint16_t)(source_height *
        ((flags & TWODFX_SPRITE_Y_DOUBLE) ? 2u : 1u));
}

static void initialize_sprite_states(void)
{
    memset(g_sprite, 0, sizeof(g_sprite));

    g_sprite[EGG_C1_PANEL0_SPRITE].x = EGG_C1_PANEL0_INITIAL_X;
    g_sprite[EGG_C1_PANEL0_SPRITE].y =
        movement_min_y() + EGG_C1_PANEL0_INITIAL_Y_OFFSET;
    g_sprite[EGG_C1_PANEL0_SPRITE].width = EGG_C1_PANEL_WIDTH;
    g_sprite[EGG_C1_PANEL0_SPRITE].height = EGG_C1_PANEL_HEIGHT;
    g_sprite[EGG_C1_PANEL0_SPRITE].resource_address = EGG_C1_PANEL0_ADDRESS;

    g_sprite[EGG_C1_PANEL1_SPRITE].x = EGG_C1_PANEL1_INITIAL_X;
    g_sprite[EGG_C1_PANEL1_SPRITE].y =
        movement_min_y() + EGG_C1_PANEL1_INITIAL_Y_OFFSET;
    g_sprite[EGG_C1_PANEL1_SPRITE].width = EGG_C1_PANEL_WIDTH;
    g_sprite[EGG_C1_PANEL1_SPRITE].height = EGG_C1_PANEL_HEIGHT;
    g_sprite[EGG_C1_PANEL1_SPRITE].resource_address = EGG_C1_PANEL1_ADDRESS;

    uint8_t sprite = EGG_C1_FIRST_SMALL_SPRITE;

    for (uint8_t row = 0u;
         row < EGG_C1_SMALL_GRID_ROWS && sprite < TWODFX_SPRITE_COUNT;
         ++row) {
        for (uint8_t column = 0u;
             column < EGG_C1_SMALL_GRID_COLUMNS &&
             sprite < TWODFX_SPRITE_COUNT;
             ++column) {
            if (g_is_tvc) {
                g_sprite[sprite].x =
                    EGG_C1_TVC_SMALL_GRID_X +
                    (int32_t)column * EGG_C1_TVC_SMALL_STEP_X;
                g_sprite[sprite].y =
                    EGG_C1_TVC_SMALL_GRID_Y +
                    (int32_t)row * EGG_C1_TVC_SMALL_STEP_Y;
            } else {
                g_sprite[sprite].x =
                    EGG_C1_ENTERPRISE_SMALL_GRID_X +
                    (int32_t)column * EGG_C1_ENTERPRISE_SMALL_STEP_X;
                g_sprite[sprite].y =
                    EGG_C1_ENTERPRISE_SMALL_GRID_Y +
                    (int32_t)row * EGG_C1_ENTERPRISE_SMALL_STEP_Y;
            }

            g_sprite[sprite].width = EGG_C1_SMILEY_WIDTH;
            g_sprite[sprite].height = EGG_C1_SMILEY_HEIGHT;
            g_sprite[sprite].resource_address = EGG_C1_SMILEY_ADDRESS;
            ++sprite;
        }
    }
}

static void randomize_sprite_velocities(void)
{
    for (uint8_t sprite = 0u;
         sprite < EGG_C1_LARGE_SPRITE_COUNT;
         ++sprite) {
        g_sprite[sprite].velocity_x = random_large_velocity();
        g_sprite[sprite].velocity_y = random_large_velocity();
    }

    for (uint8_t sprite = EGG_C1_FIRST_SMALL_SPRITE;
         sprite < TWODFX_SPRITE_COUNT;
         ++sprite) {
        g_sprite[sprite].velocity_x = random_small_velocity();
        g_sprite[sprite].velocity_y = random_small_velocity();
    }
}

static void move_sprite_with_wrap(egg_C1_sprite_state_t *sprite,
                                  uint8_t transform_flags)
{
    uint16_t width = displayed_width(sprite->width, transform_flags);
    uint16_t height = displayed_height(sprite->height, transform_flags);

    sprite->x += sprite->velocity_x;
    sprite->y += sprite->velocity_y;

    if (sprite->x > movement_max_x()) {
        sprite->x = movement_min_x();
    } else if ((sprite->x + (int32_t)width) < movement_min_x()) {
        sprite->x = movement_max_x();
    }

    if (sprite->y > movement_max_y()) {
        sprite->y = movement_min_y();
    } else if ((sprite->y + (int32_t)height) < movement_min_y()) {
        sprite->y = movement_max_y();
    }
}

static void move_sprite_with_bounce(egg_C1_sprite_state_t *sprite,
                                    uint8_t transform_flags)
{
    uint16_t width = displayed_width(sprite->width, transform_flags);
    uint16_t height = displayed_height(sprite->height, transform_flags);

    int32_t minimum_x = movement_min_x();
    int32_t minimum_y = movement_min_y();
    int32_t maximum_x = movement_max_x() - (int32_t)width;
    int32_t maximum_y = movement_max_y() - (int32_t)height;

    if (maximum_x < minimum_x) maximum_x = minimum_x;
    if (maximum_y < minimum_y) maximum_y = minimum_y;

    sprite->x += sprite->velocity_x;
    sprite->y += sprite->velocity_y;

    if (sprite->x < minimum_x) {
        sprite->x = minimum_x;
        sprite->velocity_x = (int16_t)-sprite->velocity_x;
    } else if (sprite->x > maximum_x) {
        sprite->x = maximum_x;
        sprite->velocity_x = (int16_t)-sprite->velocity_x;
    }

    if (sprite->y < minimum_y) {
        sprite->y = minimum_y;
        sprite->velocity_y = (int16_t)-sprite->velocity_y;
    } else if (sprite->y > maximum_y) {
        sprite->y = maximum_y;
        sprite->velocity_y = (int16_t)-sprite->velocity_y;
    }
}

static void move_all_sprites(void)
{
    uint8_t small_flags = small_transform_flags(g_phase);
    uint8_t large_flags = large_transform_flags(g_phase);

    move_sprite_with_bounce(&g_sprite[EGG_C1_PANEL0_SPRITE], large_flags);
    move_sprite_with_bounce(&g_sprite[EGG_C1_PANEL1_SPRITE], large_flags);

    for (uint8_t sprite = EGG_C1_FIRST_SMALL_SPRITE;
         sprite < TWODFX_SPRITE_COUNT;
         ++sprite) {
        move_sprite_with_wrap(&g_sprite[sprite], small_flags);
    }
}

/* ------------------------------------------------------------------------- */
/* Public sprite scene                                                       */
/* ------------------------------------------------------------------------- */

static void build_scene_for_phase(egg_C1_phase_t phase)
{
    uint8_t small_flags = small_transform_flags(phase);
    uint8_t large_flags = large_transform_flags(phase);

    twodfx_reset_sprites();

    for (uint8_t sprite = 0u;
         sprite < EGG_C1_LARGE_SPRITE_COUNT;
         ++sprite) {
        twodfx_sprite_set(
            sprite,
            g_sprite[sprite].resource_address,
            clamp_sprite_coordinate(g_sprite[sprite].x),
            clamp_sprite_coordinate(g_sprite[sprite].y),
            g_sprite[sprite].width,
            g_sprite[sprite].height,
            true,
            false,
            large_flags);
    }

    for (uint8_t sprite = EGG_C1_FIRST_SMALL_SPRITE;
         sprite < TWODFX_SPRITE_COUNT;
         ++sprite) {
        uint8_t small_index =
            (uint8_t)(sprite - EGG_C1_FIRST_SMALL_SPRITE);

        if (small_index >= g_visible_small_count) {
            continue;
        }

        twodfx_sprite_set(
            sprite,
            g_sprite[sprite].resource_address,
            clamp_sprite_coordinate(g_sprite[sprite].x),
            clamp_sprite_coordinate(g_sprite[sprite].y),
            g_sprite[sprite].width,
            g_sprite[sprite].height,
            true,
            false,
            small_flags);
    }
}

static void enter_moving_stage(void)
{
    g_stage = EGG_C1_STAGE_MOVING_TRANSFORMS;
    g_frame_counter = 0u;
    g_phase = EGG_C1_PHASE_NORMAL;

    randomize_sprite_velocities();
    build_scene_for_phase(g_phase);
}


static void begin_visible_benchmark(void)
{
    seed_random_generator();
    initialize_sprite_states();
    randomize_sprite_velocities();

    g_stage = EGG_C1_STAGE_INTRO_WAIT;
    g_phase = EGG_C1_PHASE_NORMAL;
    g_frame_counter = 0u;
    g_visible_small_count = 0u;

    build_scene_for_phase(g_phase);
}

static void service_asset_loading(void)
{
    twodfx_resource_job_status_t status = twodfx_resource_job_status();

    if (status == TWODFX_RESOURCE_JOB_BUSY) {
        return;
    }

    if (status == TWODFX_RESOURCE_JOB_ERROR) {
        g_asset_load_state = EGG_C1_ASSET_LOAD_ERROR;
        return;
    }

    if (g_asset_load_state == EGG_C1_ASSET_LOAD_PANEL0) {
        if (!queue_panel1_decompression()) {
            g_asset_load_state = EGG_C1_ASSET_LOAD_ERROR;
            return;
        }
        g_asset_load_state = EGG_C1_ASSET_LOAD_PANEL1;
        return;
    }

    if (g_asset_load_state == EGG_C1_ASSET_LOAD_PANEL1) {
        g_asset_load_state = EGG_C1_ASSET_LOAD_READY;
        begin_visible_benchmark();
    }
}

void egg_C1_benchmark_start(void)
{
    g_is_tvc = twodfx_get_machine() == TWODFX_MACHINE_TVC;

    /* Keep the screen blank until both large resources are ready. */
    twodfx_reset_sprites();
    twodfx_reset_2dpt_all();

    upload_selected_assets_to_rram();

    g_asset_load_state = EGG_C1_ASSET_LOAD_PANEL0;
    if (!queue_panel0_decompression()) {
        g_asset_load_state = EGG_C1_ASSET_LOAD_ERROR;
    }
}

void egg_C1_benchmark_tick(void)
{
    if (g_asset_load_state != EGG_C1_ASSET_LOAD_READY) {
        service_asset_loading();
        return;
    }

    switch (g_stage) {
        case EGG_C1_STAGE_INTRO_WAIT:
            ++g_frame_counter;

            if (g_frame_counter >= intro_wait_frames()) {
                g_frame_counter = 0u;
                g_stage = EGG_C1_STAGE_SPAWN_SMALL;
            }
            break;

        case EGG_C1_STAGE_SPAWN_SMALL:
            ++g_frame_counter;

            if (g_frame_counter >= EGG_C1_SPAWN_FRAMES) {
                g_frame_counter = 0u;

                if (g_visible_small_count < EGG_C1_SMALL_SPRITE_COUNT) {
                    ++g_visible_small_count;
                    build_scene_for_phase(EGG_C1_PHASE_NORMAL);
                }

                if (g_visible_small_count >= EGG_C1_SMALL_SPRITE_COUNT) {
                    g_stage = EGG_C1_STAGE_STATIC_TRANSFORMS;
                    g_phase = EGG_C1_PHASE_NORMAL;
                    g_frame_counter = 0u;
                    build_scene_for_phase(g_phase);
                }
            }
            break;

        case EGG_C1_STAGE_STATIC_TRANSFORMS:
            ++g_frame_counter;

            if (g_frame_counter >= transform_phase_frames()) {
                g_frame_counter = 0u;
                g_phase = (egg_C1_phase_t)(
                    (g_phase + 1) % EGG_C1_PHASE_COUNT);

                if (g_phase == EGG_C1_PHASE_NORMAL) {
                    enter_moving_stage();
                } else {
                    build_scene_for_phase(g_phase);
                }
            }
            break;

        case EGG_C1_STAGE_MOVING_TRANSFORMS:
        default:
            move_all_sprites();
            ++g_frame_counter;

            if (g_frame_counter >= transform_phase_frames()) {
                g_frame_counter = 0u;
                g_phase = (egg_C1_phase_t)(
                    (g_phase + 1) % EGG_C1_PHASE_COUNT);
            }

            build_scene_for_phase(g_phase);
            break;
    }

}
