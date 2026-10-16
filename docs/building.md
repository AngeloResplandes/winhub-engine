**English** · [Português (Brasil)](pt-BR/building.md)

# Building

## Requirements

- A Mac with Apple silicon (with Rosetta 2) or Intel. Everything is built as x86_64.
- Xcode 16 or later. From Xcode 26 on, the Metal compiler is a separate component:
  `xcodebuild -downloadComponent MetalToolchain` (about 840 MB, once).
- Homebrew packages:

```bash
brew install mingw-w64 bison meson ninja freetype gnutls sdl2 glslang cmake pkgconf gettext
```

- `/usr/bin/python3` with `distutils`: Apple's Python 3.9 from Xcode or the Command Line Tools has it. GLib 2.78's
  code generators need it, and Homebrew's Python (3.12 and later) does not have it. The Xcode on GitHub's
  `macos-15` runners is enough; nothing to install there.

Homebrew only supplies tools and **headers** (`gettext` only for `libintl.h`). The Wine build links against the
x86_64 libraries of the official WineHQ build, which `build-wine.sh` downloads and checks, and GStreamer is built
by `build-gstreamer.sh`.

## Steps

Run them in this order from the repository root. Intermediate files go to `work/` (ignored by Git); set
`WORK=/other/place` to move it and `JOBS=n` to limit parallel jobs.

| Script | What it does | Time (M-series Mac) |
|---|---|---|
| `./scripts/test-compatdb.sh` | Unit tests of the rule engine (native, no Wine) | seconds |
| `./scripts/build-llvm.sh` | LLVM 15 x86_64 libraries for DXMT's shader converter (CI caches it) | long |
| `./scripts/build-gstreamer.sh` | GLib and GStreamer (core, base, good, bad with VideoToolbox) as x86_64 for game videos; **before** `build-wine.sh` (CI caches it) | ~3 min |
| `./scripts/build-wine.sh` | All of Wine from CrossOver's LGPL sources with `patches/wine`, including `winegstreamer` | ~40–60 min |
| `./scripts/build-winhubcompat.sh` | `winhubcompat.so` against the Wine build tree | seconds |
| `./scripts/build-dxmt.sh` | DXMT v0.80 as Wine builtin DLLs, with `patches/dxmt` | minutes |
| `./scripts/build-dxvk.sh` | DXVK 1.10.3 from CrossOver's sources, with `patches/dxvk`, marked builtin | minutes |
| `./scripts/build-cabextract.sh` | Universal `cabextract` for winetricks | seconds |
| `./scripts/package.sh` | `work/dist/`: overlay, sources, `SHA256SUMS`, `winhub-manifests.json` | ~2 min |
| `./scripts/smoke-test.sh` | Installs like WinHub; tests D3D11 through DXMT and DXVK and decodes an H.264/AAC video | minutes |

## What `build-gstreamer.sh` does, and why

