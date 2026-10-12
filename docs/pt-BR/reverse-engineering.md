[English](../reverse-engineering.md) · **Português (Brasil)**

# Engenharia reversa: como o CrossOver roda jogos

> **Última revisão:** 2026-10-12 · **Alvo:** CrossOver 26.3.0 para macOS (build de julho de 2026)

O winhub-engine foi desenhado estudando como o CrossOver 26.3 roda jogos do Windows no Mac, **só pelo
comportamento**, e depois construindo a mesma arquitetura com fontes abertas. Esta página registra o que foi
estudado, o que se aprendeu e as regras que mantiveram o projeto limpo.

## Regras

- Estudamos **o que o CrossOver faz**: como o pacote é organizado, que arquivos e variáveis de ambiente usa,
  como decide as coisas em tempo de execução.
- **Nunca** copiamos código, binários, textos, traduções, ícones nem dados do CrossOver. Em particular, o módulo
  fechado `cxcompatdb.so` não é usado, o banco de compatibilidade do CrossOver (`compatdb-*.dat`, cifrado) nunca
  foi decifrado, e o catálogo de aplicativos dele não é redistribuído.
- O código que a CodeWeavers publica sob a LGPL, a árvore do Wine do CrossOver, **é** usado, com atribuição, e
  cada versão leva as fontes correspondentes.
- Esta documentação descreve comportamento. Código descompilado, endereços, nomes de símbolos internos e logs
  da análise ficam fora do repositório.

## Método

1. **Inspeção do pacote.** A organização do `CrossOver.app/Contents/SharedSupport/CrossOver`: que bibliotecas,
   backends e arquivos de dados vêm e onde; os modelos de garrafa e os scripts do lançador (Perl), que são texto
   legível.
2. **Análise estática** dos módulos nativos do motor com o [REA](https://github.com/morluto/rea) (provider
   Hopper), para entender a ordem das decisões: quando o módulo de ajustes por jogo é carregado, que variáveis
   ele lê, que condições tornam um backend utilizável, que pastas ele põe no caminho de busca do Wine. Nada foi
   executado nessa etapa, e nenhum código foi aproveitado.
