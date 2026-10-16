[English](../building.md) · **Português (Brasil)**

# Compilação

## Requisitos

- Um Mac com Apple silicon (com o Rosetta 2) ou Intel. Tudo é compilado como x86_64.
- Xcode 16 ou mais novo. A partir do Xcode 26, o compilador Metal é um componente à parte:
  `xcodebuild -downloadComponent MetalToolchain` (cerca de 840 MB, uma vez).
- Pacotes do Homebrew:

```bash
brew install mingw-w64 bison meson ninja freetype gnutls sdl2 glslang cmake pkgconf gettext
```

- `/usr/bin/python3` com `distutils`: o Python 3.9 da Apple, que vem com o Xcode ou com as Command Line Tools, tem.
  Os geradores de código do GLib 2.78 precisam dele, e o Python do Homebrew (3.12 em diante) não tem. O Xcode dos
  runners `macos-15` do GitHub basta; lá não há nada a instalar.

O Homebrew só fornece ferramentas e **cabeçalhos** (o `gettext` só para o `libintl.h`). O build do Wine é ligado
às bibliotecas x86_64 do pacote oficial do WineHQ, que o `build-wine.sh` baixa e confere, e o GStreamer é
compilado pelo `build-gstreamer.sh`.

## Passos

Rode nesta ordem, a partir da raiz do repositório. Os arquivos intermediários vão para `work/` (fora do Git);
use `WORK=/outro/lugar` para mudar e `JOBS=n` para limitar os processos paralelos.

| Script | O que faz | Tempo (Mac série M) |
|---|---|---|
| `./scripts/test-compatdb.sh` | Testes unitários do motor de regras (nativo, sem Wine) | segundos |
| `./scripts/build-llvm.sh` | Bibliotecas do LLVM 15 x86_64 para o conversor de shaders do DXMT (o CI guarda em cache) | demorado |
| `./scripts/build-gstreamer.sh` | GLib e GStreamer (core, base, good, bad com VideoToolbox) em x86_64 para os vídeos dos jogos; **antes** do `build-wine.sh` (o CI guarda em cache) | ~3 min |
| `./scripts/build-wine.sh` | O Wine inteiro das fontes LGPL do CrossOver, com `patches/wine`, inclusive o `winegstreamer` | ~40–60 min |
| `./scripts/build-winhubcompat.sh` | O `winhubcompat.so`, contra a árvore de build do Wine | segundos |
| `./scripts/build-dxmt.sh` | O DXMT v0.80 como DLLs builtin do Wine, com `patches/dxmt` | minutos |
| `./scripts/build-dxvk.sh` | O DXVK 1.10.3 das fontes do CrossOver, com `patches/dxvk`, marcado como builtin | minutos |
| `./scripts/build-cabextract.sh` | O `cabextract` universal para o winetricks | segundos |
| `./scripts/package.sh` | `work/dist/`: overlay, fontes, `SHA256SUMS`, `winhub-manifests.json` | ~2 min |
| `./scripts/smoke-test.sh` | Instala como o WinHub; testa D3D11 pelo DXMT e pelo DXVK e decodifica um vídeo H.264/AAC | minutos |

## O que o `build-gstreamer.sh` faz, e por quê

O Wine toca os vídeos dos jogos pelo Media Foundation → `winegstreamer` → GStreamer. O pacote oficial não tem
GStreamer, então o motor o compila e o leva junto. H.264 e AAC são decodificados pelo próprio macOS (`vtdec` e
`atdec`, do `applemedia` do gst-plugins-bad); não há FFmpeg, x264 nem plug-ins GPL (`-Dgpl=disabled`).

1. Baixa as fontes do CrossOver (`sources/glib`, `sources/gstreamer`) e o pcre2 10.42 com o wrap do Meson, ambos
   fixados por SHA-256, e põe os arquivos do pcre2 no `packagecache` do GLib (o Meson roda com `--wrap-mode
   nodownload`, então nunca baixa nada).
2. Baixa o gvdb no commit fixado em `versions.env` para `work/gvdb` e o copia para os subprojetos do GLib.
3. Prepara o `work/gst-deps`: a libintl copiada do pacote base com install name `@rpath`, e um `libffi.pc` para a
   libffi do SDK.
4. Escreve o `work/gst-cross.ini`, um cross file do Meson para x86_64, com alvo de implantação 10.15.
5. Compila o GLib e depois o core, o plugins-base, o plugins-good e o plugins-bad do GStreamer 1.24.4, todos com
   `-Dauto_features=disabled` e só as opções que o decodebin do winegstreamer usa, em `work/gstreamer-install`.
   Depois de cada instalação, dá a toda biblioteca um install name `@rpath/<arquivo>`.