Wine plays game videos through Media Foundation → `winegstreamer` → GStreamer. The official build has no
GStreamer, so the engine builds and ships it. H.264 and AAC are decoded by macOS itself (`vtdec`, `atdec` in
gst-plugins-bad's `applemedia`); there is no FFmpeg, x264 or GPL plug-in (`-Dgpl=disabled`).

1. Fetches the CrossOver sources (`sources/glib`, `sources/gstreamer`) and pcre2 10.42 with its Meson wrap, both
   pinned by SHA-256, and puts the pcre2 files in GLib's `packagecache` (Meson runs with `--wrap-mode
   nodownload`, so it never downloads anything).
2. Fetches gvdb at the commit pinned in `versions.env` into `work/gvdb` and copies it into GLib's subprojects.
3. Prepares `work/gst-deps`: libintl copied from the base build with an `@rpath` install name, and a `libffi.pc`
   for the SDK's libffi.
4. Writes `work/gst-cross.ini`, a Meson cross file for x86_64, with deployment target 10.15.
5. Builds GLib, then GStreamer 1.24.4 core, plugins-base, plugins-good and plugins-bad, all with
   `-Dauto_features=disabled` and only the options winegstreamer's decodebin needs, into
   `work/gstreamer-install`. After each install it gives every library an `@rpath/<file>` install name.
6. Checks the result: 14 plug-ins in `lib/gstreamer-1.0` (x86_64, with a `@loader_path/..` rpath),
   `vtdec` and `atdec` symbols in `applemedia`, and no dependency or rpath on the build folder or Homebrew.

## What `build-wine.sh` does, and why

1. Fetches `crossover-sources-<version>.tar.gz` and unpacks only `sources/wine`; applies `patches/wine`.
2. Unpacks the official build once (`base_wine` in `scripts/common.sh`, reached through the space-free link
   `work/base-wine`).
3. Copies freetype, gnutls, SDL2 and MoltenVK into `work/deps/lib` with **install names that carry the
   versioned file name**, and links the rest of the base libraries next to them. Wine's `configure` takes the
   name it will `dlopen` at run time (its "soname") from the install name of the library it links against;
   the base build's libraries have bare install names (`libSDL2.dylib`), which would give wrong sonames.
4. Leaves `libvulkan` out on purpose, so `configure` picks `libMoltenVK.dylib` for Vulkan.
5. Configures with `--enable-archs=i386,x86_64` (WoW64) and rpaths that reach the base build's `lib/`, and
   `--with-gstreamer` against `work/gstreamer-install` (so it stops if `build-gstreamer.sh` was not run). Checks
   that the sonames came out right and that `winegstreamer.so` links GStreamer through `@rpath` only, then builds
   and installs into `work/wine-install`.

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
| GLib: `Subproject exists but has no meson.build file` (gvdb) | the CrossOver archive ships `glib/subprojects/gvdb` empty (it is a git subproject) | fetch gvdb at the commit pinned in `versions.env`, check `HEAD`, copy it in before `meson setup` |
| GLib and GStreamer come out arm64 | Homebrew's Meson and Python run as arm64 | cross file (`work/gst-cross.ini`) for x86_64; Rosetta runs the configure checks |
| `Run-time dependency intl found: NO` | Meson's `intl` check compiles with the C++ compiler, which `c_args` do not reach | the include and library flags are `[constants]` in the cross file, used for C, C++, Objective-C and Objective-C++ |
| libffi not found | the base build has none | the SDK's libffi through `work/gst-deps/pkgconfig/libffi.pc` |
| A library asks for a bare `libintl.dylib` | the base build's libintl has the install name `libintl.dylib`, which dyld cannot resolve | link against a copy with id `@rpath/libintl.8.dylib` in `work/gst-deps/lib`; at run time `@loader_path` finds the base build's own file in `lib/` |
| `gdbus-codegen`: `No module named 'distutils'` | Meson runs Homebrew's Python, which dropped `distutils` (3.12) | `python = '/usr/bin/python3'` in the cross file; the installed GLib scripts get it too |
| `'pipe2' is only available on macOS 27.0 or newer` | Meson's `has_function` only checks that the symbol links, so GLib would call `pipe2` on macOS 10.15 to 26 | `has_function_pipe2 = false` in the cross file; `-Werror=unguarded-availability-new` turns any other unguarded newer API into a build error |
| Libraries with absolute install names and rpaths | `meson install` writes `work/…` paths | `rpath_names` after each install: `@rpath/<file>` ids and references, build rpaths deleted, `@loader_path/..` added to the plug-ins |
| `Program 'glib-mkenums' not found` (GStreamer core) | Meson looks for it through a build-machine GLib that does not exist | `glib-mkenums = ['/usr/bin/python3', '<prefix>/bin/glib-mkenums']` in the cross file |
| GStreamer core: bison too old | `gst/parse` needs bison 2.4 or later, macOS has 2.3 | `-Dgst_parse=false`; winegstreamer never calls the parser |
| `Unknown option introspection` / `tools` in plugins-good | gst-plugins-good 1.24 does not have them | pass them only to base and bad |
| applemedia does not compile | its GL sources need gstreamer-gl, and `auto_features=disabled` turns bad's `gl` off | `-Dgl=enabled` in bad (and base with the CGL/Cocoa options) |
| `External plugin loader failed … GST_PLUGIN_SCANNER` | no `gst-plugin-scanner` is built (`-Dtools=disabled`) | none needed: winegstreamer starts GStreamer with `--gst-disable-registry-fork`, so it scans in-process and prints nothing |
| `winegstreamer.so` carries an rpath to `work/gstreamer-install` | `gstreamer-1.0.pc` has `-Wl,-rpath,${libdir}` | `build-wine.sh` strips it from `GSTREAMER_LIBS` and checks the result |
| `winegstreamer.so` asks for a bare `libintl.dylib` | `-lintl` from `glib-2.0.pc` finds the base build's copy in `work/deps/lib` | `-L$WORK/gst-deps/lib` added to `GSTREAMER_LIBS`; `build-wine.sh` rejects any dependency that is not `@rpath/`, `/usr/lib/` or `/System/Library/` |
| `configure` cannot use GStreamer: `libffi` | `gobject-2.0.pc` requires `libffi.pc` | `PKG_CONFIG_LIBDIR` also holds `work/gst-deps/pkgconfig` |
| `strip` invalidates the signatures of the GStreamer libraries | `build-gstreamer.sh` signs them ad hoc | `package.sh`: remove the signature, `strip -x`, sign again |
| `vtdec` seems missing from the smoke log | `WINE:4` logs long names ("Apple VideoToolbox decoder (hardware only)") | `GST_ELEMENT_FACTORY:4` as well, which logs `creating element "vtdec_hw"` |

## Continuous integration

`.github/workflows/build.yml` runs on pushes to `main`, on pull requests and on `wine-*` tags (changes to
documentation only are skipped): unit tests, signature check of the rule database, LLVM (cached), GLib and
GStreamer (cached), Wine, winhubcompat, DXMT, DXVK, cabextract, package and smoke test. The GStreamer cache holds
everything the later steps read (`gstreamer-install`, `gst-src`, `gst-deps`, `gvdb`, the pcre2 downloads) and its
key is `CROSSOVER_VERSION`, `PCRE2_SHA256`, `PCRE2_WRAP_SHA256`, `WINE_BASE_SHA256`, `GVDB_COMMIT` and a hash of `build-gstreamer.sh`. A tag also publishes
the release, see [Releasing](releasing.md).
