# Validação desta revisão — RASTRO PLAY

## Ambiente e resultado local

Debian 13 **Linux x86_64**, GCC 14.2, CMake 3.31.6, JUCE 8.0.6, Release/Ninja.
Build: VST3 e Standalone PLAY + VST3 Capture, sem SDK Apple ou Windows local.
**CTest 11/11** passou. **pluginval 1.0.3 nível 5, seed 42** passou para os dois VST3.
`git diff --check` sem erros. Os resultados CI de Mac são separados e devem ser conferidos
no workflow do commit publicado; não decorrem desses resultados Linux. Windows não foi executado nesta rodada.

| Suíte | Evidência principal |
| --- | --- |
| dsp | Voicing, rack/ordem/bypass, limites/extremos e alocação no callback. |
| character | WOBBLE/DUST/CRUSH, estabilidade, XY/cantos/centro/independência e suavização. |
| playback | Cinco modos, One Shot após note-off até o trim, Slice, REPITCH/FIXED PITCH em quatro taxas. |
| mp3 | minimp3 offline, mono/estéreo e erros. |
| synth | Polifonia/limite, ADSR/LPF/env/LFO, Mono/Legato/glide, velocity/CC64/bend/CC1, dança contínua/repouso. |
| capture | Buffer limitado, posse exportador/gravador, canais/ganho, seis taxas, não finitos e nenhuma alocação no processamento. |
| timing | FREE independente de BPM, SYNC audível/quatro divisões/PPQ, LOOP em cinco modos com One Shot/Gate, reflexão Ping-Pong e redução da emenda. |
| patches | Renderização real/ranking/diversidade, fiel/transformado, pausa/cancelamento e fonte silenciosa. |
| processor | Integração MIDI/estado/migração/importação/GUI, captura sintética, arquivo compartilhado, patches e transporte/XY. |
| capture_bridge | IPC por arquivos entre workers, start/stop/ack, vazio/silêncio, prepare na mesma taxa, WAV PCM24/44,1 kHz, descoberta, erro real de escrita e desconexão. |
| recordings | Seis M4A AAC-LC/ALAC: PCM finito, duração/taxa, conteúdo, repetição determinística e truncamento rejeitado. |

Integração adicional: botões reais GRAVAR NO INSERT/PARAR no PLAY comandam o servidor por arquivo,
automaticamente selecionam a tomada e USAR GRAVAÇÃO retorna à waveform. Captura sintética estéreo sem monitoramento automático, monitor -20 dB,
waveform/prévia/MIDI e recall do WAV embutido; instrumento hosted permanece sem entrada. Save/load `.rastro`
assíncrono preserva sample exato; arquivo inválido é rejeitado. Painel PATCHES executa a busca real,
aplica valores, toca nota MIDI, compara A/B e desfaz variação exatamente. Teste SYNTH rola até o último
controle no compacto. Quatro knobs ADSR alteram APVTS, pontos/imagem do gráfico e energia
da renderização; sustain em %. UTF-8 de Apresentação e migração da auréola legada têm regressões explícitas. Note-off/CC com offsets reais de amostra são processados sob trap de alocação.

Transporte: One Shot com LOOP continua após note-off, mas para suavemente quando o host muda de playing
para stopped. BPM de host não sobrescreve manual; ausência usa manual e mostra o estado. Cinco posições
XY (cantos + centro quantizado 64/127) produzem valores e áudio equivalentes por mouse, automação e CC.

## Compatibilidade

IDs/ranges e ordem dos parâmetros anteriores mantidos. Os novos IDs foram acrescentados ao fim;
estados até v4 sem os novos campos usam Pilot/cenário legado. Novas instâncias usam Mooncat/quarto.
A aparência atual persiste na sessão; undo/redo é local à abertura do personalizador. avatarHalo foi acrescentado no fim;
estados antigos com acessório halo mantêm a auréola, novos estados começam sem ela. Engine/EffectRack/SynthVoice não foram alterados nesta revisão. DanceMotion ganhou estilos visuais
com testes de continuidade, taxa e repouso em silêncio. Migração exercita estado antigo em instância previamente editada:
synth/LOOP novos neutros, relógios legados e PLAY Y antigo sem CRUMBLE. Estado, cortes, codecs, idioma,
aparência, CC, ordem e histórico JAM mantidos. Aparências diferentes renderizam o mesmo áudio bit a bit.

