**English** · [Português (Brasil)](README.pt-BR.md)

<h1 align="center">winhub-engine</h1>

<p align="center">
  <b>The Wine engine behind <a href="https://github.com/AngeloResplandes/winhub">WinHub</a>: Wine 11 from CrossOver's LGPL
  sources, DXMT and DXVK, and an open module that picks each process's Direct3D.</b>
</p>

<p align="center">
  <a href="https://angeloresplandes.github.io/winhub-engine/">Documentation</a> ·
  <a href="ROADMAP.md">Roadmap</a> ·
  <a href="docs/reverse-engineering.md">How CrossOver was studied</a> ·
  <a href="docs/crossover-parity.md">Parity with CrossOver</a> ·
  <a href="https://github.com/AngeloResplandes/winhub-engine/releases">Releases</a>
</p>

winhub-engine rebuilds, from open sources, the architecture CrossOver 26.3 uses to run Windows games on the
Mac. It was designed by studying CrossOver **by its behaviour** (how its bundle is laid out and how it decides
which Direct3D a game gets), then built from the Wine tree CodeWeavers publishes under the LGPL, DXMT, DXVK and
our own `winhubcompat`. No closed CrossOver code or data is used.

This repository holds no binaries. CI builds everything from pinned sources and publishes the result as GitHub
releases, which WinHub downloads and checks.

## What you get

| | |
|---|---|
| **Wine 11.0 from CrossOver's sources** | msync, WoW64 (32-bit programs inside 64-bit processes, no 32-bit macOS libraries) and the `winemac.drv` hooks DXMT and D3DMetal need |
| **Direct3D 10/11 over Metal** | [DXMT](https://github.com/3Shain/dxmt) v0.80 in 64- and 32-bit processes |
| **Direct3D 9/10/11 over Vulkan** | DXVK 1.10.3 from CrossOver's sources on the official MoltenVK, in 64-bit processes |
| **Direct3D 11/12 with D3DMetal** | when the user has Apple's Game Porting Toolkit; never shipped by the project |
| **Per-process backend choice** | `winhubcompat` puts the chosen backend first in Wine's DLL path: nothing is copied into bottles |
| **Open compatibility rules** | per game, per bottle or from a signed database: backend, environment, DLL overrides, arguments |

## How it works

```
WinHub ── WINHUB_GRAPHICS_BACKEND=auto|wined3d|dxmt|dxvk|d3dmetal ──► wine
                                                                         │
ntdll.so (start_main_thread) ── dlopen ──► winhubcompat.so ◄── rules (built-in, downloaded, per bottle)
                                              │
                    picks the backend that can run here (macOS 14+, Rosetta, 64-bit…)
                                              │
                    prepend_dll_path(lib/dxmt | lib/dxvk | the user's D3DMetal)
                                              │
               the backend's d3d11.dll is found before Wine's own; the bottle stays untouched
```

Details: [Architecture](docs/architecture.md) and [winhubcompat](docs/compatdb.md).

## Status

| | |
|---|---|
| Engine revision | 5, release [`wine-11.0-r5`](https://github.com/AngeloResplandes/winhub-engine/releases/tag/wine-11.0-r5), WinHub's default engine |
| Parity with CrossOver's engine | **~73%** (estimated, [details](docs/crossover-parity.md)) |
| Tested | DXMT 64/32-bit and DXVK 64-bit at D3D feature level 11_0 (smoke test); H.264/AAC MP4 video through Media Foundation (smoke test); Hollow Knight through DXMT with Steam (2026-10-12) |
| Biggest gaps | D3D12 without D3DMetal, small rule database (2 rules), videos in Direct3D 11 Unity games ([all gaps](docs/gaps.md)) |

## Build

macOS on Apple silicon (with Rosetta 2) or Intel, Xcode 16+ (from Xcode 26, also
`xcodebuild -downloadComponent MetalToolchain`), and:

```bash
brew install mingw-w64 bison meson ninja freetype gnutls sdl2 glslang cmake pkgconf gettext
```

```bash
./scripts/build-llvm.sh && ./scripts/build-gstreamer.sh && ./scripts/build-wine.sh && ./scripts/build-winhubcompat.sh
./scripts/build-dxmt.sh && ./scripts/build-dxvk.sh && ./scripts/build-cabextract.sh
./scripts/package.sh && ./scripts/smoke-test.sh
```

Step by step, with the pitfalls already solved: [Building](docs/building.md). Releases: [Releasing](docs/releasing.md).

## Documentation

The full documentation is a book, in English and Brazilian Portuguese:
**[angeloresplandes.github.io/winhub-engine](https://angeloresplandes.github.io/winhub-engine/)** (sources in [`docs/`](docs/)).

## Rules

- Open and public sources only: WineHQ, the LGPL sources of CrossOver's Wine published by CodeWeavers, DXMT,
  DXVK, cJSON, cabextract, LLVM. Every release ships the corresponding sources, as the LGPL requires.
- CrossOver was studied for behaviour only; its closed parts (`cxcompatdb.so`, the `compatdb-*.dat` database,
  the app) are never used. See [Reverse engineering](docs/reverse-engineering.md) and [CONTRIBUTING](CONTRIBUTING.md).
- Apple's D3DMetal is never redistributed. No binaries in Git.

## License

Scripts under MIT ([LICENSE](LICENSE)); `src/winhubcompat` and the Wine patches under LGPL-2.1-or-later, like
Wine; DXMT and DXVK patches under those projects' licenses. Everything else: [THIRD_PARTY_NOTICES](THIRD_PARTY_NOTICES.md).

---

**Independent project.** Not affiliated with CodeWeavers, Apple, Microsoft or Valve. CrossOver is a trademark of
CodeWeavers; the name is used only to describe where the Wine sources come from and what was studied.
