[English](../README.md) · **Português (Brasil)**

# Documentação do winhub-engine

O winhub-engine é o motor Wine por trás do [WinHub](https://github.com/AngeloResplandes/winhub), o app de
macOS que instala e roda programas do Windows. É o Wine 11 compilado das fontes LGPL que a CodeWeavers publica
para o CrossOver, mais DXMT, DXVK e o `winhubcompat`, um módulo aberto e pequeno que escolhe o Direct3D de cada
processo do Wine sem copiar nada para as garrafas.

O desenho do motor saiu do estudo de como o CrossOver 26.3 roda jogos, só pelo comportamento:
[Engenharia reversa](reverse-engineering.md) explica o que foi estudado, o que se aprendeu e onde fica o limite.
Nenhum código ou dado do CrossOver fora das fontes LGPL publicadas está neste projeto.

## Por onde começar

| Se você quer | Leia |
|---|---|
| Entender do que o motor é feito e como o backend é escolhido | [Arquitetura](architecture.md) |
| Compilar no seu Mac | [Compilação](building.md) |
| Rodar os testes, ou testar jogos sem arriscar garrafas reais | [Testes](testing.md) |
| Publicar uma versão | [Publicação](releasing.md) |
| Escrever ou mudar regras de compatibilidade | [winhubcompat e regras de compatibilidade](compatdb.md) |
| Saber por que cada patch existe | [Patches](patches.md) |
| Ver o quanto ele se aproxima do motor do CrossOver | [Paridade com o CrossOver](crossover-parity.md) e [Lacunas](gaps.md) |
| Saber por que as coisas são como são | [Decisões](decisions.md) |

Arquivos do repositório que não fazem parte deste livro:
[README](https://github.com/AngeloResplandes/winhub-engine/blob/main/README.pt-BR.md),
[ROADMAP](https://github.com/AngeloResplandes/winhub-engine/blob/main/ROADMAP.pt-BR.md),
[SECURITY](https://github.com/AngeloResplandes/winhub-engine/blob/main/SECURITY.pt-BR.md),
[CONTRIBUTING](https://github.com/AngeloResplandes/winhub-engine/blob/main/CONTRIBUTING.pt-BR.md),
[THIRD_PARTY_NOTICES](https://github.com/AngeloResplandes/winhub-engine/blob/main/THIRD_PARTY_NOTICES.pt-BR.md).
