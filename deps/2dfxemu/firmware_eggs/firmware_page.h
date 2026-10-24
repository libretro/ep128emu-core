#ifndef FIRMWARE_PAGE_H
#define FIRMWARE_PAGE_H

/*
 * Builds the terminal SHOW_FW shadow scene through the supported twodfx_* C
 * API.  The implementation selects a TVC or Enterprise-safe layout and colour
 * set from the boot-time hardware profile.
 */
void twodfx_firmware_page_build_shadow_scene(const char *version_string);

#endif
