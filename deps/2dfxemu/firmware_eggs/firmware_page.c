#include "firmware_page.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "twodfx_api.h"
#include "logo_2dfx_asset.h"
#include "firmware_page_font.h"
#include "firmware_page_qr.h"

/*
 * SHOW_FW is an internal, terminal firmware page, but its scene is deliberately
 * built through the same supported C API used by the converted eggs.  The
 * firmware-page job owns the shadow scene exclusively while this function is
 * running, so no normal core0 egg/API publication can race these calls.
 */

#define FW_PAGE_FONT_ID          0u
#define FW_PAGE_LOGO_BITMAP_ID   0u
#define FW_PAGE_QR_BITMAP_ID     1u

#define FW_PAGE_FONT_ADDR        0x780000u
#define FW_PAGE_LOGO_ADDR        0x782000u
#define FW_PAGE_QR_ADDR          0x7C3000u
#define FW_PAGE_STRING_BASE      0x7C5000u
#define FW_PAGE_STR_VER_ADDR     (FW_PAGE_STRING_BASE + 0x000u)
#define FW_PAGE_STR_MODE_ADDR    (FW_PAGE_STRING_BASE + 0x040u)

/*
 * QR data is doubled on X to compensate for the display's 2:1 pixel aspect.
 * Enterprise keeps the established four-visible-pixel horizontal margin.
 * TVC uses two visible horizontal pixels per side, matching the two vertical
 * pixels and making the outer panel visually square (140 raw X pixels = 70
 * visible-width pixels, 70 Y pixels).
 */
#define FW_PAGE_EP_QR_MARGIN_X_VISIBLE   4u
#define FW_PAGE_EP_QR_MARGIN_X_STORED \
    (FW_PAGE_EP_QR_MARGIN_X_VISIBLE * 2u)
#define FW_PAGE_TVC_QR_MARGIN_X_VISIBLE  2u
#define FW_PAGE_TVC_QR_MARGIN_X_STORED \
    (FW_PAGE_TVC_QR_MARGIN_X_VISIBLE * 2u)
#define FW_PAGE_QR_MARGIN_Y              2u
#define FW_PAGE_EP_QR_BOX_W \
    (FW_PAGE_QR_WIDTH + (2u * FW_PAGE_EP_QR_MARGIN_X_STORED))
#define FW_PAGE_TVC_QR_BOX_W \
    (FW_PAGE_QR_WIDTH + (2u * FW_PAGE_TVC_QR_MARGIN_X_STORED))
#define FW_PAGE_QR_BOX_H \
    (FW_PAGE_QR_HEIGHT + (2u * FW_PAGE_QR_MARGIN_Y))
#define FW_PAGE_QR_BOX_RX                4u
#define FW_PAGE_QR_BOX_RY                2u

/*
 * The source logo is 515 pixels wide but stores 258 packed bytes per row. BLIT
 * derives stride from descriptor width, therefore the descriptor uses the
 * padded 516-pixel storage width and retains the source's final pad nibble.
 */
#define FW_PAGE_LOGO_STORAGE_W       (LOGO_2DFX_BYTES_PER_ROW * 2u)

#define FW_PAGE_TEXT_X_OFFSET        6u
#define FW_PAGE_TEXT_LINE_GAP        22u

/*
 * TVC layout. The logo source has two transparent columns before its first
 * visible pixel, so X=124 places the visible artwork exactly at BASIC-safe
 * X=126. The QR box shifts right while its horizontal margin is reduced, which
 * keeps the QR bitmap itself at the established X=497 position.
 */
#define FW_PAGE_TVC_LOGO_X           124u
#define FW_PAGE_TVC_LOGO_Y            70u
#define FW_PAGE_TVC_TEXT_X           (FW_PAGE_TVC_LOGO_X + FW_PAGE_TEXT_X_OFFSET)
#define FW_PAGE_TVC_TEXT_Y1           71u
#define FW_PAGE_TVC_TEXT_Y2          (FW_PAGE_TVC_TEXT_Y1 + FW_PAGE_TEXT_LINE_GAP)
#define FW_PAGE_TVC_QR_BOX_X         493u
#define FW_PAGE_TVC_QR_BOX_Y          55u

