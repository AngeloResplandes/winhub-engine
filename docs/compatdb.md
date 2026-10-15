**English** · [Português (Brasil)](pt-BR/compatdb.md)

# winhubcompat and compatibility rules

`winhubcompat.so` (source in [`src/winhubcompat`](https://github.com/AngeloResplandes/winhub-engine/tree/main/src/winhubcompat))
is loaded by `ntdll.so` in every Wine process, right after `ntdll.dll` and before any other DLL
([`patches/wine/0001`](patches.md#wine)). It:

1. reads the compatibility rules;
2. applies the matching rule's DLL overrides and environment variables to the process itself;
3. picks the Direct3D backend and puts its folder **first** in Wine's DLL search path; nothing is copied
   into the bottle;
4. applies environment variables and command-line additions to child processes (through
   `NtCreateUserProcess`);
5. with D3DMetal, sends DLSS's `nvngx.dll` to D3DMetal's own copy.

## Environment variables

| Variable | Set by | Purpose |
|---|---|---|
| `WINHUB_GRAPHICS_BACKEND` | WinHub (bottle) | `auto` (default), `wined3d`, `dxmt`, `dxvk` or `d3dmetal` |
| `WINHUB_D3DMETAL_DIR` | WinHub | The `lib` folder of the user's Apple Game Porting Toolkit (with `wine/` and `external/`). The engine never ships D3DMetal |
| `CX_APPLEGPTK_LIBD3DSHARED_PATH` | WinHub | `<WINHUB_D3DMETAL_DIR>/external/libd3dshared.dylib`; read by the `ntdll` of CodeWeavers' tree |
| `WINHUB_HOME` | WinHub | WinHub's data folder; the downloaded database is `compat/compatdb.json` in it |
| `WINHUB_COMPAT_RULES` | WinHub | The bottle's own rules file (for example a backend chosen for one game) |
| `WINHUB_ACTIVE_GRAPHICS_BACKEND`, `CX_ACTIVE_GRAPHICS_BACKEND` | winhubcompat | The backend chosen; the second is read by CodeWeavers' `win32u` |

To see what it did: `WINEDEBUG=+winhubcompat`. The line `using <backend> as the graphics backend` gives the
backend of each process.

## Choosing the backend

Candidates, in order; the first one that can run wins:

1. the program's rule, if it has `"force": true`;
2. the user's choice (`WINHUB_GRAPHICS_BACKEND`), unless it is `auto`;
3. the program's rule;
4. `d3dmetal`, then `dxmt`;
5. `wined3d` (always possible).

When each one can run:

| Backend | Requires |
|---|---|
| `dxmt` | macOS 14 or later, a process translated by Rosetta (Apple silicon) and `lib/dxmt/x86_64-windows/d3d11.dll` in the engine |
| `d3dmetal` | the same, a 64-bit process and a valid `WINHUB_D3DMETAL_DIR` |
| `dxvk` | a 64-bit process and `lib/dxvk/x86_64-windows/d3d11.dll` in the engine (Vulkan through MoltenVK). In 32-bit processes, without `VK_EXT_map_memory_placed`, `win32u` creates buffers backed by host memory and MoltenVK refuses them (both the official one and CodeWeavers') |
| `wined3d` | nothing |

Backend folders are laid out like a Wine library folder (`x86_64-windows/`, `i386-windows/`, `x86_64-unix/`)
and hold builtin DLLs (Wine ignores ordinary DLLs in that path).

## Where the rules come from

Read in this order of precedence:

1. `WINHUB_COMPAT_RULES` (the bottle's rules; `timestamp` optional);
2. the newer (by `timestamp`) of the database built into the engine (`share/winhub-engine/compatdb.json`,
   from [`data/compatdb.json`](https://github.com/AngeloResplandes/winhub-engine/blob/main/data/compatdb.json))
   and the downloaded one (`$WINHUB_HOME/compat/compatdb.json`). On a tie, the built-in one.

For each kind of hack, the **first** one found among the matching rules wins.

WinHub checks the downloaded database before writing it; the module trusts what is on disk.

### Publishing and signing

The published database is `data/compatdb.json` on `main`. WinHub downloads it with `data/compatdb.json.sig`,
checks the Ed25519 signature with the public key built into the app (the same as
[`data/compatdb-public-key.txt`](https://github.com/AngeloResplandes/winhub-engine/blob/main/data/compatdb-public-key.txt))
and refuses a database whose `timestamp` is not newer than the one it has. Steps to change it:
[Releasing](releasing.md#compatibility-rules).

## Format

```json
{
  "version": 1,
  "timestamp": 1791763200,
  "rules": [
    {
      "desc": "Hollow Knight",
      "os": "macos",
      "match": { "type": "path_suffix", "suffix": "Hollow Knight\\hollow_knight.exe" },
      "hacks": [
        { "type": "graphics_backend", "backend": "dxmt" },
        { "type": "env_vars", "vars": { "DXMT_LOG_LEVEL": "none" } }
      ]
    },
    {
      "desc": "A Steam game by app ID",
      "match": { "type": "steam_game_id", "game_id": "367520" },
      "hacks": [
        { "type": "dll_overrides", "overrides": ["d3d9=n,b"] },
        { "type": "append_cmd_line", "append": "-dx11", "forbidden_args": "-dx12", "insert": true }
      ]
    }
  ]
}
```

- `version`: always `1`. `timestamp`: seconds since 1970; required in the built-in and downloaded databases.
- `os`: optional; when present it must be `"macos"` (rules for other systems are skipped).
- `match`:
  - `path_suffix`: the end of the program's Windows path (`\` or `/`), ignoring case, starting at a folder or
    file name boundary (`game.exe` does not match `notgame.exe`);
  - `steam_game_id`: the value of `SteamGameId` in the process's Unix environment. Steam sets it in the game's
    **Windows** environment, which Wine does not copy to the Unix one, so on revision 3 these rules do not match
    games Steam starts (found on 2026-10-15); use `path_suffix` for games until the engine reads the Windows
    environment.
- `hacks`:
  - `graphics_backend`: `backend` (`wined3d`, `dxmt`, `dxvk`, `d3dmetal`) and optional `force`;
  - `dll_overrides`: a list in `WINEDLLOVERRIDES` syntax (`"d3d11=n,b"`);
  - `env_vars`: a `name: value` object. In the process itself only the Unix side sees them (Metal, MoltenVK,
    DXMT); child processes also get them in their Windows environment;
  - `append_cmd_line`: `append`; `insert` (right after the program name instead of at the end);
    `required_args` / `forbidden_args` (apply only if the command line contains / does not contain this
    text). Not added twice. Applies to processes started by another Wine process (for example games
    started by Steam);
  - `replace_exe_path` (revision 3): `path`, the Windows path of a program to start instead of the one that
    matched (`"C:\\Game\\Game.exe"`), with the same arguments. For launchers that must be skipped. Like
    `append_cmd_line`, it applies to processes started by another Wine process, and never to the target
    itself (a rule cannot start it in a loop).

Invalid rules are skipped with a warning (`WINEDEBUG=warn+winhubcompat`); an invalid file is ignored as a
whole.

## D3DMetal and DLSS

With `d3dmetal` active, `winhubcompat` also wraps `NtCreateFile` and `NtOpenFile` so that
`C:\windows\system32\nvngx.dll` (where DLSS looks for NVIDIA's library) opens D3DMetal's `nvngx.dll`, which
plays that role on top of MetalFX.

With `dxmt` (revision 3), the engine's `lib/dxmt` also has DXMT's `nvapi64.dll` and `nvngx.dll` (DLSS over
MetalFX). Wine finds them through the DLL path `winhubcompat` sets, so no file is copied into the bottle. They only
answer with `DXMT_ENABLE_NVEXT=1`, which WinHub sets per bottle (feature `nvext`); without it `NvAPI_Initialize`
returns "NVIDIA device not found", as on a Mac without the option.

## Tests

`./scripts/test-compatdb.sh` covers parsing, precedence, backend choice, environment and command line without
Wine. See [Testing](testing.md).

## Design choices

An open, readable JSON database; rules per bottle; `auto` picks DXMT when there is no D3DMetal. How this
compares with CrossOver's closed module: [Parity with CrossOver](crossover-parity.md).
