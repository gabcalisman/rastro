# RASTRO FX / Orbital Playground

Plugin de áudio com rack próprio de quinze módulos. O FX recebe áudio externo,
não depende de sample ou MIDI e não substitui o instrumento PLAY nem o gravador
Capture. O código, os IDs e o comportamento desses dois produtos permanecem
inalterados nesta branch. O PR #5 não foi modificado nem mesclado.

| Produto | Bundle ID | Código / fabricante | Formatos |
| --- | --- | --- | --- |
| PLAY (binário RASTRO, preservado) | `com.gabcalisman.rastro` | `Rstr / GbCa` | AU, VST3, Standalone |
| Capture (preservado) | `com.gabcalisman.rastro.capture` | `Rcpt / GbCa` | AU, VST3 |
| FX | `com.gabcalisman.rastro.fx` | `Rfx1 / GbCa` | AU, VST3 |

AU é exclusivo do macOS. O FX não tem Standalone. Seu estado usa
`RASTRO_FX_STATE`, separado dos outros produtos. Os três parâmetros da primeira
etapa (`fxInputGain`, `fxOutputGain`, `fxBypass`) mantêm os IDs e índices; estados
antigos sem rack são completados com os novos módulos desligados.

## Rack e áudio

Arraste o cabeçalho de um card ou use **UP / DOWN**. A ordem também é automatizável
pelas posições dos módulos. Cada card tem **ON**, **Mix** e todos os controles
avançados expostos. O rack e o painel de modulação têm rolagem independente, sem
abas que escondam os controles avançados. Janela redimensionável de 980×720 a
1800×1200; testes cobrem 980×720, 1180×840 e 1440×1000.

- **SERIES**: a saída de cada módulo alimenta o próximo.
- **PARALLEL**: cada módulo recebe a mesma entrada. A saída é a média dos ramos
  ativos, cada qual com seu mix seco/molhado. Durante bypass, o peso do ramo muda
  gradualmente. Módulo ON com Mix zero conta como um ramo seco.
- Ganhos de entrada e saída: −24…+24 dB. **DRY / WET** mistura o rack com a entrada
  após o ganho de entrada. Os medidores IN/OUT mostram picos em dBFS, com queda de
  250 ms e indicação de sinal acima de 0 dBFS.
- **BYPASS** global, inclusive o callback de bypass do host, retorna ao áudio
  original antes dos ganhos. Rampas de 10 ms; ganho de entrada/saída em 20 ms.
  Ativação e mix dos módulos são suavizados. Trocar ordem ou série/paralelo faz
  uma transição de 10 ms para o sinal seco e de volta ao rack, sem silenciar o sinal seco.
- Ganho unitário com todos os módulos OFF ou com Dry/Wet zero conserva os bits
  do sinal seco. O código não passa esse caminho por tanh, limiter ou conversão
  float intermediária; preserva silêncio, sinal de zero e headroom.
- Float/double na entrada/saída; o DSP dos efeitos usa float, como o PLAY. Latência
  reportada: zero; os delays dos efeitos são parte do som, não compensação do host.
  Cauda conservadora reportada: 300 s (feedbacks longos em série). Há suavização e limitação de feedback nos
  algoritmos; não há limiter oculto na saída global.
- Memória de delays/filtros é preparada fora do callback. O callback usa apenas
  estruturas prealocadas, arrays locais e parâmetros atômicos; sem alocações C++,
  locks, arquivos ou ValueTrees. Os módulos continuam seus estados e relógios
  durante bypass, sem contribuir para a saída.

## Os quinze módulos

