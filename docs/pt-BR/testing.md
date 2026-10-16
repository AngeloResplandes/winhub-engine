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
| Vídeo de jogo (`tests/mf-smoke.c`) | o Media Foundation abre um MP4 e o lê até o fim: `OK h264 128x96 frames=15 …` e `OK aac rate=44100 channels=2`; o log mostra o `winhubcompat` apontando o GStreamer para o motor, o H.264 decodificado pelo `vtdec_hw` (ou `vtdec`) e o AAC pelo `atdec` |
| A garrafa | o `system32` só tem as `d3d11`, `dxgi`, `d3d10core` e `winemetal` do próprio Wine |

`SMOKE_BACKENDS="dxmt"` limita os backends. O CI roda o teste completo.

### O caso do vídeo

O `scripts/make-test-video.swift` escreve o MP4 quando o teste roda (nada fica no Git): 15 quadros cinza-claro de
128x96 e um segundo de AAC estéreo mudo, codificados pelo AVFoundation do próprio macOS, para o arquivo não
depender dos decodificadores testados. O `tests/mf-smoke.c` o lê com o source reader do Media Foundation, e o
teste confere os decodificadores pelo log:

```bash
GST_DEBUG=WINE:4,GST_ELEMENT_FACTORY:4 wine mf-smoke64.exe 'C:\video.mp4'
```

O `GST_ELEMENT_FACTORY:4` imprime `creating element "vtdec_hw"` e `creating element "atdec"`, os nomes das
fábricas; só com `WINE:4` saem nomes longos ("Apple VideoToolbox decoder (hardware only)"). A leitura é limitada
(1000 amostras) e precisa chegar ao fim do fluxo, então um decodificador travado falha em vez de pendurar.

Duas linhas a cada vídeo aberto são esperadas, inofensivas e vêm do Wine oficial:
`GStreamer-Video-CRITICAL **: <hora>: gst_video_info_from_caps: assertion 'gst_caps_is_fixed (caps)' failed`.
A decodificação está correta; elas também aparecem nos logs dos usuários. Um aviso de esvaziamento do `atdec`
quando o leitor é liberado depois de uma amostra de áudio também é esperado.

A primeira execução varre os plug-ins e grava o registro em `~/Library/Caches/WinHub/gstreamer/`, com o `HOME`
verdadeiro; apague o arquivo para ver a varredura de novo.

### As ferramentas do spike de Direct3D 12

O spike de Direct3D 12 (2026-10-16, [decisão](decisions.md#2026-10-16-o-direct3d-12-continua-com-o-d3dmetal)) deixou
três ferramentas no repositório, em `tests/` e `scripts/`: `tests/d3d12-smoke.c` (device, feature level, limpar e
apresentar), `tests/vulkan-features.c` (lista os recursos do Vulkan de que o vkd3d-proton precisa) e
`scripts/build-dxmt-d3d12-spike.sh` (compila um DXMT com Direct3D 12 num commit próprio e fixo). Elas não fazem parte
do teste de fumaça e o CI não as compila.

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