6. Confere o resultado: 14 plug-ins em `lib/gstreamer-1.0` (x86_64, com rpath `@loader_path/..`), símbolos
   `vtdec` e `atdec` no `applemedia`, e nenhuma dependência nem rpath para a pasta de build ou o Homebrew.

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
5. Configura com `--enable-archs=i386,x86_64` (WoW64) e rpaths que chegam ao `lib/` do pacote oficial, e com
   `--with-gstreamer` contra o `work/gstreamer-install` (então para se o `build-gstreamer.sh` não rodou). Confere
   se os sonames saíram certos e se o `winegstreamer.so` liga o GStreamer só por `@rpath`, compila e instala em
   `work/wine-install`.

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
| GLib: `Subproject exists but has no meson.build file` (gvdb) | o pacote do CrossOver traz `glib/subprojects/gvdb` vazio (é um subprojeto git) | baixar o gvdb no commit fixado em `versions.env`, conferir o `HEAD` e copiá-lo antes do `meson setup` |
| GLib e GStreamer saem arm64 | o Meson e o Python do Homebrew rodam como arm64 | cross file (`work/gst-cross.ini`) para x86_64; o Rosetta roda as verificações do configure |
| `Run-time dependency intl found: NO` | a verificação `intl` do Meson compila com o compilador C++, que os `c_args` não alcançam | as flags de include e de biblioteca são `[constants]` do cross file, usadas em C, C++, Objective-C e Objective-C++ |
| libffi não encontrada | o pacote base não tem | a libffi do SDK, pelo `work/gst-deps/pkgconfig/libffi.pc` |
| Uma biblioteca pede a `libintl.dylib` sem versão | a libintl do pacote base tem o install name `libintl.dylib`, que o dyld não resolve | ligar contra uma cópia com id `@rpath/libintl.8.dylib` em `work/gst-deps/lib`; em tempo de execução o `@loader_path` acha o arquivo do próprio pacote base em `lib/` |
| `gdbus-codegen`: `No module named 'distutils'` | o Meson roda o Python do Homebrew, que perdeu o `distutils` (3.12) | `python = '/usr/bin/python3'` no cross file; os scripts instalados do GLib também o recebem |
| `'pipe2' is only available on macOS 27.0 or newer` | o `has_function` do Meson só confere se o símbolo liga, então o GLib chamaria `pipe2` no macOS 10.15 a 26 | `has_function_pipe2 = false` no cross file; `-Werror=unguarded-availability-new` faz qualquer outro uso sem guarda de API mais nova virar erro de compilação |
| Bibliotecas com install names e rpaths absolutos | o `meson install` escreve caminhos de `work/…` | `rpath_names` depois de cada instalação: ids e referências `@rpath/<arquivo>`, rpaths de build apagados, `@loader_path/..` nos plug-ins |
| `Program 'glib-mkenums' not found` (core do GStreamer) | o Meson o procura por um GLib da máquina de build que não existe | `glib-mkenums = ['/usr/bin/python3', '<prefix>/bin/glib-mkenums']` no cross file |
| Core do GStreamer: bison velho demais | o `gst/parse` pede bison 2.4 ou mais novo, e o macOS tem o 2.3 | `-Dgst_parse=false`; o winegstreamer nunca chama o parser |
| `Unknown option introspection` / `tools` no plugins-good | o gst-plugins-good 1.24 não tem essas opções | passá-las só ao base e ao bad |
| O applemedia não compila | as fontes de GL dele precisam do gstreamer-gl, e `auto_features=disabled` desliga o `gl` do bad | `-Dgl=enabled` no bad (e no base, com as opções CGL/Cocoa) |
| `External plugin loader failed … GST_PLUGIN_SCANNER` | nenhum `gst-plugin-scanner` é compilado (`-Dtools=disabled`) | não precisa: o winegstreamer inicia o GStreamer com `--gst-disable-registry-fork`, então ele varre no próprio processo e não imprime nada |
| O `winegstreamer.so` leva um rpath para `work/gstreamer-install` | o `gstreamer-1.0.pc` tem `-Wl,-rpath,${libdir}` | o `build-wine.sh` o tira de `GSTREAMER_LIBS` e confere o resultado |
| O `winegstreamer.so` pede a `libintl.dylib` sem versão | o `-lintl` do `glib-2.0.pc` acha a cópia do pacote base em `work/deps/lib` | `-L$WORK/gst-deps/lib` primeiro em `GSTREAMER_LIBS`; o `build-wine.sh` recusa qualquer dependência que não seja `@rpath/`, `/usr/lib/` ou `/System/Library/` |
| O `configure` não consegue usar o GStreamer: `libffi` | o `gobject-2.0.pc` exige o `libffi.pc` | o `PKG_CONFIG_LIBDIR` também inclui `work/gst-deps/pkgconfig` |
| O `strip` invalida as assinaturas das bibliotecas do GStreamer | o `build-gstreamer.sh` as assina ad hoc | `package.sh`: remover a assinatura, `strip -x`, assinar de novo |
| O `vtdec` parece faltar no log do teste de fumaça | o `WINE:4` registra nomes longos ("Apple VideoToolbox decoder (hardware only)") | também `GST_ELEMENT_FACTORY:4`, que registra `creating element "vtdec_hw"` |

## Integração contínua

O `.github/workflows/build.yml` roda em pushes na `main`, em pull requests e em tags `wine-*` (mudanças só de
documentação são puladas): testes unitários, conferência da assinatura do banco de regras, LLVM (em cache), GLib
e GStreamer (em cache), Wine, winhubcompat, DXMT, DXVK, cabextract, empacotamento e teste de fumaça. O cache do
GStreamer guarda tudo o que os passos seguintes leem (`gstreamer-install`, `gst-src`, `gst-deps`, `gvdb`, os
downloads do pcre2) e a chave dele é `CROSSOVER_VERSION`, `PCRE2_SHA256`, `GVDB_COMMIT` e um hash do
`build-gstreamer.sh`. Uma tag também publica a versão, veja [Publicação](releasing.md).
