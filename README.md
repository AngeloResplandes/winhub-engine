# winhub-engine

Motor do [WinHub](https://github.com/AngeloResplandes/winhub): o Wine oficial do WineHQ para macOS com o
que falta para jogos Direct3D 10/11 rodarem sobre o Metal.

Este repositório não guarda binários. O CI compila tudo a partir das fontes oficiais, com versões
fixadas por SHA-256 ou commit em [`versions.env`](versions.env), e publica o resultado nas Releases. O
WinHub baixa e confere cada arquivo antes de usar.

## O que ele entrega

| Arquivo da release | Conteúdo |
|---|---|
| `winhub-wine-<wine>-r<rev>-overlay.tar.xz` | Arquivos colocados por cima da pasta `wine` do build oficial do WineHQ (`wine-devel-<versão>-osx64`). |
| `cabextract-<versão>-macos.tar.xz` | `cabextract` universal (arm64 + x86_64), usado por alguns verbos do winetricks. |
| `wine-<versão>.tar.xz`, `cabextract-<versão>.tar.gz`, `winhub-engine-r<rev>-patches.tar.xz` | Código-fonte correspondente. |
| `SHA256SUMS`, `winhub-manifests.json` | Somas de verificação e as entradas prontas para o `runtimes.json` e o `components.json` do WinHub. |

Conteúdo do overlay:

- `lib/wine/x86_64-unix/winemac.so`: o driver de Mac do Wine compilado com
  [`patches/wine/0001-winemac.drv-Export-a-function-table-for-DXMT.patch`](patches/wine). O patch exporta
  `macdrv_functions` no formato que o DXMT espera e dá à janela uma client surface quando ela não tem.
- `lib/wine/x86_64-unix/winemetal.so` e `lib/wine/{x86_64,i386}-windows/winemetal.dll`: a ponte do DXMT
  com o Metal.
- `lib/dxmt/{x64,x32}/`: `d3d11.dll`, `dxgi.dll`, `d3d10core.dll` e `winemetal.dll` do
  [DXMT](https://github.com/3Shain/dxmt), compilados como DLLs "nativas" (`wine_builtin_dll=false`). O
  WinHub as copia para a garrafa (ou para a pasta de um jogo) só quando o DXMT é escolhido. Com as DLLs
  fora da garrafa, nada muda em relação ao Wine oficial.
  O código é o da tag fixada, com as correções de `patches/dxmt/` (trazidas do próprio DXMT) para
  compilar com o mingw-w64 atual.

## Como compilar

Precisa de macOS em Apple silicon (com Rosetta 2) ou Intel, Xcode 16 ou mais novo e
`brew install mingw-w64 bison meson ninja`.

```bash
./scripts/build-llvm.sh        # LLVM 15 x86_64 para o conversor de shaders do DXMT (demorado; o CI guarda em cache)
./scripts/build-wine.sh        # Wine com os patches: winemac.so e o que o DXMT usa para compilar
./scripts/build-dxmt.sh
./scripts/build-cabextract.sh
./scripts/package.sh           # gera work/dist
./scripts/smoke-test.sh        # baixa o Wine oficial, aplica o overlay e testa D3D11 via DXMT (64 e 32 bits)
```

Os arquivos intermediários ficam em `work/` (fora do Git). Use `WORK=/outro/lugar` para mudar.

## Publicar uma versão

1. Ajuste `versions.env` (e aumente `ENGINE_REVISION` se algo mudou).
2. Faça o push de uma tag `wine-<WINE_VERSION>-r<ENGINE_REVISION>`, por exemplo `wine-11.18-r1`.
3. O CI compila, testa e cria a release com os arquivos e o `winhub-manifests.json`.

## Regras

- Só fontes abertas e públicas: WineHQ, DXMT, cabextract, LLVM. Nada de produtos comerciais (sala
  limpa, como no WinHub). O D3DMetal da Apple não é usado nem redistribuído.
- Nenhum binário no Git.
- Licenças: scripts sob MIT ([LICENSE](LICENSE)); patches do Wine sob LGPL-2.1, como o Wine; patches do DXMT sob a licença do DXMT; demais
  componentes em [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).
