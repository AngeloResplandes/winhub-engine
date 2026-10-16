**English** · [Português (Brasil)](pt-BR/gaps.md)

# Gaps

> **Last reviewed:** 2026-10-15

Known shortfalls of the engine, ranked by how many games they block. Each entry says what is missing, what it
blocks, and the likely way forward. Parity numbers: [Parity with CrossOver](crossover-parity.md).

| # | Gap | Blocks | Way forward |
|---|---|---|---|
| 1 | **Direct3D 12 without D3DMetal** | D3D12-only games (The Witcher 3 current version, Resident Evil 4 Remake, Hogwarts Legacy) for users without the Game Porting Toolkit | DXMT's experimental D3D12 (LGPL after v0.80); vkd3d-proton on MoltenVK; or guide users to the Game Porting Toolkit |
| 2 | **Small rule database** (2 rules: Dark Souls Remastered and Skyrim Special Edition → DXVK) | every game that needs a backend, a variable or an argument to start | add rules as games are tested (WinHub's [compatibility reports](https://github.com/AngeloResplandes/winhub/tree/main/docs/compat)); sign and publish ([Releasing](releasing.md#compatibility-rules)) |
| 3 | **DXVK in 32-bit processes** | 32-bit games that run better on DXVK than on DXMT (DXMT covers most) | MoltenVK support for `VK_EXT_map_memory_placed` or for host-memory buffers; until then the fallback to DXMT |
| 4 | **x86_64 only** | performance and the future: Rosetta 2's general support ends with macOS 28 | follow upstream Wine's ARM64 macOS work |
| 5 | **msync not measured** | knowing the benefit | the measurements in [msync](msync.md#still-to-measure) |
| 6 | **Other video formats** (WMV/WMA, MPEG-1, Theora) | cut-scenes and intro videos in those formats do not play; revision 5 only decodes H.264 and AAC (MP4) | macOS has no decoder for them. Theora is free and could be added with libtheora and libogg; WMV/WMA and MPEG-1 would need FFmpeg's decoders (gst-libav), which the project has ruled out so far: [decision](decisions.md#2026-10-15-game-videos-through-gstreamer-with-videotoolbox) |
