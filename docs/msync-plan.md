# msync (sincronização rápida) no Wine 11 para macOS

Status: **resolvido com as fontes do CrossOver** (motor r2). Este arquivo guarda o histórico do plano.

## Como ficou

As fontes LGPL do Wine 11.0 que a CodeWeavers publica para o CrossOver já trazem o msync como backend
da camada de sincronização dentro do processo (`inproc_sync`) do Wine 10/11:

- `server/msync.c`, `server/inproc_sync.c`: objetos na memória compartilhada entre os processos da
  garrafa, com portas Mach para acordar quem espera;
- `dlls/ntdll/unix/msync.c`: operações no próprio processo; `msync_init()` é chamado em
  `start_main_thread`;
- ligado por `WINEMSYNC=1`. Servidor e processos precisam concordar: o `ntdll` recusa um processo sem
  `WINEMSYNC` quando o `wineserver` foi aberto com ele.

O motor compila essa árvore inteira (`scripts/build-wine.sh`) e declara `msync` em `features`; o WinHub
liga `WINEMSYNC=1` por garrafa (`Bottle.msync`). Ao mudar a opção, a garrafa precisa ser encerrada
(`wineserver -k`) antes de abrir programas de novo.

## Plano original (substituído)

A ideia era escrever do zero um backend para a `inproc_sync` no lugar do `/dev/ntsync` do Linux, porque
o [wine-msync](https://github.com/marzent/wine-msync) parou no Wine 9.15. A árvore da CodeWeavers faz
exatamente isso, então o trabalho não é mais necessário.

## O que falta medir

Um jogo limitado por CPU (FPS e tempo de quadro com e sem `WINEMSYNC=1`), e os testes de sincronização
do Wine (`dlls/ntdll/tests/sync.c`, `dlls/kernel32/tests/sync.c`) com e sem `WINEMSYNC`.
