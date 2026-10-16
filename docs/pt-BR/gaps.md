[English](../gaps.md) · **Português (Brasil)**

# Lacunas

> **Última revisão:** 2026-10-15

Faltas conhecidas do motor, em ordem de quantos jogos elas bloqueiam. Cada entrada diz o que falta, o que isso
bloqueia e o caminho provável. Números de paridade: [Paridade com o CrossOver](crossover-parity.md).

| # | Lacuna | Bloqueia | Caminho |
|---|---|---|---|
| 1 | **Direct3D 12 sem D3DMetal** | jogos só D3D12 (The Witcher 3 na versão atual, Resident Evil 4 Remake, Hogwarts Legacy) para quem não tem o Game Porting Toolkit | o D3D12 experimental do DXMT (LGPL depois da v0.80); vkd3d-proton sobre o MoltenVK; ou guiar o usuário até o Game Porting Toolkit |
| 2 | **Banco de regras pequeno** (2 regras: Dark Souls Remastered e Skyrim Special Edition → DXVK) | todo jogo que precisa de um backend, uma variável ou um argumento para abrir | acrescentar regras conforme os jogos são testados ([relatórios de compatibilidade](https://github.com/AngeloResplandes/winhub/tree/main/docs/pt-BR/compat) do WinHub); assinar e publicar ([Publicação](releasing.md#regras-de-compatibilidade)) |
| 3 | **DXVK em processos de 32 bits** | jogos de 32 bits que rodam melhor no DXVK que no DXMT (o DXMT cobre a maioria) | suporte do MoltenVK ao `VK_EXT_map_memory_placed` ou a buffers com memória do host; até lá, a troca para o DXMT |
| 4 | **Só x86_64** | desempenho e o futuro: o suporte geral do Rosetta 2 acaba no macOS 28 | acompanhar o trabalho do Wine oficial em ARM64 para macOS |
| 5 | **msync sem medição** | saber o ganho | as medições de [msync](msync.md#o-que-falta-medir) |
| 6 | **Outros formatos de vídeo** (WMV/WMA, MPEG-1, Theora) | cenas e vídeos de abertura nesses formatos não tocam; a revisão 5 só decodifica H.264 e AAC (MP4) | o macOS não tem decodificador para eles. O Theora é livre e poderia entrar com libtheora e libogg; WMV/WMA e MPEG-1 precisariam dos decodificadores do FFmpeg (gst-libav), que o projeto descartou até agora: [decisão](decisions.md#2026-10-15-vídeos-dos-jogos-pelo-gstreamer-com-o-videotoolbox) |
