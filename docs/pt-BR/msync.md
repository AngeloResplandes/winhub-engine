[English](../msync.md) · **Português (Brasil)**

# msync

O msync troca a maior parte das idas ao `wineserver` dos objetos de sincronização do Windows (eventos, semáforos,
mutexes) por operações dentro de cada processo, numa memória compartilhada entre os processos da garrafa, com
portas Mach para acordar as threads que esperam. Faz diferença em jogos limitados pela CPU.

## De onde vem

As fontes LGPL do Wine 11.0 que a CodeWeavers publica para o CrossOver já implementam o msync como backend da
camada de sincronização dentro do processo (`inproc_sync`) do Wine 10/11:

- `server/msync.c`, `server/inproc_sync.c`: os objetos compartilhados e a criação das portas Mach;
- `dlls/ntdll/unix/msync.c`: as operações dentro do processo; o `msync_init()` roda no `start_main_thread`.

O motor compila essa árvore sem mudanças e declara `msync` em `features`.

## Como usar

- O WinHub define `WINEMSYNC=1` nas garrafas com o msync ligado (`Bottle.msync`).
- O servidor e os processos precisam concordar: o `ntdll` recusa um processo sem `WINEMSYNC` quando o
  `wineserver` da garrafa foi aberto com ele. Depois de mudar a opção, encerre a garrafa (`wineserver -k`) antes
  de abrir programas de novo.
- Para conferir que está ligado: `WINEDEBUG=+msync` mostra `msync: up and running`.

## Histórico

O primeiro plano (outubro de 2026) era escrever esse backend do zero, porque os patches abertos do
[wine-msync](https://github.com/marzent/wine-msync) param no Wine 9.15 e o Wine 10 reorganizou a sincronização
em torno do `inproc_sync`. A árvore da CodeWeavers já faz exatamente isso, então o plano foi abandonado quando o
motor passou a usar as fontes dela (revisão 2). Veja [Decisões](decisions.md).

## O que falta medir

FPS e tempo de quadro de um jogo limitado pela CPU, com e sem `WINEMSYNC=1`, e os testes de sincronização do
próprio Wine (`dlls/ntdll/tests/sync.c`, `dlls/kernel32/tests/sync.c`) com e sem ele.
