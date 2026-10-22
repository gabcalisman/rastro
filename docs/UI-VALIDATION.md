# Etapa 1 — refinamento visual e validação

Base: commit `4f27e91` + alterações locais anteriores preservadas, reunidas na branch `prepare-mac-testing` do PR #1 para teste no Mac. O PR permanece em rascunho, sem merge.
UI, parâmetros visuais adicionais e importação M4A offline. O rack DSP, MIDI, note-off e identidade do plugin são preservados; PITCH MODE acrescenta a opção explícita de não transpor novas vozes. Detalhes de codecs/licenças em [AUDIO-IMPORT.md](AUDIO-IMPORT.md).

## O que mudou

- Padrão 1180×840; Compacto 1000×700; mínimo 980×700 e máximo 1680×1100. Redimensionar abaixo de 780 pontos
  de altura também usa a organização compacta. `uiCompact` é novo e salvo no estado; IDs/ranges antigos preservados.
- Quatro macros em fila com knobs maiores; controles detalhados do módulo selecionado permanecem abaixo do rack.
  No compacto, a área do slider de macro tem pelo menos 80 pontos de altura. Sliders do sample continuam lineares.
- Nove conjuntos novos (After Hours, Solar Studio, Mint Condition, Sunrise, Sunset Drive, Poolside, Electric Soul, Burnt Velvet, Cosmic Arcade), além do estado legado, coordenam fundo, superfícies, preenchimentos de botões, bordas, combos, sliders e knobs;
  as identidades dos módulos conservam suas cores com uma mistura da paleta. Materiais de knobs e sombra do
  mascote conservam valores próprios para manter leitura. Temas claros usam texto escuro; rótulos de ComboBox são atualizados explicitamente, corrigindo a cor antiga em cache. Não é um seletor RGB irrestrito.
- Chassi modifica o arredondamento de painéis/botões, além da relação com os fundos legados.
- Rack: nome, símbolo específico do algoritmo, descrição auditada, estado, controle principal e Mix em cada
  módulo. Os demais controles aparecem na seleção; no compacto, o detalhe usa sliders para caber sem reduzir
  áreas clicáveis. Teste de arrasto compara posições reais dos cards à ordem real do DSP.
- Grooveiro: olhar menor, sorriso assimétrico, pernas mais longas, botas plataforma, lapelas/cromado,
  halo/fones/lenço, cabelo/metal/óculos/capas/cabo/ribbon, keytar e beat pads. Mesmos IDs/valores de peças.
- Personalizador com CORES, KNOBS e GROOVEIRO. Knobs têm quatro miniaturas e um slider JUCE interativo sem
  ligação a parâmetro sonoro. CORES mostra uma captura da interface real com a aparência escolhida;
  usa os mesmos componentes, sem criar outro editor/timer/DSP, somente ao abrir ou modificar escolhas.
  Não há animação contínua de interface dentro dessa miniatura. As escolhas estilizam os mesmos controles do instrumento.
- Desfazer visual (até 32 ações na abertura atual), retorno ao visual de abertura e defaults. Histórico não é
  salvo na sessão; as escolhas são. Fechar confirma as escolhas correntes; Escape fecha e devolve foco ao botão.
- X atua diretamente em balanço/direção; Y em alongamento/postura/expressão, condicionados à atividade real.
  Eventos MIDI alimentam uma fase de gesto e molas amortecidas com agachar/impulsionar/recuperar. Não se usa
  paridade simples nem animação independente de eventos. Notas rápidas agrupadas entre frames não são exibidas
  individualmente. O corpo inteiro é enquadrado no seu painel, incluindo peças. Ao reabrir uma prévia, notas que
  aconteceram enquanto ela estava escondida são descartadas visualmente, sem alterar o áudio.
- Completo/reduzido/desligado continuam 24/12/6 Hz. UI parada não redesenha tudo a cada tick. Layout só muda
  com redimensionamento, seleção/ordem/presença do rack. Personalizador aberto: mascote principal suspenso;
  só a aba GROOVEIRO avança a prévia. Controles cobertos ficam fora da interação/foco.

- `Hardware.*`: deck/cassete original com duas bobinas ao lado da waveform e ponteiro de pico de saída em
  dBFS (sem calibração VU/RMS). Rotação indicativa de vozes/preview, nunca de uma cauda isolada; detecta
  pausa do callback em ~300 ms e espera áudio novo ao voltar. 24/12 Hz; off congela bobinas, mantendo leitura
  de nível em 6 Hz. Sem timer adicional. Nenhuma atualização de deck atrás do personalizador.
- Materiais: Vintage/marfim canelado, Amplifier/metal escovado, Synth/borracha, Console/plástico; ponteiros
  claros em todos. Texturas são vetoriais discretas. Sem assets ou código proprietário de terceiros.
- Cosmic Arcade: paleta selecionável, roxo/magenta/ciano, grão espacial estático e deck luminoso; desfazer e
  sessão preservados. Range de `colourSet` continua 0–8; parâmetro visual adicional `cosmicArcade` 0–1.
