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
| The bottle | `system32` only has Wine's own `d3d11`, `dxgi`, `d3d10core`, `winemetal` |

`SMOKE_BACKENDS="dxmt"` limits the backends. The CI runs the full test.

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
