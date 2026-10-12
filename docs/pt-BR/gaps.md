[English](../gaps.md) · **Português (Brasil)**

# Lacunas

> **Última revisão:** 2026-10-12

Faltas conhecidas do motor, em ordem de quantos jogos elas bloqueiam. Cada entrada diz o que falta, o que isso
bloqueia e o caminho provável. Números de paridade: [Paridade com o CrossOver](crossover-parity.md).

| # | Lacuna | Bloqueia | Caminho |
|---|---|---|---|
| 1 | **Direct3D 12 sem D3DMetal** | jogos só D3D12 (The Witcher 3 na versão atual, Resident Evil 4 Remake, Hogwarts Legacy) para quem não tem o Game Porting Toolkit | o D3D12 experimental do DXMT (LGPL depois da v0.80); vkd3d-proton sobre o MoltenVK; ou guiar o usuário até o Game Porting Toolkit |
| 2 | **Banco de regras vazio** | todo jogo que precisa de um backend, uma variável ou um argumento para abrir | acrescentar regras conforme os jogos são testados ([relatórios de compatibilidade](https://github.com/AngeloResplandes/winhub/tree/main/docs/pt-BR/compat) do WinHub); assinar e publicar ([Publicação](releasing.md#regras-de-compatibilidade)) |
| 3 | **DXVK em processos de 32 bits** | jogos de 32 bits que rodam melhor no DXVK que no DXMT (o DXMT cobre a maioria) | suporte do MoltenVK ao `VK_EXT_map_memory_placed` ou a buffers com memória do host; até lá, a troca para o DXMT |
| 4 | **Vídeos dos jogos** (GStreamer, H.264) | cenas e vídeos de abertura de muitos jogos | compilar o GStreamer e os plug-ins para x86_64 no overlay (o pacote base não tem) |
| 5 | **`nvapi64` com o DXMT** | DLSS e caminhos específicos da NVIDIA sem o D3DMetal | feito na revisão 3 (`nvapi64`/`nvngx` do DXMT, ligados com `DXMT_ENABLE_NVEXT=1`; veja [DLSS](compatdb.md#d3dmetal-e-dlss)); sai desta lista quando o r3 for publicado |
| 6 | **Regras de troca do caminho do executável** | os poucos jogos cujo lançador precisa ser trocado | feito na revisão 3 (`replace_exe_path`, veja [Regras de compatibilidade](compatdb.md#formato)); sai desta lista quando o r3 for publicado |
| 7 | **Só x86_64** | desempenho e o futuro: o suporte geral do Rosetta 2 acaba no macOS 28 | acompanhar o trabalho do Wine oficial em ARM64 para macOS |
| 8 | **msync sem medição** | saber o ganho | as medições de [msync](msync.md#o-que-falta-medir) |
