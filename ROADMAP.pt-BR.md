[English](ROADMAP.md) · **Português (Brasil)**

# Roadmap do winhub-engine

**Estágio: revisão 2 na `main`**, versão `wine-11.0-r2` pendente

> **Última revisão:** 2026-10-12 · **Alvo:** o motor que vem no CrossOver 26.3.0 para macOS

Resumo em uma página. Detalhes: [paridade com o CrossOver](docs/pt-BR/crossover-parity.md) (pesos e evidências),
[lacunas](docs/pt-BR/gaps.md) (a lista de trabalho em ordem), [decisões](docs/pt-BR/decisions.md).

## Números principais

| | Valor | Tipo |
|---|---|---|
| **Paridade do motor com o CrossOver** | **~68%** | estimado, ponderado por dimensão |
| Direct3D 10/11 | DXMT 64/32 bits e DXVK 64 bits no feature level 11_0 | medido |
| Direct3D 12 | só com o D3DMetal do usuário | medido |
| Regras de compatibilidade | formato, assinatura e canal de atualização prontos; 0 regras | medido |

## Feito

- Revisão 1 (2026-10-11): pacote oficial do WineHQ + `winemac.so` modificado + DXMT, copiado para as garrafas
  pelo WinHub.
- Revisão 2 (2026-10-12): Wine das fontes LGPL do CrossOver, `winhubcompat`, DXMT e DXVK como builtins escolhidas
  por processo, msync, D3DMetal do usuário, banco de regras assinado, perfil de usuário real mantido.

## Próximos passos

1. **Publicar a `wine-11.0-r2`** e acrescentá-la ao `runtimes.json` do WinHub ([ordem](docs/pt-BR/releasing.md#ordem-com-o-winhub)).
2. **Primeiras regras:** uma por jogo testado nos relatórios de compatibilidade do WinHub.
3. **Vídeos dos jogos:** GStreamer e H.264 no overlay.
4. **Direct3D 12 sem D3DMetal:** avaliar o D3D12 do DXMT e o vkd3d-proton.
5. **DXVK em 32 bits:** acompanhar o suporte do MoltenVK a mapeamentos de memória posicionados.
6. **Números do msync:** medir um jogo limitado pela CPU.

## Histórico de revisões

| Data | Mudança |
|---|---|
| 2026-10-12 | Primeiro roadmap, revisão 2 do motor. |