- PITCH MODE: seletor/tooltip separado, parâmetro adicional 0–1, REPITCH padrão; FIXED PITCH conserva
  velocidade original para qualquer nota. Slice mantém mapa por notas. Mudança vale para novas vozes.
  Estado antigo migra para REPITCH; sem time-stretch ou multisample. No áudio, só muda o cálculo de step
  da nova voz e há telemetria limitada por bloco; sem novos locks/alocações no callback.

## Validação executada — Linux x86_64

- Release VST3 e Standalone: build concluído.
- CTest: **5/5** (`dsp`, `character`, `playback`, `mp3`, `processor`).
- pluginval 1.0.3: nível 5, seed 42, saída zero para o VST3 desta revisão.
- `git diff --check`: sem erros.
- M4A: seis fixtures AAC-LC/ALAC, com metadados verificados por ffprobe; WAV/AIFF/MP3 continuam passando.
  Nome/waveform passam a repintar quando a fonte muda, inclusive no silêncio; não dependem de uma nota MIDI.
- Testes de UI usam peer X11 e eventos JUCE simulados. Verificam hit-test, Return/Escape, foco e navegação por
  setas; abas visíveis; regiões clicáveis ≥24×24 pontos; limites e ausência de sobreposição entre controles
  interativos; formato padrão, compacto, mínimo e máximo; idioma inglês; 27 combinações de corpo/olhos/acessório;
  desfazer/reverter/defaults; preset antigo v2 e preferências atuais; resposta direta X/Y; molas limitadas e repouso.
- Texto principal >7:1 e secundário >4,5:1 nos fundos/superfícies/botões testados, por luminância sRGB. Não é
  certificação completa de acessibilidade, cobertura de todos os estados hover/disabled ou leitores de tela.
- Teste anterior de áudio bit a bit idêntico entre escolhas visuais continua passando; alocação no callback é
  vigiada pelas suítes existentes. Não foi criada alocação, desenho ou lock no callback de áudio.
- Capturas reais da árvore JUCE, não mockups: padrão, compacto, três abas, inglês, performance com notas/PLAY, oito paletas e render 2× (2360×1680).
  HiDPI é render simulado, não execução em um monitor Retina. Nenhum teste de escuta ou hardware físico aqui.

### Comandos usados neste workspace

Toolchain/dependências locais: `. /workspace/work/build-env.sh`.

```sh
cmake -S /workspace/rastro -B /workspace/work/build -DRASTRO_UI_BENCHMARK=ON
cmake --build /workspace/work/build -j2
DISPLAY=:99 XDG_CACHE_HOME=/workspace/work/runtime/cache \
  python /workspace/work/run-ui-benchmark.py /workspace/work/deps/usr/bin/ctest \
  --test-dir /workspace/work/build --output-on-failure
DISPLAY=:99 python /workspace/work/run-ui-benchmark.py
DISPLAY=:99 XDG_CONFIG_HOME=/workspace/work/runtime/config XDG_CACHE_HOME=/workspace/work/runtime/cache \
  /workspace/work/pluginval/pluginval --strictness-level 5 --random-seed 42 \
  --validate /workspace/work/build/Rastro_artefacts/Release/VST3/RASTRO.vst3
python scripts/export_previews.py --build /workspace/work/build --out /workspace/output/RASTRO-Etapa1
git diff --check
```

`run-ui-benchmark.py` é um auxiliar **do ambiente**: cria atoms X11 que normalmente o window manager
fornece e mantém a conexão aberta enquanto roda o comando filho. Foi necessário porque este Xvfb não tem
window manager (`BadAtom` ao abrir a primeira janela). Não faz parte do plugin ou de uma dependência no Mac.
Numa máquina Linux com desktop/X11 normal, use diretamente `ctest` e `rastro_ui_benchmark`.

## Medição de custo

`tests/UiBenchmark.cpp` abre uma janela JUCE real com loop de mensagens. Reproduz o mesmo fixture MP3,
renderiza áudio em blocos de 1920 frames a 48 kHz e envia notas de forma controlada. Cada fase dura ~2 s.
O tempo medido é **CPU do processo / tempo de parede**, expresso em percentual de um núcleo; inclui DSP,
GUI e o harness, não apenas desenho. Xvfb/renderização software; sem hardware de áudio. Uma passagem
antes/depois do refinamento anterior neste ambiente, portanto números indicativos com ruído, sem promessa de desempenho no Mac.
A fase 3 antiga usa o formulário anterior (mesma tela com avatar); a nova usa a aba GROOVEIRO.

| Fase | Antes do refinamento | Refinamento anterior | Acabamento atual |
| --- | ---: | ---: | ---: |
| Repouso, animação desligada | 23,1% | 9,4% | 7,4% |
| Instrumento em performance | 40,5% | 13,4% | 7,9% |
| Personalizador / CORES | 47,6% | 5,0% | 5,1% |
| Personalizador / GROOVEIRO | 50,8% | 15,4% | 15,3% |

