**English** · [Português (Brasil)](CONTRIBUTING.pt-BR.md)

# Contributing

Thanks for your interest in winhub-engine.

## Clean-room rules (required)

- **We study CrossOver for behaviour only:** what it does, how its bundle is laid out, which files and
  variables it uses. Findings go in [`docs/reverse-engineering.md`](docs/reverse-engineering.md), described as
  behaviour.
- **Never copy** CrossOver code, binaries, texts, translations, icons or data. This includes `cxcompatdb.so`,
  the encrypted `compatdb-*.dat` database (never decrypt it), the `crossover.tie` catalog and the app's interface.
- **Code CodeWeavers publishes under the LGPL** (the Wine tree of CrossOver) may be used with attribution;
  releases ship the corresponding sources.
- **Do not commit** decompiled code, addresses, internal symbol names or analysis logs. Names that appear in
  public material (variables read by the LGPL code, file names in the bundle) may be quoted.
- **Never redistribute Apple's D3DMetal.**
- **No binaries in Git.** New inputs are pinned by SHA-256 or commit in `versions.env`, and listed in
  [THIRD_PARTY_NOTICES](THIRD_PARTY_NOTICES.md).

By opening a pull request you state that the contribution is yours and follows these rules.

## Development

See [Building](docs/building.md) and [Testing](docs/testing.md). Before a pull request:

- `./scripts/test-compatdb.sh` passes;
- a change to Wine, DXMT, DXVK or `winhubcompat` passes `./scripts/smoke-test.sh`;
- a change to `data/compatdb.json` raises `timestamp` and is signed (`swift scripts/compatdb.swift sign`), see
  [Releasing](docs/releasing.md#compatibility-rules);
- a new patch has a header saying why it exists, and an entry in [Patches](docs/patches.md);
- `ENGINE_REVISION` goes up when `versions.env`, `patches/` or `src/` change.

## Documentation

English and Brazilian Portuguese are kept in step: every page in `docs/` has its mirror in `docs/pt-BR/`, and
every root page its `.pt-BR.md`. Change both in the same pull request.

## Commits

Small and focused, with an English subject in the imperative mood.
