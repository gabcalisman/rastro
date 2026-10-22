# RASTRO PLAY · by Gab Calisman

Instrumento MIDI baseado em samples, C++17 / JUCE 8.0.6 / CMake. Um sample, até 16 vozes e seis efeitos reordenáveis. Interface PT-BR/EN, paletas coordenadas, materiais de hardware, waveform estéreo e personagem vetorial reativo. O binário e os identificadores continuam **RASTRO** para preservar sessões.

Protótipo para testes no [PR #1 em rascunho](https://github.com/gabcalisman/rastro/pull/1), branch `prepare-mac-testing`. Sem merge ou release. **RASTRO Capture** é um gravador complementar, não RASTRO FX.

## Baixar e instalar no Mac — sem compilar

1. Entre no GitHub com acesso ao repositório privado. Abra o [PR #1](https://github.com/gabcalisman/rastro/pull/1), selecione o check **macOS build** do commit mais recente e abra seus detalhes. O resultado precisa estar verde. Em **Artifacts**, baixe **RASTRO-macOS-universal**. [Lista dos builds](https://github.com/gabcalisman/rastro/actions). Artefatos expiram em 14 dias.
2. Extraia o download e depois o ZIP interno `RASTRO-macOS-universal.zip`. Confira o commit em `BUILD-INFO.txt`. O pacote contém **AU**, **VST3**, **Standalone**, o gravador opcional e instruções. Os executáveis são universais **arm64 + x86_64**, macOS 12 ou posterior; o Logic pode exigir um macOS mais novo.
3. Feche o Logic. No Finder, use **Ir → Ir para a pasta** e digite `~/Library/Audio/Plug-Ins/Components/`. Crie a pasta se necessário. Copie o bundle **inteiro** `AU/RASTRO.component` para ela. Mova uma versão antiga para fora antes de copiar; evite duplicatas em `/Library/Audio/Plug-Ins/Components/`.
4. Opcional: copie `AU/RASTRO Capture.component` para a mesma pasta. Para outros hosts, os bundles VST3 vão em `~/Library/Audio/Plug-Ins/VST3/`. **Logic usa AU**. O app `Standalone/RASTRO.app` pode ficar em `~/Applications/`.
5. Abra o Logic → Ajustes → Gerenciador de Plug-ins, procure **Gab Calisman / RASTRO** e faça uma nova verificação se necessário. Crie uma faixa de **Instrumento de Software**; no slot Instrumento, escolha **AU Instruments → Gab Calisman → RASTRO → Stereo**.
6. Clique em **IMPORTAR**, carregue um áudio e teste o teclado na tela antes do controlador MIDI. Salve o projeto, feche/reabra, confira o áudio embutido e os parâmetros e teste um bounce.

O download é assinado **ad-hoc**, sem Developer ID/notarização. Se o macOS ou o Logic bloquear o bundle, confira origem, commit, arquitetura e a mensagem de validação. Não desative Gatekeeper nem force um AU que falha. Consulte [instruções que acompanham o ZIP](docs/INSTALL-MAC.txt).

Verificação opcional no Terminal após instalar:

```sh
auval -v aumu Rstr GbCa
# Gravador complementar, se instalado:
auval -v aufx Rcpt GbCa
```

Compilar e passar testes no runner não comprova execução no Logic, áudio físico, instalação/Gatekeeper, Retina ou execução nativa em Apple silicon. O CI executa os testes no runner **Intel**, embora compile as duas arquiteturas.

## Baixar e instalar no Windows

Abra o check **Windows build** verde do mesmo commit e baixe **RASTRO-Windows-x64** em Artifacts. Extraia os dois níveis de ZIP. Feche a DAW e copie as pastas inteiras de `VST3/` para `C:\Program Files\Common Files\VST3\` (pode pedir permissão de administrador). Reabra e faça o rescan. RASTRO é um instrumento MIDI; RASTRO Capture é um insert de áudio opcional. O Standalone roda como `Standalone/RASTRO.exe`, fora do ZIP, sem instalador de runtime adicional. Veja [passos completos](docs/INSTALL-WINDOWS.txt).

Windows x64, runtime MSVC estático, executáveis não assinados. **M4A ainda não é decodificado no Windows**: use WAV/AIFF/MP3. Não há AU para Windows. Os testes CI não usam DAW nem dispositivo físico.

## Tocar, editar e transformar

- **IMPORTAR** aceita WAV, AIFF/AIF, MP3 e M4A renderizado nas plataformas com decoder. Seletor e drag and drop usam o mesmo worker offline. O áudio é imutável durante o processamento; a sessão armazena o arquivo original, sem depender do caminho externo.
- Waveform estéreo com picos multirresolução, zoom **− / +**, **ALL**, **SEL**, roda/gesto horizontal e teclado `+`, `−`, `Home`, `F`. Zoom não muda o áudio. Marcadores têm áreas generosas e mostram posições em ms. **TRIM / LOOP** alterna a região editada.
- **Ouvir trecho** toca uma passagem do trim para a frente na velocidade original, através do rack e das configurações atuais de synth. Parar prévia libera somente essa voz.
- **PITCH MODE** é separado dos modos: **REPITCH** transpõe altura e velocidade pela distância à raiz; **FIXED PITCH** conserva altura/velocidade para qualquer nota. **Slice** sempre seleciona cortes por notas na velocidade original. A mudança vale para novas vozes; as que já soam preservam os limites e a velocidade capturados.
- One Shot, Loop, Reverse, Ping-Pong e Slice continuam disponíveis. Slice oferece 2–16 cortes editáveis, sem detecção automática de transientes. Com raiz 60, notas 60…67 selecionam oito cortes; notas fora do mapa são ignoradas.
- **ALEATÓRIO / RANDOM** cria variação limitada dos efeitos ativos; **Desfazer** volta exatamente à combinação anterior. Não altera sample, raiz, trims, pitch mode ou modo de reprodução.
- **PLAY X:** estável → WOBBLE. **PLAY Y:** limpo → degradação por CRUMBLE, DUST e CRUSH ativos. Centro preserva os valores ajustados. Bypass continua bypass; o pad explica destinos desligados. Mouse, automação e MIDI Learn usam os mesmos parâmetros. Duplo clique/tecla centraliza.

### Rack — seis algoritmos reais

| Módulo | DSP e sinal visual |
| --- | --- |
| GHOST | Eco filtrado; rastros. |
| CRUMBLE | Repetição de fragmentos; chão segmentado. |
| BLOOM | Cauda difusa; aura/expansão. |
| WOBBLE | Atraso modulado com variação de afinação/tempo; ondas. |
| DUST | Ruído/textura que segue a atividade da fonte; partículas. |
| CRUSH | Redução de resolução/taxa; fragmentos digitais. |

Cada card mostra descrição, estado real, controle principal e mix; seleção abre os controles adicionais. Arraste o título para reordenar: a ordem visual é a ordem do DSP. Não há módulos dedicados de chorus, flanger, drive ou tape além desses algoritmos. Materiais e bobinas são elementos visuais.

GHOST, CRUMBLE e WOBBLE oferecem **FREE / SYNC** no seletor de tempo. FREE usa ms ou Hz; SYNC oferece `1/4`, `1/8`, `1/8.` e `1/8T`, BPM 30–300. BPM do host é mostrado como **HOST**; sem BPM válido usa **MANUAL**, sem sobrescrever o parâmetro manual. Modulações sincronizadas seguem PPQ/transporte quando fornecidos; sem PPQ seguem BPM livremente. Delay pode deixar cauda após parar. BLOOM, DUST e CRUSH não mostram sync sem um parâmetro temporal pertinente.

### Synth, note-off e LOOP

Abra **SYNTH / PATCHES**. SYNTH oferece ADSR de amplitude, filtro low-pass ressonante de dois polos, envelope e LFO para cutoff ou pitch, Poly (16), Mono, Legato, glide, sustain CC64, pitch bend e mod wheel CC1. Mod wheel amplia o LFO. O filtro/modulações adicionais são ativados por **SYNTH**. Gate/Hold usa ADSR mesmo com esse switch desligado. Glide funciona em Mono/Legato.

| Disparo | Comportamento ao soltar |
| --- | --- |
| Legado + One Shot, LOOP desligado | Continua até o fim do trim. |
| Legado + Loop/Reverse/Ping-Pong/Slice | Fade de até ~30 ms; pode alcançar a borda antes. Loop legado não dá nova volta durante release. |
| One Shot explícito, LOOP desligado | Ignora note-off e termina o trecho no modo/direção selecionado. |
| One Shot explícito + LOOP ligado | Ignora note-off e repete até All Sound Off / parada do transporte. |
| Gate/Hold | Note-off inicia release ADSR; CC64 pode adiar até soltar o pedal. |

**LOOP** é independente do menu de modo: repete para a frente em One Shot/Loop, para trás em Reverse, reflete em Ping-Pong e fica dentro do corte tocado em Slice. Ative uma região própria ou use o trim. Crossfade ajustável reduz a emenda nos loops para a frente/trás; Ping-Pong usa reflexão contínua. Não existe sincronização da **duração do sample** ao BPM, time-stretch ou mudança silenciosa da afinação.

All Notes Off/All Sound Off, proteção de overflow e parada do host liberam as vozes com fade; caudas do rack podem continuar. One Shot explícito com ADSR usa decay/sustain, mas a voz ainda termina na borda se não houver LOOP. Mono retriggera, Legato retém continuidade quando compatível; notas anteriores ainda pressionadas podem retornar ao soltar a última.

### Gerar patches a partir da fonte

1. Importe/capture um sample e selecione seu trim. Abra **SYNTH / PATCHES → PATCHES**.
2. Escolha **Mais fiel / Transformado** e clique **Gerar patches**. O worker renderiza 48 candidatos com o próprio motor, analisando até 0,4 s a 16 kHz. Compara envelope e bandas espectrais; retorna até seis opções distintas com distância `Δ` (menor significa descritores mais próximos).
3. Clique numa opção para aplicar e ouvir uma nota MIDI curta. Edite no SYNTH. **A/B** compara parâmetros anteriores com o patch escolhido. **Variar patch / Desfazer variação** é separado do RANDOM do rack.
4. **Salvar patch** grava `.rastro` com áudio embutido e estado completo do instrumento, incluindo rack/aparência; **Abrir patch** restaura esse estado. O arquivo externo original não é necessário. Limite de arquivo: 64 MiB, mantendo os limites de importação/memória abaixo.

Busca aproximada, não IA, transcrição, identificação de raiz ou synth independente da fonte. O rack atual continua durante a audição; a comparação do worker usa o synth sem efeitos. A busca avalia o trim para a frente na raiz; outros modos de reprodução permanecem preservados na instância. Fonte só no canal direito também é reconhecida. Pausa/cancelamento preservam o patch atual; cancelamento não cria falso resultado. Trabalho limitado, com descanso proporcional ao tempo de cálculo (~50% de um núcleo, não um teto rígido), fora do callback. Se trocar a fonte, gere novamente.

## Capturar áudio diretamente

**Standalone:** abra CAPTURAR → DISPOSITIVO, selecione entrada/saída, conceda acesso ao microfone no macOS, confira canais/ganho/medidor e clique REC/STOP. Monitoramento começa **desligado**; quando ativado é atenuado em 20 dB. Prefira fones para evitar realimentação. Limite de 30 s ou 2.097.152 frames, o que vier primeiro. A captura é convertida em WAV PCM24 no worker e vira o sample editável/tocável por MIDI. Espere a waveform aparecer antes de salvar a sessão.

**Logic/host:** o instrumento preserva seu barramento de saída sem adicionar entrada incompatível. O componente opcional **RASTRO Capture** recebe áudio como insert, mantém o sinal de passagem e publica a tomada local ao parar. Insira-o em **Audio FX** numa faixa de áudio/aux com sinal; no PLAY abra CAPTURAR → atualizar lista → receber tomada. Não exige exportar para outro aplicativo. Esse fluxo foi validado com entrada sintética e arquivo compartilhado no Linux; **a ligação no Logic ainda precisa de teste real**.

Tomadas ficam em `~/Library/Application Support/RASTRO/Captures` no Mac, `%APPDATA%\RASTRO\Captures` no Windows ou `$XDG_CONFIG_HOME/RASTRO/Captures` (`~/.config` como fallback) no Linux. Não são apagadas automaticamente. O PLAY embute a tomada recebida/capturada ao salvar projeto/patch; o insert de captura guarda configurações, não uma cópia de todas as tomadas. Nenhum áudio sai da máquina.

## Aparência, hardware e personagem

**CORES / KNOBS / PERSONAGEM** abre três áreas com prévias reais/aplicadas, desfazer até 32 ações e voltar à aparência de abertura. Nove paletas coordenam fundo, superfícies, bordas, botões, knobs e texto: After Hours, Solar Studio, Mint Condition, Sunrise, Sunset Drive, Poolside, Electric Soul, Burnt Velvet e **Cosmic Arcade** (magenta/roxo/ciano, espaço discreto e cassete luminoso). As escolhas legadas continuam válidas.

Quatro famílias afetam os knobs reais: marfim canelado, metal escovado, borracha e plástico de console, com ponteiros legíveis. O deck gira quando há vozes/prévia reais, não apenas cauda de efeito. O medidor mostra pico em dBFS; não é VU RMS calibrado.

Personagem original vetorial com apresentações andrógina/masculina/feminina e peças livremente combináveis. Pés, braços, quadris, ombros e cabeça usam molas, antecipação, impacto e recuperação. Notas/velocity, áudio, BPM/PPQ, PLAY e atividade medida dos seis efeitos comandam dança/luzes/cenário. Variações de controles produzem acentos quando há atividade musical; repouso volta a uma pose estável. Não há dança aleatória contínua no silêncio.

Completo: 24 Hz; reduzido: 12 Hz e amplitude menor; desligado: 6 Hz só para leitura/estado. Interfaces cobertas/invisíveis suspendem o personagem/deck; só a aba PERSONAGEM anima sua prévia. Animação, Paths e desenho ficam na interface; áudio publica apenas telemetria atômica limitada. Aparência não entra no DSP.

Janela padrão 1180×840, compacto 1000×700, mínimo 980×700, redimensionável até 1680×1100. SYNTH usa rolagem para manter controles acessíveis. O render 2× é teste simulado de HiDPI, não prova de execução Retina.

**[Abrir capturas reais no GitHub](docs/previews/README.md)**. PNGs também acompanham o pacote Mac em `Previews/`. São imagens da interface JUCE no Linux; não mockups nem capturas do Logic. Links locais de workspace não são downloads no Codex do navegador.

## Importação offline, variantes e licenças

- WAV/AIFF PCM mono/estéreo e MP3 continuam disponíveis. MP3 usa minimp3 incorporado (CC0), sem instalar codecs ou FFmpeg.
- M4A renderizado: **AAC-LC mono 44,1/48 kHz, estéreo 48 kHz; ALAC mono 16 bit/44,1 kHz, mono 24 bit/48 kHz, estéreo 16 bit/44,1 kHz**. Seis fixtures sintéticos próprios cobrem essas variantes; não são exportações reais da Apple. Mac usa CoreAudio local via JUCE; Linux usa FFmpeg local separado. Windows ainda informa decoder indisponível.
- HE-AAC/xHE-AAC, DRM e áudio espacial/multicanal não têm suporte validado. QuickTime **Editável** `.mov/.qt` mostra instrução para exportar áudio renderizado `.m4a` (comprimido/AAC ou sem perdas/ALAC); renomear não converte.
- Mensagens PT/EN distinguem arquivo ilegível/corrompido, formato/codec incompatível, decoder ausente e limites. Até 32 MiB codificados e 120 s, PCM retido total limitado a 256 MiB por instância; importar repetidamente retém buffers antigos até fechar a instância para não liberar memória no callback.
- Novo DSP, gravador, análise e arte vetorial são originais. Nenhuma dependência proprietária/IA foi adicionada. JUCE mantém suas obrigações de licença; minimp3 revisão `ea99364f61c14656440e8d77e9c233ccf3124633` é CC0. FFmpeg não é linkado nem distribuído. [Detalhes de codecs/licenças](docs/AUDIO-IMPORT.md), [LICENSE](LICENSE), [third_party/minimp3/NOTICE.md](third_party/minimp3/NOTICE.md).

## Compilar no Mac — passo a passo

1. Instale **Xcode** pela App Store, abra uma vez e aceite a licença/instalações adicionais. No Terminal execute `xcode-select --install` se ainda faltar o Command Line Tools. Confira `xcodebuild -version` e `git --version`.
2. Instale **CMake 3.22+** pelo instalador de [cmake.org](https://cmake.org/download/) e habilite seu comando no PATH. Confira `cmake --version`. Homebrew é opcional (`brew install cmake`).
3. Clone o repositório privado com sua autenticação GitHub (GitHub Desktop também serve), escolha a branch do PR e abra a pasta:

```sh
git clone https://github.com/gabcalisman/rastro.git
cd rastro
git switch prepare-mac-testing
```

4. Configure e compile. O primeiro configure baixa JUCE 8.0.6 e requer internet; a execução do instrumento/importação/captura é offline. Use uma pasta de build nova ao mudar de gerador/arquitetura.

```sh
cmake -S . -B build-mac -G Xcode \
  -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=12.0 \
  -DCMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED=NO
cmake --build build-mac --config Release --parallel 2
ctest --test-dir build-mac -C Release --output-on-failure
```

São **nove suítes** no Mac por padrão (oito portáveis + importação CoreAudio). Para incluir a décima, de processador/editor, configure `-DRASTRO_INTEGRATION_TESTS=ON`, compile e execute numa sessão gráfica autenticada. O CI não inclui essa suíte GUI.

5. Os produtos esperados são:

```text
build-mac/Rastro_artefacts/Release/AU/RASTRO.component
build-mac/Rastro_artefacts/Release/VST3/RASTRO.vst3
build-mac/Rastro_artefacts/Release/Standalone/RASTRO.app
build-mac/RastroCapture_artefacts/Release/AU/RASTRO Capture.component
build-mac/RastroCapture_artefacts/Release/VST3/RASTRO Capture.vst3
```

6. Feche os hosts, crie as pastas de instalação e copie os bundles inteiros:

```sh
mkdir -p "$HOME/Library/Audio/Plug-Ins/Components" "$HOME/Library/Audio/Plug-Ins/VST3"
ditto "build-mac/Rastro_artefacts/Release/AU/RASTRO.component" \
  "$HOME/Library/Audio/Plug-Ins/Components/RASTRO.component"
ditto "build-mac/Rastro_artefacts/Release/VST3/RASTRO.vst3" \
  "$HOME/Library/Audio/Plug-Ins/VST3/RASTRO.vst3"
codesign --force --sign - --timestamp=none "$HOME/Library/Audio/Plug-Ins/Components/RASTRO.component"
codesign --verify --deep --strict "$HOME/Library/Audio/Plug-Ins/Components/RASTRO.component"
auval -v aumu Rstr GbCa
```

Para usar o gravador, copie/assine também seu AU/VST3 dos caminhos acima. Abra o app Standalone pelo Finder para configurar entrada/MIDI, ou siga os passos de Logic no início deste README. Se o AU não aparecer, confira caminho, chip/arquitetura, mensagens do `auval`, saia/entre na sessão do Mac e faça rescan somente do plugin; não apague todos os caches como primeiro passo.

## Builds e testes de desenvolvimento

Portáveis (sem JUCE/GUI, inclusive Windows):

```sh
cmake -S . -B build-core -DRASTRO_CORE_ONLY=ON
cmake --build build-core --config Release --parallel 2
ctest --test-dir build-core -C Release --output-on-failure
```

Linux com dependências de desenvolvimento JUCE/X11/ALSA instaladas:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DRASTRO_INTEGRATION_TESTS=ON
cmake --build build --parallel 2
xvfb-run -a ctest --test-dir build --output-on-failure
pluginval --strictness-level 5 --random-seed 42 \
  --validate build/Rastro_artefacts/Release/VST3/RASTRO.vst3
pluginval --strictness-level 5 --random-seed 42 \
  --validate "build/RastroCapture_artefacts/Release/VST3/RASTRO Capture.vst3"
```

[Validação, cobertura e medições desta revisão](docs/UI-VALIDATION.md). Os workflows verificam binários, testes e ZIP/checksum antes de fazer upload; `BUILD-INFO.txt` identifica source commit e checkout testado. Artefatos de um commit anterior não comprovam esta revisão. Mac: universal em Intel, nove testes; Windows: x64, oito testes. Resultados efetivos ficam no run correspondente, não são presumidos pelo conteúdo deste README.

Compatibilidade: IDs/ranges anteriores preservados; synth/LOOP/modos de tempo novos recebem defaults neutros/legados ao restaurar sessão sem esses campos, inclusive PLAY Y sem CRUMBLE nas sessões antigas. Os novos presets usam tempos FREE/SYNC explícitos. MIDI Learn aceita CC de qualquer canal; notificação de automação desses gestos ao host é diferida para interface/estado, com a limitação anterior de gravação de automação com editor fechado.

Faltam testes no Mac do usuário: instalação/segurança, `auval`, CoreAudio com exportações reais do Gravador, Finder/diálogo nativo, Logic (instrumento e Capture), recall/bounce/automação, MIDI/microfone/guitarra físicos, latência/feedback, Apple silicon, Retina e escuta musical. Multisample, motores independentes de sample e time-stretch continuam [futuros](docs/ROADMAP.md).
