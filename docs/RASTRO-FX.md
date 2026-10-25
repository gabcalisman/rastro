# RASTRO FX / Orbital Playground

Plugin de áudio com rack próprio de quinze módulos. O FX recebe áudio externo,
não depende de sample ou MIDI e não substitui o instrumento PLAY nem o gravador
Capture. O código, os IDs e o comportamento desses dois produtos permanecem
inalterados nesta branch. O PR #5 não foi modificado nem mesclado.

| Produto | Bundle ID | Código / fabricante | Formatos |
| --- | --- | --- | --- |
| PLAY (binário RASTRO, preservado) | `com.gabcalisman.rastro` | `Rstr / GbCa` | AU, VST3, Standalone |
| Capture (preservado) | `com.gabcalisman.rastro.capture` | `Rcpt / GbCa` | AU, VST3 |
| FX | `com.gabcalisman.rastro.fx` | `Rfx1 / GbCa` | AU, VST3, AAX (Mac), Standalone |

AU é exclusivo do macOS. O AAX (Pro Tools, categoria Effect) é gerado no Mac com o SDK que vem no JUCE 8; sem assinatura PACE/iLok ele só abre no Pro Tools Developer. O Standalone do FX processa a entrada de áudio escolhida (use fones para evitar microfonia). Seu estado usa
`RASTRO_FX_STATE`, separado dos outros produtos. Os três parâmetros da primeira
etapa (`fxInputGain`, `fxOutputGain`, `fxBypass`) mantêm os IDs e índices; estados
antigos sem rack são completados com os novos módulos desligados.

## Interface

No macOS e no Windows o FX abre uma interface web (`ui-fx/`, embutida no plugin e servida pelo
`RastroFxWebEditor`: WKWebView no Mac, Edge WebView2 no Windows com o loader linkado
estaticamente e dados em uma pasta temporária gravável). Sem o runtime do WebView2, o plugin abre
o editor nativo. Em Linux, sem WebView2 e nos testes continua o editor nativo `RastroFxEditor`, cujo cabeçalho agora mostra só
**RASTRO FX by Gab Calisman**.

- **Família PLAY:** logo oficial do RASTRO (`ui-fx/logo-rastro.png`, recortado de `ui/logo.png`),
  fontes RastroSerif/RastroSans, painéis violeta e knobs com arco neon. No palco fica o
  **produtor de luz** do PLAY (`ui-fx/viz.js`, copiado da branch `claude/etapa-1-som` com as cores
  estendidas aos 15 módulos), desenhado pela forma de onda real da saída. Os transientes da
  entrada disparam anéis na cor do módulo mais ativo, com ecos violeta quando o GHOST está ligado.
- **Cada efeito é um aparelho:** material próprio (vidro, cerâmica, laca, fita, vinil, LCD, veludo,
  cosmos, cobre, borracha, aço, neon…), letreiro, knobs no material do aparelho, LED, footswitch e
  um display animado que desenha o que o módulo faz com os valores atuais. O display do SWING é o
  editor do padrão de 8 pontos.
- **Cadeia:** arraste o topo de um aparelho para mudar a ordem; arraste um efeito da prateleira
  para dentro da cadeia (ou toque nele) para inserir; × tira da cadeia. SÉRIE/PARALELO no
  cabeçalho; no paralelo, o cabo se divide e a dica do painel explica a soma das saídas.
- **Modulação:** pad XY com rastro, destinos agrupados por módulo e quantidade bipolar; LFO e
  follower com osciloscópio do sinal real de modulação.
- **Presets/A-B:** navegador de fábrica, SALVAR/ABRIR `.rastrofx`, A/B e copiar para o outro lado.

Toda ação escreve um parâmetro real do host (com gesto de automação); o que se move vem de
telemetria real: picos de entrada/saída, LFO, follower e um osciloscópio da saída (anel
sem locks de 512 pontos, 1 a cada 4 amostras; só leitura pela interface, não altera o áudio).
Aberto fora do plugin, `ui-fx/index.html` usa um motor simulado, identificado na tela.
Compatibilidade: WebKit do macOS com Safari 15 ou mais novo (polyfill de `roundRect` e
fallbacks para `color-mix`).

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
  original (alinhado à latência) antes dos ganhos. Rampas de 10 ms; ganho de entrada/saída em 20 ms.
  Ativação e mix dos módulos são suavizados. Trocar ordem ou série/paralelo faz
  uma transição de 10 ms para o sinal seco e de volta ao rack, sem silenciar o sinal seco.
