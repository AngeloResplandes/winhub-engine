**English** · [Português (Brasil)](THIRD_PARTY_NOTICES.pt-BR.md)

# Third-party components

This repository's scripts are MIT (see `LICENSE`); `src/winhubcompat` and the Wine patches are
LGPL-2.1-or-later. The files published as releases are built by CI from the sources below, at the versions
pinned in `versions.env`.

| Component | Where it goes | License | Source |
|---|---|---|---|
| Wine 11.0 from CrossOver's sources (`bin/`, `lib/wine/`, `share/wine/`, with the patches in `patches/wine`) | Overlay | LGPL-2.1-or-later | https://www.codeweavers.com/crossover/source · the exact `crossover-sources-<version>.tar.gz` and the patches ship with every release |
| winhubcompat (`lib/wine/x86_64-unix/winhubcompat.so`) | Overlay | LGPL-2.1-or-later | `src/winhubcompat` in this repository |
| cJSON (inside `winhubcompat.so`) | Overlay | MIT | https://github.com/DaveGamble/cJSON · the exact `cJSON-<version>.tar.gz` ships with every release |
| DXVK 1.10.3 from CrossOver's sources (`lib/dxvk`, with the patches in `patches/dxvk`) | Overlay | zlib | https://github.com/doitsujin/dxvk · included in `crossover-sources-<version>.tar.gz` |
| Official Wine build for macOS (the base the overlay is laid over: libraries, wine-mono, wine-gecko) | Downloaded by WinHub, not redistributed here | LGPL-2.1 and the licenses of the bundled libraries | https://github.com/Gcenx/macOS_Wine_builds |
| DXMT v0.80 (`d3d11`, `dxgi`, `d3d10core`, `winemetal`; 64-bit `nvapi64`, `nvngx`) | Overlay | MIT (v0.80 is the last MIT release; later ones are LGPL) | https://github.com/3Shain/dxmt |
| LLVM 15 (libraries linked statically into `winemetal.so`) | Overlay | Apache-2.0 with LLVM Exceptions | https://github.com/llvm/llvm-project |
| DirectX headers for mingw (only used to build DXMT) | — | MIT | https://github.com/misyltoad/mingw-directx-headers |
| NVIDIA NVAPI headers (used to build DXMT's `nvapi64`; the commit DXMT pins) | Overlay (`share/winhub-engine/NVAPI-LICENSE`) | MIT | https://github.com/NVIDIA/nvapi |
| cabextract (with bundled libmspack) | `cabextract-<version>-macos.tar.xz` | GPL-3.0-or-later (libmspack: LGPL-2.1) | https://www.cabextract.org.uk · the exact `cabextract-<version>.tar.gz` ships with every release |
| mingw-w64 runtime and libgcc (in the DXMT and DXVK DLLs) | Overlay | ZPL-2.1 / public domain; GPL-3.0 with the GCC Runtime Library Exception | https://www.mingw-w64.org · https://gcc.gnu.org |

Apple's D3DMetal (Game Porting Toolkit) is **not** distributed. The engine only uses it when the user installed it
and WinHub points to its folder (`WINHUB_D3DMETAL_DIR`, see [`docs/compatdb.md`](docs/compatdb.md)).

CrossOver is a trademark of CodeWeavers. Windows and Direct3D are trademarks of Microsoft. Mac, macOS and Metal
are trademarks of Apple. The names are used only to describe origin and compatibility.
