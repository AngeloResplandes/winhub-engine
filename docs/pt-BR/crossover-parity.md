[English](../crossover-parity.md) · **Português (Brasil)**

# Paridade com o motor do CrossOver

> **Última revisão:** 2026-10-16 · **Motor:** revisão 5 (`wine-11.0-r5`) · **Alvo:** o motor que
> vem no CrossOver 26.3.0 para macOS

O quanto o winhub-engine se aproxima do motor que o CrossOver traz, dimensão por dimensão. Números "medidos" vêm
de testes deste repositório; os "estimados" são avaliação, com o motivo ao lado. O que o CrossOver faz está em
[Engenharia reversa](reverse-engineering.md); a lista de trabalho está em [Lacunas](gaps.md).

## Resumo

| Número | Valor | Tipo |
|---|---|---|
| **Paridade do motor** (ponderada, abaixo) | **~73%** | estimado |
| Direct3D 10/11 em Apple silicon | DXMT 64/32 bits e DXVK 64 bits chegam ao feature level 11_0 | medido (teste de fumaça) |
| Direct3D 12 | jogável só com o D3DMetal do próprio usuário; sem ele, o vkd3d do Wine cria um device no feature level 11_0 sem geometry shaders, stream output, depth clip nem cull distance (não testado com um jogo de verdade), mas o Automático da revisão 5 não cria device sem o D3DMetal; escolher o backend Wine (wined3d) ou o DXVK contorna isso; o D3D12 do DXMT mais novo é FL 11_1 só com DXBC; o vkd3d-proton não roda sobre o MoltenVK | medido com um Game Porting Toolkit local; spike medido em 2026-10-16 ([decisão](decisions.md#2026-10-16-o-direct3d-12-continua-com-o-d3dmetal)) |
| Jogo real | Hollow Knight (Unity 6, D3D11) roda pelo DXMT no feature level 11.1, com a Steam e o login salvo | medido em 2026-10-12 |
| Jogos de verdade, jogados por script | Dark Souls Remastered, Hollow Knight, Lethal Company e PEAK jogáveis com o D3DMetal; o Dark Souls também com o DXVK (o DXMT trava nele) | medido em 2026-10-13 ([relatórios do WinHub](https://github.com/AngeloResplandes/winhub/tree/main/docs/pt-BR/compat)) |

## Por dimensão

| Dimensão | Peso | CrossOver 26.3 | winhub-engine r5 | % | Tipo |
|---|---:|---|---|---:|---|
| Núcleo do Wine, WoW64, Rosetta | 25% | Wine 11.0 + patches da CodeWeavers | a mesma árvore, mais três patches pequenos | 100% | mesmas fontes |
| Sincronização (msync) | 10% | sim | sim, o mesmo código | 100% | mesmas fontes |
| Escolha do backend por processo | 10% | módulo fechado, regras cifradas | `winhubcompat`, regras abertas, mesmas decisões e ordem, inclusive a troca do caminho do executável e o casamento pelo ID da Steam | 100% | estimado |
| Direct3D 10/11 | 20% | DXMT v0.72+, DXVK 1.10.3 (com o MoltenVK deles), D3DMetal | DXMT v0.80 64/32 bits, DXVK 1.10.3 só 64 bits, D3DMetal se o usuário tiver | 85% | casos medidos, peso estimado |
| Direct3D 12 | 15% | D3DMetal 3.0 incluído | D3DMetal só do Game Porting Toolkit do usuário; vkd3d do Wine nos outros casos, que cria um device no feature level 11_0 sem geometry shaders, stream output, depth clip nem cull distance (não testado com um jogo de verdade); no Automático sem o D3DMetal o `dxgi` do DXMT o bloqueia, e o contorno de hoje é escolher o backend Wine (wined3d) ou o DXVK; o D3D12 do próprio DXMT recusa DXIL e tesselação | 25% | estimado; baixou de 30% em 2026-10-16: sem o D3DMetal o caminho padrão não cria device (r1-r4 não foram medidas) |
| Regras de compatibilidade (conteúdo) | 10% | banco curado por jogo, atualizado online | o formato e o canal de atualização assinado existem; 2 regras | 5% | medido (2 regras) |
| Vídeos dos jogos (GStreamer, H.264) | 5% | GStreamer e plug-ins incluídos | GStreamer 1.24.4 compilado para x86_64, com os decodificadores H.264 (VideoToolbox) e AAC (AudioToolbox) do próprio macOS; só MP4, sem WMV/WMA, MPEG-1 nem Theora. O Hollow Knight (Unity 6) decodifica o vídeo do prólogo na revisão 5, mas os quadros ficam pretos: o Unity os passa entre dois devices Direct3D 11 por texturas compartilhadas, que o DXVK-macOS não tem e que o DXMT ainda não repassa direito | 60% | estimado; MP4 H.264/AAC medido no teste de fumaça e no Hollow Knight (decodificação) |
| DLSS / APIs da NVIDIA | 5% | `nvngx`/`nvapi64` no DXMT e no D3DMetal, redirecionamento do `nvngx` | redirecionamento do `nvngx` com o D3DMetal; `nvapi64`/`nvngx` do DXMT, ligados por garrafa; ainda não testado num jogo com DLSS | 80% | estimado |
| **Ponderado** | 100% | | | **~73%** | estimado |

## Diferenças que são escolhas, não lacunas

| | CrossOver | winhub-engine | Por quê |
|---|---|---|---|
| Banco de regras | cifrado, fechado | JSON legível, assinado com Ed25519 | qualquer pessoa pode ler e contribuir regras; o WinHub continua conferindo o que baixa |
| Automático sem D3DMetal | — (o D3DMetal sempre vem junto) | DXMT | o D3DMetal não pode ser redistribuído |
| Regras por garrafa | não | sim (`WINHUB_COMPAT_RULES`) | a escolha de Direct3D por jogo do WinHub |
| Versão do DXMT | v0.72 + 25 commits | v0.80 | a v0.80 é a última versão MIT |
| MoltenVK | o 1.2.10 da CodeWeavers, que diz ter recursos que faltam | o 1.4.0 oficial, do pacote base | aberto e atual; o DXVK é que foi ajustado |
| Nome de usuário e Desktop | `crossover`, pasta Desktop | nome real, Desktop ligado ao Mac | mantém funcionando as garrafas feitas com o Wine oficial |

## Histórico de revisões

| Data | Mudança |
|---|---|
| 2026-10-12 | Primeira avaliação, revisão 2 do motor. |
| 2026-10-13 | Revisão 3: escolha do backend de 90% para 100% (`replace_exe_path`), DLSS de 50% para 80%; total ~71%. |
| 2026-10-15 | Revisão 4: regras `steam_game_id` casam com jogos abertos pela Steam; 2 regras; total ~71%. |
| 2026-10-16 | Revisão 5 publicada: vídeos dos jogos de 0% para 60% (GStreamer com VideoToolbox; MP4 H.264/AAC decodificado no teste de fumaça e no Hollow Knight, cujos quadros ainda ficam pretos pelas texturas compartilhadas do Direct3D 11); total de ~71% para ~74%. |
| 2026-10-16 | Spike de Direct3D 12: nenhum caminho para D3D12 jogável sem o D3DMetal ([decisão](decisions.md#2026-10-16-o-direct3d-12-continua-com-o-d3dmetal)); Direct3D 12 de 30% para 25% porque o Automático bloqueia até o vkd3d do Wine sem o D3DMetal; total de ~74% para ~73%. |
