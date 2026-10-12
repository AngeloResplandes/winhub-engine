[English](../building.md) · **Português (Brasil)**

# Compilação

## Requisitos

- Um Mac com Apple silicon (com o Rosetta 2) ou Intel. Tudo é compilado como x86_64.
- Xcode 16 ou mais novo. A partir do Xcode 26, o compilador Metal é um componente à parte:
  `xcodebuild -downloadComponent MetalToolchain` (cerca de 840 MB, uma vez).
- Pacotes do Homebrew:

```bash
brew install mingw-w64 bison meson ninja freetype gnutls sdl2 glslang cmake
```

O Homebrew só fornece ferramentas e **cabeçalhos**. O build do Wine é ligado às bibliotecas x86_64 do pacote
oficial do WineHQ, que o `build-wine.sh` baixa e confere.

## Passos

Rode nesta ordem, a partir da raiz do repositório. Os arquivos intermediários vão para `work/` (fora do Git);
use `WORK=/outro/lugar` para mudar e `JOBS=n` para limitar os processos paralelos.

| Script | O que faz | Tempo (Mac série M) |
|---|---|---|
| `./scripts/test-compatdb.sh` | Testes unitários do motor de regras (nativo, sem Wine) | segundos |
| `./scripts/build-llvm.sh` | Bibliotecas do LLVM 15 x86_64 para o conversor de shaders do DXMT (o CI guarda em cache) | demorado |
| `./scripts/build-wine.sh` | O Wine inteiro das fontes LGPL do CrossOver, com `patches/wine` | ~40–60 min |
| `./scripts/build-winhubcompat.sh` | O `winhubcompat.so`, contra a árvore de build do Wine | segundos |
| `./scripts/build-dxmt.sh` | O DXMT v0.80 como DLLs builtin do Wine, com `patches/dxmt` | minutos |
| `./scripts/build-dxvk.sh` | O DXVK 1.10.3 das fontes do CrossOver, com `patches/dxvk`, marcado como builtin | minutos |
| `./scripts/build-cabextract.sh` | O `cabextract` universal para o winetricks | segundos |
| `./scripts/package.sh` | `work/dist/`: overlay, fontes, `SHA256SUMS`, `winhub-manifests.json` | ~1 min |
| `./scripts/smoke-test.sh` | Instala como o WinHub e testa D3D11 pelo DXMT e pelo DXVK | minutos |

## O que o `build-wine.sh` faz, e por quê

1. Baixa o `crossover-sources-<versão>.tar.gz` e extrai só o `sources/wine`; aplica `patches/wine`.
2. Extrai o pacote oficial uma vez (`base_wine` em `scripts/common.sh`, acessado pelo link sem espaços
   `work/base-wine`).
3. Copia freetype, gnutls, SDL2 e MoltenVK para `work/deps/lib` com **install names que levam o nome do
   arquivo com versão**, e liga as outras bibliotecas do pacote ao lado delas. O `configure` do Wine tira o nome
   que vai abrir com `dlopen` em tempo de execução (o "soname") do install name da biblioteca contra a qual
   liga; as bibliotecas do pacote oficial têm install names sem versão (`libSDL2.dylib`), o que daria sonames
   errados.
4. Deixa o `libvulkan` de fora de propósito, para o `configure` escolher o `libMoltenVK.dylib` para o Vulkan.
5. Configura com `--enable-archs=i386,x86_64` (WoW64) e rpaths que chegam ao `lib/` do pacote oficial, confere
   se os sonames saíram certos, compila e instala em `work/wine-install`.

## Armadilhas já resolvidas

| Sintoma | Causa | Correção nos scripts |
|---|---|---|
| `C compiler cannot create executables` | o caminho do pacote base tem espaço (`Wine Devel.app`) | link `work/base-wine` |
| Um caminho de cabeçalho vazio em silêncio | `set -u` com um argumento opcional não definido | `${2:-}` em `inc()` |
| `libSDL2 … not found` ou sonames estranhos | install names sem versão no pacote base | passo 3 acima |
| O `sfnt2fon` aborta carregando a `libpng16` | as dylibs copiadas procuram as dependências ao lado delas | ligar todas as dylibs do pacote em `work/deps/lib` |
| O DXVK não acha nenhum dispositivo Vulkan | o carregador Vulkan e o `win32u` da CodeWeavers discordam sobre drivers de portabilidade | Vulkan direto no MoltenVK (passo 4) |
| O cJSON falha com `-Werror` | avisos de `sprintf` obsoleto | cJSON compilado à parte, sem `-Werror` |
| O DXVK 1.10.3 não compila | cabeçalhos novos do GCC/mingw-w64 | `-include cstdint` e `patches/dxvk/0001` |
| A versão do DXVK aparece como `wine-…-r1` | o `git describe` sobe até este repositório | `GIT_CEILING_DIRECTORIES` |
| Um shader do DXMT falha no Xcode 27 | o toolchain Metal mudou uma função interna | `patches/dxmt/0002` |
| Overlay de 290 MB | arquivos do lado Windows compilados com informação de depuração | `strip --strip-unneeded`, sem bibliotecas de importação |

## Integração contínua

O `.github/workflows/build.yml` roda em pushes na `main`, em pull requests e em tags `wine-*` (mudanças só de
documentação são puladas): testes unitários, conferência da assinatura do banco de regras, LLVM (em cache),
Wine, winhubcompat, DXMT, DXVK, cabextract, empacotamento e teste de fumaça. Uma tag também publica a versão,
veja [Publicação](releasing.md).
