**English** · [Português (Brasil)](pt-BR/msync.md)

# msync

msync replaces most round trips to `wineserver` for Windows synchronization objects (events, semaphores,
mutexes) with operations inside each process, on memory shared by the bottle's processes, using Mach ports to
wake waiting threads. It matters for games that are limited by CPU.

## Where it comes from

The LGPL Wine 11.0 sources CodeWeavers publishes for CrossOver already implement msync as a backend of Wine
10/11's in-process synchronization layer (`inproc_sync`):

- `server/msync.c`, `server/inproc_sync.c`: the shared objects and the Mach port bootstrap;
- `dlls/ntdll/unix/msync.c`: the in-process operations; `msync_init()` runs in `start_main_thread`.

The engine builds that tree unchanged and declares `msync` in `features`.

## Using it

- WinHub sets `WINEMSYNC=1` for bottles with msync turned on (`Bottle.msync`).
- The server and its processes must agree: `ntdll` refuses a process without `WINEMSYNC` when the bottle's
  `wineserver` was started with it. After changing the setting, end the bottle (`wineserver -k`) before
  starting programs again.
- Check it is on: `WINEDEBUG=+msync` prints `msync: up and running`.

## History

The first plan (October 2026) was to write such a backend from scratch, because the open
[wine-msync](https://github.com/marzent/wine-msync) patches stop at Wine 9.15 and Wine 10 reorganized
synchronization around `inproc_sync`. CodeWeavers' tree already does exactly that, so the plan was dropped when
the engine moved to their sources (revision 2). See [Decisions](decisions.md).

## Still to measure

FPS and frame time of a CPU-bound game with and without `WINEMSYNC=1`, and Wine's own synchronization tests
(`dlls/ntdll/tests/sync.c`, `dlls/kernel32/tests/sync.c`) with and without it.
