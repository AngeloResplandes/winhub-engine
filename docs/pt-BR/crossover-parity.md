[English](../crossover-parity.md) · **Português (Brasil)**

# Paridade com o motor do CrossOver

> **Última revisão:** 2026-10-13 · **Motor:** revisão 3 (`wine-11.0-r3`) · **Alvo:** o motor que
> vem no CrossOver 26.3.0 para macOS

O quanto o winhub-engine se aproxima do motor que o CrossOver traz, dimensão por dimensão. Números "medidos" vêm
de testes deste repositório; os "estimados" são avaliação, com o motivo ao lado. O que o CrossOver faz está em
[Engenharia reversa](reverse-engineering.md); a lista de trabalho está em [Lacunas](gaps.md).

## Resumo

| Número | Valor | Tipo |
|---|---|---|
| **Paridade do motor** (ponderada, abaixo) | **~71%** | estimado |
| Direct3D 10/11 em Apple silicon | DXMT 64/32 bits e DXVK 64 bits chegam ao feature level 11_0 | medido (teste de fumaça) |
| Direct3D 12 | só com o D3DMetal do próprio usuário | medido com um Game Porting Toolkit local |
| Jogo real | Hollow Knight (Unity 6, D3D11) roda pelo DXMT no feature level 11.1, com a Steam e o login salvo | medido em 2026-10-12 |
| Jogos de verdade, jogados por script | Dark Souls Remastered, Hollow Knight, Lethal Company e PEAK jogáveis com o D3DMetal; o Dark Souls também com o DXVK (o DXMT trava nele) | medido em 2026-10-13 ([relatórios do WinHub](https://github.com/AngeloResplandes/winhub/tree/main/docs/pt-BR/compat)) |

## Por dimensão

| Dimensão | Peso | CrossOver 26.3 | winhub-engine r3 | % | Tipo |
|---|---:|---|---|---:|---|
| Núcleo do Wine, WoW64, Rosetta | 25% | Wine 11.0 + patches da CodeWeavers | a mesma árvore, mais três patches pequenos | 100% | mesmas fontes |
| Sincronização (msync) | 10% | sim | sim, o mesmo código | 100% | mesmas fontes |
| Escolha do backend por processo | 10% | módulo fechado, regras cifradas | `winhubcompat`, regras abertas, mesmas decisões e ordem, inclusive a troca do caminho do executável | 100% | estimado |
| Direct3D 10/11 | 20% | DXMT v0.72+, DXVK 1.10.3 (com o MoltenVK deles), D3DMetal | DXMT v0.80 64/32 bits, DXVK 1.10.3 só 64 bits, D3DMetal se o usuário tiver | 85% | casos medidos, peso estimado |
| Direct3D 12 | 15% | D3DMetal 3.0 incluído | D3DMetal só do Game Porting Toolkit do usuário; vkd3d do Wine nos outros casos | 30% | estimado |
| Regras de compatibilidade (conteúdo) | 10% | banco curado por jogo, atualizado online | o formato e o canal de atualização assinado existem; 1 regra | 5% | medido (1 regra) |
| Vídeos dos jogos (GStreamer, H.264) | 5% | GStreamer e plug-ins incluídos | não compilados; o pacote oficial não tem decodificador H.264 | 0% | medido (erros no log) |
| DLSS / APIs da NVIDIA | 5% | `nvngx`/`nvapi64` no DXMT e no D3DMetal, redirecionamento do `nvngx` | redirecionamento do `nvngx` com o D3DMetal; `nvapi64`/`nvngx` do DXMT, ligados por garrafa; ainda não testado num jogo com DLSS | 80% | estimado |
| **Ponderado** | 100% | | | **~71%** | estimado |

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