| Módulo | Processamento real e controles |
| --- | --- |
| GHOST | Eco filtrado herdado do PLAY: Time, Feedback, Tone, Trail. |
| CRUMBLE | Repetição de fragmentos com janela: Rate, Variation, Mutation. |
| BLOOM | Quatro linhas difusas por canal: Size, Tone, Decay, Trail. |
| WOBBLE | Atraso modulado de fita do PLAY: Depth e Rate. |
| DUST | Textura/ruído seguindo a atividade da fonte: Amount e Character. |
| CRUSH | Redução de resolução e sample-and-hold: Amount. |
| MOUTH | Dois ressonadores de formantes, interpolando vogais A/E/I/O/U: Vowel, Formant shift e Resonance. |
| ORBIT | Flanger com delay fracionário e feedback bipolar: Rate, Depth, Feedback, Centre. |
| TRAIL | Echo separado do GHOST, com tempo contínuo: Time, Feedback, Tone e Colour (saturação no feedback). |
| CHOIR | Chorus de três vozes com delays modulados: Rate, Depth, Delay e Spread; Spread também varia a disposição das vozes em mono. |
| PRISM | Pitch echo/crystal: taps móveis com duas janelas sobrepostas e transposição dentro do feedback. Pitch −12…+24 semitons, Time, Feedback e Tone. Não é apenas um echo com nome diferente. |
| EMBER | Transferências diferentes de drive, tube assimétrico e tape com memória magnética simplificada; Drive, Tone, Bias e morph Drive/Tube/Tape. DC blocker após a saturação. Não pretende modelar um aparelho físico específico. |
| SQUASH | Saturação combinada com compressor de envelope: Drive, Threshold, Ratio e Release; ataque de 5 ms e detecção estéreo vinculada. |
| SIEVE | Filtro ressonante de estado variável: Cutoff, Resonance e morph LP/BP/HP. Frequência limitada de forma segura abaixo de Nyquist. |
| SWING | Panner com oito pontos editáveis no próprio card: Rate, Depth, Slew e Phase. Em estéreo controla o balanço; em mono faz movimento de amplitude, pois não existe um segundo canal de saída. |

Os seis módulos herdados estão em uma adaptação FX de `EffectRack`, mantendo os
algoritmos e removendo apenas a coloração global específica do instrumento.
`EffectRack.h`, `CharacterEffects.h`, `Engine` e os processadores PLAY/Capture
continuam intactos. Os testes comparam esses algoritmos amostra por amostra com o
PLAY antes da saturação global. O FX tem instâncias separadas desses estados,
permitindo intercalar os seis módulos com os novos na ordem desejada.

## MOD / XY

Arraste o pad; duplo clique centraliza. X e Y têm destino e amount bipolares
independentes. **None** desativa a rota. Cada rota pode controlar ganho de
entrada/saída, Dry/Wet global, controles/mix de qualquer módulo ou um ponto do
panner. O deslocamento XY é calculado em torno do centro.

O LFO oferece seno, triângulo, serra e quadrado, 0,05…20 Hz, destino e amount.
O envelope follower tem sensibilidade −24…+24 dB, ataque 1…200 ms, release
10…2000 ms, destino e amount. Sua fonte é o pico da entrada antes do ganho global.
LFO é bipolar; envelope é unipolar, com amount positivo ou negativo. As rotas se
somam no valor normalizado e são limitadas ao intervalo do destino; as mudanças
de modulação são suavizadas em 5 ms e não sobrescrevem os valores-base da automação.
As barras mostram os sinais reais do LFO e do follower.

## PRESETS / A/B / aparência

Dez programas de fábrica incluem INIT, Ghost postcard, Crumbling bloom, Dusty
cassette, Talking moon, Orbital choir, Crystal staircase, Velvet furnace, Rubber
postcard e Swing machine. INIT desliga todos os módulos.

Edite o nome, a cor (Mint/Coral/Lilac/Honey/Sky) e a superfície
(Paper/Night/Bubble). **SAVE / OPEN** grava/carrega arquivos `.rastrofx`; o diálogo
começa na pasta `RASTRO FX/Presets` dos dados de aplicação do usuário. O arquivo
inclui som, roteamento, modulação, nome, aparência e ambas as posições A/B. A
escrita usa arquivo temporário e substituição final; arquivo inválido é recusado
sem alterar o estado.

**A > B / B > A** troca comparações reais. **Copy to A/B** copia a configuração
atual para a outra posição. A sessão do host também conserva os dois bancos e a
posição selecionada. Estado de PLAY ou Capture não é aceito como preset FX.
A interface foi desenhada para o FX: cards de pedais, faces orbitais, knobs e
cores cartunescas. Usa apenas a marca RASTRO do asset existente, sem o sufixo PLAY.
Não constitui aprovação ou conclusão do redesign visual do PR #5.

## Compilar e validar

