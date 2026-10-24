# RASTRO PLAY — Instruções do projeto

Leia antes de qualquer tarefa. Se algo aqui conflitar com o código, o código mostra o estado real; este arquivo mostra a intenção. Aponte o conflito em vez de escolher sozinho.

## Quem sou e como trabalhar comigo

Sou Gab Calisman, músico e produtor brasileiro, aprendendo a parte técnica.

- Explique decisões e limitações em linguagem simples, em português.
- Nunca diga que algo funciona sem compilar e testar. Diga o que foi testado e o que não foi.
- Não presuma que uma função existe só porque está descrita aqui. Confira no código.
- Preserve o trabalho atual. Nunca reverta nem sobrescreva alterações sem eu pedir.
- Trabalhe sempre numa branch própria. Sem merge e sem release pública sem eu pedir.
- Não crie controles, botões ou telas falsas: cada opção faz o que promete.
- Não copie código, DSP, interface, arte ou presets de outros plugins.
- Se uma ideia minha for ruim tecnicamente, diga o motivo e proponha alternativa.
- Dois agentes (Claude e Codex) podem trabalhar no repositório: cada um na sua branch, sem mexer na branch do outro.

## A visão

RASTRO é uma família de plugins. O primeiro é o **RASTRO PLAY**: instrumento sample-based tocado via MIDI.

Meta: **o primeiro sampler/synth/hub musical gamificado**, diferente de tudo que existe. Produtores devem querer passar horas nele **como num jogo**: reativo, fluido, com fator uau, e principalmente **sonoramente inovador**. Teste dos primeiros 60 segundos: arrastar um sample, tocar e já soar interessante e divertido sem ajustar nada.

O RASTRO FX (efeitos para tracks) é futuro. Não crie agora.

## Estado real (auditoria de 22/10/2026)

