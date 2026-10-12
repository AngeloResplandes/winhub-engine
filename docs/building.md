**English** · [Português (Brasil)](pt-BR/building.md)

# Building

## Requirements

- A Mac with Apple silicon (with Rosetta 2) or Intel. Everything is built as x86_64.
- Xcode 16 or later. From Xcode 26 on, the Metal compiler is a separate component:
  `xcodebuild -downloadComponent MetalToolchain` (about 840 MB, once).
- Homebrew packages:

```bash
brew install mingw-w64 bison meson ninja freetype gnutls sdl2 glslang cmake
```

Homebrew only supplies tools and **headers**. The Wine build links against the x86_64 libraries of the
official WineHQ build, which `build-wine.sh` downloads and checks.

## Steps

Run them in this order from the repository root. Intermediate files go to `work/` (ignored by Git); set
`WORK=/other/place` to move it and `JOBS=n` to limit parallel jobs.

| Script | What it does | Time (M-series Mac) |
|---|---|---|
| `./scripts/test-compatdb.sh` | Unit tests of the rule engine (native, no Wine) | seconds |
| `./scripts/build-llvm.sh` | LLVM 15 x86_64 libraries for DXMT's shader converter (CI caches it) | long |
| `./scripts/build-wine.sh` | All of Wine from CrossOver's LGPL sources with `patches/wine` | ~40–60 min |
| `./scripts/build-winhubcompat.sh` | `winhubcompat.so` against the Wine build tree | seconds |
| `./scripts/build-dxmt.sh` | DXMT v0.80 as Wine builtin DLLs, with `patches/dxmt` | minutes |
| `./scripts/build-dxvk.sh` | DXVK 1.10.3 from CrossOver's sources, with `patches/dxvk`, marked builtin | minutes |
| `./scripts/build-cabextract.sh` | Universal `cabextract` for winetricks | seconds |
| `./scripts/package.sh` | `work/dist/`: overlay, sources, `SHA256SUMS`, `winhub-manifests.json` | ~1 min |
| `./scripts/smoke-test.sh` | Installs like WinHub and tests D3D11 through DXMT and DXVK | minutes |

## What `build-wine.sh` does, and why

1. Fetches `crossover-sources-<version>.tar.gz` and unpacks only `sources/wine`; applies `patches/wine`.
2. Unpacks the official build once (`base_wine` in `scripts/common.sh`, reached through the space-free link
   `work/base-wine`).
3. Copies freetype, gnutls, SDL2 and MoltenVK into `work/deps/lib` with **install names that carry the
   versioned file name**, and links the rest of the base libraries next to them. Wine's `configure` takes the
   name it will `dlopen` at run time (its "soname") from the install name of the library it links against;
   the base build's libraries have bare install names (`libSDL2.dylib`), which would give wrong sonames.
4. Leaves `libvulkan` out on purpose, so `configure` picks `libMoltenVK.dylib` for Vulkan.
5. Configures with `--enable-archs=i386,x86_64` (WoW64) and rpaths that reach the base build's `lib/`, checks
   that the sonames came out right, builds and installs into `work/wine-install`.

## Pitfalls already solved

| Symptom | Cause | Fix in the scripts |
|---|---|---|
| `C compiler cannot create executables` | the base build path has a space (`Wine Devel.app`) | `work/base-wine` link |
| A header path silently empty | `set -u` with an unset optional argument | `${2:-}` in `inc()` |
| `libSDL2 … not found` or odd sonames | bare install names in the base build | step 3 above |
| `sfnt2fon` aborts loading `libpng16` | copied dylibs look for dependencies next to themselves | link every base dylib into `work/deps/lib` |
| DXVK finds no Vulkan device | the Vulkan loader and CodeWeavers' `win32u` disagree about portability drivers | Vulkan straight to MoltenVK (step 4) |
| cJSON fails with `-Werror` | `sprintf` deprecation warnings | cJSON compiled on its own, without `-Werror` |
| DXVK 1.10.3 does not compile | newer GCC/mingw-w64 headers | `-include cstdint` and `patches/dxvk/0001` |
| DXVK version reads `wine-…-r1` | `git describe` climbs into this repository | `GIT_CEILING_DIRECTORIES` |
| DXMT shader fails on Xcode 27 | Metal toolchain changed an internal builtin | `patches/dxmt/0002` |
| Overlay of 290 MB | Windows-side files built with debug info | `strip --strip-unneeded`, no import libraries |

## Continuous integration

`.github/workflows/build.yml` runs on pushes to `main`, on pull requests and on `wine-*` tags (changes to
documentation only are skipped): unit tests, signature check of the rule database, LLVM (cached), Wine,
winhubcompat, DXMT, DXVK, cabextract, package and smoke test. A tag also publishes the release, see
[Releasing](releasing.md).