C++17, CMake 3.22+ e JUCE 8.0.6. Linux precisa dos headers/libs X11, ALSA,
FreeType/fontconfig e OpenGL; o editor FX é nativo, sem WebView.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DRASTRO_INTEGRATION_TESTS=ON
cmake --build build --parallel 3
xvfb-run -a ctest --test-dir build --output-on-failure
```

A suíte gráfica PLAY precisa também de um gerenciador de janelas no Xvfb; a
validação deste ambiente usa `dbus-run-session` + `xfwm4 --compositor=off` antes do
CTest. Em uma sessão gráfica normal não é necessário Xvfb. Para somente o FX:

```sh
cmake --build build --target RastroFx_VST3 rastro_fx_tests --parallel 3
xvfb-run -a ctest --test-dir build -R '^fx_processor$' --output-on-failure
```

`RASTRO_CORE_ONLY=ON` continua limitado às oito suítes portáveis do PLAY; não
substitui os testes JUCE do processador/editor FX.

A suíte FX cobre transparência bit a bit, float/double, mono/estéreo, taxas e
blocos variáveis; os seis algoritmos herdados; áudio real e controles avançados
dos nove módulos novos; transposição espectral PRISM; ordem, paralelo, dry/wet,
modulação, follower, bypass de rack/módulo/host, parâmetros reais da UI, edição
XY/panner, presets, ambos os bancos A/B, recall de áudio/estado, migração da
primeira etapa, alocações C++ no callback e render/layout do editor.
As capturas `fx-editor-*.png` são regeneradas nos testes, não são mockups.

## Builds nativos e instalação

Ambos os workflows têm `push` restrito a `codex/rastro-fx-foundation`, além de seus
gatilhos existentes. O Mac gera PLAY AU/VST3/Standalone, Capture AU/VST3 e FX
AU/VST3, universal arm64+x86_64. O Windows gera PLAY VST3/Standalone, Capture VST3
e FX VST3, x64. Os ZIPs incluem `RASTRO-FX.md`, capturas offscreen reais em
`Previews/FX` e BUILD-INFO com branch/commit e verificações do próprio run.

No Mac, copiar **todo** `AU/RASTRO FX.component` para
`~/Library/Audio/Plug-Ins/Components/`. No Logic, usar **Audio FX → Gab Calisman →
RASTRO FX** numa faixa de áudio/aux, não no slot de instrumento. O workflow valida
`auval -v aufx Rfx1 GbCa`, pluginval 1.0.3 nível 5 seed 42 no VST3 e arquitetura/
assinatura dos sete bundles. AU/VST3 continuam ad-hoc, sem notarização.

No Windows, copiar **todo** `VST3/RASTRO FX.vst3` para
`C:\Program Files\Common Files\VST3\` e usar como insert de áudio na DAW. O
workflow exige quatro executáveis PE AMD64 no pacote. Não executa pluginval.

Build Linux, capturas de editor e teste com sinal sintético não comprovam
instalação, Logic, Windows, escuta musical, áudio/MIDI físico ou execução arm64.
A produção dos artefatos nativos e seus validadores só pode ser confirmada pelos
resultados dos workflows desta revisão; não deve ser inferida do sucesso local.

## Evidência local desta revisão

- Build Release Linux completo: passou; PLAY Standalone/VST3, Capture VST3 e FX VST3.
- CTest: **12/12 suítes passaram**, incluindo a suíte ampliada do FX.
- VST3 FX real no pluginval 1.0.3 Linux: **passou**, nível 5, seed 42, incluindo
  processamento em 44,1/48/96 kHz, automação, estado, editor e barramentos mono/
  estéreo. O validador Steinberg SDK opcional não foi fornecido; AU não existe no Linux.
- actionlint, sintaxe PowerShell e empacotamento Windows com fixtures PE:
  passaram, incluindo rejeição de FX ausente/não AMD64 e binário excedente.
- Empacotamento Mac com ferramentas nativas simuladas: passou, incluindo os sete
  bundles, capturas, logs FX e checksum. Isso não executa lipo/codesign/auval reais.
- Logs em [validation-fx](validation-fx/linux-tests.txt) e
  [capturas reais do FX](previews-fx/README.md).

A consulta à API do GitHub Actions pelo ambiente recebeu `CONNECT tunnel failed,
response 403` em `api.github.com`. O Git HTTPS funciona. Os workflows estão
configurados para iniciar ambos os builds por push nesta branch; sem acesso aos
resultados do Actions, o sucesso dos builds Mac/Windows e a disponibilidade dos
ZIPs desta revisão permanecem **não confirmados**. Nenhum teste no Logic ou
hardware físico foi realizado neste host Linux.
