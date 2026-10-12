**English** · [Português (Brasil)](pt-BR/gaps.md)

# Gaps

> **Last reviewed:** 2026-10-12

Known shortfalls of the engine, ranked by how many games they block. Each entry says what is missing, what it
blocks, and the likely way forward. Parity numbers: [Parity with CrossOver](crossover-parity.md).

| # | Gap | Blocks | Way forward |
|---|---|---|---|
| 1 | **Direct3D 12 without D3DMetal** | D3D12-only games (The Witcher 3 current version, Resident Evil 4 Remake, Hogwarts Legacy) for users without the Game Porting Toolkit | DXMT's experimental D3D12 (LGPL after v0.80); vkd3d-proton on MoltenVK; or guide users to the Game Porting Toolkit |
| 2 | **Empty rule database** | every game that needs a backend, a variable or an argument to start | add rules as games are tested (WinHub's [compatibility reports](https://github.com/AngeloResplandes/winhub/tree/main/docs/compat)); sign and publish ([Releasing](releasing.md#compatibility-rules)) |
| 3 | **DXVK in 32-bit processes** | 32-bit games that run better on DXVK than on DXMT (DXMT covers most) | MoltenVK support for `VK_EXT_map_memory_placed` or for host-memory buffers; until then the fallback to DXMT |
| 4 | **Game videos** (GStreamer, H.264) | cut-scenes and intro videos in many games | build GStreamer and its plug-ins for x86_64 into the overlay (the base build has none) |
| 5 | **`nvapi64` with DXMT** | DLSS and NVIDIA-specific paths without D3DMetal | done in revision 3 (DXMT's `nvapi64`/`nvngx`, on with `DXMT_ENABLE_NVEXT=1`; see [DLSS](compatdb.md#d3dmetal-and-dlss)); leaves this list when r3 is released |
| 6 | **Executable-path replacement rules** | the few games whose launcher must be swapped | done in revision 3 (`replace_exe_path`, see [Compatibility rules](compatdb.md#format)); leaves this list when r3 is released |
| 7 | **x86_64 only** | performance and the future: Rosetta 2's general support ends with macOS 28 | follow upstream Wine's ARM64 macOS work |
| 8 | **msync not measured** | knowing the benefit | the measurements in [msync](msync.md#still-to-measure) |