- Ganho unitário com todos os módulos OFF ou com Dry/Wet zero conserva os bits
  do sinal seco, **atrasado pela latência reportada**. O seco é copiado no próprio tipo do
  host (float ou double), nunca convertido nem recalculado; preserva silêncio, zero com
  sinal, subnormais e headroom.
- Float/double na entrada/saída; o DSP dos efeitos usa float, como o PLAY.
  **Latência reportada: 46 amostras, constante** em qualquer configuração e taxa (0,96 ms a
  48 kHz; 0,48 ms a 96 kHz). Ela vem do oversampling 2x de EMBER e SQUASH (23 amostras cada).
  Os dois ficam sempre na cadeia — como atraso puro quando não estão no rack — para que a
  latência nunca mude com presets, A/B ou automação. Série, paralelo, Dry/Wet, bypass de
  módulo, bypass global e bypass do host estão alinhados à mesma latência; o host compensa.
  Cauda conservadora reportada: 300 s. Sem limiter oculto na saída global.
- O callback roda com flush de denormais (`ScopedNoDenormals`). Caudas de feedback não
  geram mais amostras subnormais (ORBIT chegava a 4e-45 e custava 4,5x mais CPU na cauda).
- Módulos fora do rack (InRack desligado e já sem fade) dormem: não processam, têm seus
  buffers limpos em fatias pequenas e reiniciam limpos ao entrar de novo — nunca tocam áudio
  de antes de sair. Rack vazio: ~0,16 µs/amostra estéreo contra ~3,4 µs antes (medição Linux
  x86 deste ambiente; ver tabela de CPU abaixo).
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
| ORBIT | Flanger com delay fracionário e feedback bipolar: Rate, Depth, Feedback, Centre. Entrada limpa; só a soma recirculante passa por um limitador suave transparente até 0 dBFS (teto +3,5 dBFS). |
| TRAIL | Echo separado do GHOST, com tempo contínuo: Time, Feedback, Tone e Colour (saturação no feedback). Colour 0 é um fio exato: as repetições só perdem o que o Tone tira. |
| CHOIR | Chorus de três vozes com delays modulados: Rate, Depth, Delay e Spread; Spread também varia a disposição das vozes em mono. Soma das vozes normalizada (×0,52) entre soma coerente e de potência. |
| PRISM | Pitch echo/crystal: taps móveis com duas janelas sobrepostas e transposição dentro do feedback. Pitch −12…+24 semitons, Time, Feedback e Tone. Mesmo limitador suave do ORBIT no feedback. Não é apenas um echo com nome diferente. |
| EMBER | Transferências diferentes de drive, tube assimétrico e tape com memória magnética simplificada; Drive, Tone, Bias e morph Drive/Tube/Tape. **Oversampling 2x**; memória magnética (~977 Hz) e DC blocker (~38 Hz) independentes da taxa. Ganho compensado: um pico de −10 dBFS sai em −10 dBFS em qualquer Drive/modo. Não pretende modelar um aparelho físico específico. |
| SQUASH | Saturação combinada com compressor de envelope: Drive, Threshold, Ratio e Release; ataque de 5 ms e detecção estéreo vinculada. **Oversampling 2x**; make-up automático (pico de −10 dBFS volta a −10 dBFS) terminando num teto suave de +3,5 dBFS, então transientes antes do ataque não explodem. |
| SIEVE | Filtro ressonante de estado variável: Cutoff, Resonance e morph LP/BP/HP. Frequência limitada de forma segura abaixo de Nyquist. |
| SWING | Panner com oito pontos editáveis no próprio card: Rate, Depth, Slew e Phase. Em estéreo controla o balanço; em mono faz movimento de amplitude, pois não existe um segundo canal de saída. |

