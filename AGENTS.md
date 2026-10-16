# AGENTS.md: guide for AI agents and contributors

winhub-engine builds the Wine runtime WinHub installs: Wine 11.0 from the LGPL sources CodeWeavers publishes for
CrossOver, DXMT, DXVK and `winhubcompat` (C, LGPL), packaged as an overlay for the official WineHQ macOS build.
The repository holds scripts, patches and sources only; CI builds and publishes the binaries.

## 1. Orientation

| Read | Why |
|---|---|
| `docs/architecture.md` | Runtime layout, build pipeline, how a backend is chosen |
| `docs/building.md` | Requirements, the scripts in order, pitfalls already solved |
| `docs/testing.md` | Rule-engine tests, smoke test, testing games without risking real bottles |
| `docs/compatdb.md` | `winhubcompat`, its variables and the rule format |
| `docs/reverse-engineering.md` | What was studied in CrossOver and the rules that keep the project clean |
| `docs/crossover-parity.md`, `docs/gaps.md`, `ROADMAP.md` | How close the engine is, and what to work on |
| `docs/decisions.md` | Why things are the way they are |

## 2. Map

```text
versions.env            every pinned input (SHA-256 or commit); bump ENGINE_REVISION when it, patches/ or src/ change
patches/wine|dxmt|dxvk  small patches with a "why" header, applied by the build scripts
src/winhubcompat/       compatdb.c (pure C: rules, backend choice, env, command line) + winhubcompat.c (ntdll glue)
scripts/                build-*.sh (build-gstreamer.sh before build-wine.sh), package.sh, smoke-test.sh, test-compatdb.sh,
                        compatdb.swift (sign/verify rules), make-test-video.swift (the smoke test's H.264/AAC MP4)
data/                   compatdb.json (+ .sig, signed), compatdb-public-key.txt
tests/                  compatdb-test.c, d3d11-smoke.c, nvapi-smoke.c, mf-smoke.c (Media Foundation video)
docs/, docs/pt-BR/      documentation (English, Brazilian Portuguese); also the mdBook sources
book/en, book/pt-BR     mdBook configuration; .github/workflows/docs.yml builds and publishes the site
```

## 3. Golden rules

1. **Clean room.** CrossOver is studied for behaviour only. Never copy its code, binaries, texts, icons or data
   (`cxcompatdb.so`, `compatdb-*.dat`, `crossover.tie`, the app). Never commit decompiled code, addresses,
   internal symbol names or analysis logs. CodeWeavers' LGPL sources may be used with attribution.
2. **No binaries in Git; pin everything.** New inputs go in `versions.env` with a SHA-256 or commit and in
   `THIRD_PARTY_NOTICES.md`. Never redistribute Apple's D3DMetal.
3. **Tests are the gate.** `./scripts/test-compatdb.sh` for `src/winhubcompat`; `./scripts/smoke-test.sh` for
   anything that changes Wine, DXMT, DXVK or the package. Report failures with their output.
4. **The rule database is signed.** Any change to `data/compatdb.json` raises `timestamp` and is re-signed with
   `swift scripts/compatdb.swift sign`; the private key never enters the repository or CI.
5. **Patches stay small and explained.** Each has a header with the reason and an entry in `docs/patches.md`.
6. **Never risk a user's real bottle.** Test games in an APFS clone, and never run two Steam instances of the same
   bottle at once (see `docs/testing.md`).
7. **Docs in two languages.** Every page in `docs/` has a mirror in `docs/pt-BR/`, every root page a `.pt-BR.md`;
   change both together. Parity pages carry a "Last reviewed" date.
8. **Releases carry their sources** (LGPL), and the WinHub `runtimes.json` entry comes after the release
   (`docs/releasing.md`).

## 4. Commits

Small and focused, English subject in the imperative mood. Commit or push only when the maintainer asks.