/*
 * Enterprise layout: everything visible is contained by the measured default
 * IS-BASIC text-screen safe rectangle X=124..763, Y=38..262 inclusive.
 *
 * The 515x198 logo is centred horizontally and bottom-aligned to Y=262.  The
 * two text rows retain the established spacing.  The QR panel starts one
 * pixel below the safe-area top, at Y=39, and remains right-aligned with the
 * logo.
 */
#define FW_PAGE_EP_LOGO_X \
    (TWODFX_ENTERPRISE_BASIC_SAFE_MIN_X + \
     ((TWODFX_ENTERPRISE_BASIC_SAFE_MAX_X - \
       TWODFX_ENTERPRISE_BASIC_SAFE_MIN_X + 1u - LOGO_2DFX_WIDTH) / 2u))
#define FW_PAGE_EP_LOGO_Y \
    (TWODFX_ENTERPRISE_BASIC_SAFE_MAX_Y - LOGO_2DFX_HEIGHT + 1u)
#define FW_PAGE_EP_TEXT_X              (FW_PAGE_EP_LOGO_X + FW_PAGE_TEXT_X_OFFSET)
#define FW_PAGE_EP_TEXT_Y1             (TWODFX_ENTERPRISE_BASIC_SAFE_MIN_Y + 8u)
#define FW_PAGE_EP_TEXT_Y2             (FW_PAGE_EP_TEXT_Y1 + FW_PAGE_TEXT_LINE_GAP)
#define FW_PAGE_EP_QR_BOX_X \
    (FW_PAGE_EP_LOGO_X + LOGO_2DFX_WIDTH - FW_PAGE_EP_QR_BOX_W)
#define FW_PAGE_EP_QR_BOX_Y            (TWODFX_ENTERPRISE_BASIC_SAFE_MIN_Y + 1u)

#define FW_PAGE_STRING_CAPACITY        32u
#define FW_PAGE_FONT_BYTES \
    (128u * FW_PAGE_FONT_WIDTH_UNITS * FW_PAGE_FONT_HEIGHT_UNITS * 8u)

/* Compile-time layout guards for the measured Enterprise safe rectangle. */
_Static_assert(FW_PAGE_EP_LOGO_X >= TWODFX_ENTERPRISE_BASIC_SAFE_MIN_X,
               "SHOW_FW Enterprise logo starts left of IS-BASIC safe area");
_Static_assert((FW_PAGE_EP_LOGO_X + LOGO_2DFX_WIDTH - 1u) <=
                   TWODFX_ENTERPRISE_BASIC_SAFE_MAX_X,
               "SHOW_FW Enterprise logo ends right of IS-BASIC safe area");
_Static_assert(FW_PAGE_EP_LOGO_Y >= TWODFX_ENTERPRISE_BASIC_SAFE_MIN_Y,
               "SHOW_FW Enterprise logo starts above IS-BASIC safe area");
_Static_assert((FW_PAGE_EP_LOGO_Y + LOGO_2DFX_HEIGHT - 1u) ==
                   TWODFX_ENTERPRISE_BASIC_SAFE_MAX_Y,
               "SHOW_FW Enterprise logo must bottom-align to safe area");
_Static_assert(FW_PAGE_EP_QR_BOX_X >= TWODFX_ENTERPRISE_BASIC_SAFE_MIN_X,
               "SHOW_FW Enterprise QR starts left of safe area");
_Static_assert((FW_PAGE_EP_QR_BOX_X + FW_PAGE_EP_QR_BOX_W - 1u) <=
                   TWODFX_ENTERPRISE_BASIC_SAFE_MAX_X,
               "SHOW_FW Enterprise QR ends right of safe area");
