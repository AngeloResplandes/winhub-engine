[English](../decisions.md) · **Português (Brasil)**

# Decisões

Registro datado das decisões que deram forma ao motor, das mais novas para as mais antigas.

## 2026-10-16: O Direct3D 12 continua com o D3DMetal

Um spike de um dia procurou um jeito de jogar Direct3D 12 sem o D3DMetal do próprio usuário (Game Porting Toolkit)
e não achou. O Direct3D 12 jogável ainda precisa do D3DMetal, e o próximo passo do WinHub é o guia do Game Porting
Toolkit no app. Medido na revisão 5, em Apple silicon com o MoltenVK 1.4.0:

- **vkd3d do Wine** (o do próprio pacote): com o `dxgi` do Wine ele cria um device no feature level 11_0, sem
  geometry shaders, stream output, depth clip nem cull distance (o MoltenVK não os tem); não foi testado com um jogo
  de verdade. No Automático sem o D3DMetal (e em processos de 32 bits) o `dxgi` do DXMT vem na frente e o vkd3d não
  cria device: o `dxgi` do DXMT v0.80 responde `E_NOINTERFACE` à interface de adaptador que o vkd3d pede. Contorno de
  hoje, medido: escolher o backend Wine (wined3d) ou o DXVK faz o vkd3d do Wine criar o device no 11_0.
- **vkd3d-proton v3.0.1 sobre o MoltenVK:** para nos requisitos. O MoltenVK 1.4.0 não tem
  `VK_EXT_transform_feedback`, nem `robustBufferAccess2` e `nullDescriptor` (`VK_EXT_robustness2`), e não
  oferece o alinhamento de um só texel nos texel buffers (informa 16 bytes). Só os recursos do MoltenVK foram
  sondados; o vkd3d-proton não foi compilado.
- **O D3D12 do próprio DXMT** (commit oficial `50102f9`, LGPL-2.1, compilado com `-Denable_d3d12`): o programa de
  fumaça funciona em 64 bits (device, limpar e conferir a leitura, 30 quadros apresentados), e ele também resolve o
  bloqueio do Automático. Mas é inicial: feature level limitado a 11_1, shader model 5_1, shaders DXIL (SM 6) recusados com
  `E_NOTIMPL`, e os de geometria, hull, domain e stream output também. Em 32 bits ele aborta. O teste de fumaça de Direct3D 11
  é idêntico à da revisão 5.

Onde cada caminho termina: o vkd3d do Wine cria um device no 11_0 sem geometry shaders, stream output, depth clip
nem cull distance, e não foi testado com um jogo de verdade; o vkd3d-proton exige recursos do Vulkan que o MoltenVK
não tem e o projeto não altera o MoltenVK; o D3D12 do DXMT não roda o que os jogos atuais trazem (DXIL e tesselação
são comuns).

Deixado de lado de propósito: jogar um jogo Direct3D 12 de verdade. O D3D12 do DXMT recusa DXIL e tesselação, que os
jogos modernos costumam exigir, então um teste nele falharia no primeiro shader e não mediria nada novo; o vkd3d do
Wine no 11_0 também não foi testado.

Próximo passo, registrado como lacuna e não como decisão de publicar (veja [Lacunas](gaps.md)): o Automático da
revisão 5 bloqueia até o vkd3d do Wine, então sem o D3DMetal (e em processos de 32 bits) um programa Direct3D 12 não
cria device, a menos que se escolha o backend Wine ou o DXVK. Duas opções a avaliar para uma revisão 6: passar para o DXMT mais novo (LGPL; resolve o bloqueio e traz D3D12 básico no
11_1 com shaders DXBC, mas troca todo o caminho de Direct3D 11), ou uma correção estreita (um patch pequeno para o
`dxgi` do DXMT entregar o adaptador ao vkd3d, ou o `winhubcompat` não pôr o `dxgi` do DXMT na frente nos programas
Direct3D 12).

As três ferramentas do spike ficam no repositório, em `tests/` e `scripts/`: `tests/d3d12-smoke.c`,
`tests/vulkan-features.c` e `scripts/build-dxmt-d3d12-spike.sh`. Elas não fazem parte do teste de fumaça e o CI não as
compila. Nenhum binário foi publicado e o motor instalado não foi tocado.

## 2026-10-15: Vídeos dos jogos pelo GStreamer com o VideoToolbox

A revisão 5 toca os vídeos dos jogos: o Media Foundation do Wine os entrega ao `winegstreamer`, que precisa do
GStreamer, e o pacote oficial não tem (o CrossOver o inclui). O motor compila o GLib 2.78.0 e o GStreamer 1.24.4
das fontes LGPL do CrossOver, e H.264 e AAC são decodificados pelo próprio macOS (VideoToolbox e AudioToolbox,
pelo `applemedia` do gst-plugins-bad) em vez de pelo FFmpeg. Por quê:

