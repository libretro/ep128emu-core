# Built-in 2dfx v1.1.0 firmware programs

The `egg_*.c` files, their corresponding headers, the public `twodfx_api.h`, firmware-page source and original artwork are copied from the hardware-validated v1.1.0 source with original-byte provenance recorded by `ORIGIN_SHA256.json`.

This directory is compiled into the portable C99 device. It uses `src/core/twodfx_public_api_adapter.inc` for public commands (sprites, background/foreground 2DPT, resource uploads and meshes); the emulator uses the same retained state and renderer for guest F8h/F9h commands.

Portable replacements under `portable/` are **not** the MCU SDK. Hardware TRNG, wall-clock diagnostic time and Core0 worker scheduling are represented by portable logical substitutions. MCU-private renderer source, PIO/DMA, scanout and hardware fast paths are not included.

Caution: original egg C modules retain **file-static global state** and cannot be assumed independent when multiple emulated 2dfx devices run active eggs concurrently. Snapshot v13 also does not serialize all of their private animation variables.

**Licensing:** Original author-owned egg C files, headers, SHOW_FW, public API and owned artwork are available under MIT; see [`LICENSE_SCOPE.md`](LICENSE_SCOPE.md) for the explicit copyright grant and exceptions, the root `LICENSE` for the full MIT terms, and root `ASSET_LICENSES.md` for third-party game-derived assets that are **outside MIT**.