- C++17, JUCE 8.0.6, CMake. AU (Mac), VST3, Standalone; universal arm64 + x86_64 pelo GitHub Actions (macOS). Assinatura ad-hoc, sem notarização.
- Branch do Codex: `prepare-mac-testing` (PR #1, rascunho). `main` só tem README.
- Motor pronto e testado: 16 vozes, Poly/Mono/Legato, glide, ADSR, filtro LP ressonante, envelope de filtro, LFO (cutoff/pitch), velocity, CC64, CC1, pitch bend, MIDI Learn.
- Reprodução: One Shot, Loop, Reverse, Ping-Pong, Slice (2–16 cortes), LOOP independente (`regionLoop`) com crossfade, REPITCH/FIXED PITCH, disparo Legado/One Shot/Gate.
- Rack com **seis** algoritmos reais e reordenáveis: GHOST (eco), CRUMBLE (fragmentos), BLOOM (cauda difusa), WOBBLE (atraso modulado), DUST (textura), CRUSH (bits/taxa). FREE/SYNC em GHOST, CRUMBLE e WOBBLE.
- **Limitação importante:** o rack é fixo em 6 módulos; a ordem fica num parâmetro de permutação (`rackOrder`, 0–719). Adicionar Tape/Chorus exige reformar isso com migração de sessões.
- JAM (variação dos efeitos com desfazer e bloqueios), pad PLAY XY, Patch Search aproximado, patches `.rastro` com áudio embutido, visuais `.rastro-look`.
- Importação: WAV, AIFF, MP3 (minimp3), M4A AAC/ALAC no Mac (CoreAudio).
- Gravação no Logic: instrumento AU sem entrada de áudio; existe o insert complementar **RASTRO Capture**.
- Testes: 8 suítes portáveis (passam no Linux) + 2 no Mac + suíte de integração opcional; pluginval e auval no CI.

## Decisão: interface web (marco 1, em andamento)

- O editor do plugin no Mac é uma **interface web** (`ui/`) dentro do JUCE `WebBrowserComponent` (WebKit). Código: `Source/WebEditor.*`. Opção CMake `RASTRO_WEB_UI` (ligada no Mac). Linux/Windows continuam com o editor nativo antigo (`Source/Editor.*`).
- O motor de áudio não muda por causa da interface. A ponte só: lê telemetria atômica, lê/escreve parâmetros na thread de mensagens (com gestos para automação) e envia notas pela fila do teclado.
- Arquivos da interface: `index.html`, `style.css`, `bridge.js` (ponte + motor **simulado** só para prévias no navegador), `art.js` (arte vetorial em partes), `app.js` (controles, animação, jogo). Nomes de arquivo únicos e planos: são embutidos com `juce_add_binary_data`.
- Compatibilidade: WKWebView do macOS 12+ (Safari 15). Sem CSS aninhado, sem recursos muito novos.
- Prévia: abrir `ui/index.html` num navegador mostra a interface com motor simulado (avisado na tela).

## Direção visual

Referências em `docs/referencias/`. **São direção visual, não lista de funções.**

- Base: conceito 2, **Sunset** (creme, retrô anos 70). Tema **Noite** com a paleta neon do conceito 1.
- Skin **Ilustrado** (conceito 4): molduras grossas com sombra dura, **folhas nas bordas** (decoração que nunca cobre controles), **módulos do rack como cartões ilustrados** cuja arte mostra o que o efeito faz.
- Modulações também aparecem: cutoff do filtro = luz do quarto; LFO = balanço/pulso de luz no ritmo do LFO; pad PLAY X = câmera balançando, Y = imagem degradando (só com os módulos ativos).
- O palco reage ao som de verdade: personagem dança no BPM só quando há som, reage a cada nota, e **os efeitos visuais espelham os efeitos sonoros** (GHOST deixa rastros do personagem, CRUMBLE gera falhas, BLOOM brilha, WOBBLE balança, DUST solta poeira, CRUSH pixeliza).
- Logo: **RASTRO** em destaque, **Play** em script colado, **by Gab Calisman** visível.
- Contraste, áreas clicáveis confortáveis, animação completa/reduzida/desligada.

## Personagens

Quatro personagens aprovados nos conceitos: monstrinho peludo de um olho (Fuzz), alien azul (Nova), cara afro anos 70 (Soul), personagem roxa de cachos (Lua).

- **Fuzz** usa a arte pintada (gerada a partir do conceito, fundo magenta recortado): `ui/fuzz_body.png` + `ui/fuzz_head.png` (cabeça separada no pescoço para balançar). Coordenadas em `fuzzArt()` no `art.js`.
- **Nova** ainda é vetor provisório em partes (`art.js`).
- Pipeline de arte: o GPT gera cada personagem e cenário a partir dos conceitos, com fundo de cor chapada (#FF00FF para azuis/turquesa, #00FF00 para os outros); recortamos com chave de cor só nas regiões ligadas à borda (preserva rosa/roxo internos).
- Cenário **Noite** usa o quarto pintado `ui/room_neon.jpg`, com brilhos reativos posicionados sobre os objetos (caixas, luminárias, fliperama, janela). Cenário **Sunset** ainda é vetorial até chegar a arte pintada.
- Para animação mais rica, o próximo passo é pedir partes separadas (cabeça, tronco, braços, pernas, instrumento) e objetos soltos do cenário.

## Gamificação

Opcional, com **Modo PRO** que desliga tudo. Hoje: XP por tocar e criar, combo, bônus "no tempo" com o transporte do host, níveis que liberam acessórios (coleção de 8). Guardado localmente (`localStorage`), sem afetar o som. Nada disso toca a thread de áudio.

## Decisões recentes (22/10/2026)

- Gab é o **único desenvolvedor**. Não há outra equipe no projeto.
- Interface em **abas** (SOUND, RACK, PATCHES, CHARACTER) no layout 1440×900 dos conceitos; nada de tudo numa tela só.
- Quatro personagens pintados (Fuzz, Soul, Lua, Nova) e quatro quartos pintados (Sunset Loft, Quarto Neon, Lo-fi, Base Lunar), escolhidos por `avatarCharacter` e `sceneRoom` (parâmetros só visuais, adicionados ao fim).
- Personagem só se mexe com som real; movimento suavizado; proporções preservadas (squash mínimo).
- Toda edição real no som rende um pouco de XP (com limite de frequência), não só o JAM.
- **Síntese wavetable** entra no roadmap como pilar: o RASTRO PLAY deve ser sampler + synth de verdade, com ambição de ser revolucionário.
- Ideias futuras: rede/nuvem de produtores para compartilhar patches e presets, tarefas diárias, customização infinita de personagens e sons.

## Decisões (23/10/2026) — etapa 1 do redesenho

- **Personagem principal: alien-sombra** desenhado em código (`ui/alien.js`) como rig 2D de verdade: pés, pernas (IK), quadril, tronco, ombros, braços (IK), mãos, cabeça. Roupas e itens são desenhados ao longo dos ossos (acompanham as juntas). Os personagens pintados viram **convidados** de visual fixo.
- Danças: Groove 70s, Disco, Half-Time (livres), Funk Bounce (nível 2), Cosmic Flow (nível 4). SYNC segue o pulso do host; FREE usa BPM próprio. Notas e velocity são só acentos, com limite para rajadas.
- Instrumentos visuais (keytar, guitarra, baixo, pads) mudam a pose e o jeito de tocar; nunca o som.
- **Look** (roupa, dança, sync, knob) fica salvo na sessão (`look`, texto JSON no estado do plugin) e não muda o som. Patch "somente som" preserva o look atual.
- **Progresso cosmético** (XP, looks salvos, desafios) fica em `~/Library/Application Support/RASTRO/progress.json`. XP só libera aparência.
- Modos de tela: **Limpo** (abas SOM, PERFORM, RACK, PATCHES, ATELIÊ), **Palco** (personagem e quarto na janela toda, faixa de X/Y opcional) e **Captura** (página de gravação).
- Transporte global ▶ ❚❚ ■ controla só a prévia interna. Pausa congela a animação; Stop volta ao início e à pose inicial.
- Padrões novos: **tema noturno**, **todos os efeitos desligados**, alien e Quarto Neon. Sessões antigas mantêm o que tinham (migração em `restoreState`). O pad X/Y continua no centro porque o centro é o ponto neutro (X em 0 zeraria o WOBBLE).
- Rack **Serial/Parallel** real (`rackParallel`, parâmetro de som novo).
- Dado (GERAR PATCH) sorteia instrumento + efeitos em B, com A guardado; JAM continua só variando efeitos ativos.

## Decisões (23/10/2026, noite) — interface refeita pelas referências aprovadas

- A interface segue **exatamente** as telas de referência do Gab (Limpo, Palco, Perform, Locker, Patch Lab): fundo violeta escuro, painéis de vidro com borda neon fina, dourado para o item ativo. Não reinventar layout.
- Logo: RASTRO em serifa suave creme (Fraunces Black/SOFT, OFL) com estrela no A, PLAY em magenta, "by Gab Calisman" em ciano. UI em Poppins (OFL). Fontes em `ui/*.woff`, licenças em `third_party/fonts/`.
- **Personagem oficial = arte pintada do alien** (`ui/al_*.png`, recortadas das folhas aprovadas), animada como marionete 2.5D em `ui/puppet.js` (pernas cisalham a partir dos pés, tronco gira no quadril, cabeça com atraso). Um visual por instrumento: keytar, guitarra, baixo, pads, flare 70s. `ui/alien.js` (rig vetorial com IK) fica para quando a arte vier em partes.
- Peças avulsas (chapéu, fones, óculos) só aparecem quando houver arte em partes; até lá o Locker troca visuais completos.
- Biblioteca de fábrica (`ui/factory.js`): sons sintetizados em código (FM, Karplus-Strong, supersaw, ruído filtrado), afinados em C4, renderizados para WAV só quando escolhidos. Patches do usuário em ~/Música/RASTRO/Patches (o SAVE abre lá).
- Reações visuais por efeito (cores do mapa): GHOST violeta (rastros), CRUMBLE âmbar (estilhaços), BLOOM rosa (aura/lótus, cauda segue o DECAY), WOBBLE ciano (contorno que balança), DUST dourado (partículas), CRUSH roxo (pixels). Intensidade = atividade real do módulo; desligado = sem reação. Filtro esquenta/escurece a luz; envelope real comanda o brilho; LFO pulsa luzes; VISUAL REACTIVITY (só visual) escala tudo.
- Reverb, Chorus, Delay, Tape e Wavetable **não existem no DSP**: nenhuma reação visual para eles até existirem.

## Próximos marcos

1. **Visual (em andamento):** interface web, palco reativo, jogo inicial. Falta validar no Logic.
2. **Rack expansível:** mais de 6 módulos com migração; Tape e Chorus com DSP real.
3. **Motor wavetable**: osciladores próprios (wavetables, morph de posição, unison) tocando junto ou no lugar do sample, pelo mesmo filtro, ADSR e rack. Incluir criar wavetable a partir do próprio sample.
4. **RASTRO TRAIL** (assinatura sonora): cada nota deixa repetições que se transformam, guiadas pela forma do próprio sample. Duelo Claude × Codex nesta etapa.
5. Arte final dos personagens em partes (animação mais rica), tarefas diárias, idioma EN.
6. Nuvem/rede de patches (produto separado: servidor, contas, moderação).

Ao terminar cada marco, proponha o plano do próximo e espere aprovação.

## Testes

Automáticos sempre que possível: testes portáveis (`-DRASTRO_CORE_ONLY=ON`), build Mac no CI, auval, pluginval. A interface web pode ser testada no navegador com o motor simulado, mas isso **não** prova funcionamento no Logic.

O que só eu verifico: som no Logic, sensação de tocar, legibilidade, animações, desempenho no meu Mac.

## Relatório ao fim de cada tarefa

Separe: o que já existia; o que mudou; o que ficou incompleto; testes que passaram de verdade; o que preciso verificar no Mac e no Logic; branch e commit.

## Decisões (23/10/2026, madrugada) — palco vivo, logo e arte do Gab

- **Logo oficial = a arte do Gab** (`ui/logo.png`, PNG transparente recortado do arquivo dele). Não redesenhar com fontes.
- **Cenário padrão: Estúdio Palco** (`ui/room_stage.jpg`, `sceneRoom` = 5; sessões antigas mantêm o quarto salvo). Tudo vive num grupo "mundo" com câmera de jogo (enquadra plataforma + personagem, respira, segue o mouse, empurra no beat). Luzes presas a objetos reais da pintura: luminárias, caixas, lava lamp, fitas neon, janelas da cidade, planeta, anéis e barras da plataforma, cones de luz, bokeh na frente. Personagem em pé na plataforma com sombra e reflexo.
- Personagem com sensação 3D: pulos com squash na aterrissagem (Disco/Funk/Expressivo), giro falso (yaw), inclinação, luz de contorno, respiração e balanço parado. Dança só com som; no Locker, prévia leve.
- **Knobs = imagens do Gab** (`ui/knob_*.png`, 43 modelos). Vista de cima: a imagem gira. Vista 3/4: só o topo gira, recortado na elipse do topo. Look salvo com `knob` em texto; índices antigos são migrados.
- **Locker** sobre o palco real (o quarto ocupa a janela). Itens com a arte do Gab (`ui/it_*.jpg`). Outfits e instrumentos funcionam (trocam o visual pintado completo). Chapéus, fones, óculos e as 4 roupas novas aparecem como **EM BREVE**: precisam da arte em partes.

## Decisões (23/10/2026, tarde) — abas do prompt e Estúdio

- Abas principais: **PLAY** (sample), **SOM** (instrumento), **RACK** (efeitos), **PATCHES** (biblioteca + Patch Lab + A/B), **ESTÚDIO** (Locker). Por baixo, são as telas Limpo/Lab/Locker de antes; nada de estado foi perdido.
- **JAM agora é modo de performance** (botão no topo): mostra personagem, patch, teclado e a faixa X/Y; "SAIR DO JAM" volta à aba anterior. A antiga ação JAM (variar os efeitos ligados) virou **VARIAR FX** em MACROS, com DESFAZER VARIAÇÃO. O motor e o nome interno (`jam`) não mudaram.
- **ESTÚDIO** no estilo das referências: moldura com o personagem à esquerda (cartão RASTRO com o resumo do look), à direita **WARDROBE** (looks completos em cartões) e **MIX & MATCH** (categorias + itens). Knobs, paletas, instrumentos, cenário e desafios ficam dentro do MIX & MATCH.
- 9 roupas novas da folha do Gab (`ui/al_o_*.png`) são visuais completos de verdade (sem instrumento, mão apontando). Cabelo, chapéu, olhos e áudio continuam **EM BREVE** (as folhas novas viram miniaturas, mas o corpo ainda não está em camadas).
- Expressão: olhos brilham a cada nota (intensidade = velocity) e a boca se abre em acentos fortes.

## Auditoria do rack (23/10/2026) — nomes e melhorias

- Equivalências (o nome RASTRO fica, o subtítulo explica): **GHOST · Delay/Echo**, **CRUMBLE · Stutter/Granular**, **BLOOM · Reverb**, **WOBBLE · Chorus/Vibrato** (wow de fita), **DUST · Noise/Vinil**, **CRUSH · Bitcrusher**. Não duplicar esses efeitos.
- Não existem: saturação de **Tape**, **Flanger**, **COMP**, **EQ**.
- Novos parâmetros de som (anexados): `bDiffusion` (BLOOM, difusão por dois allpass, padrão 0,6) e `wWidth` (WOBBLE, defasagem L/R, padrão 0,5). Sessões antigas restauram 0 = som idêntico ao anterior.
- Testes novos (suíte dsp): cada controle de cada módulo muda o som; bypass e Mix 0 saem idênticos ao seco; o bypass faz fade (o teste falha se o fade for removido); largura 0 fica mono e largura alta abre o estéreo.
- A antiga macro "JAM %" virou **VARIAÇÃO** (mesmo parâmetro `mutation`).

## Princípio (23/10/2026): aventura sônica premium

- O RASTRO PLAY é **um produto de áudio premium acima de tudo**, com a experiência de ser (talvez) o primeiro synth gamificado. O jogo serve ao som, nunca o contrário.
- Todo efeito novo precisa soar como plugin profissional: sem aliasing audível (oversampling onde houver distorção), parâmetros suavizados, sem cliques, ganho controlado, testes de qualidade além de "funciona".
- Ordem acordada: fechar o PR #2 (validação no Logic + etapa de som: rack expansível, TAPE, COMP, EQ, SIDECHAIN) antes do PR #3 (protótipo do personagem 3D com visual 2.5D).

## Correção (23/10/2026, 17h) — personagem

- O visual "Varsity · Keytar" (`al_keytar.png`) tinha **três mãos** na arte gerada. Retirado até ser redesenhado; looks salvos com ele abrem com o Flare 70s (keytar, duas mãos), que virou o padrão.
- O cenário não se mexe mais sozinho (sem respiração de câmera, sem seguir o mouse, sem tremer a cada nota). Só as luzes e o personagem reagem; a câmera só balança com WOBBLE + eixo X (efeito real).
- Limite honesto: com uma pintura única por pose, o personagem só consegue balançar, inclinar e pular; braços e pernas não mudam de pose. Para ele ficar vivo de verdade é preciso arte de animação (quadros de dança, ou partes separadas, ou modelo 3D). Conferir sempre a contagem de mãos e braços em toda arte nova antes de usar.

## Layout (23/10/2026, 17h30) — como nas referências do Gab

- Telas PLAY, SOM e RACK: o Estúdio Palco ocupa a janela inteira, o personagem fica **grande no meio**, em cima da plataforma; painéis nas colunas laterais (452 px cada) e teclado embaixo. Nada de palco numa caixa no canto.
- PLAY: waveform + playback à esquerda, rack à direita. SOM: ADSR + MIDI/voz à esquerda, filtro/LFO + macros à direita. RACK: rack em coluna à esquerda, efeito selecionado + pad XY à direita.
- Antes de mudar layout, abrir as referências (anexos do Gab) e comparar lado a lado.

## Decisão (23/10/2026, 21h) — Nova cartunesca como personagem principal

- Gab trocou a direção: personagem **cartunesco** (Nova, alien azul) com animação de game: esqueleto, física, ação e reação. Referências: folha de 5 poses + telas PLAY/SOUND/FX RACK/PATCH LAB/LOCKER/JAM.
- `ui/nova.js`: Nova desenhada em código (traço escuro, cores chapadas). Esqueleto quadril → peito → pescoço → cabeça; IK de dois ossos nas pernas (pés plantados) e nos braços (mãos no instrumento, sempre 2). Física: pontas do cabelo, brinco de estrela e cordões do moletom. Ação/reação: tecla sob o dedo pela nota, cabeça e sorriso nos acentos, groove de 6 poses no beat, vida parada (respira, pisca, olha em volta).
- Slots independentes: cabelo, chapéu (um por vez), fones, óculos, roupa de cima, calça, tênis, instrumento. Miniaturas do Locker renderizadas pela própria Nova. Look salvo em `look.nova`.
- O alien pintado (`puppet.js`) e o 3D (PR #3, pausado) ficam como estão, sem uso no padrão.
- Áudio: corrigidos cliques reais (loop/ping-pong caíam a zero a cada volta, roubo de voz cortava seco, janela do CRUMBLE cortada). Detectores de clique na suíte dsp.

## Decisão (23/10/2026, 22h) — Nova exatamente como o Gab desenhou

- Personagem padrão = **a arte do Gab** (alien azul com keytar), recortada em partes (`ui/nv_*.png`) e animada como marionete recortada em `ui/novaart.js`: pés plantados, pernas dobram a partir dos pés, quadril → tronco → pescoço/cabeça (com atraso), keytar com a mão direita no braço do instrumento, braço esquerdo apertando as teclas, brinco de estrela como pêndulo, piscar, respiração, frase de dança de 8 tempos (antecipação, passo, acento, follow-through, recuperação). Em repouso, as partes remontam o desenho pixel a pixel.
- Limite honesto: a arte é um desenho só. Braços e pernas giram pouco (alguns graus); poses grandes pedem a arte em partes desenhadas separadamente.
- Locker: card **Original (arte do Gab)** no WARDROBE. Trocar peça passa para a Nova editável desenhada em código (`nova.js`). `look.art` (1 = original) salvo no look; looks antigos abrem com a original.
- Próximo layout (referência do Gab, 21h53): abas no topo, SAMPLE à esquerda, palco no centro, EFFECT RACK em cartões coloridos à direita, PERFORMANCE / MACROS / GLOBAL embaixo, teclado, painéis creme.

## Layout (23/10/2026, 22h) — ideia de interface do Gab

- Abas no cabeçalho: **PLAY, SOUND, RACK, PATCH LAB, LOCKER, JAM** (JAM abre o modo de performance). Patch à direita; salvar, desfazer, refazer e sortear viraram ícones.
- Telas PLAY/SOUND/RACK: palco **emoldurado no centro** (450–1066 px), coluna esquerda e direita, faixa de baixo com **XY PERFORMANCE / MACROS / GLOBAL** e o teclado. Painéis creme com moldura escura e sombra dura (skin ilustrada); waveform, pad e gráficos ficam em poços escuros.
- EFFECT RACK em cartões na cor de cada módulo, com liga/desliga, ícone, nome + subtítulo, **AMOUNT** (= MIX real do módulo) e alça de arrastar.
- GLOBAL: transporte interno (▶ ❚❚ ■, só prévia) + VOLUME + medidor de saída.
- Diferenças honestas em relação à imagem: MACROS continuam as reais (PRESENÇA, RASTRO, VARIAÇÃO, MIX); BRILHO/MOVIMENTO/TEXTURA/ESPAÇO não existem no motor. "MIDI AUDITION" não foi criado (não existe função de prévia de nota). PATCH LAB e LOCKER ainda no estilo escuro.

## Interface v2 (23/10/2026, 22h18–22h30) — telas de referência do Gab

- As 4 imagens do Gab (PLAY, SOUND, LOCKER, PATCH LAB) são a direção da interface, com tamanhos de painéis, knobs e botões medidos nelas (imagem 1586 px → app 1440 px, ×0,908). Logo e fonte continuam os aprovados (logo.png + Poppins); a Nova é a arte do Gab.
- **Tema escuro é o padrão** ("mais chic"); o tema creme continua disponível (ícone sol/lua no topo e no menu). Só visual, salvo no look (`look.dark`).
- Envelopes e filtro com eixo, grade, preenchimento em degradê e alças com brilho; pad XY com grade de pontos e mira laranja.
- SOUND: faixa SAMPLE (modos, LOOP de região, GATE, XFADE), AMP ENVELOPE, FILTER (CUTOFF, RESONANCE), FILTER ENVELOPE (o motor usa a forma do AMP ENVELOPE × AMOUNT, destino CUTOFF/PITCH), LFO (RATE, DEPTH, FREE/SYNC, divisão, destino), PITCH (REPITCH/FIXED, TUNE, BEND), VOICE (POLY/MONO/LEGATO, GLIDE, disparo), GLOBAL (VOLUME, ORIG/RACK, VEL→FILT).
- Não existem no motor e por isso não aparecem: DRIVE, formas de onda e FADE IN do LFO, ADSR próprio do filtro, PAN, FINE, MIDI AUDITION, macros BRILHO/MOVIMENTO/TEXTURA/ESPAÇO, descrição em texto na busca de patches. Candidatos para a etapa de som.
- LOCKER: Nova grande à esquerda, WARDROBE (categorias + grade; "Looks" inclui a Original), PREVIEW OUTFIT IN (cenários reais), CURRENT LOOK, SAVE/RANDOM LOOK.
- PATCH LAB: FONTE DO SOM (waveform, biblioteca na pastinha, CAPTURA, TUNE), banner da Nova, PATCHES SUGERIDOS (A/B), BUSCA DE PATCHES (direção, GERAR, VARIAR, progresso, salvar), FAMÍLIA DE PATCHES.
- (22h40) Cenários: a câmera agora mantém os pés da Nova sempre no chão e dentro da moldura, em todos os 6 quartos. Looks desenhados em código foram retirados (não combinavam com a arte). **HAIR** usa os cabelos da referência do Locker do Gab: mesmo rosto, outro cabelo, trocando a cabeça inteira na marionete (`ui/nv_hair_*.png`, miniaturas `ui/hth_*.jpg`). São recortes provisórios em baixa resolução; a arte final deve vir em boa resolução. Starlight e Galaxy aparecem bloqueados (sem arte da cabeça ainda). Demais slots: EM BREVE.

## Nova modular (23/10/2026, 23h) — kit RASTRO_NOVA_MODULAR_KIT do Gab

- Personagem padrão = **Nova modular** (`ui/novakit.js` + `ui/novakit_data.js` + `ui/nk_*.png`). Base articulada no espaço da prancha de pivôs (1024×1460): pescoço, ombros, cotovelos, punhos, pélvis, quadris, tornozelos. Cada peça foi recortada das pranchas do kit e encaixada uma vez nesse espaço; em tempo real só segue o seu osso. Script de recorte: `/tmp/nk/build.py` (não versionado; refazer a partir do kit se preciso).
- Slots independentes: cabelo (6), cabeça (gorro, boné, fedora, chifres — um por vez), óculos (3), fones (2), brincos (3), parte de cima (5), parte de baixo (5, inclui saia), sapatos (4), instrumentos (keytar, guitarra, baixo, pad). Trocar um item muda só o slot. Instrumento na alça, duas mãos por IK (cotovelos sempre para fora).
- Movimento: frase de 8 tempos com as poses do kit (antecipação, passos, acento, groove baixo, recuperação), interpolada; pés plantados, levantam só ao mudar de apoio; follow-through do cabelo, chapéu e brinco.
- Luzes dos FX: cor de cada módulo (GHOST violeta, CRUMBLE âmbar, BLOOM rosa, WOBBLE ciano, DUST dourado, CRUSH lavanda) com força = atividade real; bypass/mix 0 apaga; várias cores misturadas com limite para manter rosto e roupa legíveis.
- XP só libera itens visuais. Look salvo em `look.kit`; a Nova "Original" (desenho fixo) segue em LOOKS.
- Limites honestos: peças recortadas de pranchas geradas (bordas um pouco suaves, alguns recortes imperfeitos); pele de braços/pernas desenhada em código onde a roupa não cobre; poses grandes limitadas pela vista frontal. Starlight/Galaxy (cabelos da tela antiga) não estão no kit.
