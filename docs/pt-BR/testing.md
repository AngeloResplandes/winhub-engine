[English](../testing.md) · **Português (Brasil)**

# Testes

## Motor de regras

```bash
./scripts/test-compatdb.sh
```

Compila o `tests/compatdb-test.c` nativamente, com AddressSanitizer e UndefinedBehaviorSanitizer, e confere: a
leitura e o descarte de regras inválidas, a ordem das regras e "vale o primeiro ajuste de cada tipo", o casamento
por caminho e por ID da Steam, a escolha do backend (regras forçadas, escolha do usuário, alternativas), as
mudanças de ambiente e de linha de comando, e a ordem dos arquivos embutido, baixado e local.

## Teste de fumaça

```bash
./scripts/smoke-test.sh
```

Instala o motor como o WinHub faz (pacote oficial, sem `bin/` e `lib/wine/`, com o overlay por cima), cria uma
garrafa e roda o `tests/d3d11-smoke.c`, que cria um dispositivo Direct3D 11 com swap chain e apresenta 30
quadros:

| Caso | Esperado |
|---|---|
| DXMT, 64 e 32 bits | feature level 11_0, `OK presented 30 frames`, `winhubcompat` escolheu `dxmt` |
| DXVK, 64 bits | o mesmo com `dxvk` |
| DXVK pedido por um programa de 32 bits | o `winhubcompat` cai para o `dxmt`, que passa |
| Extensões da NVIDIA (`tests/nvapi-smoke.c`, DXMT) | com `DXMT_ENABLE_NVEXT=1`, o `nvapi64` carrega, o `NvAPI_Initialize` dá certo e o `system32\nvngx.dll` abre o do DXMT; sem ela, o `NvAPI_Initialize` recusa |
| `replace_exe_path` | uma regra local faz o `cmd` abrir o teste de 64 bits no lugar de um "lançador" de 32 bits; o trace mostra `replacing` e o teste passa |
| `steam_game_id` | o `cmd` define `SteamGameId` só no seu ambiente Windows, como a Steam faz, e abre o teste de 64 bits; uma regra local com esse id escolhe o DXVK e o teste passa |
| A garrafa | o `system32` só tem as `d3d11`, `dxgi`, `d3d10core` e `winemetal` do próprio Wine |

`SMOKE_BACKENDS="dxmt"` limita os backends. O CI roda o teste completo.

## Ver o que o winhubcompat fez

```bash
WINEDEBUG=+winhubcompat wine programa.exe
```

mostra as regras lidas, a regra que casou, `using <backend> as the graphics backend` e, nos processos-filho, as
variáveis e as mudanças de linha de comando. `WINEDEBUG=+loaddll` mostra quais DLLs foram carregadas.

## Testar jogos com segurança

Os jogos são testados numa **cópia** de uma garrafa real, nunca na original:

1. Clone com APFS (instantâneo, sem ocupar espaço extra): `cp -cR "<garrafa>" work/steam-clone`.
2. Aponte o `WINEPREFIX` para o clone e rode o motor de `work/engine` (instalado pelo teste de fumaça).
3. **Uma Steam por vez.** Em 2026-10-11, uma Steam de teste ainda estava aberta num clone quando a Steam da
   garrafa original foi aberta; essa Steam marcou dois jogos como "arquivos faltando" e os desinstalou. Encerre
   todos os processos do Wine do clone (confira com `pgrep -fl steam`) antes de abrir a garrafa real, e
   vice-versa.
4. No fim, apague o clone só depois de conferir que nada nele faz falta.

Os resultados dos jogos ficam nos
[relatórios de compatibilidade](https://github.com/AngeloResplandes/winhub/tree/main/docs/pt-BR/compat) do WinHub.
