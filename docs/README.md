**English** · [Português (Brasil)](pt-BR/README.md)

# winhub-engine documentation

winhub-engine is the Wine runtime behind [WinHub](https://github.com/AngeloResplandes/winhub), the macOS
app that installs and runs Windows programs. It is Wine 11 built from the LGPL sources CodeWeavers
publishes for CrossOver, plus DXMT, DXVK and `winhubcompat`, a small open module that picks the Direct3D
implementation of every Wine process without copying anything into bottles.

The engine's design was worked out by studying how CrossOver 26.3 runs games, by its behaviour only:
[Reverse engineering](reverse-engineering.md) explains what was studied, what was learned and where the
line is. No CrossOver code or data outside its published LGPL sources is in this project.

## Where to start

| If you want to | Read |
|---|---|
| Understand what the engine is made of and how a backend gets chosen | [Architecture](architecture.md) |
| Build it on your Mac | [Building](building.md) |
| Run the tests, or test games without risking real bottles | [Testing](testing.md) |
| Publish a release | [Releasing](releasing.md) |
| Write or change compatibility rules | [winhubcompat and compatibility rules](compatdb.md) |
| Know why each patch exists | [Patches](patches.md) |
| See how close it is to CrossOver's engine | [Parity with CrossOver](crossover-parity.md) and [Gaps](gaps.md) |
| Know why things are the way they are | [Decisions](decisions.md) |

Repository files that are not part of this book: [README](https://github.com/AngeloResplandes/winhub-engine#readme),
[ROADMAP](https://github.com/AngeloResplandes/winhub-engine/blob/main/ROADMAP.md),
[SECURITY](https://github.com/AngeloResplandes/winhub-engine/blob/main/SECURITY.md),
[CONTRIBUTING](https://github.com/AngeloResplandes/winhub-engine/blob/main/CONTRIBUTING.md),
[THIRD_PARTY_NOTICES](https://github.com/AngeloResplandes/winhub-engine/blob/main/THIRD_PARTY_NOTICES.md).