_Static_assert((FW_PAGE_EP_QR_BOX_Y + FW_PAGE_QR_BOX_H - 1u) <=
                   TWODFX_ENTERPRISE_BASIC_SAFE_MAX_Y,
               "SHOW_FW Enterprise QR ends below safe area");

#define FW_PAGE_LOGO_VISIBLE_MIN_SOURCE_X 2u
#define FW_PAGE_LOGO_VISIBLE_MAX_SOURCE_X 511u
_Static_assert((FW_PAGE_TVC_LOGO_X + FW_PAGE_LOGO_VISIBLE_MIN_SOURCE_X) >=
                   TWODFX_TVC_BASIC_SAFE_MIN_X,
               "SHOW_FW TVC visible logo starts left of BASIC safe area");
_Static_assert((FW_PAGE_TVC_LOGO_X + FW_PAGE_LOGO_VISIBLE_MAX_SOURCE_X) <=
                   TWODFX_TVC_BASIC_SAFE_MAX_X,
               "SHOW_FW TVC visible logo ends right of BASIC safe area");
_Static_assert((FW_PAGE_TVC_QR_BOX_X + FW_PAGE_TVC_QR_BOX_W - 1u) <=
                   TWODFX_TVC_BASIC_SAFE_MAX_X,
               "SHOW_FW TVC QR ends right of BASIC safe area");

typedef struct {
    uint16_t logo_x;
    uint16_t logo_y;
    uint16_t text_x;
    uint16_t text_y1;
    uint16_t text_y2;
    uint16_t qr_box_x;
    uint16_t qr_box_y;
    uint16_t qr_box_w;
    uint16_t qr_margin_x_stored;
    uint8_t black;
    uint8_t white;
    uint8_t red;
    const char *machine_name;
} fw_page_profile_t;

static fw_page_profile_t fw_page_profile(void)
{
    if (twodfx_get_machine() == TWODFX_MACHINE_TVC) {
        return (fw_page_profile_t) {
            .logo_x = FW_PAGE_TVC_LOGO_X,
            .logo_y = FW_PAGE_TVC_LOGO_Y,
            .text_x = FW_PAGE_TVC_TEXT_X,
            .text_y1 = FW_PAGE_TVC_TEXT_Y1,
            .text_y2 = FW_PAGE_TVC_TEXT_Y2,
            .qr_box_x = FW_PAGE_TVC_QR_BOX_X,
            .qr_box_y = FW_PAGE_TVC_QR_BOX_Y,
            .qr_box_w = FW_PAGE_TVC_QR_BOX_W,
            .qr_margin_x_stored = FW_PAGE_TVC_QR_MARGIN_X_STORED,
            .black = 0u,
            .white = 15u,
            .red = 3u,
            .machine_name = "TVC",
        };
    }

    return (fw_page_profile_t) {
        .logo_x = FW_PAGE_EP_LOGO_X,
        .logo_y = FW_PAGE_EP_LOGO_Y,
        .text_x = FW_PAGE_EP_TEXT_X,
        .text_y1 = FW_PAGE_EP_TEXT_Y1,
        .text_y2 = FW_PAGE_EP_TEXT_Y2,
        .qr_box_x = FW_PAGE_EP_QR_BOX_X,
        .qr_box_y = FW_PAGE_EP_QR_BOX_Y,
        .qr_box_w = FW_PAGE_EP_QR_BOX_W,
        .qr_margin_x_stored = FW_PAGE_EP_QR_MARGIN_X_STORED,
        .black = TWODFX_ENTERPRISE_COLOUR_BLACK,
        .white = TWODFX_ENTERPRISE_COLOUR_WHITE,
        .red = TWODFX_ENTERPRISE_COLOUR_RED,
        .machine_name = "ENTERPRISE",
    };
}

