**English** · [Português (Brasil)](pt-BR/reverse-engineering.md)

# Reverse engineering: how CrossOver runs games

> **Last reviewed:** 2026-10-12 · **Target:** CrossOver 26.3.0 for macOS (July 2026 build)

winhub-engine was designed by studying how CrossOver 26.3 runs Windows games on the Mac, **by its behaviour
only**, and then building the same architecture from open sources. This page records what was studied, what was
learned, and the rules that kept the project clean.

## Rules

- We studied **what CrossOver does**: how its bundle is laid out, which files and environment variables it uses,
  how it decides things at run time.
- We **never** copied CrossOver code, binaries, texts, translations, icons or data. In particular, the closed
  `cxcompatdb.so` is not used, CrossOver's compatibility database (`compatdb-*.dat`, encrypted) was never
  decrypted, and its application catalog is not redistributed.
- Code CodeWeavers publishes under the LGPL, the Wine tree of CrossOver, **is** used, with attribution, and each
  release ships the corresponding sources.
- This documentation describes behaviour. Decompiled code, addresses, internal symbol names and analysis logs
  stay out of the repository.

## Method

1. **Bundle inspection.** The layout of `CrossOver.app/Contents/SharedSupport/CrossOver`: which libraries,
   backends and data files ship and where; the bottle templates and launcher scripts (Perl), which are readable
   text.
2. **Static analysis** of the engine's native modules with [REA](https://github.com/morluto/rea) (Hopper
   provider), to learn the order in which decisions are made: when the per-game module is loaded, which variables
   it reads, which conditions make a backend usable, which folders it adds to Wine's search path. Nothing was
   executed during this step, and no code was carried over.
3. **Confirmation in the published sources.** Every behaviour that touches Wine was checked against
   `crossover-sources-26.3.0.tar.gz` from [codeweavers.com/crossover/source](https://www.codeweavers.com/crossover/source):
   the hook that loads the per-game module, `prepend_dll_path`, msync, the `winemac.drv` Metal hooks.
4. **Re-implementation and measurement.** The engine was built from open code and tested against the same
   cases; anything learned while doing so is listed at the end.

## What the CrossOver engine is made of

| Part | Where in the bundle | What it is |
|---|---|---|
| Wine 11.0 with CodeWeavers' patches | `lib/wine/{x86_64-unix,x86_64-windows,i386-windows}` | x86_64 only, with WoW64 (no 32-bit macOS libraries); runs through Rosetta 2 |
| `wineloader`, `wineserver` | `bin/` | x86_64 Mach-O binaries |
| The `wine` launcher | `bin/wine` (Perl) | builds the environment from the bottle's `cxbottle.conf`; sets `ROSETTA_ADVERTISE_AVX=1`, `DOTNET_EnableWriteXorExecute=0`, the GStreamer plug-in path and registry, and the D3DMetal shared library path. **It does not choose the graphics backend.** |
| `cxcompatdb.so` | `lib/wine/x86_64-unix/` | closed module: per-game adjustments and the choice of the graphics backend |
| DXMT v0.72 (+25 commits) | `lib/dxmt/` | `d3d11`, `dxgi`, `d3d10core`, `winemetal`, and on 64-bit `nvapi64`, `nvngx` |
| DXVK 1.10.3 + MoltenVK 1.2.10 | `lib/dxvk/`, `lib64/libMoltenVK.dylib` | `d3d9`, `d3d10`, `d3d10_1`, `d3d10core`, `d3d11`; no Vulkan loader |
| D3DMetal 3.0 (Apple) | `lib64/apple_gptk/` | `d3d11`, `d3d12`, `dxgi`, `nvapi64`, `nvngx`, `atidxx64`, 64-bit only |
| Unix libraries | `lib64/` | GStreamer (game videos), SDL2 (controllers), freetype, gnutls, glib |

## How CrossOver picks the graphics backend

- CodeWeavers' `ntdll` loads `cxcompatdb.so` at the end of process start-up, in every Wine process (visible in
  the LGPL `dlls/ntdll/unix/loader.c`, marked "CW Hack 24067").
- The module reads a signed, encrypted database of per-game rules from the user's CrossOver folder, falling back
  to a copy built into it when the downloaded one is missing or older. Only rules marked for macOS apply.
- The user's choice comes from the bottle, as the variable `CX_GRAPHICS_BACKEND`: `wined3d`, `dxmt`, `dxvk`,
  `d3dmetal`, or empty for automatic.
- A rule's backend applies when the user left the choice on automatic; when the user picked a backend, a rule
  only wins if it is marked as forced. Automatic with no rule means D3DMetal.
