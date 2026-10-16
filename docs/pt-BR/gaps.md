[English](../gaps.md) · **Português (Brasil)**

# Lacunas

> **Última revisão:** 2026-10-16

Faltas conhecidas do motor, em ordem de quantos jogos elas bloqueiam. Cada entrada diz o que falta, o que isso
bloqueia e o caminho provável. Números de paridade: [Paridade com o CrossOver](crossover-parity.md).

| # | Lacuna | Bloqueia | Caminho |
|---|---|---|---|
| 1 | **Direct3D 12 sem D3DMetal** | jogos só D3D12 (The Witcher 3 na versão atual, Resident Evil 4 Remake, Hogwarts Legacy) para quem não tem o Game Porting Toolkit | o D3D12 experimental do DXMT (LGPL depois da v0.80); vkd3d-proton sobre o MoltenVK; ou guiar o usuário até o Game Porting Toolkit |
| 2 | **Vídeos em jogos Unity com Direct3D 11** (Hollow Knight) | os vídeos de abertura e as cenas ficam pretos, embora sejam decodificados: o Unity passa cada quadro de um device de vídeo para o device principal por texturas compartilhadas | o DXVK-macOS não tem recursos compartilhados (o `GetSharedHandle` falha); o DXMT v0.80 sobrescreve a textura compartilhada com o preenchimento de zeros adiado e devolve handle nulo para texturas que o Unity consulta. Corrigir o DXMT (upstream ou um patch pequeno) e testar de novo |
| 3 | **Banco de regras pequeno** (2 regras: Dark Souls Remastered e Skyrim Special Edition → DXVK) | todo jogo que precisa de um backend, uma variável ou um argumento para abrir | acrescentar regras conforme os jogos são testados ([relatórios de compatibilidade](https://github.com/AngeloResplandes/winhub/tree/main/docs/pt-BR/compat) do WinHub); assinar e publicar ([Publicação](releasing.md#regras-de-compatibilidade)) |
| 4 | **DXVK em processos de 32 bits** | jogos de 32 bits que rodam melhor no DXVK que no DXMT (o DXMT cobre a maioria) | suporte do MoltenVK ao `VK_EXT_map_memory_placed` ou a buffers com memória do host; até lá, a troca para o DXMT |
| 5 | **Só x86_64** | desempenho e o futuro: o suporte geral do Rosetta 2 acaba no macOS 28 | acompanhar o trabalho do Wine oficial em ARM64 para macOS |
| 6 | **msync sem medição** | saber o ganho | as medições de [msync](msync.md#o-que-falta-medir) |
| 7 | **Outros formatos de vídeo** (WMV/WMA, MPEG-1, Theora) | cenas e vídeos de abertura nesses formatos não tocam; a revisão 5 só decodifica H.264 e AAC (MP4) | o macOS não tem decodificador para eles. O Theora é livre e poderia entrar com libtheora e libogg; WMV/WMA e MPEG-1 precisariam dos decodificadores do FFmpeg (gst-libav), que o projeto descartou até agora: [decisão](decisions.md#2026-10-15-vídeos-dos-jogos-pelo-gstreamer-com-o-videotoolbox) |
