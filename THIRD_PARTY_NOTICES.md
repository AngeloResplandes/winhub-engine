# Componentes de terceiros

Os scripts deste repositório são MIT (veja `LICENSE`). Os arquivos publicados nas Releases são
compilados pelo CI a partir das fontes abaixo, nas versões fixadas em `versions.env`.

| Componente | Onde entra | Licença | Código-fonte |
|---|---|---|---|
| Wine (`winemac.so`, com os patches de `patches/`) | Overlay | LGPL-2.1-or-later | https://gitlab.winehq.org/wine/wine · o `wine-<versão>.tar.xz` exato e os patches vão em cada release |
| Pacote oficial do Wine para macOS (base sobre a qual o overlay é aplicado) | Baixado pelo WinHub, não redistribuído aqui | LGPL-2.1 e as licenças das bibliotecas incluídas | https://github.com/Gcenx/macOS_Wine_builds |
| DXMT v0.80 (`d3d11`, `dxgi`, `d3d10core`, `winemetal`) | Overlay | MIT (v0.80 é a última versão MIT; as seguintes são LGPL) | https://github.com/3Shain/dxmt |
| LLVM 15 (bibliotecas ligadas estaticamente ao `winemetal.so`) | Overlay | Apache-2.0 with LLVM Exceptions | https://github.com/llvm/llvm-project |
| DirectX headers para mingw (usados só na compilação do DXMT) | — | MIT | https://github.com/misyltoad/mingw-directx-headers |
| cabextract (com libmspack embutida) | `cabextract-<versão>-macos.tar.xz` | GPL-3.0-or-later (libmspack: LGPL-2.1) | https://www.cabextract.org.uk · o `cabextract-<versão>.tar.gz` exato vai em cada release |
| Runtime do mingw-w64 e libgcc (nas DLLs do DXMT) | Overlay | ZPL-2.1 / domínio público; GPL-3.0 com GCC Runtime Library Exception | https://www.mingw-w64.org · https://gcc.gnu.org |

O D3DMetal (Game Porting Toolkit da Apple) **não** é usado nem distribuído.

Windows e Direct3D são marcas da Microsoft. Mac, macOS e Metal são marcas da Apple. Os nomes aparecem
apenas para descrever compatibilidade.