- Before using a backend the module checks it can run: DXMT needs macOS 14 or later and a process translated by
  Rosetta; D3DMetal needs the same and a 64-bit process. If the chosen one cannot run, it falls back to the
  user's choice, and then to wined3d.
- The chosen backend's folder is put **first** in Wine's builtin DLL search path through `prepend_dll_path`
  (exported by the LGPL `ntdll`), so its `d3d11.dll` is found before Wine's own. **Nothing is copied into the
  bottle.** The result is published as `CX_ACTIVE_GRAPHICS_BACKEND`, which CodeWeavers' `win32u` reads (to pick
  a Vulkan library for wined3d and to report hardware GPU scheduling for D3DMetal).
- Per-game adjustments can also set environment variables, add DLL overrides, add to the command line or replace
  the executable path of processes started by other Wine processes, and match programs by the end of their path
  or by their Steam app ID. With D3DMetal, a fixed adjustment sends `system32\nvngx.dll` (DLSS) to D3DMetal's own
  copy.
- Bottle templates clean up older bottles: they remove DXVK DLLs that CrossOver used to copy into prefixes
  ("copied until CrossOver 24") and convert old settings (`WINEDXVK`, `WINED3DMETAL`, `WINEESYNC`) to the
  current ones (`CX_GRAPHICS_BACKEND`, `WINEMSYNC`).

**How winhub-engine reproduces it:** [`winhubcompat`](compatdb.md), loaded at the same point by
[`patches/wine/0001`](patches.md#wine), makes the same decisions in the same order, with an open JSON database
and two intentional differences: automatic picks DXMT when the user has no D3DMetal, and rules can also live in
the bottle. WinHub's migration does what the bottle templates do.

## msync

The `wineserver` and `ntdll` of CrossOver implement msync on Mach ports as a backend of Wine 10/11's in-process
synchronization layer; server and processes must agree on `WINEMSYNC`. This is in the LGPL sources and the engine
uses it unchanged. See [msync](msync.md).

## Other findings in the published sources

- `start_main_thread` replaces `localtime` with its own version: Wine keeps the PEB in a pthread TLS slot that
  macOS's `localtime()` also uses.
- CodeWeavers' tree reports every user as `crossover` (unless `CX_REPORT_REAL_USERNAME` is set) and makes the
  Desktop a folder with a link to the Mac desktop (unless `CX_DIRECT_DESKTOP` is set). The engine restores
  upstream Wine's behaviour ([`patches/wine/0003`](patches.md#wine)).
- Other CrossOver switches in the tree: `CX_HACK_FILESYSTEM_TYPE`, `CX_HACK_REMOTE_DRIVES`,
  `CX_HIDE_BUILTIN_VERSION_RESOURCES`, `CX_FWD_COMPAT_GL_CTX`, `CX_TURN_OFF_FONT_REPLACEMENTS`,
  `WINE_SIMULATE_WRITECOPY`.
- The source archive contains `wine`, `dxvk`, `moltenvk`, `vkd3d`, `gstreamer`, `gnutls`, `freetype`, `glib`,
  `cabextract` and others, but neither DXMT nor `cxcompatdb`.

## What building the engine confirmed

| Finding | Consequence for the engine |
|---|---|
| CodeWeavers' `win32u` loads Vulkan by soname without asking the loader for portability drivers | Vulkan goes straight to `libMoltenVK.dylib` |
| CodeWeavers' MoltenVK claims features Metal lacks (geometry shaders, cull distance, pipeline statistics, robust buffer access) "required by DXVK", and their DXVK requires them | `patches/dxvk/0002`: DXVK asks only for what the official MoltenVK has |
| Wine ignores DLLs in the builtin path that lack the builtin marker; all of CrossOver's backend DLLs carry it | DXMT built as builtin, DXVK marked with `winebuild --builtin` |
| A builtin Wine does not know (`winemetal.dll`) is only found through its file in `system32`; CrossOver also ships it in `lib/wine` | the engine ships it in `lib/wine` too; WinHub restores it in migrated bottles |
| In WoW64 processes without `VK_EXT_map_memory_placed`, `win32u` backs buffers with host memory, which both the official and CodeWeavers' MoltenVK refuse | DXVK is limited to 64-bit processes; 32-bit ones fall back to DXMT. It is unclear whether CrossOver's DXVK works in 32-bit at all; its automatic mode sends 32-bit processes to wined3d |
| The `crossover` user name breaks the profile of bottles made with the official Wine (Steam's saved login, game saves) | `patches/wine/0003` |

How close the result is, dimension by dimension: [Parity with CrossOver](crossover-parity.md).
