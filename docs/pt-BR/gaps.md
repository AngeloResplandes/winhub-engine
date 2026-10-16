[English](../gaps.md) · **Português (Brasil)**

# Lacunas

> **Última revisão:** 2026-10-16

Faltas conhecidas do motor, em ordem de quantos jogos elas bloqueiam. Cada entrada diz o que falta, o que isso
bloqueia e o caminho provável. Números de paridade: [Paridade com o CrossOver](crossover-parity.md).

| # | Lacuna | Bloqueia | Caminho |
|---|---|---|---|
| 1 | **Direct3D 12 sem D3DMetal** | jogos só D3D12 (The Witcher 3 na versão atual, Resident Evil 4 Remake, Hogwarts Legacy) para quem não tem o Game Porting Toolkit | medido em 2026-10-16 ([decisão](decisions.md#2026-10-16-o-direct3d-12-continua-com-o-d3dmetal)): o D3D12 do DXMT é inicial (feature level 11_1, só shaders DXBC, sem DXIL, geometria nem tesselação) e o vkd3d-proton é barrado pelo MoltenVK, então o Direct3D 12 jogável precisa do D3DMetal: guiar o usuário até o Game Porting Toolkit (o próximo passo do WinHub) |
| 2 | **Vídeos em jogos Unity com Direct3D 11** (Hollow Knight) | os vídeos de abertura e as cenas ficam pretos, embora sejam decodificados: o Unity passa cada quadro de um device de vídeo para o device principal por texturas compartilhadas | o DXVK-macOS não tem recursos compartilhados (o `GetSharedHandle` falha); o DXMT v0.80 sobrescreve a textura compartilhada com o preenchimento de zeros adiado e devolve handle nulo para texturas que o Unity consulta. Corrigir o DXMT (upstream ou um patch pequeno) e testar de novo |
| 3 | **O Automático bloqueia programas Direct3D 12** (revisão 5) | um programa D3D12 não cria dispositivo com o `dxgi` do DXMT (ele não responde à interface de adaptador do vkd3d), nem com o vkd3d do próprio Wine em 11_0, a menos que o D3DMetal esteja ligado | avaliar para a revisão 6: (a) o DXMT mais novo, que resolve e traz D3D12 básico no 11_1, mas troca todo o caminho de Direct3D 11; ou (b) uma correção estreita: um patch pequeno no `dxgi` do DXMT para o vkd3d receber o adaptador, ou o `winhubcompat` não pôr o `dxgi` do DXMT na frente nos programas D3D12 |
| 4 | **Banco de regras pequeno** (2 regras: Dark Souls Remastered e Skyrim Special Edition → DXVK) | todo jogo que precisa de um backend, uma variável ou um argumento para abrir | acrescentar regras conforme os jogos são testados ([relatórios de compatibilidade](https://github.com/AngeloResplandes/winhub/tree/main/docs/pt-BR/compat) do WinHub); assinar e publicar ([Publicação](releasing.md#regras-de-compatibilidade)) |
| 5 | **DXVK em processos de 32 bits** | jogos de 32 bits que rodam melhor no DXVK que no DXMT (o DXMT cobre a maioria) | suporte do MoltenVK ao `VK_EXT_map_memory_placed` ou a buffers com memória do host; até lá, a troca para o DXMT |
| 6 | **Só x86_64** | desempenho e o futuro: o suporte geral do Rosetta 2 acaba no macOS 28 | acompanhar o trabalho do Wine oficial em ARM64 para macOS |
| 7 | **msync sem medição** | saber o ganho | as medições de [msync](msync.md#o-que-falta-medir) |
| 8 | **Outros formatos de vídeo** (WMV/WMA, MPEG-1, Theora) | cenas e vídeos de abertura nesses formatos não tocam; a revisão 5 só decodifica H.264 e AAC (MP4) | o macOS não tem decodificador para eles. O Theora é livre e poderia entrar com libtheora e libogg; WMV/WMA e MPEG-1 precisariam dos decodificadores do FFmpeg (gst-libav), que o projeto descartou até agora: [decisão](decisions.md#2026-10-15-vídeos-dos-jogos-pelo-gstreamer-com-o-videotoolbox) |
