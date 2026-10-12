**English** · [Português (Brasil)](SECURITY.pt-BR.md)

# Security policy

winhub-engine runs Windows programs, which have the same access to your files as any app you run. The engine
does not sandbox them. What it does protect is the **supply chain**: what gets built, what WinHub downloads,
and the rules that change how programs start.

## Reporting a vulnerability

Please do not open a public issue with a working exploit, a secret or a weaponized file. Open an issue that only
says you have a security report and asks for a private channel, or use GitHub's private vulnerability reporting
on this repository if it is enabled. Include the engine revision or release, macOS version and Mac model, the
affected part (build scripts, `winhubcompat`, the rule database, a patch), and the steps to reproduce.

## Trust chain

| Step | Protection |
|---|---|
| Inputs of the build | every download pinned by SHA-256 or commit in `versions.env`; checked by `scripts/common.sh` (`fetch`, `clone_pinned`) |
| Build | GitHub Actions from the tagged commit; no binaries in Git |
| Release assets | `SHA256SUMS`; WinHub checks the SHA-256 of the base build and of the overlay before installing |
| Overlay installation | WinHub only removes the folders listed in `overlay.replace`, and refuses paths that leave the Wine folder |
| Rule database | Ed25519 signature over the exact bytes; WinHub accepts it only with a valid signature and a newer `timestamp` |
| Rules at run time | `winhubcompat` trusts files on disk: the built-in database, the one WinHub verified, and the bottle's own file |

## The signing key

- The private key signs `data/compatdb.json`. It lives outside any repository, by default in
  `~/.winhub-signing/compatdb-ed25519.key` (mode 600), and is never committed or uploaded to CI.
- The public key is `data/compatdb-public-key.txt` and is built into WinHub (`CompatDatabaseSync.publicKey`).
- Keep a copy of the private key in a password manager.

**If the key leaks or is lost:** generate a new pair, replace `data/compatdb-public-key.txt` and the key in
WinHub, re-sign the database, and publish a new WinHub version. Old WinHub versions keep trusting the old key
until they update, so revoke by raising the database `timestamp` with the new key as soon as possible.

## Rules can change behaviour

A rule can set environment variables, add DLL overrides and add command-line arguments to a program. That is why
downloaded rules must be signed. Rules in a bottle's `compat-rules.json` are written by WinHub and are local to
that bottle.
