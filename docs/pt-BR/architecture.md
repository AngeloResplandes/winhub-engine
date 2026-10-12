[English](../architecture.md) · **Português (Brasil)**

# Arquitetura

## O que uma versão contém

O motor é um **overlay**: o WinHub primeiro instala o pacote oficial do WineHQ para macOS
(`wine-devel-11.18-osx64`, do Gcenx), apaga o `bin/` e o `lib/wine/` dele e desempacota o overlay por cima. O
pacote base continua fornecendo as bibliotecas (freetype, gnutls, SDL2, MoltenVK, ICU…), o wine-mono e o
wine-gecko, que este repositório não compila.

```
<pasta wine> (do pacote oficial, com bin/ e lib/wine/ substituídos)
├── bin/                     wine, wineserver e os atalhos de sempre (wineboot, winecfg…)
├── lib/
│   ├── wine/                Wine 11.0 das fontes LGPL do CrossOver
│   │   ├── x86_64-unix/     ntdll.so, win32u.so, winemac.so… e winhubcompat.so
│   │   ├── x86_64-windows/  DLLs do lado Windows, 64 bits (+ winemetal.dll)
│   │   └── i386-windows/    DLLs do lado Windows, 32 bits (+ winemetal.dll), rodando pelo WoW64
│   ├── dxmt/                DXMT v0.80, DLLs builtin: x86_64-windows, i386-windows, x86_64-unix
│   ├── dxvk/                DXVK 1.10.3, DLLs builtin: x86_64-windows, i386-windows
│   └── *.dylib              bibliotecas do pacote oficial (MoltenVK, freetype, gnutls, SDL2…)
└── share/
    ├── wine/                wine.inf, nls, fontes (nossos) + wine-mono, wine-gecko (pacote oficial)
    └── winhub-engine/       engine.json, compatdb.json (regras embutidas), licenças
```

O `share/winhub-engine/engine.json` lista as versões e as `features` que o WinHub confere:
`dxmt`, `dxvk`, `msync`, `backend-dispatch`, `compatdb`, `nvext` (revisão 3).

## Pipeline de build

```
crossover-sources-26.3.0.tar.gz ──► sources/wine + patches/wine ──► build-wine.sh ──► wine-install/
        (CodeWeavers, LGPL)        └─ sources/dxvk + patches/dxvk ──► build-dxvk.sh ──► dxvk-install/
DXMT v0.80 (MIT) + patches/dxmt + LLVM 15 ───────────────────────► build-dxmt.sh ──► dxmt-install/
src/winhubcompat + cJSON ─────────────────────────────────────────► build-winhubcompat.sh
                                                    package.sh ──► dist/ (overlay, fontes, manifestos)
                                                 smoke-test.sh ──► instala como o WinHub e testa D3D11
```

Todas as entradas são fixadas por SHA-256 ou commit em
[`versions.env`](https://github.com/AngeloResplandes/winhub-engine/blob/main/versions.env). Detalhes de cada
passo: [Compilação](building.md).

## Wine

- **Versão:** Wine 11.0 com os patches da CodeWeavers para o CrossOver 26.3 (msync, os ganchos de Metal no
  `winemac.drv` que o DXMT e o D3DMetal usam, e outras correções do CrossOver).
- **Arquitetura:** só x86_64, rodando pelo Rosetta 2 nos Macs com Apple silicon. Programas Windows de 32 bits
  rodam dentro de processos de 64 bits (WoW64), então não há bibliotecas de 32 bits do macOS.
- **Bibliotecas ligadas:** freetype, gnutls e SDL2 vêm do `lib/` do pacote oficial; o Vulkan vai direto ao
  `libMoltenVK.dylib`, sem o carregador Vulkan (o `win32u` da CodeWeavers não pede ao carregador os drivers de
  "portabilidade", então o carregador não acharia nenhum).
- **Nossos patches:** carregar o `winhubcompat.so` no lugar do módulo fechado do CrossOver, usar o wine-mono do
  pacote oficial, e manter o nome de usuário, a pasta de perfil e o link do Desktop do Wine oficial. Veja
  [Patches](patches.md).

## winhubcompat

O `ntdll.so` carrega o `lib/wine/x86_64-unix/winhubcompat.so` em todo processo do Wine, depois de mapear o
`ntdll.dll` e antes de qualquer outra DLL. O construtor dele:

1. lê as regras de compatibilidade: o arquivo da própria garrafa (`WINHUB_COMPAT_RULES`) e depois o mais novo
   entre o banco embutido e o que o WinHub baixou (`WINHUB_HOME/compat/compatdb.json`);
2. aplica ao processo os overrides de DLL e as variáveis de ambiente da regra que casar;
3. escolhe o backend Direct3D e põe a pasta dele **no começo** do caminho de busca de DLLs do Wine
   (`prepend_dll_path`), para o `lib/dxmt/x86_64-windows/d3d11.dll` ser achado antes do do Wine;
4. envolve o `NtCreateUserProcess` para os processos-filho receberem as variáveis de ambiente e os acréscimos
   de linha de comando da regra;
5. com o D3DMetal, envolve o `NtCreateFile` e o `NtOpenFile` para o `C:\windows\system32\nvngx.dll` abrir a
   cópia do próprio D3DMetal (DLSS).

Ele também define `CX_ACTIVE_GRAPHICS_BACKEND`, lida pelo `win32u` da CodeWeavers. Formato das regras e lista
completa de variáveis: [winhubcompat e regras de compatibilidade](compatdb.md).

### Como o backend é escolhido

Candidatos, em ordem; vale o primeiro que puder rodar:

1. uma regra do programa com `"force": true`;
2. a escolha do usuário (`WINHUB_GRAPHICS_BACKEND`), se não for `auto`;
3. uma regra do programa;
4. `d3dmetal`, depois `dxmt`;
5. `wined3d` (sempre possível).

| Backend | Pode rodar quando |
|---|---|
| `dxmt` | macOS 14 ou mais novo, processo traduzido pelo Rosetta, DXMT presente |
| `d3dmetal` | o mesmo, mais processo de 64 bits e o D3DMetal do próprio usuário (`WINHUB_D3DMETAL_DIR`) |
| `dxvk` | processo de 64 bits e DXVK presente (processos de 32 bits caem para outro, veja [Lacunas](gaps.md)) |
| `wined3d` | sempre |

### Por que as DLLs dos backends são "builtin"

O Wine ignora uma DLL achada no caminho de busca de builtins se ela não tiver a marca "Wine builtin DLL". O
DXMT é compilado com `wine_builtin_dll=true` e as DLLs do DXVK são marcadas com `winebuild --builtin`. Uma
builtin que o Wine não conhece (`winemetal.dll`) só é achada pelo arquivo dela no `system32`, que o `wineboot`
cria para as DLLs que estão em `lib/wine`; por isso o `winemetal.dll` também vai em `lib/wine`, como no
CrossOver.

## Como o WinHub usa o motor

- Instala o pacote base e o overlay, respeitando o `overlay.replace` (`["bin", "lib/wine"]`).
- Define `WINHUB_GRAPHICS_BACKEND` pela garrafa, `WINHUB_COMPAT_RULES` como `<garrafa>/compat-rules.json`,
  `WINHUB_HOME`, e as variáveis do D3DMetal quando o usuário apontou um Game Porting Toolkit.
- Baixa o `data/compatdb.json` deste repositório só quando a assinatura Ed25519 confere.

O lado do WinHub está na
[arquitetura do WinHub](https://github.com/AngeloResplandes/winhub/blob/main/docs/pt-BR/architecture.md).