- os codecs são os do sistema, então o motor não redistribui nenhum decodificador com patentes nem código GPL
  (`-Dgpl=disabled`; sem FFmpeg, gst-libav, x264 nem gst-plugins-ugly);
- a decodificação em hardware vem de graça, e o overlay cresce só 3,7 MB compactado;
- os únicos downloads novos são o pcre2 10.42 (o GLib precisa dele) e o gvdb num commit fixado (o pacote do
  CrossOver o traz vazio); ambos são fixados e vão com as fontes da release.

Ficam de fora: WMV/WMA, MPEG-1 e Theora, que o macOS não decodifica (veja [Lacunas](gaps.md)). Os vídeos Bink não
são afetados: os jogos os tocam com código próprio, não pelo Media Foundation. As bibliotecas do GStreamer ficam em
`lib/`, junto das do pacote base, então o `overlay.replace` só ganhou o `lib/gstreamer-1.0`.

O `winhubcompat` define `GST_PLUGIN_SYSTEM_PATH_1_0` e `GST_REGISTRY_1_0` em vez de o WinHub fazê-lo, então um
WinHub anterior à revisão 5 toca vídeos sem mudar nada, e os valores da própria garrafa continuam valendo.

## 2026-10-12: As extensões da NVIDIA do DXMT entram no motor, desligadas até a garrafa pedir

A revisão 3 compila o `nvapi64` e o `nvngx` do DXMT (opções `enable_nvapi` e `enable_nvngx` do meson; cabeçalhos
NVAPI da NVIDIA, MIT, no commit que o DXMT fixa). Eles ficam em `lib/dxmt` e só são alcançados com o DXMT ativo. O
próprio DXMT os mantém inertes sem `DXMT_ENABLE_NVEXT=1`, que o WinHub liga por garrafa; por padrão o jogo vê "nenhum
dispositivo NVIDIA", como antes. O DXMT não precisa de redirecionamento do `nvngx`: o Wine resolve o
`system32\nvngx.dll`, que não existe no disco, para a builtin do caminho de DLLs.

## 2026-10-12: A documentação descreve a engenharia reversa abertamente

O projeto documenta o que foi estudado no CrossOver e como isso é reproduzido, no estilo de outras
reimplementações em "sala limpa". Código descompilado, endereços, nomes de símbolos internos e logs da análise
ficam fora do repositório. Veja [Engenharia reversa](reverse-engineering.md).

## 2026-10-12: Manter o nome de usuário, o perfil e o Desktop do Wine oficial

A árvore da CodeWeavers diz que todo usuário se chama `crossover`. Uma garrafa do WinHub feita com o Wine oficial
perdeu o login salvo da Steam ao rodar no motor, porque o Wine criou um perfil novo `C:\users\crossover`. O
`patches/wine/0003` volta ao comportamento do Wine oficial.

## 2026-10-12: O banco de regras é assinado com Ed25519

O WinHub baixa o `data/compatdb.json` da `main` e só o instala quando a assinatura confere com a chave pública
embutida no app e quando ele é mais novo que o instalado. A chave privada nunca entra no repositório. Veja
[SECURITY](https://github.com/AngeloResplandes/winhub-engine/blob/main/SECURITY.pt-BR.md).

## 2026-10-12: DXVK só em processos de 64 bits

Processos de 32 bits criam os buffers do Vulkan com memória do host, que o MoltenVK recusa (o da CodeWeavers
também). O `winhubcompat` considera o DXVK inutilizável nesses processos, que caem no DXMT.

## 2026-10-11: Compilar o Wine das fontes LGPL do CrossOver (revisão 2)

A revisão 1 era o pacote oficial do WineHQ com um `winemac.so` modificado. A revisão 2 compila o Wine inteiro do
`crossover-sources-26.3.0`, que já tem msync, WoW64 e os ganchos do `winemac.drv` que o DXMT e o D3DMetal
precisam, e mantém o pacote oficial só como base de bibliotecas, wine-mono e wine-gecko. Usar código que a
CodeWeavers publica sob a LGPL é permitido; as partes fechadas dela não são usadas.

## 2026-10-11: Escolher o backend dentro do motor

Em vez de copiar o DXMT ou o DXVK para cada garrafa (revisão 1, como o CrossOver fazia até a versão 24), o
`winhubcompat` põe a pasta do backend escolhido no começo do caminho de DLLs do Wine, por processo. As garrafas
ficam limpas e um jogo pode ter backend próprio por uma regra.

## 2026-10-11: D3DMetal só do usuário

A licença da Apple não permite redistribuir o D3DMetal. O motor o suporta quando o usuário aponta para o WinHub
o próprio Game Porting Toolkit; o projeto nunca o distribui nem o baixa.

## 2026-10-11: DXMT v0.80

A última versão do DXMT sob licença MIT; as seguintes são LGPL.
