# Plano: sincronização rápida (msync) para o Wine 11 no macOS

Status: **planejado, não iniciado**. Começa quando um jogo testado no WinHub mostrar gargalo de CPU
causado pela sincronização. O Hollow Knight, por exemplo, roda a 60 FPS com folga e não precisa disso.

## Por que não dá para só aplicar o msync antigo

- O [wine-msync](https://github.com/marzent/wine-msync) (LGPL-2.1) tem cerca de 176 KB de patch e o último
  alvo é o Wine 9.15 (agosto de 2024).
- O Wine 10/11 reorganizou a sincronização: agora há uma camada de **sincronização dentro do processo**
  (`inproc_sync`), em `dlls/ntdll/unix/sync.c` e `server/inproc_sync.c`. O único backend dela usa o
  `/dev/ntsync` do kernel Linux (`linux/ntsync.h`), que não existe no macOS.
- Portar o patch antigo linha a linha brigaria com essa camada nova. O caminho limpo é **um backend
  novo para ela**, no lugar do `/dev/ntsync`.

## Desenho proposto

1. **Objetos:** cada semáforo, mutex e evento do NT vira um registro numa área de memória compartilhada
   entre os processos da garrafa (criada pelo wineserver, como no msync), com um semáforo Mach para
   acordar quem espera.
2. **Operações rápidas sem o wineserver:** `NtReleaseSemaphore`, `NtSetEvent`, `NtResetEvent`,
   `NtReleaseMutant` e esperas por um único objeto resolvem tudo no próprio processo (operações
   atômicas na memória compartilhada + `semaphore_signal`/`semaphore_timedwait`, ou `__ulock_wait2`
   quando houver).
3. **Esperas por vários objetos (`WaitForMultipleObjects`, inclusive "wait all"):** a parte difícil.
   Primeira versão: esperar no processo com tentativa e nova checagem; casos raros ("wait all" com
   mutexes abandonados, APCs) continuam passando pelo wineserver, como hoje.
4. **Integração:** o backend responde às mesmas chamadas `inproc_*` que o ntsync, ligado por
   `WINEMSYNC=1`. O WinHub já tem a chave por garrafa (`Bottle.msync`) e só a mostra quando o motor
   declara `msync` em `features`.

## Etapas

1. Ler a camada `inproc_sync` do Wine 11.18 e listar o contrato exato que o backend precisa cumprir.
2. Protótipo só com eventos e semáforos, atrás de `WINEMSYNC=1`.
3. Testes: os testes de sincronização do próprio Wine (`dlls/ntdll/tests/sync.c`,
   `dlls/kernel32/tests/sync.c`) com e sem `WINEMSYNC`, mais um teste de estresse entre processos.
4. Mutexes, "wait all" e abandono de mutex.
5. Medição com um jogo limitado por CPU (FPS e tempo de quadro com e sem `WINEMSYNC`) antes de
   declarar `msync` no `engine.json` e no manifesto.

## Critério para começar

Um jogo do relatório de compatibilidade do WinHub em que o perfil (Instruments, ou `WINEDEBUG=+server`)
mostre tempo relevante em chamadas de sincronização ao wineserver.
