[English](SECURITY.md) · **Português (Brasil)**

# Política de segurança

O winhub-engine roda programas do Windows, que têm o mesmo acesso aos seus arquivos que qualquer app que você
abre. O motor não os isola. O que ele protege é a **cadeia de fornecimento**: o que é compilado, o que o WinHub
baixa e as regras que mudam como os programas abrem.

## Como reportar uma vulnerabilidade

Não abra uma issue pública com um exploit funcional, um segredo ou um arquivo malicioso. Abra uma issue dizendo
só que você tem um relato de segurança e pedindo um canal privado, ou use o relato privado de vulnerabilidades
do GitHub neste repositório, se estiver ligado. Inclua a revisão ou versão do motor, a versão do macOS e o
modelo do Mac, a parte afetada (scripts de build, `winhubcompat`, o banco de regras, um patch) e os passos para
reproduzir.

## Cadeia de confiança

| Etapa | Proteção |
|---|---|
| Entradas do build | todo download fixado por SHA-256 ou commit no `versions.env`; conferido pelo `scripts/common.sh` (`fetch`, `clone_pinned`) |
| Build | GitHub Actions a partir do commit da tag; nada de binários no Git |
| Arquivos da versão | `SHA256SUMS`; o WinHub confere o SHA-256 do pacote base e do overlay antes de instalar |
| Instalação do overlay | o WinHub só remove as pastas listadas em `overlay.replace` e recusa caminhos que saiam da pasta do Wine |
| Banco de regras | assinatura Ed25519 sobre os bytes exatos; o WinHub só o aceita com assinatura válida e `timestamp` mais novo |
| Regras em execução | o `winhubcompat` confia nos arquivos do disco: o banco embutido, o que o WinHub conferiu e o arquivo da própria garrafa |

## A chave de assinatura

- A chave privada assina o `data/compatdb.json`. Ela fica fora de qualquer repositório, por padrão em
  `~/.winhub-signing/compatdb-ed25519.key` (modo 600), e nunca é commitada nem enviada ao CI.
- A chave pública é o `data/compatdb-public-key.txt` e está embutida no WinHub (`CompatDatabaseSync.publicKey`).
- Guarde uma cópia da chave privada num gerenciador de senhas.

**Se a chave vazar ou se perder:** gere um par novo, troque o `data/compatdb-public-key.txt` e a chave no WinHub,
assine o banco de novo e publique uma versão nova do WinHub. Versões antigas do WinHub continuam confiando na
chave antiga até serem atualizadas, então revogue o quanto antes subindo o `timestamp` do banco com a chave nova.

## Regras podem mudar o comportamento

Uma regra pode definir variáveis de ambiente, acrescentar overrides de DLL e acrescentar argumentos à linha de
comando de um programa. Por isso as regras baixadas precisam ser assinadas. As regras do `compat-rules.json` de
uma garrafa são escritas pelo WinHub e valem só para aquela garrafa.
