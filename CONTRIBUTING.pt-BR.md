[English](CONTRIBUTING.md) · **Português (Brasil)**

# Como contribuir

Obrigado pelo interesse no winhub-engine.

## Regras de "sala limpa" (obrigatórias)

- **Estudamos o CrossOver só pelo comportamento:** o que ele faz, como o pacote é organizado, que arquivos e
  variáveis usa. Os achados vão para [`docs/reverse-engineering.md`](docs/reverse-engineering.md) e o espelho em
  [`docs/pt-BR/`](docs/pt-BR/reverse-engineering.md), descritos como comportamento.
- **Nunca copie** código, binários, textos, traduções, ícones ou dados do CrossOver. Isso inclui o
  `cxcompatdb.so`, o banco cifrado `compatdb-*.dat` (nunca o decifre), o catálogo `crossover.tie` e a interface
  do app.
- **O código que a CodeWeavers publica sob a LGPL** (a árvore do Wine do CrossOver) pode ser usado com
  atribuição; as versões levam as fontes correspondentes.
- **Não commite** código descompilado, endereços, nomes de símbolos internos nem logs da análise. Nomes que
  aparecem em material público (variáveis lidas pelo código LGPL, nomes de arquivos do pacote) podem ser citados.
- **Nunca redistribua o D3DMetal da Apple.**
- **Nada de binários no Git.** Entradas novas são fixadas por SHA-256 ou commit no `versions.env` e listadas no
  [THIRD_PARTY_NOTICES](THIRD_PARTY_NOTICES.pt-BR.md).

Ao abrir um pull request, você declara que a contribuição é sua e segue estas regras.

## Desenvolvimento

Veja [Compilação](docs/pt-BR/building.md) e [Testes](docs/pt-BR/testing.md). Antes de um pull request:

- o `./scripts/test-compatdb.sh` passa;
- uma mudança no Wine, no DXMT, no DXVK ou no `winhubcompat` passa no `./scripts/smoke-test.sh`;
- uma mudança no `data/compatdb.json` sobe o `timestamp` e é assinada (`swift scripts/compatdb.swift sign`), veja
  [Publicação](docs/pt-BR/releasing.md#regras-de-compatibilidade);
- um patch novo tem um cabeçalho dizendo por que existe e uma entrada em [Patches](docs/pt-BR/patches.md);
- o `ENGINE_REVISION` sobe quando `versions.env`, `patches/` ou `src/` mudam.

## Documentação

Inglês e português do Brasil andam juntos: toda página de `docs/` tem o espelho em `docs/pt-BR/`, e toda página
da raiz tem o seu `.pt-BR.md`. Mude os dois no mesmo pull request.

## Commits

Pequenos e focados, com o assunto em inglês no imperativo.