Na passagem anterior, cada fase teve **0 layouts e 0 repaint completos solicitados pelo sincronizador**.
Isso não significa zero desenho: controles/mascote/indicadores visíveis repintam suas regiões quando necessário.
CORES: 49 ticks, 0 atualizações de ambos os mascotes. GROOVEIRO: 48 ticks, 0 do principal, 48 da prévia.
Os testes também confirmam que KNOBS não avança nenhum mascote e não recalcula layout.

Passagem atual isolada (sem pluginval em paralelo): `/workspace/work/finish-ui-metrics.log`.
Em cada fase: **0 layouts e 0 repaints completos do sincronizador**. CORES: 50 ticks, 0 passos de ambos
os mascotes; GROOVEIRO: 48 ticks, 0 do principal, 48 da prévia. A miniatura da interface é uma captura
estática refeita somente quando necessário. Cards com gráficos estáticos deixam de repintar a cada
bloco de atividade; controles e Grooveiro conservam suas próprias atualizações. Valores indicativos
numa única passagem Xvfb, sujeitos a ruído; não são medição no Mac nem garantia de CPU numa DAW.

## Avaliação visual e o que falta

`python scripts/export_previews.py --build <diretório-do-CTest> --out <pasta-de-saída>` produz
`ABRIR-PREVIAS.html` autocontido, PNGs originais e ZIP. Extraia e dê duplo clique no HTML; não exige rede.
No workspace desta tarefa: `/workspace/output/RASTRO-Etapa1/ABRIR-PREVIAS.html`.
O script exige os PNGs reais gerados pelos testes, valida assinatura/dimensões e inclui checksums.

A integridade e o conteúdo das imagens foram conferidos aqui. O usuário confirmou que a imagem não aparece
no Codex no navegador. Os PNGs versionados podem ser abertos pelo [índice no GitHub](previews/README.md)
na branch do PR, com acesso ao repositório privado. Caminhos locais não são URLs públicas.
Não afirmar que as capturas estão visíveis no chat sem confirmação.

Mac ainda precisa: validar AAC-LC/ALAC pelo CoreAudio com os seis fixtures e gravações reais, diálogo nativo/arrasto do Finder; conferir o workflow do commit atual para compilação AU/VST3/Standalone, auval, instalação/Gatekeeper, Logic Pro,
recall/automação, MIDI/áudio físicos, monitor Retina e medição de CPU em uso real. O artefato antigo do PR
não contém estas alterações. Multisample, fontes A/B e efeitos adicionais não foram iniciados.

## Arquivos desta rodada

`Source/Hardware.*`, `Engine.h`, `EngineTypes.h`, `Source/RecordingImport.*`, `Processor.*`, `Editor.*`, `Customizer.*`, `Visuals.h`, `Performance.*`,
`SampleView.cpp`, `Strings.h`, `CMakeLists.txt`; `tests/ProcessorTests.cpp`, `PlaybackTests.cpp`, `UiBenchmark.cpp`,
seis fixtures M4A e sua documentação; `scripts/generate_recordings.py`, `export_previews.py`;
README, guias de importação/validação/roadmap e PNGs em `docs/previews/`.
A lista inclui trabalho local anterior preservado; não representa somente o diff desta última mensagem.
A publicação para teste usa o PR #1 existente; o resultado macOS deve ser conferido no workflow associado ao commit publicado.

## Validação do acréscimo hardware / Cosmic Arcade / pitch

Comandos iguais aos anteriores, logs específicos: `hardware-build.log`, `hardware-tests.log`,
`hardware-pluginval.log`, `hardware-ui-metrics.log` em `/workspace/work/`.

- Release VST3/Standalone, CTest **5/5**, pluginval nível 5/seed 42 e `git diff --check`.
- REPITCH: frequência medida por cruzamentos de zero; oitava dobra frequência e velocidade de leitura.
  FIXED PITCH: PCM exato entre notas 12/48/60/72/108/127 e raiz nos quatro modos não Slice; quatro sample rates.
  Slice: mesmo áudio/mapa nos dois pitch modes, inclusive rejeição fora do mapa. Note-off testado em ambos.
- Sessão e migração de pitch/Cosmic, MUTATE preserva pitch; seletor independente e tooltip na UI.
- Bobinas: MIDI, prévia, One Shot/gated, fim de voz, repouso, pausa do host, retorno, reduzido/off; estado
  real do áudio. UI: controle de pitch e deck sem sobrepor waveform, quatro materiais reais capturados,
  Cosmic Arcade selecionável/desfazível, contraste e atualização da prévia aplicada.
- Capturas adicionadas: Cosmic Arcade, personalizador Cosmic e quatro famílias de hardware. Total 22 PNGs
  no pacote offline; HiDPI continua sendo render 2× simulado, sem teste físico Retina.

Medição indicativa isolada, Xvfb, mesmo harness com DSP incluído: repouso 8,4%; performance 10,4%; CORES 5,5%;
GROOVEIRO 15,9% de um núcleo. Uma passagem curta, não benchmark no Mac. Todas as fases: 0 layouts e 0 repaints
completos do sincronizador. CORES/GROOVEIRO: **0 atualizações do deck coberto**; mascote principal também 0.
Os resultados acima são do Linux. A compilação Mac/AU é separada e registrada no workflow do commit atual; não houve teste no Logic.
