[English](../releasing.md) · **Português (Brasil)**

# Publicação

## Versão do motor

1. Atualize o [`versions.env`](https://github.com/AngeloResplandes/winhub-engine/blob/main/versions.env) e aumente
   o `ENGINE_REVISION` se algo nele, em `patches/` ou em `src/` mudou.
2. Faça o push de uma tag `wine-<WINE_VERSION>-r<ENGINE_REVISION>`, por exemplo `wine-11.0-r2`. A tag precisa
   bater com o `versions.env`; senão o workflow a recusa.
3. O CI compila, roda o teste de fumaça e cria a release com o overlay, o `cabextract`, as fontes
   correspondentes (o pacote de fontes do CrossOver, cJSON, cabextract, este repositório), o `SHA256SUMS` e o
   `winhub-manifests.json`.

Toda versão precisa levar as fontes correspondentes: o Wine dela é LGPL.

## Ordem com o WinHub

O WinHub só instala o que o `Sources/WinHub/Resources/runtimes.json` dele lista. Versões antigas do WinHub não
entendem o `overlay.replace` e desempacotariam um overlay r2 por cima do pacote oficial sem apagar o `lib/wine`
dele. Então:

1. publique uma versão do WinHub que entenda `overlay.replace` e `backend-dispatch` (a 0.2.7 e as seguintes
   entendem);
2. publique a versão do motor;
3. só então ponha a entrada `runtime` do `winhub-manifests.json` no `runtimes.json` do WinHub, em primeiro.

## Regras de compatibilidade

O banco de regras que o WinHub baixa é o
[`data/compatdb.json`](https://github.com/AngeloResplandes/winhub-engine/blob/main/data/compatdb.json) da `main`,
com a assinatura `data/compatdb.json.sig`. Ele não depende das versões do motor. Para mudá-lo:

1. edite as regras e **aumente o `timestamp`** (segundos desde 1970); o WinHub recusa um banco que não seja mais
   novo que o que já tem;
2. assine: `swift scripts/compatdb.swift sign` (chave privada de `~/.winhub-signing/compatdb-ed25519.key` ou
   de `$WINHUB_COMPATDB_KEY`);
3. faça commit dos dois arquivos; o CI roda `swift scripts/compatdb.swift verify`.

Guarda da chave e o que fazer se ela vazar:
[SECURITY](https://github.com/AngeloResplandes/winhub-engine/blob/main/SECURITY.pt-BR.md).