Comparação auxiliar contra headers extraídos do HEAD anterior, sem adicionar cópia ao produto:
**630.450 frames estéreo** de rack bit a bit idênticos em 22,05/44,1/48/96 kHz, alterando tempo/controles
com relógios legados. Comparação anterior do Engine: **315.225 frames estéreo** idênticos com synth/LOOP
novos desligados. Não é promessa de que todas as combinações possíveis foram comparadas.

## Interface, capturas e custo

Testes em peer X11 usam eventos JUCE simulados: hit-test, foco/teclado/Return/Escape, abas e overlays,
alvos de pelo menos 24 pontos, limites/sobreposições, padrão/compacto/mínimo/máximo, desfazer/reverter,
acessórios/paletas, resposta PLAY, freeze/reduzido e áudio inalterado pela aparência. Contraste sRGB testa
texto principal >7:1 e secundário >4,5:1 nas superfícies principais; não é auditoria completa de leitores
de tela/hover/disabled. Há teste explícito de UTF-8 nas mensagens novas em português.

O auxiliar GUI foi corrigido para bombear mensagens com tempo limitado; não encerra mais o dispatcher na
primeira espera. A medição também identificou que um editor dentro de uma janela inicialmente oculta
podia deixar o timer parado. Agora, oculto, mantém só uma sondagem leve de visibilidade a 6 Hz; nenhum
passo de animação/layout nesse estado. Teste fecha/abre a janela pai e confirma suspensão/retomada.

Última medição isolada em janela JUCE/Xvfb (~2 s por fase, CPU do **processo** / parede, percentual de um
núcleo, inclui DSP/harness, sem placa de som):

| Fase | CPU observada | Layouts / repaints completos do sincronizador |
| --- | ---: | --- |
| Inicialização/repouso off | 8,6% | 4 / 2, durante configuração inicial |
| Performance | 22,2% | 0 / 1 (mudança inicial do modo de animação) |
| CORES | 7,0% | 0 / 0 |
| PERSONAGEM | 25,5% | 0 / 0 |
| CAPTURAR | 7,1% | 0 / 0 |

CORES: 49 ticks, **zero** atualizações de ambos os personagens e deck cobertos.
PERSONAGEM: 49 ticks, zero do personagem principal/deck, 49 da prévia.
CAPTURAR: 49 ticks, zero do personagem principal/prévia/deck. Janela 1100×760. Não significa zero desenho:
controles visíveis atualizam regiões quando necessário. Passagem curta/software render, com ruído;
não prevê CPU/latência em Mac/Windows/DAW. Uma medição antiga tinha 40,5% em performance; o escopo DSP
agora é maior, portanto não tratar os números como benchmark controlado entre versões.

[Capturas reais verificadas](previews/README.md): 67 PNGs atuais + duas referências do commit f852c34, padrão/compacto, Cosmic Arcade, paletas,
quatro materiais, três apresentações, oito quadros de dança, ADSR padrão/compacto, PATCHES e captura gravando/pronta/carregada/insert.
Personagem inclui três apresentações em fundo claro/escuro, 380×300 e 160×144. Sem mockups.
HiDPI é render 2× simulado, não Retina físico. Os PNGs foram abertos/inspecionados aqui, incluindo
acentos, área de knobs, rack e painéis. Não houve confirmação de exibição no chat do navegador.
Acesso: índice no GitHub privado ou `Previews/ABRIR-PREVIAS.html` no ZIP do build.

## Comandos neste workspace

