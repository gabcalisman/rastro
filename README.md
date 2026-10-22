# RASTRO by Gab Calisman

A native MIDI sample instrument and reorderable texture rack. First working prototype, C++17 / JUCE 8 / CMake. Portuguese (Brazil) is the default; English is selectable and saved with the session.

## RASTRO PLAY: sample, cortes e execução MIDI

Esta revisão continua sendo **somente o instrumento sampler MIDI RASTRO PLAY**. O nome/binário e os identificadores permanecem RASTRO para preservar compatibilidade. Não foi criado um RASTRO FX nem uma entrada para áudio de outras faixas. A Etapa 1, incluindo Cosmic Arcade, hardware, PITCH MODE e M4A, integra a branch `prepare-mac-testing` do [PR #1 em rascunho](https://github.com/gabcalisman/rastro/pull/1), sem merge em `main`. Para baixar, use o workflow do **commit atual** e confira `BUILD-INFO.txt`; o artefato antigo do build #3 (`4f27e91`) não contém estas mudanças.

### Carregar, recortar e ouvir

1. Use **CARREGAR SOM** para abrir WAV, AIFF/AIF, MP3 ou M4A renderizado (AAC/ALAC), ou arraste **um arquivo** desses formatos para qualquer área da interface. Os dois caminhos importam em segundo plano. No Mac, M4A usa CoreAudio; a validação desta rodada foi no Linux, onde M4A requer FFmpeg local. MP3 funciona offline: o decoder faz parte do executável; não requer FFmpeg, internet ou codec instalado no Mac.
2. Arraste os **triângulos/linhas laranja** na waveform para mudar início e fim. Os sliders continuam disponíveis para ajuste/automação. Os limites não se cruzam ao arrastar. Novas notas usam a edição atual; vozes em andamento preservam os limites capturados para não saltar no meio do som.
3. **Ouvir trecho** toca uma passagem para a frente do intervalo inteiro, no pitch original e através do rack, independentemente de Reverse/Slice. **Parar prévia** libera apenas essa voz; o MIDI permanece independente. Comece com volume de monitoração baixo.
4. O estado do host preserva bytes originais do arquivo, trims, modo, cortes, parâmetros, rack, idioma e PLAY. O arquivo externo não é necessário para restaurar; MP3 é decodificado novamente pelo mesmo decoder local.

### PITCH MODE — separado do modo de execução

O seletor **PITCH MODE** fica na faixa de parâmetros do sample, separado de One Shot/Loop/Reverse/Ping-Pong/Slice:

- **REPITCH** (padrão): uma nota MIDI acima/abaixo da raiz muda altura **e velocidade**. Uma oitava acima
  dobra a frequência e a velocidade de leitura; uma abaixo reduz ambas à metade. É o comportamento anterior.
- **FIXED PITCH**: toda nota dispara a mesma altura e velocidade originais do arquivo, com conversão normal
  para o sample rate do host. Velocity, polifonia, trims, efeitos e note-off continuam funcionando.
- **Slice** continua usando notas MIDI para escolher cortes, sempre na velocidade original, nos dois pitch modes.

A escolha é automatizável e salva na sessão. Aplica-se às **novas vozes**; uma nota em andamento mantém
sua velocidade capturada para não saltar ao mudar o seletor. Sessões antigas recebem REPITCH. MUTATE e
reset visual não alteram essa escolha. A prévia continua no pitch original. O tooltip explica os dois modos.
**Não há time-stretch com preservação de altura**: essa possibilidade está no roadmap e exige um projeto
DSP próprio, orçamento realtime e migração antes de ser implementada. Multisample também permanece futuro.

### Modos e cortes por MIDI

Escolha no menu ao lado da waveform:

| Modo | Comportamento |
| --- | --- |
| Disparo único / One Shot | Início → fim uma vez, com afinação cromática relativa à raiz. |
| Loop | Repete para a frente enquanto a voz estiver ativa, com afinação cromática. |
| Reverso / Reverse | Fim → início uma vez, com afinação cromática. |
| Ping-Pong | Alterna para a frente/trás dentro do intervalo, com afinação cromática. |
| Cortes MIDI / MIDI Slices | Uma parte por nota, no pitch/velocidade original do arquivo. |

**One Shot ignora o note-off comum:** soltar a tecla não interrompe o sample, que toca até o marcador de fim. Esta revisão corrige o comportamento anterior de liberação ao soltar.

| Modo | Ao soltar a tecla (note-off) |
| --- | --- |
| One Shot | Continua até o fim do trim, mesmo após soltar. |
| Loop | Inicia liberação de até ~30 ms; não faz outra volta se alcançar o fim durante a liberação. |
| Reverse | Inicia liberação de até ~30 ms; pode terminar antes se alcançar o início do trim. |
| Ping-Pong | Continua o movimento durante a liberação de até ~30 ms e então para. |
| Slice | Libera o corte em até ~30 ms, ou termina antes na borda final do corte. |

A liberação depende do nível atual do envelope: soltar durante o ataque pode parar antes de 30 ms. Caudas do rack (delay/reverb) podem continuar depois que a voz acaba. A flag legada `loop=1` continua sendo Loop e obedece note-off. O modo capturado no note-on vale para a voz, com a exceção já existente da automação legada de `loop`. **All Notes Off/All Sound Off**, proteção de overflow MIDI e **Parar prévia** continuam liberando as vozes (com fade, não corte abrupto), inclusive One Shot. Retrigger cria outra voz até o limite de 16; não há choke monofônico. Velocity controla volume e filtro ajustável; há até 16 vozes. Mudar a nota transpõe nos quatro primeiros modos e escolhe outro corte no Slice.

Em **Cortes MIDI**, o menu numérico define **2–16 partes**. Alterar a quantidade pela UI redistribui os cortes igualmente dentro do trim. Depois arraste as **linhas verdes** para editar as divisões manualmente. Os números sob as partes são notas MIDI: com raiz 60 e oito cortes, **60–67** tocam as partes 1–8; notas fora do intervalo são ignoradas. A raiz é editável. Não há detecção automática de transientes. Automação de `sliceCount` usa os marcadores existentes; mudar pelo menu também redistribui os marcadores. Cada marcador é automatizável e persistente.

O parâmetro antigo `loop` continua existindo: sessões antigas abrem no modo compatível e mantêm sua automação. O novo menu escreve uma escolha explícita e sincroniza a flag para escolhas novas. O checkbox foi substituído pelo menu, sem remover seu ID do host.

### MP3: decoder, licença e limites

O adaptador original `Source/Mp3Decoder.cpp` usa **minimp3**, revisão `ea99364f61c14656440e8d77e9c233ccf3124633`, sob **CC0 1.0 Universal**. A licença foi conferida; o cabeçalho não foi alterado e a dedicação/termos completos foram preservados em `third_party/minimp3/NOTICE.md` e `LICENSE`. CC0 permite incorporar e redistribuir esse código, inclusive comercialmente. Não foi habilitado o decoder MP3 opcional do JUCE; arquivos `.mp3` usam explicitamente o decoder vendorizado em todas as plataformas. As condições de licença do próprio JUCE continuam aplicáveis.

Limites: até 32 MB codificados e 120 s; MP3 mono/estéreo de 8–48 kHz, convertido para PCM estéreo. Não há compensação automática de delay/padding de encoder ou edição gapless dos metadados Xing/LAME; recorte eventual silêncio nas extremidades. O estado armazena o MP3 original e a restauração é determinística com esse decoder. Os fixtures são senóides sintéticas próprias com dedicação CC0 documentada. FFmpeg/libmp3lame foi usado somente para gerá-los, não como dependência de build/runtime.

### Gravações do Gravador: M4A AAC / ALAC

Suporte offline implementado para áudio renderizado. No **Linux x86_64**, os fixtures passaram em:
**AAC-LC mono 44,1/48 kHz e estéreo 48 kHz; ALAC mono 16 bit/44,1 kHz e 24 bit/48 kHz;
ALAC estéreo 16 bit/44,1 kHz**. WAV/AIFF PCM mono/estéreo e MP3 continuam passando.
Esses exemplos são senóides originais, não gravações exportadas do aplicativo Apple.

No **Mac**, o adaptador usa os codecs do CoreAudio do sistema por meio do JUCE, sem instalar FFmpeg;
**a decodificação por esse caminho ainda precisa de teste no Mac**. O CI compila o adaptador, mas não executa a suíte opcional de importação/editor. HE-AAC, xHE-AAC, DRM e áudio espacial
não foram validados. Não se promete suporte a toda variante do contêiner M4A.

**QuickTime “Editável” (.mov/.qt) fica fora desta etapa:** seletor/drop mostram orientação para exportar
áudio renderizado `.m4a` (Comprimido/AAC ou Sem perdas/ALAC); renomear a extensão não converte o áudio.
Erros de leitura, corrupção, formato, codec, limites e decoder ausente têm mensagens distintas PT/EN.
Veja [variantes, uso, limites, dependências, licenças e testes](docs/AUDIO-IMPORT.md).

### Preparação para um FX futuro

`EngineTypes.h` define fonte/configuração; `Engine.h` cuida das vozes/modos MIDI; `EffectRack.h` processa um par de amostras sem depender de notas, GUI ou carregamento; `CharacterEffects.h` implementa os três DSPs novos. Um futuro RASTRO FX poderá reaproveitar o rack com seu próprio adaptador de host e barramentos. Hoje não existe target, categoria ou modo de áudio externo: o processador tem somente saída e `IS_SYNTH TRUE`.

## Etapa 1 — cor, knobs, rack e Grooveiro

Nove conjuntos distintos coordenam textos, fundos, superfícies, botões, bordas e controles:
**After Hours**, **Solar Studio**, **Mint Condition**, **Sunrise**, **Sunset Drive**, **Poolside**,
**Electric Soul**, **Burnt Velvet** e **Cosmic Arcade**. Há noite/neon, creme solar, verde claro, coral/laranja, roxo/rosa,
teal/limão e ferrugem/dourado. As quatro escolhas legadas permanecem disponíveis nas sessões antigas.
Cosmic Arcade combina roxo/magenta saturados, ciano, textura espacial estática discreta e cassete/deck luminoso original, sem copiar layouts de referência.
O texto troca entre claro e escuro; menus JUCE também atualizam a cor, sem rótulos brancos presos em tema claro.

O rack tem seis módulos de 124 pontos, trilhos coloridos, símbolos próprios e descrições auditadas no DSP:
**GHOST — eco filtrado; CRUMBLE — fragmentos; BLOOM — cauda difusa; WOBBLE — atraso modulado;
DUST — textura dinâmica; CRUSH — bits + taxa**. Não há anúncio de chorus, flanger, drive ou tape extra.
Cada módulo mostra estado, controle principal direto e Mix; seleção abre os demais controles abaixo.
Arraste o título para reordenar: posição visual e ordem real do DSP permanecem sincronizadas.
Os símbolos identificam o algoritmo; a reação animada ao áudio fica no Grooveiro, sem relógios decorativos.

### Materiais e transporte do sampler

As quatro famílias aplicam-se aos controles reais e às miniaturas: **Vintage / marfim canelado**,
**Amplifier / metal escovado**, **Synth / borracha** e **Console / plástico de console**.
Ponteiros de alto contraste, chanfros, ranhuras e acabamento discreto de painel mantêm nomes e valores livres.

O ícone pequeno foi substituído por um deck de duas bobinas ao lado da waveform. A rotação é um indicador
suave de vozes realmente em reprodução (MIDI e prévia), não um relógio independente nem simulação física
exata de fita. As bobinas continuam após note-off em One Shot até o sample acabar; nos modos gated param
após a liberação. Não seguem apenas o medidor, portanto não giram só por haver cauda de efeito. Se o host
parar de fornecer blocos, a indicação para em até ~300 ms. Ao reabrir o painel, espera um bloco novo.

O ponteiro analógico mostra **pico de saída em dBFS**, com suavização visual: **não é um VU RMS calibrado**.
A faixa não acrescenta efeito tape nem altera timbre. Completo/reduzido usam 24/12 Hz; reduzido também
reduz a velocidade visual das bobinas. Desligado congela as bobinas e mantém leitura operacional do nível
em 6 Hz. Deck e mascote principal não avançam atrás do personalizador.

### Onde mudar os knobs e cores

1. Clique em **CORES / KNOBS / GROOVEIRO** no cabeçalho. O painel tem três abas explícitas.
2. **CORES:** compare as nove miniaturas e a captura da interface real com a escolha aplicada. Fundo, superfícies, botões, bordas e
   textos e controles mudam juntos. Fundo/destaque/controles podem seguir a paleta ou ser escolhidos separadamente. Chassi muda o arredondamento dos painéis/botões; nas paletas legadas, também influencia o fundo.
3. **KNOBS:** clique em Vintage, Amplifier, Synth ou Console. A escolha afeta os knobs reais — agora
   maiores, com os quatro macros numa faixa horizontal. Experimente o knob de prévia: ele é um slider
   JUCE interativo, mas **não altera o áudio**. Os sliders de sample continuam lineares.
4. **GROOVEIRO:** escolha corpo pelas miniaturas e combine as outras peças com a prévia ao lado.
5. **Desfazer** volta uma ação visual (até 32 durante a abertura atual). **Voltar ao abrir** restaura
   a aparência que existia quando abriu o painel e também pode ser desfeito. **Padrão** restaura apenas
   o visual. Sample, som, PLAY, MIDI, rack, animação e tamanho compacto são preservados por essas ações.
6. **VOLTAR** ou Escape fecha o painel, mantendo as escolhas. O foco volta ao botão de abertura.
   Return abre pelo teclado; Tab percorre controles e as setas mudam a aba quando o foco está nela.
   O histórico visual não é salvo; as escolhas visuais são salvas na sessão.

**Tamanho:** padrão 1180×840, botão **Compacto** para 1000×700, mínimo 980×700. A preferência compacta
é salva na sessão. A organização compacta também entra ao redimensionar para menos de 780 pontos de altura.
Todas as funções continuam disponíveis, com seis módulos em uma faixa e detalhes do selecionado abaixo.

### Monte seu Grooveiro

O Grooveiro tem silhueta inteira mais alongada, olhar discreto, sorriso assimétrico, roupa com lapelas,
botas plataforma e materiais cromados. Combine formas Orbit/Soul/Metro, olhos Soul/Mono/Digital,
halo/fones de estúdio/lenço de seda, cabelo pompadour/sleek, aletas metálicas, jaqueta/colete,
óculos aviador/visor, cabo/ribbon, capa plissada/placas e keytar/beat pads. Peças antigas foram
redesenhadas com os mesmos IDs/valores; não se promete equivalência visual com o desenho anterior.
Cor do Grooveiro pode seguir a paleta ou ser independente. Tudo é vetorial original, sem assets externos.
São peças predefinidas: não há desenho livre, editor de posições, imagens do usuário ou desbloqueios.
Os instrumentos desenhados são **acessórios visuais** e não acrescentam motores sonoros.

Notas MIDI e velocity disparam gestos com antecipação, impacto e recuperação amortecidos; o nível de áudio modifica expressão/pose;
WOBBLE inclina o corpo, DUST revela partículas, CRUSH introduz fragmentos visuais e BLOOM deixa um rastro.
PLAY orienta diretamente a pose **quando há atividade real**: X muda balanço/direção, Y muda alongamento/postura/expressão. Rajadas de notas alimentam a energia e uma fase de gesto derivada dos eventos, sem simples alternância por paridade. A fase contínua do WOBBLE é bloqueada visualmente
no repouso: não faz o Grooveiro dançar sozinho no silêncio. Após o decaimento suave, ele para.
Não há análise automática de beat/transientes, coreografia gravada nem oscilador visual independente.

O seletor de animação continua no cabeçalho: completo **24 Hz**, reduzido **12 Hz** com aproximadamente
um quarto do deslocamento, desligado **6 Hz** para controles/estado e mascote estático. Somente a aba GROOVEIRO avança a prévia; o mascote principal fica suspenso atrás do painel. CORES e KNOBS não animam mascotes. A UI não recalcula layout nem pede repaint completo em cada tick. Desenho, caminhos e interpolação
visual acontecem na thread da interface; a thread de áudio publica somente atividade das vozes e contador de blocos por operações atômicas limitadas, sem novos locks ou alocações. O desenho fica na UI.

### Verificação da Etapa 1, PITCH MODE e M4A

Linux x86_64: build Release **VST3/Standalone**, **CTest 5/5**, **pluginval nível 5/seed 42** e
`git diff --check` aprovados. A suíte testa REPITCH/FIXED PITCH em quatro sample rates, Slice nos dois modos, migração/estado, telemetria das bobinas e pausa do host. A suíte visual também testa hit-tests em janela X11, foco/teclado,
undo/reverter, dimensões de clique, sobreposições, compacto, combinações, restauração, resposta X/Y, controle principal dos seis módulos e drag da ordem real. A importação atualiza nome/waveform no silêncio, sem esperar uma nota MIDI.
Foram produzidas capturas reais e render 2× simulado. O áudio permanece bit a bit idêntico entre
personalizações nos testes. Esses resultados são do Linux; o resultado macOS deve ser conferido no workflow do commit atual. **Não houve teste no Logic.**

**Medição anterior do refinamento de UI**, em janela JUCE/Xvfb: CPU de processo passou de aproximadamente 40,5% para 13,4% na
performance e 47,6% para 5,0% na aba de cores. Inclui DSP e harness, não prevê o desempenho no Mac.
Contadores verificaram zero layouts em cada intervalo e zero atualizações do mascote coberto.
Veja [validação e comandos completos](docs/UI-VALIDATION.md), incluindo método, limites e checklist do Mac.

**Capturas:** após os testes, execute:

```sh
python scripts/export_previews.py --build /workspace/work/build --out /workspace/output/RASTRO-Etapa1
```

Extraia o ZIP resultante e abra **ABRIR-PREVIAS.html** por duplo clique. É autocontido e offline,
com PNGs originais, nove paletas, quatro famílias de materiais e performance MIDI. As capturas também estão no [índice do repositório](docs/previews/README.md), incluindo [Cosmic Arcade](docs/previews/cosmic.png) e [materiais dos knobs](docs/previews/knobs.png). Abra esses links no GitHub com acesso ao repositório privado; o botão de download de cada PNG permite salvá-lo. São capturas reais da interface JUCE no Linux, não mockups. A exibição das imagens no chat não foi confirmada; caminhos `/workspace/...` são locais, não links de download.

### Compatibilidade e próximas etapas

Todos os parâmetros antigos preservam ID e range, inclusive `controlStyle` 0–2. O `colourSet` mantém seu range 0–8; Cosmic Arcade usa um campo visual adicional, sem renormalizar automação anterior. O novo `knobDesign`
usa 0 para seguir esse estilo legado e 1–4 para as quatro escolhas novas. Estados antigos recebem
defaults explícitos para peças/campos adicionais, preservando as escolhas antigas e o áudio.
As novas escolhas são salvas pelo APVTS e ficam fora de `Settings`/DSP. O desenho de uma peça antiga
foi aprimorado; equivalência visual pixel a pixel com a versão anterior não é prometida.

**Somente Etapa 1 implementada.** Multisamples, fontes A/B/Layer, novos efeitos, IR, analógico virtual e FM
estão no [plano das próximas etapas](docs/ROADMAP.md), com os pontos de integração propostos. Nenhum
motor/fonte/efeito novo foi criado nesta rodada. MIDI, note-off, importação offline, efeitos e ordem DSP
mantêm o comportamento sonoro da base publicada; a importação acrescenta M4A renderizado.

## PLAY e caráter

Os recursos sonoros abaixo já pertencem à base `4f27e91` e foram preservados no redesenho da Etapa 1. O build macOS #3 valida apenas aquela base; consulte o workflow do commit atual para esta revisão. Não houve teste no Logic.

- **PLAY:** arraste o pad. X multiplica a profundidade individual do WOBBLE; Y multiplica o nível do DUST e a intensidade do CRUSH. A fórmula é `valor individual × 2 × eixo`, limitada a 0–1: o centro (50%, 50%) preserva os valores do rack; a borda esquerda elimina a profundidade do WOBBLE e a borda inferior elimina a transformação DUST/CRUSH. Mix e bypass continuam independentes. Duplo clique, **Centralizar** ou Home volta ao centro. As setas permitem ajustes de 2%. Os dois eixos são parâmetros automatizáveis e têm MIDI Learn separado; os três mapeamentos de macros anteriores continuam disponíveis.
- **WOBBLE:** atraso interpolado com oscilação lenta de 0,15–3 Hz e flutter mais rápido. Controles: intensidade, velocidade e mix. É DSP próprio de fita instável, não uma emulação calibrada de equipamento nem um algoritmo de terceiro. Mix parcial pode produzir coloração de fase; o atraso varia em torno de 8 ms e não é compensado como latência fixa do host.
- **DUST:** mistura de chiado, ruído filtrado e estalos sintéticos limitados. Nível, caráter e mix. O envelope segue o sinal original das vozes, antes do rack, com ataque de 2 ms e liberação de 30 ms. Isso evita ruído contínuo quando não há execução; o resíduo do envelope é zerado abaixo do limiar. Silêncio dentro do próprio sample também fecha a textura. Caráter vai de chiado aberto a grão filtrado/estalos. Não usa samples externos.
- **CRUSH:** um único controle combina quantização e redução de taxa por retenção de amostras. Mix independente. O retorno tem suavização curta (80 µs), os parâmetros têm dezipper de 15 ms e o estágio de saída continua limitado por saturação suave. Não há compensação automática de loudness nem anti-aliasing de alta qualidade: aliasing/grão faz parte da transformação, e material muito baixo pode desaparecer em quantização extrema. Não há ganho de saída adicional no módulo.
- **GHOST, CRUMBLE e BLOOM** mantêm seus algoritmos anteriores. Nenhum foi substituído ou simplificado nesta etapa. Cada um dos seis módulos pode ser desligado, removido, reinserido e reordenado. Arraste um título até outro cartão para inserir o módulo naquela posição. O rack faz fade de 5 ms antes/depois da mudança de ordem.
- **Rack compacto:** mix e bypass ficam visíveis nos seis cartões. Clique no título/corpo de um cartão para mostrar apenas seus controles detalhados na faixa inferior. Isso evita exibir todos os knobs simultaneamente.
- **MUTAR / MUTATE:** varia apenas módulos presentes e ativos, inclusive mix, dentro de limites moderados; usa o macro MUTAÇÃO como intensidade. Não muda sample, raiz, trims, nível de saída, PLAY ou ordem. **Voltar variação / Undo mutation** restaura exatamente os parâmetros afetados pela última mutação; há um nível de histórico, salvo na sessão. Uma nova mutação substitui esse histórico. Restaurar efeitos limpa o histórico e centraliza PLAY. Com MUTAÇÃO em zero, MUTATE não altera parâmetros.
- **Mascote vetorial original:** notas MIDI geram um pulso; o nível medido muda o corpo; a fase real do WOBBLE inclina o avatar; a energia realmente adicionada pelo DUST controla partículas; a diferença de sinal do CRUSH altera contorno/expressão; o retorno medido do BLOOM produz o rastro. PLAY altera direção/posição. Mix, bypass e mix global também entram nas medições. Não existe um relógio visual de oscilação independente do DSP.
- **Animação completa/reduzida/desligada:** máximo de 24/12/6 atualizações da interface por segundo. O modo reduzido diminui o movimento a um quarto; desligado deixa o avatar estático e remove o cursor móvel da waveform. MIDI/valores dos controles continuam sendo indicadores funcionais. A preferência é salva e não muda o som. A UI continua atualizando lentamente para refletir CC/host, mesmo com o movimento desligado.

### Compatibilidade, arquitetura e limites

Os IDs antigos de parâmetros e o parâmetro de ordem dos três efeitos foram preservados. A nova ordem de seis módulos usa um parâmetro adicional, com 720 permutações; automação do parâmetro antigo continua compondo a ordem dos três módulos originais. Sessões sem `stateVersion=2` ganham defaults explícitos, PLAY centralizado e **WOBBLE/DUST/CRUSH desligados**, preservando a intenção sonora da versão antiga. Sessões novas salvam os seis módulos, os dois eixos/CC, animação e o snapshot de desfazer. Não se garante abrir sessões v2 em versões antigas do plugin.

`CharacterEffects.h` contém o novo DSP independente do JUCE; `Rack.h` define IDs e ordem; `Performance.*` contém PLAY e avatar. O callback usa buffers preparados e escalares/arrays fixos: sem alocação, locks, arquivos ou desenho. A telemetria é publicada por atomics a cada 64 amostras e no fim do bloco para o medidor; a UI faz a suavização visual e os desenhos. Importação via seletor usa uma fila de trabalho do processador e produz uma waveform de 512 picos no carregamento, fora da thread gráfica. A restauração de sessão continua decodificando no thread chamador não realtime do host, como antes; estados grandes ainda podem custar tempo e memória ao salvar/restaurar.

Referências de categoria e interação: [página oficial](https://www.xlnaudio.com/products/addictive_fx/effect/rc-20_retro_color) e [visão geral dos módulos](https://support.xlnaudio.com/hc/en-us/articles/16382602368541-Interface-overview-FX-Module-section) do RC-20. A análise foi limitada à ideia de controles imediatos, efeitos independentes e textura variável. Interface, mascote, algoritmos de efeitos e sons desta implementação são próprios; não foram incorporados código proprietário, presets ou assets. O código novo de terceiro é somente o decoder minimp3 sob CC0, documentado acima. JUCE continua com a licença já exigida pelo projeto.

### Validar esta versão

Verificação da base sonora anterior (Linux x86_64): build Release de **VST3 e Standalone** concluído; CTest **5/5**; testes portáveis com AddressSanitizer/UndefinedBehaviorSanitizer **4/4**; **pluginval nível 5, seed 42**, concluído com código de saída zero; `git diff --check` sem erros. LeakSanitizer ficou desligado (`ASAN_OPTIONS=detect_leaks=0`) por limitação de ptrace do ambiente. Os testes de UI geraram snapshots em quatro combinações visuais; a suíte também confirma estado visual persistente e áudio bit a bit idêntico com personalizações diferentes. Não houve teste de escuta nem execução macOS/Logic nesta revisão.

Comandos usados no diretório de build local já configurado:

```sh
cmake --build /workspace/work/build -j2
DISPLAY=:99 ctest --test-dir /workspace/work/build --output-on-failure
cmake --build /workspace/work/play-sanitized -j2
ASAN_OPTIONS=detect_leaks=0 ctest --test-dir /workspace/work/play-sanitized --output-on-failure
DISPLAY=:99 pluginval --strictness-level 5 --random-seed 42 --validate /workspace/work/build/Rastro_artefacts/Release/VST3/RASTRO.vst3
git diff --check
```

Esses caminhos são do ambiente Linux desta tarefa, não comandos para seu Mac; use a seção macOS abaixo no seu computador. O build sanitizado foi configurado com `RASTRO_CORE_ONLY=ON`, Debug e `-fsanitize=address,undefined -fno-omit-frame-pointer`.

A compilação normal inclui quatro testes portáveis (`dsp`, `character`, `playback`, `mp3`); com `RASTRO_INTEGRATION_TESTS=ON` são cinco, incluindo `processor`. O workflow macOS compila e executa os quatro portáveis; a suíte opcional de importação/editor deve ser executada separadamente no Mac. O resultado de cada execução está nos Checks do PR.

- `character`: 720 ordens; diferença audível de WOBBLE/DUST/CRUSH; comparação seca com mix zero; DUST sem sample e após note-off; limites/valores finitos; PLAY/telemetria; quatro sample rates; proteção contra alocação no caminho DSP.
- `playback`: cinco modos, cortes manuais por MIDI, trims e quatro sample rates. Compara One Shot com/sem note-off antecipado (áudio idêntico até o fim do trim), liberação dos quatro modos restantes, note-off de outra nota, Loop legado, all-off e parada de prévia. A suíte de processor também testa note-off MIDI com offset e teclado da interface nos cinco modos.
- `mp3`: decoder offline mono/estéreo, sample rate, PCM, canais e entradas inválidas/truncadas.
- `processor`: mantém WAV/AIFF, mono/estéreo, offset MIDI, tempo de host, restauração e renderização anteriores; acrescenta migração v1 para v2 numa instância já editada, estado dos seis módulos/XY/CC/animação, mutação apenas dos efeitos ativos, proteção de sample/raiz/trims/nível, desfazer exato após salvar/restaurar, PLAY com mouse/teclado/CC e avatar reativo/estático enquanto o áudio continua, velocity visual, personalização persistente e independência do áudio. Também verifica os seis fixtures AAC-LC/ALAC, importação assíncrona do seletor/drop, erros e restauração M4A; restauração idêntica de MP3, modo/cortes/trims persistentes, arrasto de marcadores, prévia e file drop assíncrono.
- No Mac: baixar o build aprovado do commit atual ou recompilar AU/VST3/Standalone, executar a suíte de integração/importação, `auval`, verificar performance/interação do avatar, automação XY, MIDI real, instalação/assinatura, recall no Logic, audição comparativa dos efeitos e bounce. Um build aprovado não substitui esses testes.

## Play

1. Load a WAV, AIFF, MP3 or rendered M4A (AAC/ALAC) using **CARREGAR SOM / LOAD SOUND**, or drop one audio file onto the interface.
2. Play MIDI notes or the onscreen keyboard. MIDI note 60 plays at the original pitch with the default root of 60. Root and trims apply to newly triggered voices.
3. Select One Shot, Loop, Reverse, Ping-Pong or MIDI Slices in the playback menu. One Shot ignores ordinary note-off and plays to the trim end; Loop, Reverse, Ping-Pong and MIDI Slices release within about 30 ms (or earlier at a non-looping boundary). Effect tails can continue. Velocity controls amplitude and the adjustable low-pass response.
4. Adjust the six effects. Drag a card's heading onto another card to insert it at that position. Select a card to edit its detailed controls. **Active / Bypassed** compares a module; **×** removes it; **+ Module** restores it. One instance of each type is available.
5. **Dry / rack** preserves a direct comparison with the sampler signal. At 0%, the rack is dry; output gain and the protective soft saturation still apply.
6. Learn a MIDI CC for PRESENCE, TRAIL, MUTATION or either PLAY axis by clicking **Learn CC**, then sending a controller message. Click again to cancel. Mappings persist; MIDI is received on all channels.
7. Save the host session/preset to store the original sample bytes, parameters, rack order/presence/bypass, language and CC mappings. The original file is no longer required.

### Effects and macros

- **GHOST:** interpolated, filtered feedback delay. Division selects 1/32, 1/16, 1/8, 1/4 or 1/2 note. Feedback, tone and mix work independently.
- **CRUMBLE:** a simple windowed fragment repeater, not a full granular synthesizer. Rate spans 1/8–1/128 note; Fragment changes captured length; mix blends the repeated sound. MUTATION offsets fragment selection.
- **BLOOM:** a simple four-line diffuse stereo tail, not a convolution or premium algorithmic reverb. Size changes delay lengths, tone filters the return, decay controls bounded feedback, and mix blends the tail.
- **PRESENCE:** gentle output gain scaling. **TRAIL:** scales GHOST feedback and BLOOM decay. **MUTATION:** controls fragment variation and MUTATE intensity.
- **MUTATE:** bounded changes to active/present modules, including their mixes; sample, trim, root, PLAY, order and output level are preserved. Undo mutation restores the last affected settings and its snapshot survives session restore. **Reset effects** restores effect and macro defaults; sample and rack order remain intact.
- Host BPM is used when available; otherwise Manual tempo applies. Tempo sync is interval-based; it does not align fragments to the DAW bar/transport phase.

## Build

CMake 3.22+, a C++17 compiler, Git, and JUCE 8 are required. CMake uses an installed JUCE 8 package or fetches the pinned JUCE **8.0.6** tag. Initial fetching requires network access. JUCE and bundled SDK licensing apply; choose the appropriate JUCE license before distributing a product.

### Linux

On a normal Debian/Ubuntu development machine:

```sh
sudo apt-get update
sudo apt-get install cmake ninja-build g++ git libasound2-dev libx11-dev libxext-dev \
  libxinerama-dev libxrandr-dev libxcursor-dev libfreetype-dev libfontconfig-dev \
  libgl1-mesa-dev libpng-dev libjpeg-dev
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build -j2
ctest --test-dir build --output-on-failure
```

Built formats: **VST3 instrument** and **Standalone**. Outputs are in `build/Rastro_artefacts/Release/`. Copy the complete `VST3/RASTRO.vst3` bundle into `~/.vst3/` and rescan your Linux DAW. Standalone requires a working ALSA audio device; enable the desired MIDI input through its audio/MIDI settings.

### Download a Mac build from GitHub Actions (no compiler needed)

The **macOS build** workflow in `.github/workflows/macos-build.yml` runs for PRs into `main`, including draft PRs. It builds AU, VST3 and Standalone using Xcode on `macos-15-intel`, explicitly targeting **both Apple silicon (`arm64`) and Intel (`x86_64`)**. Each bundle's two architectures are checked with `lipo`. The deployment target is macOS 12.0; your version of Logic Pro may require a newer macOS. The runner executes the portable DSP tests natively on Intel; packaging arm64 code does not prove arm64 execution works.

**A workflow definition is not proof of a successful build.** Check the actual run for the commit you want. These are local-test bundles: ad-hoc signed, not Developer ID signed or notarized. Automated checks do not include `auval` or Logic Pro playback.

1. Sign into GitHub with access to this private repository. Open [draft PR #1](https://github.com/gabcalisman/rastro/pull/1), choose **Checks**, and open its **macOS build** run. Alternatively open the repository's [Actions page](https://github.com/gabcalisman/rastro/actions/workflows/macos-build.yml) and choose the run for `prepare-mac-testing`. Keep the PR in draft; merging is not required to get an artifact.
2. Wait for the build job to finish with a **green success** status. Open the run's summary and scroll to **Artifacts**. Download **RASTRO-macOS-universal**. If the run failed, inspect the first failed step; do not assume a ZIP was produced. If Actions is disabled or GitHub requires approval, a repository owner must enable/approve the workflow in GitHub. No developer secrets or signing certificate are required by this workflow.
3. In Finder, open the downloaded artifact ZIP. Inside is **`RASTRO-macOS-universal.zip`** plus a SHA-256 file. Open that **inner ZIP** too. This extra ZIP preserves the app/plugin permissions and symlinks that artifact downloads otherwise lose. Keep the complete `.component`, `.app` and `.vst3` bundles intact.
4. Open the extracted `RASTRO-macOS-universal` folder. Check `BUILD-INFO.txt` for the source commit, actual bundle architectures, toolchain and testing limits. Artifacts expire after **14 days**. Re-run the workflow from its run page if GitHub offers that control and you have access; manual **Run workflow** is available once this workflow exists on the default branch.
5. Quit Logic Pro. In Finder choose **Go → Go to Folder**, enter `~/Library/Audio/Plug-Ins/Components/`, and copy **`AU/RASTRO.component`** there. If the folder is absent, create it using the Terminal command below. Move any older RASTRO bundle to a backup outside the plugin folders first. Logic uses the AU `.component`; it cannot load the VST3 bundle.
6. Optionally copy **`Standalone/RASTRO.app`** into your user `~/Applications` folder. The VST3 bundle is optional for another compatible Mac host; place it in `~/Library/Audio/Plug-Ins/VST3/`. You do not need Xcode/CMake just to download and install these bundles.

For exact copy commands, open Terminal, type `cd ` (including the space), drag the **extracted inner `RASTRO-macOS-universal` folder** into Terminal, press Return, then run:

```sh
mkdir -p "$HOME/Library/Audio/Plug-Ins/Components" "$HOME/Applications"
ditto "AU/RASTRO.component" "$HOME/Library/Audio/Plug-Ins/Components/RASTRO.component"
ditto "Standalone/RASTRO.app" "$HOME/Applications/RASTRO.app"
# Optional, for a VST3 host (not Logic):
mkdir -p "$HOME/Library/Audio/Plug-Ins/VST3"
ditto "VST3/RASTRO.vst3" "$HOME/Library/Audio/Plug-Ins/VST3/RASTRO.vst3"
```

The AU is now expected at **`~/Library/Audio/Plug-Ins/Components/RASTRO.component`** and the app at **`~/Applications/RASTRO.app`**. Downloaded unsigned/notarized test builds may be blocked by macOS security checks even with a valid ad-hoc signature. Follow macOS's explicit approval flow only for a build/commit you trust (for the app, look in System Settings → Privacy & Security if it was blocked). Do not disable Gatekeeper globally. If the AU cannot be approved/validated, use the local source-build steps below or share the exact error for diagnosis.

Validate the installed AU in Terminal:

```sh
auval -v aumu Rstr GbCa
```

Look for **`AU VALIDATION SUCCEEDED`**, then open Logic Pro → Settings → Plug-in Manager and rescan RASTRO if necessary. Add it to a **Software Instrument** track through **AU Instruments → Gab Calisman → RASTRO → Stereo**, load a WAV/AIFF, and start at low listening volume. Follow the detailed Logic test checklist below for MIDI, note-off, effects, tempo, automation, project save/reopen and offline bounce. Those tests, Apple-silicon execution, physical audio/MIDI, and downloaded-bundle security behavior still require testing on your own Mac. **No Logic Pro playback test is claimed by the workflow.**

### macOS / Logic Pro: first build and test

**These are instructions to run on your Mac, not a report of a successful Mac test.** This chat workspace is Linux. The separate GitHub Actions macOS workflow can build Mac bundles; check its actual run status for build evidence. Neither that workflow nor this workspace runs Logic Pro. Linux `.so` files and the Linux Standalone executable cannot be installed on a Mac.

#### 1. Review the existing draft PR

The source and tests are on **`prepare-mac-testing`**, linked to [draft PR #1](https://github.com/gabcalisman/rastro/pull/1) against `main`. Review its **Files changed** and **Checks** tabs. Use the successful **macOS build** run associated with the current commit, not an older run. You can download and test the plugin while the PR stays a draft; no merge or repository visibility change is needed.

A GitHub connection does not automatically save future Codex edits. For later updates, review `git status`, the diff and any new files; commit and push to this same branch when authorized. Do not create another PR for this test version. If you only want to install the binary, use the download instructions above and skip the compiler prerequisites.

#### 2. Install the Mac prerequisites

You need a Mac with a macOS version supported by your installed Xcode and Logic Pro, enough disk space for Xcode/build files, and an internet connection for the first JUCE download.

1. Install **full Xcode** from the Mac App Store. Open it once, accept the license, and let it finish installing its components. The commands below use the **Xcode generator**, so Command Line Tools alone are insufficient.
2. Open **Terminal** (Applications → Utilities → Terminal). Select the full Xcode installation and finish its setup:

   ```sh
   sudo xcode-select --switch /Applications/Xcode.app/Contents/Developer
   sudo xcodebuild -runFirstLaunch
   xcodebuild -version
   git --version
   ```

   `sudo` asks for your Mac login password; Terminal does not display the characters as you type. If Xcode is installed elsewhere, substitute its actual path. `xcodebuild -version` should print an Xcode version, not a “requires Xcode” error.
3. Install **CMake 3.22 or newer**. If you already use [Homebrew](https://brew.sh), run:

   ```sh
   brew install cmake
   cmake --version
   ```

   If you do not use Homebrew, get the macOS CMake application from <https://cmake.org/download/> and place it in `/Applications`. In each Terminal session, make its commands available with:

   ```sh
   export PATH="/Applications/CMake.app/Contents/bin:$PATH"
   cmake --version
   ```

4. Install/open Logic Pro for the later host test. You do **not** need a separate JUCE download, VST SDK installation, or paid font. CMake fetches JUCE **8.0.6** if that version is not installed. JUCE licensing still applies to distribution.

#### 3. Get the published project onto your Mac

To build the current draft version without merging it, run in your Mac Terminal:

```sh
mkdir -p "$HOME/Developer"
cd "$HOME/Developer"
git clone https://github.com/gabcalisman/rastro.git
cd rastro
git switch prepare-mac-testing
git pull --ff-only
test -f CMakeLists.txt && test -f Source/Processor.cpp && echo "Prototype source is present"
```

This repository is private: authenticate with your GitHub account using an existing credential manager/GitHub CLI setup or use GitHub Desktop to clone it, then open that clone in Terminal. A normal GitHub account password is not a Git HTTPS password. Do not share authentication tokens in this chat.

If you have already cloned it, open that folder and run `git status` before pulling; preserve any local edits. Run `git fetch origin`, switch to `prepare-mac-testing`, then `git pull --ff-only`. Run the remaining commands from the folder containing `CMakeLists.txt`. If the final source check prints nothing, check that you selected the correct branch before building.

#### 4. Configure and build the Mac plugins

For the simplest first test, build for the architecture of this Terminal session:

```sh
uname -m
cmake -S . -B build-mac -G Xcode -DCMAKE_OSX_ARCHITECTURES="$(uname -m)"
cmake --build build-mac --config Release --target Rastro_AU Rastro_VST3 Rastro_Standalone --parallel 2
```

`uname -m` normally prints `arm64` on Apple silicon or `x86_64` on an Intel Mac. On Apple silicon, use a native Terminal (disable its “Open using Rosetta” setting) for a native arm64 build and run Logic natively too. Allow the initial JUCE download and compilation to finish. A failed command is not a completed build; keep the first compiler error for diagnosis.

Expected outputs, relative to the project folder:

| Format | Build output | Where to test it |
| --- | --- | --- |
| AU instrument | `build-mac/Rastro_artefacts/Release/AU/RASTRO.component` | Logic Pro |
| VST3 instrument | `build-mac/Rastro_artefacts/Release/VST3/RASTRO.vst3` | A Mac DAW/host that supports VST3 |
| Standalone | `build-mac/Rastro_artefacts/Release/Standalone/RASTRO.app` | Open as a Mac application |

**Logic Pro loads AU, not VST3.** A `.component`, `.vst3` or `.app` is a bundle; copy the entire bundle, not a file from inside it. CMake has automatic plugin copying disabled, so building alone does not install the plugins.

Optional: build both Intel and Apple silicon architectures into universal bundles using a **separate build folder**:

```sh
cmake -S . -B build-mac-universal -G Xcode -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"
cmake --build build-mac-universal --config Release --target Rastro_AU Rastro_VST3 Rastro_Standalone --parallel 2
```

For that build, replace `build-mac` with `build-mac-universal` in the commands below. Verify architectures with `lipo -archs build-mac/Rastro_artefacts/Release/AU/RASTRO.component/Contents/MacOS/RASTRO`. This guide does not claim that either architecture has been built here.

#### 5. Run the available checks on your Mac

The portable DSP test is included in the normal configuration:

```sh
cmake --build build-mac --config Release --target rastro_tests rastro_character_tests rastro_playback_tests rastro_mp3_tests --parallel 2
ctest --test-dir build-mac -C Release --output-on-failure
```

For additional processor/state/editor checks, reconfigure with the integration tests enabled:

```sh
cmake -S . -B build-mac -DRASTRO_INTEGRATION_TESTS=ON
cmake --build build-mac --config Release --target rastro_tests rastro_character_tests rastro_playback_tests rastro_mp3_tests rastro_integration --parallel 2
ctest --test-dir build-mac -C Release --output-on-failure
```

The expected suite count is four without integration tests and five with them; success means all enabled tests pass. Run the editor checks in a logged-in graphical Mac session. Passing these tests is separate from passing AU validation and actually using Logic Pro.

#### 6. Install the bundles for your Mac user

Quit Logic Pro and any other audio host before installing or replacing a plugin. From the project folder:

```sh
mkdir -p "$HOME/Library/Audio/Plug-Ins/Components"
mkdir -p "$HOME/Library/Audio/Plug-Ins/VST3"
ditto "build-mac/Rastro_artefacts/Release/AU/RASTRO.component" \
  "$HOME/Library/Audio/Plug-Ins/Components/RASTRO.component"
ditto "build-mac/Rastro_artefacts/Release/VST3/RASTRO.vst3" \
  "$HOME/Library/Audio/Plug-Ins/VST3/RASTRO.vst3"
```

The expected installed paths are **`~/Library/Audio/Plug-Ins/Components/RASTRO.component`** and **`~/Library/Audio/Plug-Ins/VST3/RASTRO.vst3`**. Here `~` means your user home folder; these instructions install only for your account and require no `sudo`. In Finder, choose Go → Go to Folder and enter either parent folder to inspect it. If replacing an older test build, move the old bundle to a backup folder outside the plugin folders first to avoid merging stale bundle contents. Avoid keeping duplicate copies in both your user Library and `/Library/Audio/Plug-Ins/`.

For a local test, if code-signature verification fails, ad-hoc sign **your locally built installed copies**, then verify them:

```sh
codesign --force --sign - "$HOME/Library/Audio/Plug-Ins/Components/RASTRO.component"
codesign --force --sign - "$HOME/Library/Audio/Plug-Ins/VST3/RASTRO.vst3"
codesign --verify --verbose=2 "$HOME/Library/Audio/Plug-Ins/Components/RASTRO.component"
codesign --verify --verbose=2 "$HOME/Library/Audio/Plug-Ins/VST3/RASTRO.vst3"
```

Ad-hoc signing is for local testing; it is not Developer ID signing or Apple notarization. Distribution signing/notarization is not configured. Do not disable Gatekeeper or other system security settings to make a failed build load.

#### 7. Validate the AU and test in Logic Pro

First, in Terminal, validate the installed AU:

```sh
auval -v aumu Rstr GbCa
```

The identifiers match the CMake configuration: instrument type `aumu`, subtype `Rstr`, manufacturer `GbCa` (case-sensitive). Look for **`AU VALIDATION SUCCEEDED`**. If it fails or cannot find the AU, retain the output; do not treat installation as validation success.

1. Open Logic Pro. Go to **Logic Pro → Settings → Plug-in Manager** (older versions call Settings “Preferences”). Find **RASTRO**, manufacturer **Gab Calisman**. If needed, select it and use **Reset & Rescan Selection**. Confirm its validation status before proceeding.
2. Create an empty project and a **Software Instrument** track. If the Library opens with another instrument, open the channel strip's **Instrument** slot and choose **AU Instruments → Gab Calisman → RASTRO → Stereo**. Do not look for it in an Audio FX slot. If your Logic version also offers Mono, test that after the Stereo test.
3. Start with low monitor/headphone volume. Load a short, known-good WAV or AIFF. Check the waveform appears. Play the plugin's onscreen keyboard, then Logic's Musical Typing (**Command-K**) or a MIDI keyboard with the track selected/record-enabled. Confirm MIDI activity and sound. Root note **60** means the original sample pitch; octave naming can differ between Logic and the plugin.
4. Release keys and check notes stop. Test one-shot and Loop, root note, start/end trim, velocity/filter amount, chords, and output level. Root/trim changes apply to newly triggered notes.
5. Compare each effect's active and bypassed sound; adjust every control. Drag card headers to reorder, remove/re-add a module, compare Dry/rack, use MUTATE and Reset effects. Confirm MUTATE does not replace the sample or jump the output level. Try CC learn if you have a controller.
6. Change the Logic project tempo and confirm the plugin's host BPM display and repeat timing follow it. Test the manual fallback in Standalone, where there is no host BPM. Timing follows BPM, not bar position.
7. Switch Portuguese/English, resize the window, and record a short MIDI region. Automate a macro using Logic's automation controls and play it back. Play, stop, seek, and repeat to check for stuck notes or glitches.
8. Save the Logic project. Quit and reopen Logic, then reopen the project. Confirm the sample, trims/root, effect values/order/bypass/removal, CC assignments and language return. To check sample embedding, after saving and quitting temporarily rename/move **your test source file**, reopen the project and check it still plays; restore the source file afterwards.
9. Bounce a short passage offline and listen to the resulting file. Compare it with playback and confirm the effect tail is not cut off. The declared host tail is 12 seconds; extreme settings can ring longer. Repeat at your usual buffer size and, if useful, 44.1/48 kHz.

Record your macOS/Logic/Xcode versions, Mac chip, plugin architecture, sample rate, buffer size, AU validation output, and any reproduction steps. Hardware MIDI, audible output, AU scanning, project recall and bounce are all **pending Mac tests**, even though Linux checks have passed.

#### If something fails

- **CMake/Xcode not found:** revisit prerequisites; `xcode-select -p` should point inside full Xcode, and `cmake --version` must work in this Terminal.
- **Wrong generator or architecture cache:** use a new build folder; do not mix Linux build files or an old generator into the Mac build folder.
- **JUCE download fails:** check internet/repository access and retry configuration. The first build needs the pinned JUCE source; copied Linux binaries will not help.
- **Plugin not listed:** verify the `.component` path and architecture, quit/reopen Logic, run `auval`, and rescan just RASTRO. If a newly installed AU is still undiscovered, log out/in or restart your Mac, then rescan. Do not delete all Audio Unit caches as a first step.
- **Validation/signature failure:** check the first `auval`/`codesign` error, confirm native/universal architecture matches your Logic process, and rebuild/reinstall. Do not simply force-enable a failing AU.
- **No sound:** use a Software Instrument track, select it for MIDI input, load a sample, check output/trim values, test the onscreen keyboard, and verify Logic's audio device settings.
- **Uninstall:** quit hosts, then move only the installed `RASTRO.component` and `RASTRO.vst3` bundles out of the two user plugin folders. Keep the source/build folder if you plan to rebuild.


### Tests

Portable DSP checks can run without JUCE:

```sh
cmake -S . -B build-core -DRASTRO_CORE_ONLY=ON
cmake --build build-core
ctest --test-dir build-core --output-on-failure
```

Optional processor/editor integration checks require JUCE and a display (Xvfb works on Linux):

```sh
cmake -S . -B build -DRASTRO_INTEGRATION_TESTS=ON
cmake --build build -j2
xvfb-run -a ctest --test-dir build --output-on-failure
```

`rastro_tests` checks playback, release, effect audibility/stability, extreme trims, voice stealing and four sample rates, with allocation trapping in the DSP path. `rastro_integration` generates WAV/AIFF fixtures, checks mono/stereo import/rendering, sample-accurate MIDI onset, host tempo, state roundtrip, CC learn, invalid input, mutation preservation and editor snapshots at minimum/maximum sizes. Processor audio processing also runs under an allocation trap. It writes `editor-preview.png` into the test working directory.

## Structure and realtime behavior

- `Source/Engine.h`: JUCE-independent 16-voice MIDI sampler; `EngineTypes.h` holds sample/settings data and `EffectRack.h` holds the shared effects DSP. Buffers allocate only in prepare; audio rendering performs no file I/O, locking or allocation. Level, filter amount, mixes, effect controls and bypass use smoothing. Order changes fade out/in over 5 ms.
- `Source/Processor.*`: host/MIDI adapter, parameters, tempo, file decoder and embedded state. Keyboard events use a fixed SPSC queue; host parameter notifications from CC changes are deferred off the audio thread. With the editor closed, CC changes affect audio immediately and are flushed to parameters when state is saved or the editor reopens.
- `Source/Editor.*`, `Performance.*`, `SampleView.*`: resizable vector interface, interactive waveform, PLAY/avatar, draggable cards, CC learn and keyboard.
- `Source/Strings.h`: centralized pt-BR / English UI text, tooltips, errors and About/help content. Module names remain unchanged.
- `tests/`: portable engine tests and optional JUCE integration tests.

The file chooser dispatches decoding and waveform precomputation to a processor-owned worker. The loader publishes immutable samples through an atomic pointer. Old sample buffers remain owned until processor destruction, so no sample can be freed on the audio thread or while a voice/editor reads it. Import is limited to **32 MB encoded, 120 seconds, 16 million frames**, with a **256 MB decoded-audio budget per instance lifetime**. Reopening the instance releases that budget. This simple retention strategy is deliberate for this prototype; a background retirement queue is a future improvement. Loading and state restoration decode on their caller's non-audio thread; the audio callback never decodes files. Hosts must use the normal non-realtime state lifecycle.

## Known limitations

- Prototype DSP: linear interpolation can alias at high pitches; no high-quality resampling, sustain pedal, MPE, per-channel voice ownership or voice-steal crossfade. Overlapping same-pitch notes release together. Loop edges fade briefly rather than using an overlapping crossfade; very short loops can sound quiet.
- One sample and one module of each type. Imported audio uses the first two channels. Source replacement stops active voices. One-level mutation undo is provided; there is no general undo/preset browser or standalone preset-file UI. Use host state/presets. Restore errors retain the prior state.
- Effect timing follows BPM but not bar phase. Delay time/size movement can produce intentional pitch smearing. BLOOM is a basic diffuse tail; effect bypass keeps internal buffers warm, so old tails may reappear when re-enabled. The declared host tail is 12 seconds; extreme feedback can ring longer.
- File-chooser decoding runs on a worker; state restoration still runs on its host caller outside the callback. Session states embed base64 source audio and can be large.
- CC learn uses the first incoming CC, on any channel. Host automation notification for learned CC is deferred to the editor/state thread; closed-editor recording of those CC gestures as host automation is not implemented. Hardware MIDI, physical audio output and DAW workflow require hands-on testing.
- The minimum editor size is 980 × 700; standard is 1180 × 840, compact is 1000 × 700. The waveform cursor follows a single most-recent voice position; the avatar uses aggregate measurements. Animation can be reduced or disabled.
- Video-derived audio and the separate RASTRO FX plugin are deliberately not implemented. Decoder, sampler, shared rack and host/editor layers remain separate for later reuse.

## Historical verification of the initial prototype

This section records the initial prototype checks. Current local Stage 1/M4A results are described above and in `docs/UI-VALIDATION.md` and `docs/AUDIO-IMPORT.md`.

Built on Debian 13 x86_64 with GCC 14.2, CMake 3.31.6 and JUCE 8.0.6. Since system package installation was unavailable, Debian development packages were downloaded and extracted into workspace-local build tooling; no privileged installation was needed.

- VST3 and Standalone Release builds completed.
- CTest: **2/2 suites passed** (portable DSP and processor/editor integration).
- Engine checks passed under AddressSanitizer and UndefinedBehaviorSanitizer. Leak detection was disabled because LeakSanitizer is incompatible with the sandbox's tracing environment.
- **pluginval 1.0.3, strictness 5, seed 42: exit 0**, including VST3 scan/open, editor, processing, state, automation and bus checks. The separate Steinberg validator was not configured; pluginval's auval placeholder on Linux is not an AU validation.
- Portuguese and English editors were rendered with Xvfb; visual issues found in the snapshots were fixed. Minimum and maximum window sizes were exercised.
- Standalone launched under Xvfb and remained running until the smoke test's 4-second timeout. ALSA reported `/dev/snd/seq` unavailable; hardware MIDI and audible output were not tested.
- **No macOS build, AU validation or Logic Pro test was performed.**

To repeat external validation after installing [pluginval](https://github.com/Tracktion/pluginval):

```sh
pluginval --strictness-level 5 --random-seed 42 --validate build/Rastro_artefacts/Release/VST3/RASTRO.vst3
```

### Mac preparation follow-up (October 21, 2026)

This follow-up changed only the README. The CMake source was inspected: VST3 and Standalone are enabled generally; AU is appended under `if(APPLE)`, and `IS_SYNTH TRUE`, `NEEDS_MIDI_INPUT TRUE`, manufacturer `GbCa` and subtype `Rstr` are configured. Linux CMake configuration succeeded again, the existing Release build was up to date, CTest passed **2/2 suites**, and `git diff --check` passed. No Apple compiler/SDK or Logic Pro was available. macOS AU/VST3 compilation, installation, signing/architecture compatibility, `auval`, and all Logic Pro listening/recall/bounce tests remain unverified. The publication instructions above do not themselves publish the workspace.
