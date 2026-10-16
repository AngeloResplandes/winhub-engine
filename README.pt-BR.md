[English](README.md) · **Português (Brasil)**

<h1 align="center">winhub-engine</h1>

<p align="center">
  <b>O motor Wine por trás do <a href="https://github.com/AngeloResplandes/winhub">WinHub</a>: Wine 11 das fontes LGPL do
  CrossOver, DXMT e DXVK, e um módulo aberto que escolhe o Direct3D de cada processo.</b>
</p>

<p align="center">
  <a href="https://angeloresplandes.github.io/winhub-engine/pt-BR/">Documentação</a> ·
  <a href="ROADMAP.pt-BR.md">Roadmap</a> ·
  <a href="docs/pt-BR/reverse-engineering.md">Como o CrossOver foi estudado</a> ·
  <a href="docs/pt-BR/crossover-parity.md">Paridade com o CrossOver</a> ·
  <a href="https://github.com/AngeloResplandes/winhub-engine/releases">Versões</a>
</p>

O winhub-engine reconstrói, com fontes abertas, a arquitetura que o CrossOver 26.3 usa para rodar jogos do
Windows no Mac. Ele foi desenhado estudando o CrossOver **pelo comportamento** (como o pacote é organizado e como
ele decide que Direct3D um jogo recebe) e depois construído a partir da árvore do Wine que a CodeWeavers publica
sob a LGPL, do DXMT, do DXVK e do nosso `winhubcompat`. Nenhum código ou dado fechado do CrossOver é usado.

Este repositório não guarda binários. O CI compila tudo a partir de fontes fixadas e publica o resultado como
versões no GitHub, que o WinHub baixa e confere.

## O que vem nele

| | |
|---|---|
| **Wine 11.0 das fontes do CrossOver** | msync, WoW64 (programas de 32 bits dentro de processos de 64 bits, sem bibliotecas de 32 bits do macOS) e os ganchos do `winemac.drv` que o DXMT e o D3DMetal precisam |
| **Direct3D 10/11 sobre Metal** | [DXMT](https://github.com/3Shain/dxmt) v0.80 em processos de 64 e 32 bits |
| **Direct3D 9/10/11 sobre Vulkan** | DXVK 1.10.3 das fontes do CrossOver sobre o MoltenVK oficial, em processos de 64 bits |
| **Direct3D 11/12 com o D3DMetal** | quando o usuário tem o Game Porting Toolkit da Apple; nunca distribuído pelo projeto |
| **Escolha do backend por processo** | o `winhubcompat` põe o backend escolhido no começo do caminho de DLLs do Wine: nada é copiado para as garrafas |
| **Regras de compatibilidade abertas** | por jogo, por garrafa ou de um banco assinado: backend, ambiente, overrides de DLL, argumentos |

## Como funciona

```
WinHub ── WINHUB_GRAPHICS_BACKEND=auto|wined3d|dxmt|dxvk|d3dmetal ──► wine
                                                                         │
ntdll.so (start_main_thread) ── dlopen ──► winhubcompat.so ◄── regras (embutidas, baixadas, da garrafa)
                                              │
                escolhe o backend que pode rodar aqui (macOS 14+, Rosetta, 64 bits…)
                                              │
                prepend_dll_path(lib/dxmt | lib/dxvk | o D3DMetal do usuário)
                                              │
            o d3d11.dll do backend é achado antes do do Wine; a garrafa fica intocada
```

Detalhes: [Arquitetura](docs/pt-BR/architecture.md) e [winhubcompat](docs/pt-BR/compatdb.md).

## Estado

| | |
|---|---|
| Revisão do motor | 5, versão [`wine-11.0-r5`](https://github.com/AngeloResplandes/winhub-engine/releases/tag/wine-11.0-r5), o motor padrão do WinHub |
| Paridade com o motor do CrossOver | **~73%** (estimado, [detalhes](docs/pt-BR/crossover-parity.md)) |
| Testado | DXMT 64/32 bits e DXVK 64 bits no feature level 11_0 do D3D (teste de fumaça); vídeo MP4 H.264/AAC pelo Media Foundation (teste de fumaça); Hollow Knight pelo DXMT com a Steam (2026-10-12) |
| Maiores lacunas | D3D12 sem D3DMetal, banco de regras pequeno (2 regras), vídeos em jogos Unity com Direct3D 11 ([todas as lacunas](docs/pt-BR/gaps.md)) |

## Compilação

macOS em Apple silicon (com Rosetta 2) ou Intel, Xcode 16+ (a partir do Xcode 26, também
`xcodebuild -downloadComponent MetalToolchain`), e:

```bash
brew install mingw-w64 bison meson ninja freetype gnutls sdl2 glslang cmake pkgconf gettext
```

```bash
./scripts/build-llvm.sh && ./scripts/build-gstreamer.sh && ./scripts/build-wine.sh && ./scripts/build-winhubcompat.sh
./scripts/build-dxmt.sh && ./scripts/build-dxvk.sh && ./scripts/build-cabextract.sh
./scripts/package.sh && ./scripts/smoke-test.sh
```

Passo a passo, com as armadilhas já resolvidas: [Compilação](docs/pt-BR/building.md). Versões:
[Publicação](docs/pt-BR/releasing.md).

## Documentação

A documentação completa é um livro, em inglês e português do Brasil:
**[angeloresplandes.github.io/winhub-engine/pt-BR](https://angeloresplandes.github.io/winhub-engine/pt-BR/)**
(fontes em [`docs/pt-BR/`](docs/pt-BR/)).

## Regras

- Só fontes abertas e públicas: WineHQ, as fontes LGPL do Wine do CrossOver publicadas pela CodeWeavers, DXMT,
  DXVK, cJSON, cabextract, LLVM. Toda versão leva as fontes correspondentes, como a LGPL exige.
- O CrossOver foi estudado só pelo comportamento; as partes fechadas dele (`cxcompatdb.so`, o banco
  `compatdb-*.dat`, o app) nunca são usadas. Veja [Engenharia reversa](docs/pt-BR/reverse-engineering.md) e
  [CONTRIBUTING](CONTRIBUTING.pt-BR.md).
- O D3DMetal da Apple nunca é redistribuído. Nada de binários no Git.

## Licença

Scripts sob MIT ([LICENSE](LICENSE)); `src/winhubcompat` e os patches do Wine sob LGPL-2.1-or-later, como o
Wine; patches do DXMT e do DXVK sob as licenças desses projetos. O resto: [THIRD_PARTY_NOTICES](THIRD_PARTY_NOTICES.pt-BR.md).

---

**Projeto independente.** Sem vínculo com a CodeWeavers, a Apple, a Microsoft ou a Valve. CrossOver é marca da
CodeWeavers; o nome é usado só para dizer de onde vêm as fontes do Wine e o que foi estudado.
