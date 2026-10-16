**English** · [Português (Brasil)](pt-BR/gaps.md)

# Gaps

> **Last reviewed:** 2026-10-16

Known shortfalls of the engine, ranked by how many games they block. Each entry says what is missing, what it
blocks, and the likely way forward. Parity numbers: [Parity with CrossOver](crossover-parity.md).

| # | Gap | Blocks | Way forward |
|---|---|---|---|
| 1 | **Direct3D 12 without D3DMetal** | D3D12-only games (The Witcher 3 current version, Resident Evil 4 Remake, Hogwarts Legacy) for users without the Game Porting Toolkit | measured 2026-10-16 ([decision](decisions.md#2026-10-16-direct3d-12-stays-with-d3dmetal)): DXMT's D3D12 is early (feature level 11_1, DXBC shaders only, no DXIL, geometry or tessellation) and vkd3d-proton is blocked by MoltenVK, so playable Direct3D 12 needs D3DMetal: guide users to the Game Porting Toolkit (WinHub's next step) |
| 2 | **Videos in Direct3D 11 Unity games** (Hollow Knight) | intro and cut-scene videos stay black although they decode: Unity passes each frame from a video device to its main device through shared textures | DXVK-macOS has no shared resources (`GetSharedHandle` fails); DXMT v0.80 overwrites a shared texture with its deferred zero fill and returns a null handle for textures Unity asks about. Fix DXMT (upstream or a small patch) and retest |
| 3 | **Automático blocks Direct3D 12 programs** (revision 5) | a D3D12 program cannot create a device with DXMT's `dxgi` (it does not answer vkd3d's adapter interface), not even Wine's own vkd3d at 11_0, unless D3DMetal is set | evaluate for revision 6: (a) the newer DXMT, which fixes it and adds basic D3D12 at 11_1 but replaces the whole Direct3D 11 path; or (b) a narrow fix: a small DXMT `dxgi` patch so vkd3d gets its adapter, or `winhubcompat` not putting DXMT's `dxgi` first for D3D12 programs |
| 4 | **Small rule database** (2 rules: Dark Souls Remastered and Skyrim Special Edition → DXVK) | every game that needs a backend, a variable or an argument to start | add rules as games are tested (WinHub's [compatibility reports](https://github.com/AngeloResplandes/winhub/tree/main/docs/compat)); sign and publish ([Releasing](releasing.md#compatibility-rules)) |
| 5 | **DXVK in 32-bit processes** | 32-bit games that run better on DXVK than on DXMT (DXMT covers most) | MoltenVK support for `VK_EXT_map_memory_placed` or for host-memory buffers; until then the fallback to DXMT |
| 6 | **x86_64 only** | performance and the future: Rosetta 2's general support ends with macOS 28 | follow upstream Wine's ARM64 macOS work |
| 7 | **msync not measured** | knowing the benefit | the measurements in [msync](msync.md#still-to-measure) |
| 8 | **Other video formats** (WMV/WMA, MPEG-1, Theora) | cut-scenes and intro videos in those formats do not play; revision 5 only decodes H.264 and AAC (MP4) | macOS has no decoder for them. Theora is free and could be added with libtheora and libogg; WMV/WMA and MPEG-1 would need FFmpeg's decoders (gst-libav), which the project has ruled out so far: [decision](decisions.md#2026-10-15-game-videos-through-gstreamer-with-videotoolbox) |