Os seis módulos herdados estão em uma adaptação FX de `EffectRack`, removendo a
coloração global específica do instrumento. GHOST e BLOOM usam ADAA residual nos
caminhos de feedback (uma memória por linha no BLOOM). As divisões de nota do GHOST e do
CRUMBLE seguem o BPM do host, como no PLAY (antes ficavam presas a 120 BPM). `EffectRack.h`, `CharacterEffects.h`, `Engine` e os
processadores PLAY/Capture continuam intactos. CRUMBLE, WOBBLE, DUST e CRUSH continuam
comparados amostra por amostra com o PLAY; GHOST/BLOOM diferem intencionalmente apenas
nos seus shapers de feedback antialiased. O FX tem instâncias separadas desses estados,
permitindo intercalar os seis módulos com os novos na ordem desejada.

## Qualidade sonora: decisões e medições

Medido offline com o DSP real (`Source/FxDsp.h`) sobre material musical a −18 dBFS RMS
(voz+violão, bateria+baixo e um mix completo, Creative Commons) e sinais sintéticos.
São **medições objetivas**; a avaliação auditiva é feita com o pacote de escuta cega
(antes/depois nivelados por loudness), que não substitui a escuta no Logic.

**Aliasing.** O ADAA de primeira ordem (quadratura de Gauss) está correto, mas medido com
música e uma referência oversampled 16x, nos estágios de ganho alto (EMBER/SQUASH) ele não
reduzia o erro em relação ao shaper direto em drive baixo/médio e escurecia sempre: −2 dB em
10 kHz e −6 dB em 16 kHz (ganho linear do filtro de média). Só ajudava em drive extremo na
bateria (~3 dB). Decisão por estágio:

| Estágio | Antes | Agora | Por quê |
| --- | --- | --- | --- |
| EMBER, SQUASH (ganho até 24x/12x) | ADAA | Oversampling 2x, halfband FIR 47 taps, fase linear | Bateria, drive 10,8x: erro não coerente −36 → −55 dB; drive 24x: −32 → −44 dB; sem perda de agudos (±0,01 dB até 20 kHz). Tom 3011 Hz, tanh ×10,8: −29 → −55 dB alias/fundamental. Custa 23 amostras de latência. 4x (2 estágios) não justificou o custo/latência. |
| GHOST, BLOOM, ORBIT, TRAIL, PRISM (clippers de ganho unitário no feedback) | ADAA na curva inteira | ADAA **residual** (só `f(u) − u` é integrado) | Na região linear passa bit a bit: as repetições do TRAIL limpo não perdem mais ~2 dB em 10 kHz por volta, nem ganham meio sample de atraso. O antialiasing continua nas partes que clipam. Um teto explícito cobre saltos patológicos entre amostras. |
| ORBIT, PRISM | `tanh` na entrada mesmo com feedback 0 | Limitador suave transparente até 0 dBFS, teto +3,5 dBFS | Picos de −6 dBFS ganhavam ~−0,7 dB e 3º harmônico. |

**Ganho e A/B.** Mudança de loudness integrada (BS.1770) com mix 100% contra o seco:

| Caso | Antes | Agora |
| --- | --- | --- |
| EMBER padrão | +4,9 a +5,6 LU | −0,7 a 0 LU |
| EMBER drive máximo | +13,8 a +16,5 LU, pico +5 dBFS | +3,8 a +6,5 LU, pico −5 dBFS |
| SQUASH Threshold −36 dB, Ratio 16:1 | −20,7 LU | +2,0 a +2,4 LU (pico ≤ +4,7 dBFS) |
| CHOIR padrão | −5,1 a −7,0 LU | −1,2 a −3,1 LU |
| Preset Velvet furnace | +3,6 a +4,2 LU | −0,6 a −0,1 LU |
| Preset Rubber postcard | −2,0 a −2,3 LU | −0,3 a −0,1 LU |