static uint8_t recolor_logo_nibble(uint8_t source_color,
                                   const fw_page_profile_t *profile)
{
    source_color &= 0x0Fu;

    /* The shared source artwork uses 7 for its base letters and 1 for curve. */
    if (source_color == 7u) {
        return profile->white;
    }

    if (source_color == 1u) {
        return profile->red;
    }

    return source_color;
}

static void install_logo(const fw_page_profile_t *profile)
{
    uint8_t row[LOGO_2DFX_BYTES_PER_ROW];

    for (uint16_t y = 0u; y < LOGO_2DFX_HEIGHT; ++y) {
        const uint8_t *source =
            &logo_2dfx_data[(uint32_t)y * LOGO_2DFX_BYTES_PER_ROW];

        for (uint16_t byte_x = 0u;
             byte_x < LOGO_2DFX_BYTES_PER_ROW;
             ++byte_x) {
            const uint8_t source_pair = source[byte_x];
            const uint8_t left =
                recolor_logo_nibble((uint8_t)(source_pair >> 4), profile);
            const uint8_t right =
                recolor_logo_nibble((uint8_t)(source_pair & 0x0Fu), profile);

            /* Internal 2dfx packing: left pixel low nibble, right pixel high. */
            row[byte_x] = (uint8_t)(left | (uint8_t)(right << 4));
        }

        twodfx_upload_raw(FW_PAGE_LOGO_ADDR +
                              (uint32_t)y * LOGO_2DFX_BYTES_PER_ROW,
                          row,
                          sizeof(row));
    }

    twodfx_define_bitmap(FW_PAGE_LOGO_BITMAP_ID,
                         FW_PAGE_LOGO_ADDR,
                         FW_PAGE_LOGO_STORAGE_W,
                         LOGO_2DFX_HEIGHT);
}

static void install_qr(const fw_page_profile_t *profile)
{
    uint8_t row[FW_PAGE_QR_BYTES_PER_ROW];

    for (uint16_t y = 0u; y < FW_PAGE_QR_HEIGHT; ++y) {
        const uint8_t *source =
            &fw_page_qr_data[(uint32_t)y * FW_PAGE_QR_BYTES_PER_ROW];

        for (uint16_t byte_x = 0u;
             byte_x < FW_PAGE_QR_BYTES_PER_ROW;
             ++byte_x) {
            const uint8_t source_pair = source[byte_x];
            const uint8_t left = ((source_pair >> 4) & 0x0Fu)
                ? profile->black : profile->white;
            const uint8_t right = (source_pair & 0x0Fu)
                ? profile->black : profile->white;

            row[byte_x] = (uint8_t)(left | (uint8_t)(right << 4));
        }

        twodfx_upload_raw(FW_PAGE_QR_ADDR +
                              (uint32_t)y * FW_PAGE_QR_BYTES_PER_ROW,
                          row,
                          sizeof(row));
    }

    twodfx_define_bitmap(FW_PAGE_QR_BITMAP_ID,
                         FW_PAGE_QR_ADDR,
                         FW_PAGE_QR_WIDTH,
                         FW_PAGE_QR_HEIGHT);
}

static void install_font(void)
{
    twodfx_rram_fill(FW_PAGE_FONT_ADDR, 0u, FW_PAGE_FONT_BYTES);

    for (uint32_t i = 0u; i < FW_PAGE_FONT_GLYPH_COUNT; ++i) {
        const uint32_t glyph = (uint32_t)(fw_page_font_glyphs[i].ch & 0x7Fu);
        twodfx_upload_raw(FW_PAGE_FONT_ADDR + glyph * 32u,
                          fw_page_font_glyphs[i].data,
                          sizeof(fw_page_font_glyphs[i].data));
    }

    twodfx_define_font(FW_PAGE_FONT_ID,
                       FW_PAGE_FONT_ADDR,
                       FW_PAGE_FONT_WIDTH_UNITS,
                       FW_PAGE_FONT_HEIGHT_UNITS);
}

