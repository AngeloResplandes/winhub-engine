**English** · [Português (Brasil)](pt-BR/crossover-parity.md)

# Parity with CrossOver's engine

> **Last reviewed:** 2026-10-16 · **Engine:** revision 5 (`wine-11.0-r5`) · **Target:** the engine
> inside CrossOver 26.3.0 for macOS

How close winhub-engine is to the engine CrossOver ships, dimension by dimension. "Measured" numbers come from
tests in this repository; "estimated" ones are judgement, with the reasoning next to them. What CrossOver does
is described in [Reverse engineering](reverse-engineering.md); the work list is [Gaps](gaps.md).

## Headline

| Number | Value | Kind |
|---|---|---|
| **Engine parity** (weighted, below) | **~74%** | estimated |
| Direct3D 10/11 on Apple silicon | DXMT 64/32-bit and DXVK 64-bit reach feature level 11_0 | measured (smoke test) |
| Direct3D 12 | only with the user's own D3DMetal | measured with a local Game Porting Toolkit |
| Real game | Hollow Knight (Unity 6, D3D11) runs through DXMT at feature level 11.1, Steam with saved login | measured 2026-10-12 |
| Real games, played by script | Dark Souls Remastered, Hollow Knight, Lethal Company and PEAK playable with D3DMetal; Dark Souls also with DXVK (DXMT freezes on it) | measured 2026-10-13 ([WinHub's reports](https://github.com/AngeloResplandes/winhub/tree/main/docs/compat)) |

## By dimension

The game videos row and the total include revision 5, which is not released yet; the other rows are revision 4.

| Dimension | Weight | CrossOver 26.3 | winhub-engine r5 | % | Kind |
|---|---:|---|---|---:|---|
| Wine core, WoW64, Rosetta | 25% | Wine 11.0 + CodeWeavers patches | the same tree, plus three small patches | 100% | same sources |
| Synchronization (msync) | 10% | yes | yes, same code | 100% | same sources |
| Backend choice per process | 10% | closed module, encrypted rules | `winhubcompat`, open rules, same decisions and order, including executable-path replacement and Steam ID matching | 100% | estimated |
| Direct3D 10/11 | 20% | DXMT v0.72+, DXVK 1.10.3 (with their MoltenVK), D3DMetal | DXMT v0.80 64/32-bit, DXVK 1.10.3 64-bit only, D3DMetal if the user has it | 85% | measured cases, estimated weight |
| Direct3D 12 | 15% | D3DMetal 3.0 bundled | D3DMetal only from the user's Game Porting Toolkit; Wine's vkd3d otherwise | 30% | estimated |
| Compatibility rules (content) | 10% | curated per-game database, updated online | the format and the signed update channel exist; 2 rules | 5% | measured (2 rules) |
| Game videos (GStreamer, H.264) | 5% | GStreamer and plug-ins bundled | GStreamer 1.24.4 built for x86_64, with macOS's own H.264 (VideoToolbox) and AAC (AudioToolbox) decoders; MP4 only, no WMV/WMA, MPEG-1 or Theora. Hollow Knight (Unity 6) decodes its prologue video on revision 5, but the frames stay black: Unity hands them between two Direct3D 11 devices through shared textures, which DXVK-macOS lacks and DXMT does not yet pass on correctly | 60% | estimated; H.264/AAC MP4 measured in the smoke test and in Hollow Knight (decoding) |
| DLSS / NVIDIA APIs | 5% | `nvngx`/`nvapi64` in DXMT and D3DMetal, `nvngx` redirect | `nvngx` redirect with D3DMetal; DXMT's `nvapi64`/`nvngx`, on per bottle; not tried in a DLSS game yet | 80% | estimated |
| **Weighted** | 100% | | | **~74%** | estimated |

## Differences that are choices, not gaps

| | CrossOver | winhub-engine | Why |
|---|---|---|---|
| Rule database | encrypted, closed | readable JSON, signed with Ed25519 | anyone can read and contribute rules; WinHub still verifies what it downloads |
| Automatic without D3DMetal | — (D3DMetal always bundled) | DXMT | D3DMetal cannot be redistributed |
| Rules per bottle | no | yes (`WINHUB_COMPAT_RULES`) | WinHub's per-game Direct3D choice |
| DXMT version | v0.72 + 25 commits | v0.80 | v0.80 is the last MIT release |
| MoltenVK | CodeWeavers' 1.2.10, claims missing features | official 1.4.0 from the base build | open and current; DXVK patched instead |
| User name and Desktop | `crossover`, Desktop folder | real user name, Desktop linked to the Mac | keeps bottles made with the official Wine working |

## Revision history

| Date | Change |
|---|---|
| 2026-10-12 | First assessment, engine revision 2. |
| 2026-10-13 | Revision 3: backend choice 90% → 100% (`replace_exe_path`), DLSS 50% → 80%; total ~71%. |
| 2026-10-15 | Revision 4: `steam_game_id` rules match games Steam starts; 2 rules; total ~71%. |
| 2026-10-16 | Revision 5 released: game videos 0% → 60% (GStreamer with VideoToolbox; H.264/AAC MP4 decoded in the smoke test and in Hollow Knight, whose frames still stay black through Direct3D 11 shared textures); total ~71% → ~74%. |
