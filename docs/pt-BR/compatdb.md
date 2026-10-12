[English](../compatdb.md) · **Português (Brasil)**

# winhubcompat e regras de compatibilidade

O `winhubcompat.so` (código em [`src/winhubcompat`](https://github.com/AngeloResplandes/winhub-engine/tree/main/src/winhubcompat))
é carregado pelo `ntdll.so` em todo processo do Wine, logo depois do `ntdll.dll` e antes de qualquer outra DLL
([`patches/wine/0001`](patches.md#wine)). Ele:

1. lê as regras de compatibilidade;
2. aplica, no próprio processo, os overrides de DLL e as variáveis de ambiente da regra que casar;
3. escolhe o Direct3D e põe a pasta dele **no começo** do caminho de DLLs do Wine; nada é copiado para a
   garrafa;
4. aplica variáveis de ambiente e acréscimos de linha de comando aos processos-filho (via
   `NtCreateUserProcess`);
5. com o D3DMetal, manda o `nvngx.dll` do DLSS para a cópia do próprio D3DMetal.

## Variáveis de ambiente

| Variável | Quem define | Para quê |
|---|---|---|
| `WINHUB_GRAPHICS_BACKEND` | WinHub (garrafa) | `auto` (padrão), `wined3d`, `dxmt`, `dxvk` ou `d3dmetal` |
| `WINHUB_D3DMETAL_DIR` | WinHub | A pasta `lib` do Game Porting Toolkit da Apple do próprio usuário (com `wine/` e `external/`). O motor nunca traz o D3DMetal |
| `CX_APPLEGPTK_LIBD3DSHARED_PATH` | WinHub | `<WINHUB_D3DMETAL_DIR>/external/libd3dshared.dylib`; lido pelo `ntdll` da árvore da CodeWeavers |
| `WINHUB_HOME` | WinHub | A pasta de dados do WinHub; o banco baixado fica em `compat/compatdb.json` |
| `WINHUB_COMPAT_RULES` | WinHub | O arquivo com as regras da garrafa (por exemplo, o backend escolhido para um jogo) |
| `WINHUB_ACTIVE_GRAPHICS_BACKEND`, `CX_ACTIVE_GRAPHICS_BACKEND` | winhubcompat | O backend escolhido; o segundo é lido pelo `win32u` da CodeWeavers |

Para ver o que ele fez: `WINEDEBUG=+winhubcompat`. A linha `using <backend> as the graphics backend` diz o
backend de cada processo.

## Escolha do backend

Candidatos, em ordem; vale o primeiro que puder rodar:

1. a regra do programa, se tiver `"force": true`;
2. a escolha do usuário (`WINHUB_GRAPHICS_BACKEND`), se não for `auto`;
3. a regra do programa;
4. `d3dmetal`, depois `dxmt`;
5. `wined3d` (sempre possível).

Quando cada um pode rodar:

| Backend | Exige |
|---|---|
| `dxmt` | macOS 14 ou mais novo, processo traduzido pelo Rosetta (Apple silicon) e `lib/dxmt/x86_64-windows/d3d11.dll` no motor |
| `d3dmetal` | o mesmo, processo de 64 bits e `WINHUB_D3DMETAL_DIR` válido |
| `dxvk` | processo de 64 bits e `lib/dxvk/x86_64-windows/d3d11.dll` no motor (Vulkan pelo MoltenVK). Em processos de 32 bits, sem `VK_EXT_map_memory_placed`, o `win32u` cria buffers com memória do host e o MoltenVK os recusa (tanto o oficial quanto o da CodeWeavers) |
| `wined3d` | nada |

As pastas dos backends têm o formato de uma pasta de bibliotecas do Wine (`x86_64-windows/`, `i386-windows/`,
`x86_64-unix/`) e guardam DLLs builtin (o Wine ignora DLLs comuns nesse caminho).

## De onde vêm as regras

Lidas nesta ordem de prioridade:

1. `WINHUB_COMPAT_RULES` (as regras da garrafa; `timestamp` opcional);
2. o mais novo (pelo `timestamp`) entre o banco embutido no motor (`share/winhub-engine/compatdb.json`, vindo de
   [`data/compatdb.json`](https://github.com/AngeloResplandes/winhub-engine/blob/main/data/compatdb.json)) e o
   banco baixado (`$WINHUB_HOME/compat/compatdb.json`). Empate: o embutido.

Para cada tipo de ajuste, vale o **primeiro** encontrado entre as regras que casam.

O WinHub confere o banco baixado antes de gravá-lo; o módulo confia no que está no disco.

### Publicação e assinatura

O banco publicado é o `data/compatdb.json` da `main`. O WinHub o baixa junto com o `data/compatdb.json.sig`,
confere a assinatura Ed25519 com a chave pública embutida no app (a mesma de
[`data/compatdb-public-key.txt`](https://github.com/AngeloResplandes/winhub-engine/blob/main/data/compatdb-public-key.txt))
e recusa um banco cujo `timestamp` não seja mais novo que o que já tem. Passos para mudá-lo:
[Publicação](releasing.md#regras-de-compatibilidade).

## Formato

```json
{
  "version": 1,
  "timestamp": 1791763200,
  "rules": [
    {
      "desc": "Hollow Knight",
      "os": "macos",
      "match": { "type": "path_suffix", "suffix": "Hollow Knight\\hollow_knight.exe" },
      "hacks": [
        { "type": "graphics_backend", "backend": "dxmt" },
        { "type": "env_vars", "vars": { "DXMT_LOG_LEVEL": "none" } }
      ]
    },
    {
      "desc": "Um jogo da Steam pelo app ID",
      "match": { "type": "steam_game_id", "game_id": "367520" },
      "hacks": [
        { "type": "dll_overrides", "overrides": ["d3d9=n,b"] },
        { "type": "append_cmd_line", "append": "-dx11", "forbidden_args": "-dx12", "insert": true }
      ]
    }
  ]
}
```

- `version`: sempre `1`. `timestamp`: segundos desde 1970; obrigatório nos bancos embutido e baixado.
- `os`: opcional; se existir, precisa ser `"macos"` (regras de outros sistemas são puladas).
- `match`:
  - `path_suffix`: o fim do caminho Windows do programa (`\` ou `/`), sem diferenciar maiúsculas, a partir do
    começo de um nome de pasta ou arquivo (`game.exe` não casa com `notgame.exe`);
  - `steam_game_id`: o valor de `SteamGameId` no ambiente (a Steam define ao abrir um jogo).
- `hacks`:
  - `graphics_backend`: `backend` (`wined3d`, `dxmt`, `dxvk`, `d3dmetal`) e `force` opcional;
  - `dll_overrides`: uma lista no formato do `WINEDLLOVERRIDES` (`"d3d11=n,b"`);
  - `env_vars`: um objeto `nome: valor`. No próprio processo, só o lado Unix vê (Metal, MoltenVK, DXMT); os
    processos-filho recebem também no ambiente do Windows;
  - `append_cmd_line`: `append`; `insert` (logo depois do nome do programa em vez do fim);
    `required_args` / `forbidden_args` (só aplica se a linha tiver / não tiver esse texto). Não repete. Vale
    para processos abertos por outro processo do Wine (por exemplo, jogos abertos pela Steam);
  - `replace_exe_path` (revisão 3): `path`, o caminho Windows de um programa a abrir no lugar do que casou
    (`"C:\\Game\\Game.exe"`), com os mesmos argumentos. Serve para lançadores que precisam ser pulados. Como o
    `append_cmd_line`, vale para processos abertos por outro processo do Wine, e nunca para o próprio alvo (uma
    regra não consegue abri-lo em laço).

Regras inválidas são puladas com um aviso (`WINEDEBUG=warn+winhubcompat`); um arquivo inválido é ignorado
inteiro.

## D3DMetal e DLSS

Com o `d3dmetal` ativo, o `winhubcompat` também envolve o `NtCreateFile` e o `NtOpenFile` para que o
`C:\windows\system32\nvngx.dll` (onde o DLSS procura a biblioteca da NVIDIA) abra o `nvngx.dll` do D3DMetal, que
faz esse papel sobre o MetalFX.

Com o `dxmt` (revisão 3), o `lib/dxmt` do motor também tem o `nvapi64.dll` e o `nvngx.dll` do DXMT (DLSS sobre o
MetalFX). O Wine os acha pelo caminho de DLLs que o `winhubcompat` define, então nenhum arquivo é copiado para a
garrafa. Eles só respondem com `DXMT_ENABLE_NVEXT=1`, que o WinHub liga por garrafa (feature `nvext`); sem isso, o
`NvAPI_Initialize` responde "dispositivo NVIDIA não encontrado", como num Mac sem a opção.

## Testes

O `./scripts/test-compatdb.sh` cobre leitura, prioridade, escolha do backend, ambiente e linha de comando, sem o
Wine. Veja [Testes](testing.md).

## Escolhas de projeto

Banco em JSON aberto e legível; regras por garrafa; `auto` escolhe o DXMT quando não há D3DMetal. Como isso se
compara ao módulo fechado do CrossOver: [Paridade com o CrossOver](crossover-parity.md).