static void install_text_resource(uint32_t address, const char *text)
{
    uint8_t encoded[FW_PAGE_STRING_CAPACITY];
    size_t length = 0u;

    while (text[length] != '\0' &&
           length < (FW_PAGE_STRING_CAPACITY - 1u)) {
        encoded[length] = (uint8_t)(text[length] & 0x7Fu);
        ++length;
    }

    encoded[length++] = 0x80u;
    twodfx_upload_raw(address, encoded, (uint32_t)length);
}

static void install_strings(const char *version_string,
                            const fw_page_profile_t *profile)
{
    char version_line[FW_PAGE_STRING_CAPACITY];
    char mode_line[FW_PAGE_STRING_CAPACITY];

    if (version_string == NULL || version_string[0] == '\0') {
        version_string = "?";
    }

    (void)snprintf(version_line,
                   sizeof(version_line),
                   "FIRMWARE: %s",
                   version_string);
    (void)snprintf(mode_line,
                   sizeof(mode_line),
                   "MODE: %s",
                   profile->machine_name);

    install_text_resource(FW_PAGE_STR_VER_ADDR, version_line);
    install_text_resource(FW_PAGE_STR_MODE_ADDR, mode_line);
}

void twodfx_firmware_page_build_shadow_scene(const char *version_string)
{
    const fw_page_profile_t profile = fw_page_profile();
    const uint16_t qr_x =
        (uint16_t)(profile.qr_box_x + profile.qr_margin_x_stored);
    const uint16_t qr_y =
        (uint16_t)(profile.qr_box_y + FW_PAGE_QR_MARGIN_Y);

    twodfx_reset_2dpt_all();
    twodfx_reset_resource_descriptors();

    install_font();
    install_strings(version_string, &profile);
    install_logo(&profile);
    install_qr(&profile);

    /* Background: large logo, white rounded QR panel, then QR bitmap. */
    twodfx_2d_blit(TWODFX_LAYER_BACKGROUND,
                   0u,
                   profile.logo_x,
                   profile.logo_y,
                   FW_PAGE_LOGO_BITMAP_ID,
                   TWODFX_BLIT_EXACT);
    twodfx_2d_set_color(TWODFX_LAYER_BACKGROUND,
                        1u,
                        profile.white,
                        profile.black);
    twodfx_2d_fill_round_rect(TWODFX_LAYER_BACKGROUND,
                              2u,
                              profile.qr_box_x,
                              profile.qr_box_y,
                              profile.qr_box_w,
                              FW_PAGE_QR_BOX_H,
                              FW_PAGE_QR_BOX_RX,
                              FW_PAGE_QR_BOX_RY);
    twodfx_2d_blit(TWODFX_LAYER_BACKGROUND,
                   3u,
                   qr_x,
                   qr_y,
                   FW_PAGE_QR_BITMAP_ID,
                   TWODFX_BLIT_EXACT);
    twodfx_2d_ret(TWODFX_LAYER_BACKGROUND, 4u);

    /* Foreground: both firmware-information text lines. */
    twodfx_2d_set_color(TWODFX_LAYER_FOREGROUND,
                        0u,
                        profile.white,
                        profile.black);
    twodfx_2d_draw_text(TWODFX_LAYER_FOREGROUND,
                        1u,
                        profile.text_x,
                        profile.text_y1,
                        FW_PAGE_FONT_ID,
                        FW_PAGE_STR_VER_ADDR);
    twodfx_2d_draw_text(TWODFX_LAYER_FOREGROUND,
                        2u,
                        profile.text_x,
                        profile.text_y2,
                        FW_PAGE_FONT_ID,
                        FW_PAGE_STR_MODE_ADDR);
    twodfx_2d_ret(TWODFX_LAYER_FOREGROUND, 3u);
}
