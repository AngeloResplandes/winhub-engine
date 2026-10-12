**English** · [Português (Brasil)](pt-BR/architecture.md)

# Architecture

## What a release contains

The engine is an **overlay**: WinHub first installs the official WineHQ macOS build
(`wine-devel-11.18-osx64` by Gcenx), deletes its `bin/` and `lib/wine/`, and unpacks the overlay on top. The
base build keeps providing its libraries (freetype, gnutls, SDL2, MoltenVK, ICU…), wine-mono and wine-gecko,
which this repository does not build.

```
<wine folder> (from the official build, bin/ and lib/wine/ replaced)
├── bin/                     wine, wineserver and the usual program links (wineboot, winecfg…)
├── lib/
│   ├── wine/                Wine 11.0 from CrossOver's LGPL sources
│   │   ├── x86_64-unix/     ntdll.so, win32u.so, winemac.so… and winhubcompat.so
│   │   ├── x86_64-windows/  64-bit Windows-side DLLs (+ winemetal.dll)
│   │   └── i386-windows/    32-bit Windows-side DLLs (+ winemetal.dll), run through WoW64
│   ├── dxmt/                DXMT v0.80, builtin DLLs: x86_64-windows, i386-windows, x86_64-unix
│   ├── dxvk/                DXVK 1.10.3, builtin DLLs: x86_64-windows, i386-windows
│   └── *.dylib              libraries of the official build (MoltenVK, freetype, gnutls, SDL2…)
└── share/
    ├── wine/                wine.inf, nls, fonts (ours) + wine-mono, wine-gecko (official build)
    └── winhub-engine/       engine.json, compatdb.json (built-in rules), licenses
```

`share/winhub-engine/engine.json` lists the versions and the `features` WinHub checks:
`dxmt`, `dxvk`, `msync`, `backend-dispatch`, `compatdb`, `nvext` (revision 3).

## Build pipeline

```
crossover-sources-26.3.0.tar.gz ──► sources/wine + patches/wine ──► build-wine.sh ──► wine-install/
        (CodeWeavers, LGPL)        └─ sources/dxvk + patches/dxvk ──► build-dxvk.sh ──► dxvk-install/
DXMT v0.80 (MIT) + patches/dxmt + LLVM 15 ───────────────────────► build-dxmt.sh ──► dxmt-install/
src/winhubcompat + cJSON ─────────────────────────────────────────► build-winhubcompat.sh
                                                    package.sh ──► dist/ (overlay, sources, manifests)
                                                 smoke-test.sh ──► installs like WinHub and tests D3D11
```

All inputs are pinned by SHA-256 or commit in
[`versions.env`](https://github.com/AngeloResplandes/winhub-engine/blob/main/versions.env). Details per step:
[Building](building.md).

## Wine

- **Version:** Wine 11.0 with CodeWeavers' patches for CrossOver 26.3 (msync, the Metal hooks in
  `winemac.drv` that DXMT and D3DMetal use, and other CrossOver fixes).
- **Architecture:** x86_64 only, run through Rosetta 2 on Apple silicon. 32-bit Windows programs run inside
  64-bit processes (WoW64), so there are no 32-bit macOS libraries.
- **Linked libraries:** freetype, gnutls and SDL2 are loaded from the official build's `lib/`; Vulkan goes
  straight to `libMoltenVK.dylib`, without the Vulkan loader (CodeWeavers' `win32u` does not ask the loader
  for portability drivers, so the loader would find none).
- **Our patches:** load `winhubcompat.so` instead of CrossOver's closed module, use the official build's
  wine-mono, and keep upstream Wine's user name, profile folder and Desktop link. See [Patches](patches.md).

## winhubcompat

`ntdll.so` loads `lib/wine/x86_64-unix/winhubcompat.so` in every Wine process, after mapping `ntdll.dll`
and before any other DLL. Its constructor:

1. reads the compatibility rules: the bottle's own file (`WINHUB_COMPAT_RULES`), then the newer of the
   built-in database and the one WinHub downloaded (`WINHUB_HOME/compat/compatdb.json`);
2. applies the matching rule's DLL overrides and environment variables to the process;
3. picks the Direct3D backend and puts its folder **first** in Wine's DLL search path
   (`prepend_dll_path`), so `lib/dxmt/x86_64-windows/d3d11.dll` is found before Wine's own;
4. wraps `NtCreateUserProcess` so child processes get the matching rule's environment variables and
   command-line additions;
5. with D3DMetal, wraps `NtCreateFile` and `NtOpenFile` so `C:\windows\system32\nvngx.dll` opens D3DMetal's
   own copy (DLSS).

It also sets `CX_ACTIVE_GRAPHICS_BACKEND`, which CodeWeavers' `win32u` reads. Rule format and the full list of
variables: [winhubcompat and compatibility rules](compatdb.md).

### How the backend is chosen

Candidates, in order; the first one that can run wins:

1. a rule for the program with `"force": true`;
2. the user's choice (`WINHUB_GRAPHICS_BACKEND`), unless it is `auto`;
3. a rule for the program;
4. `d3dmetal`, then `dxmt`;
5. `wined3d` (always possible).

| Backend | Can run when |
|---|---|
| `dxmt` | macOS 14 or later, process translated by Rosetta, DXMT present |
| `d3dmetal` | same, plus a 64-bit process and the user's own D3DMetal (`WINHUB_D3DMETAL_DIR`) |
| `dxvk` | 64-bit process and DXVK present (32-bit processes fall back, see [Gaps](gaps.md)) |
| `wined3d` | always |

### Why the backend DLLs are "builtin"

Wine ignores a DLL found in its builtin search path unless the DLL carries the "Wine builtin DLL" marker. DXMT
is built with `wine_builtin_dll=true`, and the DXVK DLLs are marked with `winebuild --builtin`. A builtin DLL
Wine does not know (`winemetal.dll`) is only found through its file in `system32`, which `wineboot` creates
for DLLs in `lib/wine`; that is why `winemetal.dll` also ships in `lib/wine`, as in CrossOver.

## How WinHub uses the engine

- Installs the base build and the overlay, honouring `overlay.replace` (`["bin", "lib/wine"]`).
- Sets `WINHUB_GRAPHICS_BACKEND` from the bottle, `WINHUB_COMPAT_RULES` to `<bottle>/compat-rules.json`,
  `WINHUB_HOME`, and the D3DMetal variables when the user pointed WinHub at a Game Porting Toolkit.
- Downloads `data/compatdb.json` from this repository only when its Ed25519 signature checks out.

The WinHub side is described in WinHub's
[architecture](https://github.com/AngeloResplandes/winhub/blob/main/docs/architecture.md).
