[English](../patches.md) · **Português (Brasil)**

# Patches

Todo patch é pequeno, tem um cabeçalho dizendo por que existe e é aplicado pelo script de build dele. Os patches
do Wine são LGPL-2.1-or-later, como o Wine; os do DXMT e do DXVK seguem as licenças desses projetos.

## Wine

Aplicados pelo `build-wine.sh` sobre o `sources/wine` do pacote de fontes LGPL do CrossOver.

| Patch | Por quê |
|---|---|
| `0001-ntdll-Load-winhubcompat.so-instead-of-cxcompatdb.so` | O `ntdll` da CodeWeavers carrega um módulo fechado, o `cxcompatdb.so`, no fim do `start_main_thread`. O patch carrega o nosso módulo aberto, o `winhubcompat.so`, no mesmo ponto. |
| `0002-mscoree-appwiz.cpl-Use-wine-mono-11.3.0` | O Wine 11.0 espera o wine-mono 10.4.1, mas o pacote oficial sobre o qual o motor é aplicado traz o 11.3.0 em `share/wine/mono`. Mantenha em sintonia com o `WINE_BASE_URL`. |
| `0003-advapi32-shell32-Keep-the-real-user-name-and-the-Mac-desktop` | A árvore da CodeWeavers diz que todo usuário se chama `crossover`, põe o perfil em `C:\users\crossover` e transforma o Desktop numa pasta com um link para a Área de Trabalho do Mac dentro. As garrafas feitas com o Wine oficial usam o nome de usuário real (onde a Steam guarda o login e os jogos guardam os saves) e ligam o Desktop à Área de Trabalho do Mac. Descoberto quando uma garrafa de teste perdeu o login salvo da Steam. |

## DXMT

Aplicados pelo `build-dxmt.sh` sobre o DXMT v0.80 (commit `589adb7`).

| Patch | Por quê |
|---|---|
| `0001-Add-missing-standard-headers` | Compila com o mingw-w64 atual (correções trazidas do próprio DXMT). |
| `0002-airconv-Use-the-public-Metal-atomic-in-the-tessellation-shader` | O toolchain Metal do Xcode 27 mudou os argumentos de uma função interna que o shader chamava; a pública `atomic_fetch_add_explicit` faz a mesma soma relaxada no escopo do thread group. |

## DXVK

Aplicados pelo `build-dxvk.sh` sobre o `sources/dxvk` (1.10.3) do pacote de fontes do CrossOver. O build também
passa `-include cstdint` (a 1.10.3 é anterior ao GCC 13) e `-Denable_dxgi=false` (o Wine fica com o próprio
`dxgi`, como no CrossOver).

| Patch | Por quê |
|---|---|
| `0001-Build-with-mingw-w64-10-and-later` | Os cabeçalhos novos do mingw-w64 definem o `D3DDEVINFO_RESOURCEMANAGER` e o GUID do `ID3D10StateBlock`, então as definições substitutas do DXVK não compilam mais. |
| `0002-d3d9-d3d11-Ask-only-for-the-core-features-the-device-has` | O MoltenVK oficial não tem geometry shaders nem cull distance, então exigi-los não deixa nenhum adaptador utilizável. O CrossOver junta este DXVK a um MoltenVK próprio que diz ter esses recursos; pedir só o que existe faz o mesmo com o MoltenVK oficial. |