3. **Confirmação nas fontes publicadas.** Todo comportamento que toca o Wine foi conferido no
   `crossover-sources-26.3.0.tar.gz` de [codeweavers.com/crossover/source](https://www.codeweavers.com/crossover/source):
   o gancho que carrega o módulo de ajustes, o `prepend_dll_path`, o msync, os ganchos de Metal do `winemac.drv`.
4. **Reimplementação e medição.** O motor foi construído com código aberto e testado nos mesmos casos; o que se
   aprendeu nesse processo está no fim desta página.

## Do que o motor do CrossOver é feito

| Peça | Onde no pacote | O que é |
|---|---|---|
| Wine 11.0 com os patches da CodeWeavers | `lib/wine/{x86_64-unix,x86_64-windows,i386-windows}` | só x86_64, com WoW64 (sem bibliotecas de 32 bits do macOS); roda pelo Rosetta 2 |
| `wineloader`, `wineserver` | `bin/` | binários Mach-O x86_64 |
| O lançador `wine` | `bin/wine` (Perl) | monta o ambiente a partir do `cxbottle.conf` da garrafa; define `ROSETTA_ADVERTISE_AVX=1`, `DOTNET_EnableWriteXorExecute=0`, o caminho e o registro dos plug-ins do GStreamer e o caminho da biblioteca compartilhada do D3DMetal. **Não escolhe o backend gráfico.** |
| `cxcompatdb.so` | `lib/wine/x86_64-unix/` | módulo fechado: ajustes por jogo e a escolha do backend gráfico |
| DXMT v0.72 (+25 commits) | `lib/dxmt/` | `d3d11`, `dxgi`, `d3d10core`, `winemetal` e, em 64 bits, `nvapi64` e `nvngx` |
| DXVK 1.10.3 + MoltenVK 1.2.10 | `lib/dxvk/`, `lib64/libMoltenVK.dylib` | `d3d9`, `d3d10`, `d3d10_1`, `d3d10core`, `d3d11`; sem carregador Vulkan |
| D3DMetal 3.0 (Apple) | `lib64/apple_gptk/` | `d3d11`, `d3d12`, `dxgi`, `nvapi64`, `nvngx`, `atidxx64`, só 64 bits |
| Bibliotecas Unix | `lib64/` | GStreamer (vídeos dos jogos), SDL2 (controles), freetype, gnutls, glib |

## Como o CrossOver escolhe o backend gráfico

- O `ntdll` da CodeWeavers carrega o `cxcompatdb.so` no fim da inicialização de todo processo do Wine (visível
  no `dlls/ntdll/unix/loader.c` LGPL, marcado como "CW Hack 24067").
- O módulo lê um banco de regras por jogo, assinado e cifrado, da pasta do CrossOver do usuário, e usa uma cópia
  embutida quando o baixado falta ou é mais velho. Só valem regras marcadas para o macOS.
- A escolha do usuário vem da garrafa, na variável `CX_GRAPHICS_BACKEND`: `wined3d`, `dxmt`, `dxvk`, `d3dmetal`,
  ou vazia para automático.
- O backend de uma regra vale quando o usuário deixou a escolha no automático; quando o usuário escolheu um
  backend, a regra só vence se estiver marcada como forçada. Automático sem regra quer dizer D3DMetal.
- Antes de usar um backend, o módulo confere se ele pode rodar: o DXMT exige macOS 14 ou mais novo e processo
  traduzido pelo Rosetta; o D3DMetal exige o mesmo e um processo de 64 bits. Se o escolhido não pode rodar, ele
  volta para a escolha do usuário e depois para o wined3d.
- A pasta do backend escolhido vai para o **começo** do caminho de busca de DLLs builtin do Wine pelo
  `prepend_dll_path` (exportado pelo `ntdll` LGPL), para o `d3d11.dll` dele ser achado antes do do Wine.
  **Nada é copiado para a garrafa.** O resultado é publicado em `CX_ACTIVE_GRAPHICS_BACKEND`, que o `win32u` da
  CodeWeavers lê (para escolher uma biblioteca Vulkan para o wined3d e informar agendamento de GPU por hardware
  para o D3DMetal).
- Os ajustes por jogo também podem definir variáveis de ambiente, acrescentar overrides de DLL, acrescentar à
  linha de comando ou trocar o caminho do executável de processos abertos por outros processos do Wine, e
  identificam programas pelo fim do caminho ou pelo app ID da Steam. Com o D3DMetal, um ajuste fixo manda o
  `system32\nvngx.dll` (DLSS) para a cópia do próprio D3DMetal.
- Os modelos de garrafa limpam garrafas antigas: removem as DLLs do DXVK que o CrossOver copiava para os
  prefixos ("copied until CrossOver 24") e convertem configurações antigas (`WINEDXVK`, `WINED3DMETAL`,
  `WINEESYNC`) para as atuais (`CX_GRAPHICS_BACKEND`, `WINEMSYNC`).

**Como o winhub-engine reproduz isso:** o [`winhubcompat`](compatdb.md), carregado no mesmo ponto pelo
[`patches/wine/0001`](patches.md#wine), toma as mesmas decisões na mesma ordem, com um banco em JSON aberto e
duas diferenças intencionais: o automático escolhe o DXMT quando o usuário não tem D3DMetal, e as regras também
podem ficar na garrafa. A migração do WinHub faz o que os modelos de garrafa fazem.

## msync

O `wineserver` e o `ntdll` do CrossOver implementam o msync sobre portas Mach, como backend da camada de
sincronização dentro do processo do Wine 10/11; servidor e processos precisam concordar no `WINEMSYNC`. Isso
está nas fontes LGPL e o motor usa sem mudanças. Veja [msync](msync.md).

## Outros achados nas fontes publicadas

- O `start_main_thread` troca o `localtime` por uma versão própria: o Wine guarda o PEB num slot de TLS da
  pthread que o `localtime()` do macOS também usa.
- A árvore da CodeWeavers diz que todo usuário se chama `crossover` (a menos que `CX_REPORT_REAL_USERNAME`
  esteja definida) e transforma o Desktop numa pasta com um link para a Área de Trabalho do Mac (a menos que
  `CX_DIRECT_DESKTOP` esteja definida). O motor volta ao comportamento do Wine oficial
  ([`patches/wine/0003`](patches.md#wine)).
- Outras chaves do CrossOver na árvore: `CX_HACK_FILESYSTEM_TYPE`, `CX_HACK_REMOTE_DRIVES`,
  `CX_HIDE_BUILTIN_VERSION_RESOURCES`, `CX_FWD_COMPAT_GL_CTX`, `CX_TURN_OFF_FONT_REPLACEMENTS`,
  `WINE_SIMULATE_WRITECOPY`.
- O pacote de fontes traz `wine`, `dxvk`, `moltenvk`, `vkd3d`, `gstreamer`, `gnutls`, `freetype`, `glib`,
  `cabextract` e outros, mas nem o DXMT nem o `cxcompatdb`.

## O que a construção do motor confirmou

| Achado | Consequência para o motor |
|---|---|
| O `win32u` da CodeWeavers carrega o Vulkan pelo soname, sem pedir ao carregador os drivers de portabilidade | o Vulkan vai direto ao `libMoltenVK.dylib` |
| O MoltenVK da CodeWeavers diz ter recursos que o Metal não tem (geometry shaders, cull distance, estatísticas de pipeline, acesso robusto a buffers) "exigidos pelo DXVK", e o DXVK deles os exige | `patches/dxvk/0002`: o DXVK pede só o que o MoltenVK oficial tem |
| O Wine ignora DLLs no caminho de builtins sem a marca de builtin; todas as DLLs de backend do CrossOver têm a marca | DXMT compilado como builtin, DXVK marcado com `winebuild --builtin` |
| Uma builtin que o Wine não conhece (`winemetal.dll`) só é achada pelo arquivo dela no `system32`; o CrossOver também a traz em `lib/wine` | o motor também a traz em `lib/wine`; o WinHub a recoloca nas garrafas migradas |
| Em processos WoW64 sem `VK_EXT_map_memory_placed`, o `win32u` cria buffers com memória do host, que o MoltenVK oficial e o da CodeWeavers recusam | o DXVK fica limitado a processos de 64 bits; os de 32 caem no DXMT. Não está claro se o DXVK do CrossOver funciona em 32 bits; o modo automático dele manda processos de 32 bits para o wined3d |
| O nome de usuário `crossover` quebra o perfil das garrafas feitas com o Wine oficial (login salvo da Steam, saves dos jogos) | `patches/wine/0003` |

O quanto o resultado se aproxima, dimensão por dimensão: [Paridade com o CrossOver](crossover-parity.md).
