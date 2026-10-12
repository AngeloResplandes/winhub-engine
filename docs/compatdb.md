# winhubcompat: backend gráfico e regras por jogo

O `winhubcompat.so` (código em [`src/winhubcompat`](../src/winhubcompat)) é carregado pelo `ntdll.so` em
todo processo do Wine, logo depois do `ntdll.dll` e antes de qualquer outra DLL (patch
[`0001`](../patches/wine)). Ele:

1. lê as regras de compatibilidade;
2. aplica, no próprio processo, os overrides de DLL e as variáveis de ambiente da regra que casar;
3. escolhe o Direct3D e põe a pasta dele **no começo** do caminho de DLLs do Wine. Nada é copiado
   para a garrafa;
4. aplica variáveis de ambiente e acréscimos de linha de comando aos processos-filho
   (via `NtCreateUserProcess`).

## Variáveis de ambiente

| Variável | Quem define | Para quê |
|---|---|---|
| `WINHUB_GRAPHICS_BACKEND` | WinHub (garrafa) | `auto` (padrão), `wined3d`, `dxmt`, `dxvk` ou `d3dmetal`. |
| `WINHUB_D3DMETAL_DIR` | WinHub | Pasta `lib` do Game Porting Toolkit da Apple instalado pelo usuário (com `wine/` e `external/`). O motor nunca traz o D3DMetal. |
| `CX_APPLEGPTK_LIBD3DSHARED_PATH` | WinHub | `<WINHUB_D3DMETAL_DIR>/external/libd3dshared.dylib`; lido pelo `ntdll` da árvore da CodeWeavers. |
| `WINHUB_HOME` | WinHub | Pasta de dados do WinHub; o banco baixado fica em `compat/compatdb.json`. |
| `WINHUB_COMPAT_RULES` | WinHub | Arquivo com as regras da garrafa (ex.: o Direct3D escolhido para um jogo). |
| `WINHUB_ACTIVE_GRAPHICS_BACKEND`, `CX_ACTIVE_GRAPHICS_BACKEND` | winhubcompat | O backend escolhido. O segundo é lido pelo `win32u` da CodeWeavers. |

Para ver o que ele fez: `WINEDEBUG=+winhubcompat`. A linha `using <backend> as the graphics backend`
diz o backend de cada processo.

## Escolha do backend

Candidatos, em ordem; vale o primeiro que puder rodar:

1. a regra do jogo, se tiver `"force": true`;
2. a escolha do usuário (`WINHUB_GRAPHICS_BACKEND`), se não for `auto`;
3. a regra do jogo;
4. `d3dmetal`, depois `dxmt`;
5. `wined3d` (sempre pode).

Quando cada um pode rodar:

| Backend | Exige |
|---|---|
| `dxmt` | macOS 14 ou mais novo, processo no Rosetta (Apple silicon) e `lib/dxmt/x86_64-windows/d3d11.dll` no motor |
| `d3dmetal` | o mesmo, processo de 64 bits e `WINHUB_D3DMETAL_DIR` válido |
| `dxvk` | processo de 64 bits e `lib/dxvk/x86_64-windows/d3d11.dll` no motor (Vulkan pelo MoltenVK). Em 32 bits, sem `VK_EXT_map_memory_placed`, o `win32u` cria buffers com memória do host, e o MoltenVK recusa (o oficial e o da CodeWeavers) |
| `wined3d` | nada |

As pastas de backend têm o formato de uma pasta de bibliotecas do Wine (`x86_64-windows/`,
`i386-windows/`, `x86_64-unix/`) e as DLLs são *builtin* (o Wine ignora DLLs comuns nesse caminho).

## Onde ficam as regras

Lidas nesta ordem de prioridade:

1. `WINHUB_COMPAT_RULES` (regras da garrafa; `timestamp` opcional);
2. o mais novo (pelo `timestamp`) entre o banco embutido no motor
   (`share/winhub-engine/compatdb.json`, vindo de [`data/compatdb.json`](../data/compatdb.json)) e o
   banco baixado (`$WINHUB_HOME/compat/compatdb.json`). Empate: o embutido.

O banco baixado é conferido pelo WinHub antes de ser gravado; o módulo confia no que está no disco.

### Publicação e assinatura

O banco publicado é este mesmo [`data/compatdb.json`](../data/compatdb.json): o WinHub baixa ele e o
`data/compatdb.json.sig` da `main` deste repositório, confere a assinatura Ed25519 com a chave pública
embutida no app (a mesma de [`data/compatdb-public-key.txt`](../data/compatdb-public-key.txt)) e recusa
um banco com `timestamp` menor que o que já tem. Ao mudar as regras:

1. aumente o `timestamp` (segundos desde 1970);
2. `swift scripts/compatdb.swift sign` (usa a chave privada de `~/.winhub-signing/compatdb-ed25519.key`
   ou de `$WINHUB_COMPATDB_KEY`, que nunca entra no repositório);
3. faça commit dos dois arquivos. O CI roda `swift scripts/compatdb.swift verify`.

Para cada tipo de ajuste vale o **primeiro** encontrado entre as regras que casam.

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
      "desc": "Jogo da Steam pelo appid",
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
  - `path_suffix`: fim do caminho Windows do programa (`\` ou `/`), sem diferenciar maiúsculas, a partir
    do começo de um nome de pasta ou arquivo (`game.exe` não casa com `notgame.exe`);
  - `steam_game_id`: valor de `SteamGameId` no ambiente (a Steam define ao abrir um jogo).
- `hacks`:
  - `graphics_backend`: `backend` (`wined3d`, `dxmt`, `dxvk`, `d3dmetal`) e `force` opcional;
  - `dll_overrides`: lista no formato do `WINEDLLOVERRIDES` (`"d3d11=n,b"`);
  - `env_vars`: objeto `nome: valor`. No próprio processo só o lado Unix vê (Metal, MoltenVK, DXMT);
    os processos-filho recebem no ambiente do Windows também;
  - `append_cmd_line`: `append`; `insert` (logo depois do nome do programa em vez do fim);
    `required_args` / `forbidden_args` (só aplica se a linha tiver / não tiver esse texto). Não repete
    se o texto já estiver lá. Vale para processos abertos por outro processo do Wine (ex.: jogos abertos
    pela Steam).

Regras inválidas são puladas com um aviso (`WINEDEBUG=warn+winhubcompat`); um arquivo inválido é
ignorado inteiro.

## Testes

`./scripts/test-compatdb.sh` testa leitura, prioridade, escolha do backend, ambiente e linha de comando
sem precisar do Wine.

## D3DMetal e DLSS

Com `d3dmetal` ativo, o `winhubcompat` também envolve `NtCreateFile` e `NtOpenFile` para que
`C:\windows\system32\nvngx.dll` (onde o DLSS procura a biblioteca da NVIDIA) abra o `nvngx.dll` do
D3DMetal, que faz o papel dela sobre o MetalFX.

## Escolhas de projeto

Banco em JSON aberto e legível, regras por garrafa, e `auto` escolhe DXMT quando não há D3DMetal.
