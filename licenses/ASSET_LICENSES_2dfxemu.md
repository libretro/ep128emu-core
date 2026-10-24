# Rights and provenance of embedded assets

The portable emulator and original 2dfx content to which Károly Vaczkó owns the rights are offered under the MIT License in `LICENSE`. His authorization includes original 2dfx v1.1.0 Easter egg C logic, original SHOW_FW, the public `twodfx_api.h` interface and supporting own assets. The explicit, file-scope author grant for byte-exact egg sources is in `src/firmware_eggs/LICENSE_SCOPE.md`. The original private RP2354B MCU execution, fast-path rendering, PIO/DMA, multicore workers and proprietary implementation code are not included or licensed by this package.

## Third-party-derived showcase material — **excluded from MIT**

| Program | Embedded content | Attribution / rights note |
|---|---|---|
| C9h Wriggler | Screenshot-derived animation graphics, `src/firmware_eggs/egg_C9_wriggler_assets.h` | *Wriggler* (ZX Spectrum, 1985), credited to Devonshire House / Romantic Robot |
| D3h/D4h/D5h Elite | Original firmware demonstration ship geometry, including Cobra, Asp, Sidewinder, Viper and Krait, `src/firmware_eggs/egg_D3_elite_showcase_data.h` | *Elite* (1984), Ian Bell and David Braben, BBC Micro release by Acornsoft |

This content is bundled to reproduce the original hardware showcase. Ownership/redistribution rights may belong to third-party original authors or publishers. **No MIT license, redistribution permission or endorsement from those third parties is claimed or conveyed.** Separately evaluate any permissions necessary before public redistribution. Rights holders can request modification or removal; the showcases do not form part of the public expansion-device command protocol.

Additional references: [Elite source documentation](https://elite.bbcelite.com/about_site/acknowledgements.html), [Wriggler at World of Spectrum](https://worldofspectrum.net/item/0005761/).

The ZX1 compressed stream *format* is attributed to Einar Saukas. The portable decoder is independently implemented, and no rights to third-party ZX1 implementation source are implied. ep128emu VM source remains subject to its own license outside this library distribution.
