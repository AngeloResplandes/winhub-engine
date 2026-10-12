**English** · [Português (Brasil)](pt-BR/releasing.md)

# Releasing

## Engine release

1. Update [`versions.env`](https://github.com/AngeloResplandes/winhub-engine/blob/main/versions.env) and raise
   `ENGINE_REVISION` if anything in it, in `patches/` or in `src/` changed.
2. Push a tag `wine-<WINE_VERSION>-r<ENGINE_REVISION>`, for example `wine-11.0-r2`. The tag must match
   `versions.env`; the workflow refuses it otherwise.
3. The CI builds, runs the smoke test and creates the release with the overlay, `cabextract`, the
   corresponding sources (CrossOver's source archive, cJSON, cabextract, this repository), `SHA256SUMS` and
   `winhub-manifests.json`.

Every release must carry the corresponding sources: the Wine in it is LGPL.

## Order with WinHub

WinHub only installs what its `Sources/WinHub/Resources/runtimes.json` lists. Older WinHub versions do not
understand `overlay.replace` and would unpack an r2 overlay over the official build without deleting its
`lib/wine`. So:

1. publish a WinHub version that supports `overlay.replace` and `backend-dispatch` (0.2.7 and later do);
2. publish the engine release;
3. only then add the `runtime` entry from `winhub-manifests.json` to WinHub's `runtimes.json`, first in the list.

## Compatibility rules

The rule database WinHub downloads is
[`data/compatdb.json`](https://github.com/AngeloResplandes/winhub-engine/blob/main/data/compatdb.json) on `main`,
with its signature `data/compatdb.json.sig`. It is not tied to engine releases. To change it:

1. edit the rules and **raise `timestamp`** (seconds since 1970); WinHub refuses a database that is not newer
   than the one it has;
2. sign: `swift scripts/compatdb.swift sign` (private key from `~/.winhub-signing/compatdb-ed25519.key` or
   `$WINHUB_COMPATDB_KEY`);
3. commit both files; the CI runs `swift scripts/compatdb.swift verify`.

Key custody and what to do if the key leaks: [SECURITY](https://github.com/AngeloResplandes/winhub-engine/blob/main/SECURITY.md).