```sh
source /workspace/work/build-env.sh
cmake --build /workspace/work/build -j2
DISPLAY=:99 XDG_CONFIG_HOME=/workspace/work/runtime/config XDG_CACHE_HOME=/workspace/work/runtime/cache \
  python /workspace/work/run-ui-benchmark.py /workspace/work/deps/usr/bin/ctest \
  --test-dir /workspace/work/build --output-on-failure
DISPLAY=:99 XDG_CONFIG_HOME=/workspace/work/runtime/config XDG_CACHE_HOME=/workspace/work/runtime/cache \
  python /workspace/work/run-ui-benchmark.py /workspace/work/pluginval/pluginval \
  --strictness-level 5 --random-seed 42 \
  --validate /workspace/work/build/Rastro_artefacts/Release/VST3/RASTRO.vst3
# Mesmo comando para RastroCapture_artefacts/Release/VST3/RASTRO Capture.vst3
DISPLAY=:99 python /workspace/work/run-ui-benchmark.py /workspace/work/build/rastro_ui_benchmark
python scripts/export_previews.py --build /workspace/work/build --before /workspace/work/round-f852 --before-commit f852c34 --out docs/previews
git diff --check
```

O helper X11 pertence ao ambiente, mantém atoms que o window manager normalmente fornece. Não é
requisito do produto. Em desktop Linux normal use `ctest`/`pluginval` diretamente.

## Captura: causa demonstrada e fronteira do host

Parar sem frames retornava false do encoder e gerava o genérico erro de salvar. Os testes reproduzem
isso, distinguem fluxo silencioso e falha real de disco. prepare repetido na mesma taxa não para a tomada.
A descoberta/controle funciona entre processos distintos via arquivos locais; scans, XML e WAV ocorrem
somente nos workers. PLAY hospedado preserva o barramento sem entrada; a captura depende do insert.
No Mac, gravações novas usam ~/Library/Application Support/RASTRO/Captures, e a pasta legada continua legível.
Não havia prints de teste do Logic anexados neste workspace; as duas referências artísticas foram vistas no chat. A análise parte do código e dos sintomas descritos.

## CI e testes ainda necessários

O workflow Mac configura runner Intel, universal arm64+x86_64, PLAY AU/VST3/Standalone + Capture AU/VST3;
dez suítes nativas incluindo decoder CoreAudio/IPC, pluginval 1.0.3 nível 5 seed 42 nos dois VST3,
etapa auval para ambos os AU, verificação lipo, assinatura ad-hoc e ZIP/checksum. Um artefato pequeno separado contém logs/BUILD-INFO.
Isso descreve os checks exigidos; o resultado real deve ser consultado no run do commit atual e no PR.
O workflow Windows foi mantido para execução manual futura e não é disparado por este PR.

**Nenhum teste no Logic Pro foi realizado.** Faltam auval na instalação do usuário, instalação/segurança, execução Apple silicon,
Finder/seletor nativos, exportações reais do Gravador, Retina, áudio/MIDI físicos, captura mic/guitarra,
roteamento Capture no Logic, escuta, feedback/latência, DAWs Windows, automação/recall/bounce reais.
Não há novos efeitos, multisample, time-stretch ou synth independente do sample.

## Cobertura nova: workshop visual e isolamento de patches

`tests/AppearanceIntegration.h` exercita os cinco escopos `.rastro-look`, aplicação somente dos
grupos declarados, preservação de sample/raiz/ADSR, rejeição transacional de parâmetros sonoros,
sound-only versus patch completo, áudio PCM embutido exato, locks/undo JAM, migração v4, novos
seletores/undo/redo/hit-testing, cenários, duas criaturas × três apresentações em tamanho pequeno/grande
e áudio bit a bit igual com todos os parâmetros visuais diferentes. O ADSR principal abre os controles
reais por Enter. Os testes antigos de imports, captura, synth/MIDI, timing e rack permanecem ativos.

`tests/SynthTests.cpp` também testa Soul/Disco/Half-time, intensidade zero e repouso de todos os estilos.
Os diagramas dos efeitos usam controles efetivos (incluindo o parâmetro FREE/SYNC em uso) e atividade
DSP medida; não são gráficos de amostras. Quarto estático em cache; animações cobertas permanecem suspensas.

## Limites de produto desta etapa

Os cenários são composições vetoriais iniciais com escolhas discretas de layout/cor/luz/decoração. Não há
editor livre de objetos nem ilustração com o nível de detalhe das referências. O material/coordenadas
vem de código original, sem exibir as imagens anexadas dentro do plugin. A avaliação estética do usuário
continua pendente: as imagens enviadas pela ferramenta não apareceram no navegador; os PNGs/HTML serão
acessíveis no GitHub e no pacote final. A busca de patches permanece limitada/descrita no README.
