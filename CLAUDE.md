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
