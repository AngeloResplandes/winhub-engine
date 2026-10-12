**English** · [Português (Brasil)](pt-BR/patches.md)

# Patches

Every patch is small, has a header saying why it exists, and is applied by its build script. Patches to Wine are
LGPL-2.1-or-later like Wine; patches to DXMT and DXVK follow those projects' licenses.

## Wine

Applied by `build-wine.sh` on top of `sources/wine` from CrossOver's LGPL source archive.

| Patch | Why |
|---|---|
| `0001-ntdll-Load-winhubcompat.so-instead-of-cxcompatdb.so` | CodeWeavers' `ntdll` loads a closed module, `cxcompatdb.so`, at the end of `start_main_thread`. The patch loads our open module, `winhubcompat.so`, at the same point. |
| `0002-mscoree-appwiz.cpl-Use-wine-mono-11.3.0` | Wine 11.0 expects wine-mono 10.4.1, but the official build the engine is laid over ships 11.3.0 in `share/wine/mono`. Keep in step with `WINE_BASE_URL`. |
| `0003-advapi32-shell32-Keep-the-real-user-name-and-the-Mac-desktop` | CodeWeavers' tree reports every user as `crossover`, puts the profile in `C:\users\crossover` and makes Desktop a folder with a link to the Mac desktop inside. Bottles made with the official Wine use the real user name (where Steam keeps its login and games keep their saves) and link Desktop to the Mac desktop. Found when a test bottle lost Steam's saved login. |

## DXMT

Applied by `build-dxmt.sh` on top of DXMT v0.80 (commit `589adb7`).

| Patch | Why |
|---|---|
| `0001-Add-missing-standard-headers` | Builds with the current mingw-w64 (fixes taken from DXMT upstream). |
| `0002-airconv-Use-the-public-Metal-atomic-in-the-tessellation-shader` | The Metal toolchain of Xcode 27 changed the arguments of an internal builtin the shader called; the public `atomic_fetch_add_explicit` does the same thread-group relaxed add. |

## DXVK

Applied by `build-dxvk.sh` on top of `sources/dxvk` (1.10.3) from CrossOver's source archive. The build also
passes `-include cstdint` (1.10.3 predates GCC 13) and `-Denable_dxgi=false` (Wine keeps its own `dxgi`, as in
CrossOver).

| Patch | Why |
|---|---|
| `0001-Build-with-mingw-w64-10-and-later` | Newer mingw-w64 headers define `D3DDEVINFO_RESOURCEMANAGER` and the `ID3D10StateBlock` GUID, so DXVK's stand-ins no longer compile. |
| `0002-d3d9-d3d11-Ask-only-for-the-core-features-the-device-has` | The official MoltenVK has no geometry shaders or cull distances, so requiring them leaves no usable adapter. CrossOver pairs this DXVK with its own MoltenVK, which claims those features; asking only for what is present does the same job with the official MoltenVK. |
