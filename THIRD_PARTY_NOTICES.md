# Componentes de terceiros

Os scripts deste repositório são MIT (veja `LICENSE`); `src/winhubcompat` e os patches do Wine são
LGPL-2.1-or-later. Os arquivos publicados nas Releases são compilados pelo CI a partir das fontes
abaixo, nas versões fixadas em `versions.env`.

| Componente | Onde entra | Licença | Código-fonte |
|---|---|---|---|
| Wine 11.0 das fontes do CrossOver (`bin/`, `lib/wine/`, `share/wine/`, com os patches de `patches/wine`) | Overlay | LGPL-2.1-or-later | https://www.codeweavers.com/crossover/source · o `crossover-sources-<versão>.tar.gz` exato e os patches vão em cada release |
| winhubcompat (`lib/wine/x86_64-unix/winhubcompat.so`) | Overlay | LGPL-2.1-or-later | `src/winhubcompat` neste repositório |
| cJSON (dentro do `winhubcompat.so`) | Overlay | MIT | https://github.com/DaveGamble/cJSON · o `cJSON-<versão>.tar.gz` exato vai em cada release |
| DXVK 1.10.3 das fontes do CrossOver (`lib/dxvk`, com os patches de `patches/dxvk`) | Overlay | zlib | https://github.com/doitsujin/dxvk · incluído no `crossover-sources-<versão>.tar.gz` |
| Pacote oficial do Wine para macOS (base sobre a qual o overlay é aplicado: bibliotecas, wine-mono, wine-gecko) | Baixado pelo WinHub, não redistribuído aqui | LGPL-2.1 e as licenças das bibliotecas incluídas | https://github.com/Gcenx/macOS_Wine_builds |
| DXMT v0.80 (`d3d11`, `dxgi`, `d3d10core`, `winemetal`) | Overlay | MIT (v0.80 é a última versão MIT; as seguintes são LGPL) | https://github.com/3Shain/dxmt |
| LLVM 15 (bibliotecas ligadas estaticamente ao `winemetal.so`) | Overlay | Apache-2.0 with LLVM Exceptions | https://github.com/llvm/llvm-project |
| DirectX headers para mingw (usados só na compilação do DXMT) | — | MIT | https://github.com/misyltoad/mingw-directx-headers |
| cabextract (com libmspack embutida) | `cabextract-<versão>-macos.tar.xz` | GPL-3.0-or-later (libmspack: LGPL-2.1) | https://www.cabextract.org.uk · o `cabextract-<versão>.tar.gz` exato vai em cada release |
| Runtime do mingw-w64 e libgcc (nas DLLs do DXMT e do DXVK) | Overlay | ZPL-2.1 / domínio público; GPL-3.0 com GCC Runtime Library Exception | https://www.mingw-w64.org · https://gcc.gnu.org |

O D3DMetal (Game Porting Toolkit da Apple) **não** é distribuído. O motor só o usa quando o usuário o
instalou e o WinHub aponta a pasta (`WINHUB_D3DMETAL_DIR`, veja `docs/compatdb.md`).

CrossOver é marca da CodeWeavers. Windows e Direct3D são marcas da Microsoft. Mac, macOS e Metal são
marcas da Apple. Os nomes aparecem apenas para descrever origem e compatibilidade.
