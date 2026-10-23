# License scope — original 2dfx firmware Easter eggs

**SPDX-License-Identifier: MIT** (for the author-owned portions described here)

Copyright (c) 2026 Károly Vaczkó

Károly Vaczkó, as copyright holder and original 2dfx creator, authorizes
redistribution, modification, and incorporation of his original 2dfx v1.1.0
Easter egg C source, corresponding headers, original project artwork, the
SHOW_FW source, and the public `twodfx_api.h` interface under the **MIT
License**, whose complete text is provided in the package-root `LICENSE`.
This includes his original C logic in `egg_*.c`, including the D3/D4/D5
showcase logic, and the independently written portable API implementation.

This directory deliberately preserves the original firmware C files
**byte-for-byte**, without prepending license comments to each file. The
original-file digests in `ORIGIN_SHA256.json` remain valid. This scope notice
provides the copyright and licensing statement for those unmodified files;
per-file SPDX comments are not required for a complete distribution carrying
the copyright notice and the full MIT license text.

## Exception — third-party-derived display assets

The MIT grant applies **only to material that Károly Vaczkó owns or may
legitimately sublicense**. It does not grant rights to third-party material
embedded for compatibility with the physical 2dfx showcase:

- `egg_D3_elite_showcase_data.h`: Elite-derived ship geometry, including
  Cobra Mk III, Asp Mk II, Sidewinder, Viper and Krait, as well as use by D4/D5
- `egg_C9_wriggler_assets.h`: Wriggler-derived sprite imagery

The files remain included for fidelity to the hardware demonstrations but
are **not MIT-licensed in their third-party-derived portions**. Their
copyright owners may have separate rights. The use of such data in an
MIT-licensed egg *program* does not transfer copyright ownership or license
that separate asset data. See `ASSET_LICENSES.md` at the package root for
provenance and redistribution cautions.

## Not part of this distribution or grant

The original private RP2354B firmware implementation, including MCU PIO/DMA,
scanout, renderer fast paths, multicore scheduling and private `twodfx_api.c`,
is **not** included and is **not relicensed**. The portable implementation of
the public API is independently authored and MIT-licensed.

When redistributing the package or substantial portions of its author-owned
code, retain the MIT copyright and permission notice and these third-party
asset distinctions. If redistributing one file separately, accompany it with
the applicable license/copyright notice so the terms are not lost.
