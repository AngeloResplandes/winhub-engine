**English** · [Português (Brasil)](ROADMAP.pt-BR.md)

# winhub-engine roadmap

**Stage: revision 3**, released as [`wine-11.0-r3`](https://github.com/AngeloResplandes/winhub-engine/releases/tag/wine-11.0-r3) and WinHub's default engine

> **Last reviewed:** 2026-10-13 · **Target:** the engine inside CrossOver 26.3.0 for macOS

One-page summary. Detail: [parity with CrossOver](docs/crossover-parity.md) (weights and evidence),
[gaps](docs/gaps.md) (the ranked work list), [decisions](docs/decisions.md).

## Headline

| | Value | Kind |
|---|---|---|
| **Engine parity with CrossOver** | **~71%** | estimated, weighted by dimension |
| Direct3D 10/11 | DXMT 64/32-bit and DXVK 64-bit at feature level 11_0 | measured |
| Direct3D 12 | only with the user's D3DMetal | measured |
| Compatibility rules | format, signing and update channel ready; 1 rule | measured |

## Done

- Revision 1 (2026-10-11): official WineHQ build + patched `winemac.so` + DXMT, copied into bottles by WinHub.
- Revision 2 (2026-10-12): Wine from CrossOver's LGPL sources, `winhubcompat`, DXMT and DXVK as builtins chosen
  per process, msync, D3DMetal from the user, signed rule database, real user profile kept. Released as
  `wine-11.0-r2` and made WinHub's default engine the same day.
- Revision 3 (2026-10-13): `replace_exe_path` rules, DXMT's `nvapi64`/`nvngx` (DLSS over MetalFX, on per bottle),
  the first rule (Dark Souls Remastered → DXVK). Released as `wine-11.0-r3`.

## Next

1. **First rules:** one per tested game in WinHub's compatibility reports.
2. **Game videos:** GStreamer and H.264 in the overlay.
3. **Direct3D 12 without D3DMetal:** evaluate DXMT's D3D12 and vkd3d-proton.
4. **DXVK in 32-bit:** follow MoltenVK's support for placed memory maps.
5. **msync numbers:** measure a CPU-bound game.

## Revision history

| Date | Change |
|---|---|
| 2026-10-12 | First roadmap, engine revision 2. |
| 2026-10-12 | `wine-11.0-r2` released and made WinHub's default engine. |
| 2026-10-13 | `wine-11.0-r3` released; parity ~71%. |
