# winhub-engine

Motor do [WinHub](https://github.com/AngeloResplandes/winhub): o Wine 11 compilado das fontes LGPL que a
CodeWeavers publica para o CrossOver (com msync, WoW64 e os ganchos de Metal do driver de Mac), mais
DXMT, DXVK e o `winhubcompat`, que escolhe o Direct3D de cada processo sem copiar nada para as garrafas.

Este repositório não guarda binários. O CI compila tudo a partir das fontes, com versões fixadas por
SHA-256 ou commit em [`versions.env`](versions.env), e publica o resultado nas Releases. O WinHub baixa
e confere cada arquivo antes de usar.

## Como funciona

O desenho segue o do CrossOver 26, cujo `ntdll` (código LGPL) carrega um módulo nesse mesmo ponto:

```
WinHub ── WINHUB_GRAPHICS_BACKEND=auto|wined3d|dxmt|dxvk|d3dmetal ──▶ wine
                                                                       │
ntdll.so (start_main_thread) ── dlopen ──▶ winhubcompat.so ◀── regras (embutidas, baixadas, da garrafa)
                                              │
                    escolhe o backend que pode rodar aqui (macOS 14+, Rosetta, 64 bits…)
                                              │
                    prepend_dll_path(lib/dxmt | lib/dxvk | D3DMetal do usuário)
                                              │
              o d3d11.dll do backend é achado antes do do Wine; a garrafa fica intacta
```

Detalhes do módulo e do formato das regras: [docs/compatdb.md](docs/compatdb.md).

## O que ele entrega

| Arquivo da release | Conteúdo |
|---|---|
| `winhub-wine-<wine>-r<rev>-overlay.tar.xz` | O motor. O WinHub instala o Wine oficial para macOS (`wine-devel-<versão>-osx64`, que traz freetype, gnutls, SDL2, MoltenVK, wine-mono e wine-gecko), **apaga `bin/` e `lib/wine/`** dele e desempacota isto por cima (`overlay.replace` no manifesto). |
| `cabextract-<versão>-macos.tar.xz` | `cabextract` universal (arm64 + x86_64), usado por alguns verbos do winetricks. |
| `crossover-sources-<versão>.tar.gz`, `cJSON-<versão>.tar.gz`, `cabextract-<versão>.tar.gz`, `winhub-engine-r<rev>-sources.tar.xz` | Código-fonte correspondente. |
| `SHA256SUMS`, `winhub-manifests.json` | Somas de verificação e as entradas prontas para o `runtimes.json` e o `components.json` do WinHub. |

Conteúdo do overlay:

- `bin/wine`, `bin/wineserver` e os atalhos de sempre (`wineboot`, `winecfg`…); `lib/wine/` e
  `share/wine/` do Wine 11.0 das fontes do CrossOver, com [`patches/wine`](patches/wine):
  - `0001`: o `ntdll` carrega o `winhubcompat.so` no lugar do módulo fechado do CrossOver;
  - `0002`: usa o wine-mono 11.3.0 que vem no pacote oficial;
  - `0003`: mantém o nome de usuário real, o perfil `C:\users\<usuário>` e a pasta `Desktop` ligada à
    Área de Trabalho do Mac, como no Wine oficial (a árvore da CodeWeavers usa "crossover" para todos,
    o que faria garrafas existentes perderem o perfil onde a Steam e os jogos guardam dados).
- `lib/wine/x86_64-unix/winhubcompat.so`: [`src/winhubcompat`](src/winhubcompat).
- `lib/dxmt/{x86_64-windows,i386-windows,x86_64-unix}`: [DXMT](https://github.com/3Shain/dxmt) v0.80
  (`d3d11`, `dxgi`, `d3d10core`, `winemetal`) como DLLs *builtin* do Wine, com
  [`patches/dxmt`](patches/dxmt) para o mingw-w64 e o Metal Toolchain atuais. O `winemetal.dll` também vai
  em `lib/wine` (como no CrossOver): o Wine só acha uma DLL *builtin* que não conhece pelo arquivo dela
  em `system32`, e o `wineboot` só cria esse arquivo para o que está em `lib/wine`.
- `lib/dxvk/{x86_64-windows,i386-windows}`: DXVK 1.10.3 das fontes do CrossOver (`d3d9`, `d3d10`,
  `d3d10_1`, `d3d10core`, `d3d11`; o `dxgi` fica o do Wine, como no CrossOver), marcado como *builtin*,
  com [`patches/dxvk`](patches/dxvk): compilar com o mingw-w64 atual e pedir ao Vulkan só os recursos
  que o MoltenVK oficial tem (o CrossOver usa um MoltenVK próprio que finge ter geometry shaders e cull
  distance). Só em processos de 64 bits: em 32 bits, sem `VK_EXT_map_memory_placed`, o `win32u` cria buffers
  com memória do host e o MoltenVK recusa (o oficial e o da CodeWeavers); esses processos caem no DXMT.
- Vulkan vai direto ao `libMoltenVK.dylib` do pacote oficial (como no CrossOver, sem o carregador Vulkan:
  o `win32u` da CodeWeavers não pede ao carregador os drivers de "portabilidade").
- `share/winhub-engine/`: `engine.json` (versões e `features`), `compatdb.json` (regras embutidas,
  de [`data/compatdb.json`](data/compatdb.json)) e licenças.

O D3DMetal (Game Porting Toolkit da Apple) **não** vem no motor. Ele só é usado se o usuário o tiver
instalado (`WINHUB_D3DMETAL_DIR`).

## Como compilar

Precisa de macOS em Apple silicon (com Rosetta 2) ou Intel, Xcode 16 ou mais novo (no Xcode 26+, também
o Metal Toolchain: `xcodebuild -downloadComponent MetalToolchain`) e
`brew install mingw-w64 bison meson ninja freetype gnutls sdl2 glslang cmake`.

```bash
./scripts/test-compatdb.sh      # testes do winhubcompat (rápido, sem Wine)
./scripts/build-llvm.sh         # LLVM 15 x86_64 para o conversor de shaders do DXMT (demorado; o CI guarda em cache)
./scripts/build-wine.sh         # Wine inteiro (x86_64 + WoW64) com patches/wine (demorado)
./scripts/build-winhubcompat.sh
./scripts/build-dxmt.sh
./scripts/build-dxvk.sh
./scripts/build-cabextract.sh
./scripts/package.sh            # gera work/dist
./scripts/smoke-test.sh         # instala como o WinHub e testa D3D11 via DXMT e DXVK (64 e 32 bits)
```

Os arquivos intermediários ficam em `work/` (fora do Git). Use `WORK=/outro/lugar` para mudar.

## Publicar uma versão

1. Ajuste `versions.env` (e aumente `ENGINE_REVISION` se algo mudou).
2. Faça o push de uma tag `wine-<WINE_VERSION>-r<ENGINE_REVISION>`, por exemplo `wine-11.0-r2`.
3. O CI compila, testa e cria a release com os arquivos e o `winhub-manifests.json`.
4. Só ponha a entrada nova no `runtimes.json` do WinHub depois de publicar uma versão do app que
   entenda `overlay.replace` e a feature `backend-dispatch`: versões antigas desempacotariam o motor
   por cima do Wine oficial sem apagar o `lib/wine` dele.

## Regras

- Fontes abertas e públicas: WineHQ, as fontes LGPL do Wine do CrossOver publicadas pela CodeWeavers,
  DXMT, DXVK, cJSON, cabextract, LLVM. Cada release publica o código-fonte correspondente, como a LGPL
  exige. Nada do que a CodeWeavers não publica como código aberto (o `cxcompatdb.so`, o banco
  `compatdb-*.dat`, a interface) é usado.
- O D3DMetal da Apple não é redistribuído.
- Nenhum binário no Git.
- Licenças: scripts sob MIT ([LICENSE](LICENSE)); `src/winhubcompat` e patches do Wine sob LGPL-2.1,
  como o Wine; patches do DXMT e do DXVK sob as licenças deles; demais componentes em
  [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).