Presets sem EMBER/SQUASH/CHOIR ficam no mesmo nível (±0,3 LU). A saturação pesada ainda
soa mais alta que o seco por densidade (crest factor menor); isso é intencional e
mensurável acima. MOUTH (−3 a −7 LU), SIEVE com ressonância máxima (+1 a +3 LU) e o BLOOM
100% wet (−15 LU, como no PLAY) não foram compensados nesta rodada.

**CPU** (ns por amostra estéreo, só o rack, Linux x86 deste ambiente, com flush de
denormais; serve para comparação relativa, não substitui a medição no Logic/Apple silicon):

| Configuração | Antes | Agora |
| --- | --- | --- |
| Rack vazio (nenhum módulo inserido) | ~3400 | ~160 |
| Só EMBER inserido | ~3000 (15 nós rodando) | ~490 |
| Só SQUASH inserido | ~3000 | ~360 |
| Todos os 15 inseridos | ~3100 | ~3100 |

A cauda do ORBIT sem flush de denormais custava 4,5x mais; com o flush no callback, volta ao
custo normal. O processador ainda suaviza ~90 alvos de parâmetro por amostra fora do rack.

**Limites conhecidos.** O oversampling 2x não elimina o aliasing de fundamentais muito agudas
com drive extremo (harmônicos acima de 72 kHz ainda dobram). A 44,1 kHz a faixa de transição
do filtro começa em 18,4 kHz. Constantes por amostra do rack herdado (tons do GHOST/BLOOM)
continuam dependentes da taxa, por paridade com o PLAY. Nada disso foi ouvido no Logic ou em
Apple silicon neste ambiente.

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

A suíte FX cobre transparência bit a bit após a latência reportada (blocos variáveis,
subnormais, zero com sinal), float/double, mono/estéreo, taxas e blocos variáveis; os seis algoritmos herdados; áudio real e controles avançados
dos nove módulos novos; transposição espectral PRISM; ordem, paralelo, dry/wet,
modulação, follower, bypass de rack/módulo/host, parâmetros reais da UI, edição
XY/panner, presets, ambos os bancos A/B, recall de áudio/estado, migração da
primeira etapa, alocações C++ no callback e render/layout do editor.
As capturas `fx-editor-*.png` são regeneradas nos testes, não são mockups.
O grupo **FX sound** (`tests/FxSoundQualityTests.h`) cobre: latência inteira, ganho DC e
passband do oversampler; supressão de alias do estágio 2x; ADAA residual bit a bit na região
linear e teto em saltos extremos; brilho das repetições do TRAIL limpo; gain staging de
EMBER/SQUASH e teto de transientes; caráter do EMBER igual a 48 e 96 kHz; rack vazio como
atraso exato em série e paralelo; módulo reinserido sem eco antigo; custo de módulos fora do
rack; GHOST seguindo o BPM (direto e via playhead do host); ausência de saídas subnormais;
saída bit-idêntica com blocos de 1, 17 e 512 amostras; automação de Drive sem degraus.

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

Rodada de interface (branch `claude/rastro-fx-ui`, sobre a de som): `rastro_fx_tests` passou;
`FxWebEditor.cpp` e `FxProcessor.cpp` compilaram com `RASTRO_FX_WEB_UI=1`/`JUCE_WEB_BROWSER=1`
contra os headers do JUCE 8.0.6 (sem WebKit no Linux, o binário web só é gerado no Mac).
Interações testadas no Chromium com o motor simulado: inserir por toque, arrastar da prateleira,
reordenar, remover, footswitch, knobs, paralelo, pad XY e cadeia vazia, sem erros de JavaScript.
Não foi testado no WebKit/Safari nem no Logic neste ambiente.

Rodada de som (branch `claude/rastro-fx-sound`): build Release Linux do alvo FX (VST3 +
`rastro_fx_tests`) com JUCE 8.0.6; `rastro_fx_tests` **passou** (todos os grupos, incluindo
FX sound); pluginval 1.0.3 nível 5, seed 42, no VST3 FX: **passou** (sem testes de GUI; ele
imprime latência 0 porque consulta antes do `prepareToPlay`, e o host VST3 do JUCE relê a
latência na preparação). O resto desta seção descreve a revisão anterior.

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
