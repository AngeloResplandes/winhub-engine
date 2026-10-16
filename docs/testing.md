**English** · [Português (Brasil)](pt-BR/testing.md)

# Testing

## Rule engine

```bash
./scripts/test-compatdb.sh
```

Builds `tests/compatdb-test.c` natively with AddressSanitizer and UndefinedBehaviorSanitizer and checks: parsing
and skipping of invalid rules, rule order and "first hack of each type wins", path and Steam ID matching, the
backend choice (forced rules, user choice, fallbacks), environment and command-line changes, and the order of
built-in, downloaded and local files.

## Smoke test

```bash
./scripts/smoke-test.sh
```

Installs the engine the way WinHub does (official build, minus `bin/` and `lib/wine/`, overlay on top),
creates a bottle and runs `tests/d3d11-smoke.c`, which creates a Direct3D 11 device with a swap chain and
presents 30 frames:

| Case | Expected |
|---|---|
| DXMT, 64-bit and 32-bit | feature level 11_0, `OK presented 30 frames`, `winhubcompat` picked `dxmt` |
| DXVK, 64-bit | the same with `dxvk` |
| DXVK asked for by a 32-bit program | `winhubcompat` falls back to `dxmt`, which passes |
| NVIDIA extensions (`tests/nvapi-smoke.c`, DXMT) | with `DXMT_ENABLE_NVEXT=1`, `nvapi64` loads, `NvAPI_Initialize` succeeds and `system32\nvngx.dll` opens DXMT's; without it, `NvAPI_Initialize` refuses |
| `replace_exe_path` | a local rule makes `cmd` start the 64-bit test instead of a 32-bit "launcher"; the trace shows `replacing` and the test passes |
| `steam_game_id` | `cmd` sets `SteamGameId` in its Windows environment only, as Steam does, and starts the 64-bit test; a local rule matching that id picks DXVK and the test passes |
| Game video (`tests/mf-smoke.c`) | Media Foundation opens an MP4 and reads it to the end: `OK h264 128x96 frames=15 …` and `OK aac rate=44100 channels=2`; the log shows `winhubcompat` pointing GStreamer at the runtime, and H.264 decoded by `vtdec_hw` (or `vtdec`) and AAC by `atdec` |
| The bottle | `system32` only has Wine's own `d3d11`, `dxgi`, `d3d10core`, `winemetal` |

`SMOKE_BACKENDS="dxmt"` limits the backends. The CI runs the full test.

### The video case

`scripts/make-test-video.swift` writes the MP4 when the test runs (nothing is stored in Git): 15 light-gray frames at
128x96 and one second of silent stereo AAC, encoded by macOS's own AVFoundation, so the file does not depend on
the decoders under test. `tests/mf-smoke.c` reads it with Media Foundation's source reader, and the test checks
the decoders from the log:

```bash
GST_DEBUG=WINE:4,GST_ELEMENT_FACTORY:4 wine mf-smoke64.exe 'C:\video.mp4'
```

`GST_ELEMENT_FACTORY:4` prints `creating element "vtdec_hw"` and `creating element "atdec"`, the factory names;
`WINE:4` alone only gives long names ("Apple VideoToolbox decoder (hardware only)"). Reading is bounded (1000
samples) and must reach the end of the stream, so a stuck decoder fails instead of hanging.

Expect two lines on every video open, which are harmless and come from upstream Wine:
`GStreamer-Video-CRITICAL **: <time>: gst_video_info_from_caps: assertion 'gst_caps_is_fixed (caps)' failed`.
Decoding is correct; they will also show up in users' logs. An `atdec` drain warning when the reader is released
after one audio sample is expected too.

The first run scans the plug-ins and writes the registry to `~/Library/Caches/WinHub/gstreamer/`, with the real
`HOME`; delete the file to see a scan again.

### The Direct3D 12 spike tools

The Direct3D 12 spike (2026-10-16, [decision](decisions.md#2026-10-16-direct3d-12-stays-with-d3dmetal)) left three
tools in the repository, in `tests/` and `scripts/`: `tests/d3d12-smoke.c` (device, feature level, clear and present),
`tests/vulkan-features.c` (lists the Vulkan features vkd3d-proton needs) and `scripts/build-dxmt-d3d12-spike.sh`
(builds a DXMT with Direct3D 12 at its own pinned commit). They are not part of the smoke test and the CI does not
build them.

## Seeing what winhubcompat did

```bash
WINEDEBUG=+winhubcompat wine program.exe
```

prints the rules it read, the matching rule, `using <backend> as the graphics backend`, and, for child
processes, the variables and command-line changes. `WINEDEBUG=+loaddll` shows which DLLs were loaded.

## Testing games safely

Games are tested in a **copy** of a real bottle, never the original:

1. Clone it with APFS (instant, takes no extra space): `cp -cR "<bottle>" work/steam-clone`.
2. Point `WINEPREFIX` at the clone and run the engine from `work/engine` (installed by the smoke test).
3. **Run one Steam at a time.** On 2026-10-11 a test Steam was still open in a clone when the Steam of the
   original bottle was opened; that Steam then marked two games as "files missing" and uninstalled them. Close
   every Wine process of the clone (check with `pgrep -fl steam`) before opening the real bottle, and the other
   way around.
4. When done, delete the clone only after checking nothing in it is needed.

Game results are kept in WinHub's
[compatibility reports](https://github.com/AngeloResplandes/winhub/tree/main/docs/compat).
