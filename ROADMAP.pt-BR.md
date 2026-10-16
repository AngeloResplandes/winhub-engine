[English](ROADMAP.md) · **Português (Brasil)**

# Roadmap do winhub-engine

**Estágio: revisão 5**, publicada como [`wine-11.0-r5`](https://github.com/AngeloResplandes/winhub-engine/releases/tag/wine-11.0-r5) e motor padrão do WinHub

> **Última revisão:** 2026-10-16 · **Alvo:** o motor que vem no CrossOver 26.3.0 para macOS

Resumo em uma página. Detalhes: [paridade com o CrossOver](docs/pt-BR/crossover-parity.md) (pesos e evidências),
[lacunas](docs/pt-BR/gaps.md) (a lista de trabalho em ordem), [decisões](docs/pt-BR/decisions.md).

## Números principais

| | Valor | Tipo |
|---|---|---|
| **Paridade do motor com o CrossOver** | **~73%** | estimado, ponderado por dimensão |
| Direct3D 10/11 | DXMT 64/32 bits e DXVK 64 bits no feature level 11_0 | medido |
| Direct3D 12 | só com o D3DMetal do usuário | medido |
| Regras de compatibilidade | formato, assinatura e canal de atualização prontos; 2 regras | medido |

## Feito

- Revisão 1 (2026-10-11): pacote oficial do WineHQ + `winemac.so` modificado + DXMT, copiado para as garrafas
  pelo WinHub.
- Revisão 2 (2026-10-12): Wine das fontes LGPL do CrossOver, `winhubcompat`, DXMT e DXVK como builtins escolhidas
  por processo, msync, D3DMetal do usuário, banco de regras assinado, perfil de usuário real mantido. Publicada
  como `wine-11.0-r2` e transformada no motor padrão do WinHub no mesmo dia.
- Revisão 3 (2026-10-13): regras `replace_exe_path`, `nvapi64`/`nvngx` do DXMT (DLSS sobre o MetalFX, ligado por
  garrafa), a primeira regra (Dark Souls Remastered → DXVK). Publicada como `wine-11.0-r3`.
- Revisão 4 (2026-10-15): regras `steam_game_id` casam com jogos abertos pela Steam (o ID da Steam é lido do
  ambiente Windows do processo). Publicada como `wine-11.0-r4`.
- Revisão 5 (2026-10-16): vídeos dos jogos pelo GStreamer 1.24.4, com H.264 e AAC decodificados pelo macOS
  (VideoToolbox, AudioToolbox). Publicada como `wine-11.0-r5`.

## Próximos passos

1. **Mais regras:** uma por jogo testado nos relatórios de compatibilidade do WinHub.
2. **Vídeos em jogos Unity com Direct3D 11:** repassar direito as texturas compartilhadas entre devices (DXMT); veja as lacunas.
3. **Direct3D 12 sem D3DMetal:** o spike (2026-10-16) não achou caminho jogável, então continua com o D3DMetal e o WinHub guia o usuário até o Game Porting Toolkit; candidato à r6: destravar o vkd3d do Wine no "Automático" (veja as lacunas).
4. **DXVK em 32 bits:** acompanhar o suporte do MoltenVK a mapeamentos de memória posicionados.
5. **Números do msync:** medir um jogo limitado pela CPU.

## Histórico de revisões

| Data | Mudança |
|---|---|
| 2026-10-12 | Primeiro roadmap, revisão 2 do motor. |
| 2026-10-12 | `wine-11.0-r2` publicada e transformada no motor padrão do WinHub. |
| 2026-10-13 | `wine-11.0-r3` publicada; paridade ~71%. |
| 2026-10-15 | `wine-11.0-r4` publicada; 2 regras; paridade ~71%. |
| 2026-10-16 | `wine-11.0-r5` publicada (vídeos dos jogos); paridade ~74%. |
| 2026-10-16 | Spike de Direct3D 12: continua com o D3DMetal; o "Automático" bloqueia programas D3D12 (candidato à r6); paridade ~73%. |
