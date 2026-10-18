/* This file is originated from tvc256++ firmware. The structure
   is kept to enable merge of later changes easier than re-implementing
   all the functions. */
#ifndef EP128EMU_2DFX_H
#define EP128EMU_2DFX_H


namespace TWODFX {

extern uint8_t nick_to_sprite_lut[256];
extern uint8_t tvc_to_sprite_lut[256];

void build_source_conversion_luts(void);

#include <stdbool.h>
#include <stdint.h>

// ============================================================
// 2dfx firmware baseline / public project constants
// ============================================================
//
// This private project header collects constants and low-level declarations
// shared by main.c and internal firmware modules. Keeping these values in one
// place avoids duplicated pin numbers, SPB size, screen geometry and
// transform-bit definitions across several .c files.
//
// Firmware-side applications and eggs should include twodfx_api.h (and, for
// egg input/callbacks, twodfx_egg.h) rather than treating this file as a public
// interface.

#define TWO_DFX_FW_VERSION_MAJOR 1
#define TWO_DFX_FW_VERSION_MINOR 0
#define TWO_DFX_FW_VERSION_PATCH 1
#define TWO_DFX_FW_VERSION_SUFFIX ""

#define TWO_DFX_STRINGIFY_INNER(x) #x
#define TWO_DFX_STRINGIFY(x) TWO_DFX_STRINGIFY_INNER(x)

#define TWO_DFX_FW_VERSION_NUMERIC \
    TWO_DFX_STRINGIFY(TWO_DFX_FW_VERSION_MAJOR) "." \
    TWO_DFX_STRINGIFY(TWO_DFX_FW_VERSION_MINOR) "." \
    TWO_DFX_STRINGIFY(TWO_DFX_FW_VERSION_PATCH)
#define TWO_DFX_FW_VERSION         "v" TWO_DFX_FW_VERSION_NUMERIC TWO_DFX_FW_VERSION_SUFFIX
#define TWO_DFX_FW_VERSION_DISPLAY "V" TWO_DFX_FW_VERSION_NUMERIC TWO_DFX_FW_VERSION_SUFFIX



// ============================================================
// BUILT-IN DEVELOPER / DEMO FEATURES
// ============================================================
// Firmware eggs are demonstration/test helpers, not required for normal
// application use.  Set any of these to 0 before building a release image if
// you do not want that internal egg to be startable from C0h..DFh.
#ifndef TWO_DFX_ENABLE_EGGS
#define TWO_DFX_ENABLE_EGGS                 1
#endif
#ifndef TWO_DFX_ENABLE_EGG_LOGO
#define TWO_DFX_ENABLE_EGG_LOGO             TWO_DFX_ENABLE_EGGS
#endif
#ifndef TWO_DFX_ENABLE_EGG_BENCHMARK
#define TWO_DFX_ENABLE_EGG_BENCHMARK        TWO_DFX_ENABLE_EGGS
#endif
#ifndef TWO_DFX_ENABLE_EGG_PRIMITIVES
#define TWO_DFX_ENABLE_EGG_PRIMITIVES       TWO_DFX_ENABLE_EGGS
#endif
#ifndef TWO_DFX_ENABLE_EGG_TEXT
#define TWO_DFX_ENABLE_EGG_TEXT             TWO_DFX_ENABLE_EGGS
#endif
#ifndef TWO_DFX_ENABLE_EGG_BLIT
#define TWO_DFX_ENABLE_EGG_BLIT             TWO_DFX_ENABLE_EGGS
#endif
#ifndef TWO_DFX_ENABLE_EGG_PATTERN
#define TWO_DFX_ENABLE_EGG_PATTERN          TWO_DFX_ENABLE_EGGS
#endif
#ifndef TWO_DFX_ENABLE_EGG_TILEMAP
#define TWO_DFX_ENABLE_EGG_TILEMAP          TWO_DFX_ENABLE_EGGS
#endif
#ifndef TWO_DFX_ENABLE_EGG_PONG_BASIC
#define TWO_DFX_ENABLE_EGG_PONG_BASIC       TWO_DFX_ENABLE_EGGS
#endif
#ifndef TWO_DFX_ENABLE_EGG_PONG_WOW
#define TWO_DFX_ENABLE_EGG_PONG_WOW         TWO_DFX_ENABLE_EGGS
#endif
#ifndef TWO_DFX_ENABLE_EGG_WRIGGLER
#define TWO_DFX_ENABLE_EGG_WRIGGLER         TWO_DFX_ENABLE_EGGS
#endif
#ifndef TWO_DFX_ENABLE_EGG_BOULDER
#define TWO_DFX_ENABLE_EGG_BOULDER          TWO_DFX_ENABLE_EGGS
#endif
#ifndef TWO_DFX_ENABLE_EGG_ARCPIE
#define TWO_DFX_ENABLE_EGG_ARCPIE           TWO_DFX_ENABLE_EGGS
#endif
#ifndef TWO_DFX_ENABLE_EGG_DIAG
#define TWO_DFX_ENABLE_EGG_DIAG             TWO_DFX_ENABLE_EGGS
#endif
#ifndef TWO_DFX_ENABLE_EGG_BLIT_FULL
#define TWO_DFX_ENABLE_EGG_BLIT_FULL        TWO_DFX_ENABLE_EGGS
#endif
#ifndef TWO_DFX_ENABLE_EGG_LAYER
#define TWO_DFX_ENABLE_EGG_LAYER            TWO_DFX_ENABLE_EGGS
#endif
#ifndef TWO_DFX_ENABLE_EGG_TVC_BITPATTERN
#define TWO_DFX_ENABLE_EGG_TVC_BITPATTERN   TWO_DFX_ENABLE_EGGS
#endif
#ifndef TWO_DFX_ENABLE_EGG_TVC_CRISP_TEARING
#define TWO_DFX_ENABLE_EGG_TVC_CRISP_TEARING TWO_DFX_ENABLE_EGGS
#endif
#ifndef TWO_DFX_ENABLE_EGG_RULER
#define TWO_DFX_ENABLE_EGG_RULER            TWO_DFX_ENABLE_EGGS
#endif
#ifndef TWO_DFX_ENABLE_EGG_XY_OFFSET
#define TWO_DFX_ENABLE_EGG_XY_OFFSET        TWO_DFX_ENABLE_EGGS
#endif

// Built-in public "powered by 2dfx" logo overlay.  This is separate from
// firmware eggs and is intended for application developers.
#ifndef TWO_DFX_ENABLE_BUILTIN_LOGO
#define TWO_DFX_ENABLE_BUILTIN_LOGO         1
#endif

// ============================================================
// PERFORMANCE PROFILE
// ============================================================
// Cold-boot listener profile. The Pico SDK/boot ROM already starts RP2350 at
// the supported 150 MHz / nominal 1.1 V operating point; this constant records
// the intended sleeping-state clock and is checked/reapplied before listening.
#define TWO_DFX_BOOT_LISTENER_CLOCK_KHZ 150000u

// Stable active-engine performance profile used by the v0.9.0 baseline.
#define TWO_DFX_SYS_CLOCK_KHZ 500000u

// ============================================================
// RENDERER / 2D MATH
// ============================================================
// Sprite transform renderer:
//   X_FLIP + X_DOUBLE uses the RP2354 hardware interpolator path by default,
//   independent of Y_DOUBLE/Y_FLIP. This was validated on the giant Wriggler
//   sprite and is no longer treated as an experiment.
//
// ARC/PIE renderer:
//   Uses the optimized incremental per-scanline sector-test path permanently.
//   Heavy CB benchmark result: old vector path ~174.3 ms, optimized path ~80.63 ms.

// ============================================================
// /EXTC ENGINE CONTROL
// ============================================================
// The current hardware uses two MCU-controlled /EXTC mode inputs:
//   GPIO18 = /EXTC force input
//   GPIO13 = /EXTC engine enable/input select
//
// Public F8h command:
//   50h SET_ENGINE_MODE (80 decimal), followed by one F9h byte:
//
// At cold boot v0.9.45 accepts only this command. All other F8h/F9h writes,
// including egg-start commands, are discarded until a nonzero mode wakes and
// initializes the complete engine.
//     0      = ENGINE_OFF       -> GPIO18=0, GPIO13=0, /EXTC inactive/high
//     1..254 = ENGINE_ON        -> GPIO18=0, GPIO13=1, comparator-driven mode
//     255    = ENGINE_ON_FORCED -> GPIO18=1, GPIO13=1, /EXTC forced active/low

// ============================================================
// TRANSPARENT COLOR
// ============================================================
#define DEFAULT_TRANSPARENT_COLOR_INDEX  0x0

// ============================================================
// GPIO MAP
// ============================================================
#define PIN_D0          0
#define PIN_D1          1
#define PIN_D2          2
#define PIN_D3          3
#define PIN_D4          4
#define PIN_D5          5
#define PIN_D6          6
#define PIN_D7          7
#define PIN_A0          8
#define PIN_CS          9
#define PIN_14M         10
#define PIN_HSYNC       11
#define PIN_VSYNC       12

#define PIN_EXTC_SOFT   13
#define PIN_EXTC_CTRL1  PIN_EXTC_SOFT  // GPIO13 /EXTC engine enable/input select

#define PIN_EC0         14
#define PIN_EC1         15
#define PIN_EC2         16
#define PIN_EC3         17

#define PIN_UNUSED4_0   18
#define PIN_EXTC_CTRL2  PIN_UNUSED4_0  // GPIO18 /EXTC force input
#define PIN_UNUSED4_1   19
#define PIN_HW_PROFILE  20  // High = Enterprise profile, low = TVC profile; sampled once at boot.
#define PIN_UNUSED4_2   PIN_HW_PROFILE
#define PIN_UNUSED4_3   21

#define PIN_BG0         22
#define PIN_BG1         23
#define PIN_BG2         24
#define PIN_BG3         25

#define PIN_STATUS0     40
#define PIN_STATUS1     41
#define PIN_STATUS2     42
#define PIN_STATUS3     43
#define PIN_STATUS4     44
#define PIN_STATUS5     45
#define PIN_STATUS6     46
#define PIN_STATUS7     47

#define LED_ON(pin)      gpio_put((pin), 0)
#define LED_OFF(pin)     gpio_put((pin), 1)

// ============================================================
// FINAL F8h READBACK / STATUS OUTPUT MAP
// ============================================================
//
// The STATUS0..STATUS7 GPIOs are intended to feed the external F8h readback
// latch/transceiver on the final 2dfx PCB.  They are also useful as scope
// probe points during development.
//
// All developer-visible status bits are active-low, because the Enterprise
// data bus has pullups and because open-drain interrupt logic is naturally
// active-low.
//
//   F8h bit7 / STATUS7 = collision detected in previous completed frame
//                         0 = collision, 1 = no collision
//
//   F8h bit6 / STATUS6 = 2dfx awake/ready presence detector
//                         1 during the cold 150 MHz sleep/listener state and
//                           throughout the wake transition
//                         0 only after the complete active engine is ready
//                         If 2dfx is absent, Enterprise pullups also read it as 1.
//
//   F8h bit5 / STATUS5 = render-budget overrun since last clear
//                         0 = overrun, 1 = no overrun
//
//   F8h bit4 / STATUS4 = RASTER_INT4 pending
//   F8h bit3 / STATUS3 = RASTER_INT3 pending
//   F8h bit2 / STATUS2 = RASTER_INT2 pending
//   F8h bit1 / STATUS1 = RASTER_INT1 pending
//                         0 = interrupt pending, 1 = not pending
//
//   F8h bit0 / STATUS0 = render-time measurement normally; SYS_RESET busy/completion during 8Fh reset
//                         public only for reset polling while SYS_RESET is active;
//                         otherwise kept primarily as an oscilloscope timing pin.
//
// The four raster interrupt pending bits should also feed the external
// open-drain /INT generation logic on the final PCB.  The firmware only drives
// the STATUS pins; the PCB must make sure Z80 /INT is pulled low safely, not
// driven push-pull against other interrupt sources.
#define F8_STATUS_BIT_RENDER_SCOPE       0u
#define F8_STATUS_BIT_RASTER_INT1        1u
#define F8_STATUS_BIT_RASTER_INT2        2u
#define F8_STATUS_BIT_RASTER_INT3        3u
#define F8_STATUS_BIT_RASTER_INT4        4u
#define F8_STATUS_BIT_RENDER_OVERRUN    5u
#define F8_STATUS_BIT_PRESENT            6u
#define F8_STATUS_BIT_COLLISION          7u

#define COLLISION_STATUS_ACTIVE_GPIO_LEVEL     0
#define COLLISION_STATUS_INACTIVE_GPIO_LEVEL   1

#define PRESENCE_STATUS_ACTIVE_GPIO_LEVEL      0
#define PRESENCE_STATUS_INACTIVE_GPIO_LEVEL    1
#define RENDER_OVERRUN_STATUS_ACTIVE_GPIO_LEVEL    0
#define RENDER_OVERRUN_STATUS_INACTIVE_GPIO_LEVEL  1

#define RASTER_INT_COUNT                       4u
#define RASTER_INT_DISABLED_LINE               0u

// User-facing raster interrupt line range.
// Line 0 is deliberately reserved as "disabled".  Lines 1..311 are accepted
// as programmable raster interrupt positions.
#define RASTER_INT_MIN_LINE                    1u
#define RASTER_INT_MAX_LINE                    311u

// ============================================================
// BOOT-SELECTED MACHINE PROFILE
// ============================================================
// PCB v6 carries one hardware-profile strap on GPIO20:
//   high = Enterprise profile
//   low  = TVC profile
//
// It is sampled once at boot after the broad GPIO Hi-Z reset pass.  Everything
// below this point reads the selected profile through the same long-standing
// geometry names, so the public F8h/F9h command model stays unchanged.

typedef enum {
    TWO_DFX_MACHINE_ENTERPRISE = 0,
    TWO_DFX_MACHINE_TVC        = 1
} two_dfx_machine_id_t;

typedef enum {
    TWO_DFX_PIO_TIMING_ENTERPRISE = 0,
    TWO_DFX_PIO_TIMING_TVC        = 1
} two_dfx_pio_timing_id_t;

typedef struct {
    two_dfx_machine_id_t machine_id;
    const char *machine_name;
    two_dfx_pio_timing_id_t pio_timing_id;

    uint16_t left_pad_pixels;
    uint16_t right_pad_pixels;
    uint16_t scanout_pixels;
    uint16_t fb_pixels;

    uint16_t scanout_words;
    uint16_t left_pad_words;
    uint16_t fb_scan_words;
    uint16_t fb_packed_line_bytes;

    uint16_t visible_start_pixel;
    uint16_t visible_scanout_offset;

    uint16_t max_y_before_wrap;
    uint16_t first_visible_y;
    uint16_t visible_lines;
    uint16_t last_visible_y;

    uint16_t raster_int_max_line;

    // GPIO IRQ edge masks selected at boot.  Enterprise keeps the v0.9.0
    // behaviour.  TVC uses the measured active-low VSYNC start.
    uint32_t hsync_irq_event;
    uint32_t vsync_irq_event;
} two_dfx_machine_profile_t;

extern two_dfx_machine_id_t g_two_dfx_machine_id;
extern two_dfx_machine_profile_t g_two_dfx_machine_profile;

static inline bool twodfx_machine_is_tvc(void)
{
    return g_two_dfx_machine_id == TWO_DFX_MACHINE_TVC;
}

/*
 * Enterprise IS-BASIC practical safe drawing rectangle and maximum NICK
 * visible rectangle measured on real hardware.  Coordinates are inclusive.
 */
#define TWO_DFX_ENTERPRISE_BASIC_SAFE_MIN_X 124u
#define TWO_DFX_ENTERPRISE_BASIC_SAFE_MAX_X 763u
#define TWO_DFX_ENTERPRISE_BASIC_SAFE_MIN_Y  38u
#define TWO_DFX_ENTERPRISE_BASIC_SAFE_MAX_Y 262u
#define TWO_DFX_ENTERPRISE_BASIC_SAFE_W \
    (TWO_DFX_ENTERPRISE_BASIC_SAFE_MAX_X - TWO_DFX_ENTERPRISE_BASIC_SAFE_MIN_X + 1u)
#define TWO_DFX_ENTERPRISE_BASIC_SAFE_H \
    (TWO_DFX_ENTERPRISE_BASIC_SAFE_MAX_Y - TWO_DFX_ENTERPRISE_BASIC_SAFE_MIN_Y + 1u)

#define TWO_DFX_ENTERPRISE_NICK_VISIBLE_MIN_X  88u
#define TWO_DFX_ENTERPRISE_NICK_VISIBLE_MAX_X 811u
#define TWO_DFX_ENTERPRISE_NICK_VISIBLE_MIN_Y  18u
#define TWO_DFX_ENTERPRISE_NICK_VISIBLE_MAX_Y 305u

/*
 * TVC VT-BASIC practical safe drawing rectangle measured on real hardware.
 * These are inclusive raw 2dfx coordinates and are used only by built-in
 * demonstration layouts; they do not alter the public renderer coordinate
 * system or the TVC machine profile.
 */
#define TWO_DFX_TVC_BASIC_SAFE_MIN_X 126u
#define TWO_DFX_TVC_BASIC_SAFE_MAX_X 637u
#define TWO_DFX_TVC_BASIC_SAFE_MIN_Y 49u
#define TWO_DFX_TVC_BASIC_SAFE_MAX_Y 288u
#define TWO_DFX_TVC_BASIC_SAFE_W \
    (TWO_DFX_TVC_BASIC_SAFE_MAX_X - TWO_DFX_TVC_BASIC_SAFE_MIN_X + 1u)
#define TWO_DFX_TVC_BASIC_SAFE_H \
    (TWO_DFX_TVC_BASIC_SAFE_MAX_Y - TWO_DFX_TVC_BASIC_SAFE_MIN_Y + 1u)

/* Maximum TVC CRTC-visible rectangle measured with register tweaking. */
#define TWO_DFX_TVC_CRTC_VISIBLE_MIN_X 79u
#define TWO_DFX_TVC_CRTC_VISIBLE_MAX_X 686u
#define TWO_DFX_TVC_CRTC_VISIBLE_MIN_Y 41u
#define TWO_DFX_TVC_CRTC_VISIBLE_MAX_Y 296u

static inline bool twodfx_machine_is_enterprise(void)
{
    return g_two_dfx_machine_id == TWO_DFX_MACHINE_ENTERPRISE;
}

// ============================================================
// SCANOUT GEOMETRY
// ============================================================
// Compile-time allocation envelope.  Runtime geometry must fit inside these
// limits.  Enterprise v0.9.0 uses the full current envelope: 832 scanout pixels,
// 744 stored framebuffer pixels and 288 visible lines.  TVC profile uses a
// smaller boot-selected scanout/framebuffer profile: 704 x 256 visible lines.
#define SCAN_PIXELS_PER_WORD       8u
#define DFX_MAX_SCANOUT_PIXELS     832u
#define DFX_MAX_FB_PIXELS          744u
#define DFX_MAX_VISIBLE_LINES      288u
#define DFX_MAX_SCANOUT_WORDS      (DFX_MAX_SCANOUT_PIXELS / SCAN_PIXELS_PER_WORD)
#define DFX_MAX_FB_SCAN_WORDS      (DFX_MAX_FB_PIXELS / SCAN_PIXELS_PER_WORD)
#define DFX_MAX_FB_PACKED_LINE_BYTES (DFX_MAX_FB_PIXELS / 2u)

// Runtime-selected geometry aliases.  These intentionally preserve the old
// internal names so the renderer and eggs remain conceptually unchanged while
// the numbers come from the boot-selected machine profile rather than from
// preprocessor constants.
#define LEFT_PAD_PIXELS        (g_two_dfx_machine_profile.left_pad_pixels)
#define RIGHT_PAD_PIXELS       (g_two_dfx_machine_profile.right_pad_pixels)
#define SCANOUT_PIXELS         (g_two_dfx_machine_profile.scanout_pixels)
#define FB_PIXELS              (g_two_dfx_machine_profile.fb_pixels)

#define SCANOUT_WORDS          (g_two_dfx_machine_profile.scanout_words)
#define LEFT_PAD_WORDS         (g_two_dfx_machine_profile.left_pad_words)
#define FB_SCAN_WORDS          (g_two_dfx_machine_profile.fb_scan_words)
#define FB_PACKED_LINE_BYTES   (g_two_dfx_machine_profile.fb_packed_line_bytes)

#define VISIBLE_START_PIXEL    (g_two_dfx_machine_profile.visible_start_pixel)
#define VISIBLE_SCANOUT_OFFSET (g_two_dfx_machine_profile.visible_scanout_offset)

// ============================================================
// RASTER MODEL
// ============================================================
#define MAX_Y_BEFORE_WRAP      (g_two_dfx_machine_profile.max_y_before_wrap)
#define FIRST_VISIBLE_Y        (g_two_dfx_machine_profile.first_visible_y)
#define VISIBLE_LINES          (g_two_dfx_machine_profile.visible_lines)
#define LAST_VISIBLE_Y         (g_two_dfx_machine_profile.last_visible_y)

// Runtime upper bound for public raster interrupt commands.  Line 0 remains
// disabled; the minimum accepted line remains 1.
#undef RASTER_INT_MAX_LINE
#define RASTER_INT_MAX_LINE    (g_two_dfx_machine_profile.raster_int_max_line)

// ============================================================
// SPRITE PARAMETER BLOCK
// ============================================================
#define NUM_SPBS            64
#define SPB_SIZE            9

#define SPB_B0              0
#define SPB_B1_X_LOW        1
#define SPB_B2_Y_LOW        2
#define SPB_B3_ADDR_LOW     3
#define SPB_B4_ADDR_MID     4
#define SPB_B5_ADDR_HIGH    5
#define SPB_B6_FLAGS        6
#define SPB_B7_W_LOW        7
#define SPB_B8_H_LOW        8

// Short byte-index aliases used by compact indexing code.
#define SPB_B1              SPB_B1_X_LOW
#define SPB_B2              SPB_B2_Y_LOW
#define SPB_B3              SPB_B3_ADDR_LOW
#define SPB_B4              SPB_B4_ADDR_MID
#define SPB_B5              SPB_B5_ADDR_HIGH
#define SPB_B6              SPB_B6_FLAGS
#define SPB_B7              SPB_B7_W_LOW
#define SPB_B8              SPB_B8_H_LOW

// BYTE0:
//   b7     enable
//   b6     collision enable
//   b5-b4  X bits 9..8
//   b3     Y bit 8
//   b2-b1  W bits 9..8
//   b0     H bit 8
#define SPB_ENABLE_BIT      0x80u
#define SPB_COLLISION_BIT   0x40u

// BYTE6 transform flags:
//   b7     X double
//   b6     Y double
//   b5     X flip
//   b4     Y flip
//   b3-b0  reserved, write 0
#define SPB_X_DOUBLE_BIT    0x80u
#define SPB_Y_DOUBLE_BIT    0x40u
#define SPB_X_FLIP_BIT      0x20u
#define SPB_Y_FLIP_BIT      0x10u
#define SPB_KNOWN_FLAG_MASK 0xF0u

// ============================================================
// F8h COMMAND SPACE
// ============================================================
//
// Current high-level classes:
//   00ssssss = select sprite s, start sequential 9-byte SPB write session
//   0100bbbb = transparent color index = bbbb
//   01010000 = SET_ENGINE_MODE, F9 byte: 0=off, 1..254=on, 255=forced on
//   01010001 = SET_FPS, F9 byte 0=50fps/default, 1=25fps
//   01010010 = SET_RENDER_OVERRUN_VISUAL, F9 byte 0=off, nonzero=on
//   01010011 = CLEAR_RENDER_OVERRUN
//   01010100..01010111 = SET_RASTER_INT1..4
//   01011000 = CLEAR_RASTER_INT
//   01011001..01011010 = reserved/free
//   01011011..01011111 = resource definition commands
//   011xxxxx = background 2DPT slot commands, 60h..7Fh
//   10xxxxxx = bulk/resource/overlay operations:
//              80h raw byte-exact upload
//              81h converted NICK-format upload
//              82h converted TVC Graphics-16 upload
//              83h ZX1 decompress byte-exact
//              84h ZX1 decompress with NICK conversion
//              85h ZX1 decompress with TVC conversion
//              8Fh SYS_RESET, no F9 bytes
//              90h SHOW_BUILTIN_LOGO, 4 F9 bytes: highbits, x, y, colours
//                  Built-in logo source image size: 206 x 79 pixels.
//                  On TVC the colour byte is consumed but ignored; fixed
//                  white 0Fh and red 03h indexes are used.
//              91h HIDE_BUILTIN_LOGO
//   110xxxxx = Easter eggs / demos, C0h..DFh
//   111xxxxx = foreground 2DPT slot commands, E0h..FFh
//
// 80h..82h upload parameter block, F9h bytes after command:
//   BYTE0 = resource RAM address bits 7..0
//   BYTE1 = resource RAM address bits 15..8
//   BYTE2 = bit7 ignored, bits6..0 = resource RAM address bits 22..16
//   BYTE3 = real upload length bits 7..0
//   BYTE4 = real upload length bits 15..8
//   BYTE5.. = payload bytes
//
// Source interpretation:
//   80h UPLOAD_RAW  = store payload byte-exact
//   81h UPLOAD_NICK = convert Enterprise/NICK 16-colour bytes to internal packing
//   82h UPLOAD_TVC  = convert TVC Graphics-16 IIGGRRBB bytes to internal packing
//
// For 80h..82h, length 0001h..FFFFh means 1..65535 bytes. Length 0000h
// follows Z80 loop-count convention and means 65536 bytes.
//
// 83h..85h queue core1 ZX1 resource jobs. Parameter block, F9h bytes after command:
//   BYTE0..2 = compressed source address bits 0..22, bit7 of BYTE2 ignored
//   BYTE3..5 = destination address bits 0..22, bit7 of BYTE5 ignored
//
// Destination interpretation:
//   83h UNZX1_RAW  = byte-exact decompressed output
//   84h UNZX1_NICK = convert decompressed NICK bytes while producing output
//   85h UNZX1_TVC  = convert decompressed TVC bytes while producing output
//
// 8Eh SHOW_FW has no F9h parameters. It stops the current application, builds
// one static firmware-information framebuffer and enters forced-on display
// mode. Once accepted, every later F8h/F9h write (including SYS_RESET) is
// ignored. Only a physical RP2354 reset or power cycle leaves SHOW_FW.
//
// 8Fh SYS_RESET has no F9h parameters. It asynchronously restores public
// 2dfx application/graphics state to defaults and clears the complete 8 MiB
// resource RAM while preserving the caller-selected engine mode (OFF/NORMAL/
// FORCED). While reset is running, F8h STATUS0/bit0 reads 0; it returns to 1
// when reset is complete. 8Fh is deliberately recognized even during an
// active upload or UNZX1 argument session so software can recover immediately.

#define CMD_UPLOAD_RAW          0x80u
#define CMD_UPLOAD_NICK         0x81u
#define CMD_UPLOAD_TVC          0x82u
#define CMD_UNZX1_RAW           0x83u
#define CMD_UNZX1_NICK          0x84u
#define CMD_UNZX1_TVC           0x85u
#define CMD_SHOW_FW             0x8Eu
#define CMD_SYS_RESET            0x8Fu
#define CMD_SHOW_BUILTIN_LOGO   0x90u
#define CMD_HIDE_BUILTIN_LOGO   0x91u

#define CMD_SET_ENGINE_MODE     0x50u  // one F9 byte: 0=off, 1..254=on, 255=forced on
#define CMD_SET_FPS             0x51u  // one F9 byte: 0 = 50 fps, 1 = 25 fps
#define CMD_SET_RENDER_OVERRUN_VISUAL  0x52u  // one F9 byte: 0 = off, nonzero = on
#define CMD_CLEAR_RENDER_OVERRUN 0x53u

// SET_FPS does not change video output timing.  The RGB/EXTC scanout and
// raster interrupts remain 50 Hz.  It controls only the 2dfx scene snapshot,
// render and framebuffer-swap cadence:
//   0 = 50 fps update mode, default
//   1 = 25 fps update mode; each rendered frame is displayed for two video frames

// ============================================================
// 2DPT / 2DPB MODEL
// ============================================================
//
// 2DPT = 2D Parameter Table
// 2DPB = 2D Parameter Block
//
// 2D renderer v1 uses this command-buffer model.  The Enterprise writes
// shadow 2DPBs through the F8h/F9h interface; the firmware latches them
// to the active 2DPT at VSYNC.
//
// True dual-playlist 2DPT model:
//   60h..7Fh write the background 2DPT, rendered before sprites.
//   E0h..FFh write the foreground 2DPT, rendered after sprites.
//
// The foreground command opcode is simply the background opcode + 80h:
//   67h = background FILL_RECT, E7h = foreground FILL_RECT.
// The 2DPB stored in SRAM always keeps the normalized base opcode 60h..7Fh;
// bit7 on the public command byte only selects the target playlist.
//
// Each playlist has 256 slots and its own RET chain.
// Each replay also has independent transient state:
//   current foreground/background color
//   SET_XY_OFFSET signed X/Y offset, reset to 0,0 at the start of every replay
// SKIP_NEXT may only advance to a later slot; passing slot 255 ends the replay.
//
// 2DPB layout, 8 bytes:
//   BYTE0 = opcode
//   BYTE1 = command-specific bits + X/Y/W/H high bits
//           b7-b6  command-specific, for stroked primitives: stroke-1
//           b5-b4  X bits 9..8
//           b3     Y bit 8
//           b2-b1  W bits 9..8
//           b0     H bit 8
//   BYTE2 = X low byte
//   BYTE3 = Y low byte
//   BYTE4 = W low byte, X1 low for LINE/ERASE_LINE, or radius-X low for
//           ELLIPSE/FILL_ELLIPSE/ARC/PIE in center/radius mode
//   BYTE5 = H low byte, Y1 low for LINE/ERASE_LINE, or radius-Y low for
//           ELLIPSE/FILL_ELLIPSE/ARC/PIE in center/radius mode
//   BYTE6-BYTE7 = command-specific / reserved
//
// BUCKET_FILL uses BYTE1-BYTE3 like PLOT: seed X/Y high bits and lows.
// It fills the visible framebuffer-connected area matching the seed pixel with
// the current foreground color.
//
// ROUND_RECT and FILL_ROUND_RECT use BYTE6/BYTE7 as corner radii:
//   BYTE6 = radius X in raw pixels
//   BYTE7 = radius Y in framebuffer rows
// ROUND_RECT uses the normal stroke bits in BYTE1 and follows RECT fg/bg rules.
// FILL_ROUND_RECT ignores stroke and fills with the current foreground color.
//
// ELLIPSE/FILL_ELLIPSE/ARC/PIE coordinate semantics:
//   BYTE2/BYTE3 = center X/Y low bytes, with high bits in BYTE1
//   BYTE4/BYTE5 = radius X/Y low bytes, with high bits in BYTE1 W/H fields
//   The renderer internally expands this to a bounding box from
//   center-radius through center+radius.
//
// ARC and PIE use BYTE6/BYTE7 as 8-bit clockwise angle values:
//   0   = top / 12 o'clock
//   64  = right / 3 o'clock
//   128 = bottom / 6 o'clock
//   192 = left / 9 o'clock
// ARC draws the stroked ellipse segment clockwise from start to end.
// PIE draws the filled sector clockwise from start to end.
// start == end intentionally draws nothing.
#define NUM_2DPBS              256
#define NUM_2DPT_LAYERS        2
#define TWO_DFX_2DPT_LAYER_BACKGROUND 0u
#define TWO_DFX_2DPT_LAYER_FOREGROUND 1u
#define SIZE_2DPB              8

#define CMD_2D_NOP             0x60u
#define CMD_2D_SET_COLOR       0x61u
#define CMD_2D_PLOT            0x62u
#define CMD_2D_LINE            0x63u
#define CMD_2D_ERASE_LINE      0x64u
#define CMD_2D_BUCKET_FILL     0x65u
#define CMD_2D_RECT            0x66u
#define CMD_2D_FILL_RECT       0x67u
#define CMD_2D_ROUND_RECT      0x68u
#define CMD_2D_FILL_ROUND_RECT 0x69u
#define CMD_2D_ELLIPSE         0x6Au
#define CMD_2D_FILL_ELLIPSE    0x6Bu
#define CMD_2D_ARC             0x6Cu
#define CMD_2D_PIE             0x6Du

// SET_XY_OFFSET sets a signed persistent X/Y offset for following positional
// commands in this one playlist run. It is reset to 0,0 at the start of each
// background/foreground pass and is replaced, not accumulated, by later
// SET_XY_OFFSET commands. SKIP_NEXT skips only forward and therefore cannot loop.
#define CMD_2D_SET_XY_OFFSET        0x6Eu
#define CMD_2D_SKIP_NEXT       0x6Fu

// 2DPT resource-aware slot commands.
// DRAW_TEXT draws 1bpp text from a defined font bank.
// BLIT copies a defined 4bpp bitmap resource.
// PATTERN_FILL repeats a defined 16x8 4bpp pattern.
// TILEMAP draws from defined fixed 16x8 tiles and one-byte map cells.
#define CMD_2D_DRAW_TEXT       0x70u
#define CMD_2D_BLIT            0x71u
#define CMD_2D_PATTERN_FILL    0x72u
#define CMD_2D_TILEMAP         0x73u

#define CMD_2D_RET             0x7Fu

// Global/resource command range.  These are not stored in the 2DPT.
// DEFINE_FONT describes a raw 1bpp font bank.  DEFINE_BITMAP describes a
// 4bpp bitmap resource used by BLIT.  DEFINE_PATTERN, DEFINE_TILESET
// and DEFINE_TILEMAP describe pattern/tile resources.  All DEFINE_* resource
// addresses use 23-bit LSB-first byte order:
//   addr byte0 = bits 7..0
//   addr byte1 = bits 15..8
//   addr byte2 = bit7 ignored, bits6..0 = bits 22..16
// Font/string data is raw resource data and should be uploaded with 80h.
// NICK-format 4bpp pixel resources use 81h; TVC Graphics-16 resources use 82h.
// Both converted upload commands produce the same internal packed format.
#define CMD_DEFINE_FONT           0x5Bu
#define CMD_DEFINE_BITMAP         0x5Cu
#define CMD_DEFINE_PATTERN       0x5Du
#define CMD_DEFINE_TILESET        0x5Eu
#define CMD_DEFINE_TILEMAP        0x5Fu

// Raster interrupt control commands.
// SET_RASTER_INTx commands take two F9h data bytes, high byte first:
//   byte0 = raster line bits 15..8
//   byte1 = raster line bits 7..0
//
// Effective line value:
//   0      = disabled
//   1..311 = enable interrupt on that NICK scanline
//
// CLEAR_RASTER_INT clears all four pending raster interrupt latches.  It has
// no F9h data bytes.  This is intentionally simple: a machine-code ISR must
// finish before the next raster interrupt it cares about.
#define CMD_SET_RASTER_INT1       0x54u
#define CMD_SET_RASTER_INT2       0x55u
#define CMD_SET_RASTER_INT3       0x56u
#define CMD_SET_RASTER_INT4       0x57u
#define CMD_CLEAR_RASTER_INT      0x58u
#define CMD_CLEAR_INT             CMD_CLEAR_RASTER_INT

// Helpers for the common 2DPB high-bit byte.
#define TWO_DPB_STROKE_FROM_HI(hi)  (uint8_t)(((hi) >> 6) + 1u)
#define TWO_DPB_X_FROM_BYTES(hi, lo) (uint16_t)(((((uint16_t)(hi) >> 4) & 0x03u) << 8) | (uint8_t)(lo))
#define TWO_DPB_Y_FROM_BYTES(hi, lo) (uint16_t)(((((uint16_t)(hi) >> 3) & 0x01u) << 8) | (uint8_t)(lo))
#define TWO_DPB_W_FROM_BYTES(hi, lo) (uint16_t)(((((uint16_t)(hi) >> 1) & 0x03u) << 8) | (uint8_t)(lo))
#define TWO_DPB_H_FROM_BYTES(hi, lo) (uint16_t)(((((uint16_t)(hi) >> 0) & 0x01u) << 8) | (uint8_t)(lo))


// DRAW_TEXT 2DPB layout, opcode 70h:
//   BYTE1 = X/Y high bits using the common 2DPB high-bit layout
//           b5-b4 X bits 9..8
//           b3    Y bit 8
//           other bits ignored
//   BYTE2 = X low
//   BYTE3 = Y low
//   BYTE4 = font descriptor number, 0..63
//   BYTE5 = string resource address bits 7..0
//   BYTE6 = string resource address bits 15..8
//   BYTE7 = bit7 ignored, bits6..0 = string resource address bits 22..16
//
// Strings are raw resource bytes and should be uploaded with CMD_UPLOAD_RAW.
// String bytes 00h..7Fh select glyphs 0..127.  Byte 80h terminates the
// string.  Bytes 81h..FFh are reserved and currently also stop drawing safely.
//
// Fonts are defined by CMD_DEFINE_FONT in the global 50h-5Fh command space.
// Font data is 1bpp, 128 glyphs per font, stored in resource RAM:
//   bit 1 = foreground pixel
//   bit 0 = background/transparent pixel
// Each font has fixed character size in 8-pixel units.  In the DEFINE_FONT
// command byte, each 4-bit dimension field stores (units - 1): 0 = 8 pixels,
// 1 = 16 pixels, ... 15 = 128 pixels.
#define TWO_DFX_FONT_GLYPHS       128u
#define TWO_DFX_FONT_GLYPH_MASK   0x7Fu
#define TWO_DFX_TEXT_TERMINATOR   0x80u
#define TWO_DFX_TEXT_MAX_CHARS    255u

// BLIT 2DPB layout, opcode 71h:
//   BYTE1 bit7 = transparency-aware BLIT flag
//           0 = exact packed-byte copy (original maximum-speed behaviour)
//           1 = skip source pixels equal to the current transparent colour
//         bit6 = reserved
//         b5-b4 = destination X bits 9..8
//         b3    = destination Y bit 8
//         b2-b0 = reserved
//   BYTE2 = X low
//   BYTE3 = Y low
//   BYTE4 = bitmap descriptor number, 0..255
//   BYTE5-BYTE7 reserved
//
// Bitmap descriptors are defined by CMD_DEFINE_BITMAP in the global 50h-5Fh
// command space. Bitmap source data is 4bpp PIO-native packed data in resource
// RAM. Exact mode copies it byte-for-byte. Transparency-aware mode preserves
// the existing framebuffer pixel wherever the source pixel equals the current
// transparent colour index.
#define TWO_DFX_BLIT_TRANSPARENT_FLAG 0x80u
#define NUM_2D_FONTS                  64

// BLIT bitmap descriptors. A BLIT command references one descriptor by number.
// BLIT uses the byte-aligned packed fast path. Odd destination X values are
// aligned down internally (X &= FFFEh). Odd bitmap descriptor widths are
// normalized down by DEFINE_BITMAP. Source rows are tightly packed using the
// effective even width; no stride/sub-rectangle support.
#define NUM_2D_BITMAPS               256

// PATTERN_FILL 2DPB layout, opcode 72h:
//   BYTE1 = X/Y/W/H high bits using the common 2DPB high-bit layout
//   BYTE2 = X low
//   BYTE3 = Y low
//   BYTE4 = W low
//   BYTE5 = H low
//   BYTE6 = pattern definition number, 0..63
//   BYTE7 = pattern number within that table, 0..255
//
// Pattern tables are defined by CMD_DEFINE_PATTERN in the global
// 50h-5Fh command space.  Patterns are fixed 16x8 visible pixels,
// stored as 4bpp PIO-native packed bytes.  One pattern is 64 bytes:
//
//   16 pixels per row = 8 bytes
//   8 rows            = 64 bytes
//
// Pattern N starts at:
//   table_base + N * 64
//
// PATTERN_FILL copies pattern pixels exactly into the 2D framebuffer.  It does
// not skip transparent pixels; transparent-color handling remains the hardware
// /EXTC comparator's job.  Odd destination X and odd fill width are normalized
// down internally (X/W &= FFFEh) to keep the packed-byte fast path.
#define NUM_2D_PATTERNS  64
#define TWO_DFX_PATTERN_W      16
#define TWO_DFX_PATTERN_H      8
#define TWO_DFX_PATTERN_BYTES  64

// TILEMAP 2DPB layout, opcode 73h:
//   BYTE1 = packed tilemap id and destination high bits:
//           bit7..3 = tilemap descriptor number, 0..31
//           bit2..1 = destination X bits 9..8
//           bit0    = destination Y bit 8
//   BYTE2 = destination X bits 7..0
//   BYTE3 = destination Y bits 7..0
//   BYTE4 = source tile X inside the full map, 0..255
//   BYTE5 = source tile Y inside the full map, 0..255
//   BYTE6 = visible columns to draw, in tiles; 0 draws nothing
//   BYTE7 = visible rows to draw, in tiles; 0 draws nothing
//
// DEFINE_TILESET describes fixed 16x8 4bpp tile graphics.  One tile is
// 64 bytes in PIO-native packed format:
//   16 pixels per row = 8 bytes
//   8 rows            = 64 bytes
// Tile N starts at: tileset_base + N * 64.
//
// DEFINE_TILEMAP describes the full one-byte-per-cell map, its full width and
// height in cells, and the tileset descriptor used by that map.  Each one-byte
// DEFINE_TILEMAP dimension stores (cells - 1): 00h = 1 cell, ... FFh = 256 cells.
// TILEMAP then
// draws a viewport from source tile X/Y.  Destination X is aligned down to an
// even pixel internally (X &= FFFEh) so the fast packed-byte tile copier can
// stay byte-aligned.  Drawing clips safely to map bounds and screen bounds.
#define NUM_2D_TILESETS        64
#define NUM_2D_TILEMAPS        32
#define TWO_DFX_TILE_W         16
#define TWO_DFX_TILE_H         8
#define TWO_DFX_TILE_BYTES     64

// ============================================================
// PSRAM RESOURCE ADDRESS SPACE
// ============================================================
// Resource RAM is the complete 8 MiB PSRAM window, addressed with 23 bits.
#define TWO_DFX_RESOURCE_ADDR_BITS  23u
#define RESOURCE_RAM_SIZE           (8u * 1024u * 1024u)
#define TWO_DFX_RESOURCE_ADDR_MASK  (RESOURCE_RAM_SIZE - 1u)

// ============================================================
// PSRAM RESOURCE BACKEND
// ============================================================
// PSRAM lives in QMI memory window 1.  This baseline keeps PSRAM as the
// authoritative resource RAM and uses the full 8 MiB / 23-bit range.
// 2D resource descriptors use the same 23-bit LSB-first resource
// address byte order as uploads and SPBs. Font glyph banks are raw 1bpp
// resources; text strings are raw resource bytes terminated by 80h.
//
// The serial benchmark proved the cache-bypass/no-allocate alias reliable and
// much faster for writes than the cached alias, so the authoritative resource
// window remains in 0x15000000.  The accepted PSRAM optimizations are kept:
// direct wide reads for normal sprites, whole-source SRAM staging for
// transformed sprites, transient PATTERN_FILL cache, frame-local DRAW_TEXT font
// cache, and frame-local fixed 16x8 TILEMAP tile-graphics cache.
#define TWO_DFX_PSRAM_CACHED_BASE      0x11000000u
#define TWO_DFX_PSRAM_UNCACHED_BASE    0x15000000u
#define TWO_DFX_PSRAM_RESOURCE_OFFSET  0u
#define TWO_DFX_PSRAM_RESOURCE_SIZE    RESOURCE_RAM_SIZE
#define TWO_DFX_PSRAM_CS_PIN           19u

// Selective transformed-sprite source staging.  This is deliberately one reusable
// scratch buffer, not a persistent tagged cache.  It is large enough for
// the current 540x230 benchmark sprite source (~62.1 KiB) and far larger than
// the giant Wriggler source (~8 KiB), while leaving substantial SRAM headroom.
#define TWO_DFX_SPRITE_WHOLE_CACHE_BYTES (128u * 1024u)

// Transient PATTERN_FILL cache. A 16x8 4bpp pattern is 64 bytes.
// PATTERN_FILL repeatedly reuses those same bytes across a large destination,
// so staging one pattern into SRAM avoids many tiny PSRAM byte reads.
#define TWO_DFX_PATTERN_CACHE_BYTES TWO_DFX_PATTERN_BYTES

// Frame-local DRAW_TEXT font cache.  Each DEFINE_FONT bank has 128 glyphs
// of 1bpp data.  Common 2dfx font sizes fit comfortably in 8 KiB per bank:
//   8x8=1 KiB, 16x8=2 KiB, 32x8=4 KiB, 16x16=4 KiB, 32x16=8 KiB.
// Larger descriptors remain valid but fall back to direct PSRAM reads.
#define TWO_DFX_FONT_CACHE_SLOTS       8u
#define TWO_DFX_FONT_CACHE_SLOT_BYTES  (8u * 1024u)

// Frame-local TILEMAP tile cache.  Each fixed 16x8 4bpp tile is 64 bytes.
// The cache is frame-local and tagged by tileset descriptor/address plus tile
// number, so repeated cells avoid rereading the same tiny tile from PSRAM.
#define TWO_DFX_TILE_CACHE_SLOTS       64u
#define TWO_DFX_TILE_CACHE_SLOT_BYTES  TWO_DFX_TILE_BYTES

// ============================================================
// SHARED RUNTIME SYMBOLS PROVIDED BY main.c
// ============================================================
extern uint8_t *resource_ram;
extern uint8_t shadow_spt[NUM_SPBS][SPB_SIZE];

bool sprite_engine_collision_was_detected_last_frame(void);

/* Internal egg-manager hook: wake only the logical renderer gate so a
 * deferred start callback can run at the next normal safe scene update. */
void twodfx_internal_wake_renderer_for_egg_start(void);

/* Internal egg-manager hook: restore the complete public graphics state to
 * clean power-on-style defaults immediately before an egg start callback.
 * Resource RAM is deliberately retained; descriptors and every visible scene
 * producer are reset, so stale RRAM bytes cannot be referenced accidentally. */
void twodfx_internal_prepare_clean_egg_start(void);

// Private engine backends used by the typed C API and the Z80 command parser.
// Egg/demo/application code must use twodfx_api.h instead of these raw helpers.
void twodpt_clear_shadow_to_ret(void);
void twodpt_set_shadow_2dpb_layer(bool foreground, uint8_t slot, const uint8_t dpb[SIZE_2DPB]);
void twodpt_define_font_shadow(uint8_t font_id, uint32_t resource_addr,
                               uint8_t width_units, uint8_t height_units);
void twodpt_define_bitmap_shadow(uint8_t bitmap_id, uint32_t resource_addr,
                                 uint16_t width, uint16_t height);
void twodpt_define_pattern_shadow(uint8_t pattern_id, uint32_t resource_addr);
void twodpt_define_tileset_shadow(uint8_t tileset_id, uint32_t resource_addr);
void twodpt_define_tilemap_shadow(uint8_t map_id, uint32_t resource_addr,
                                  uint16_t width_cells, uint16_t height_cells,
                                  uint8_t tileset_id);



}
#endif
