# Rack, dança e controles materiais — revisão local em andamento

Estado inicial: checkout completo `/workspace/rastro`, branch `prepare-mac-testing`,
HEAD `4f36345f1a0b806483384bf258c525ba96d8420f`, status limpo. PR #1 em rascunho,
mesmo head. `CMAKE_HOME_DIRECTORY=/workspace/rastro`; fontes e build pertencem ao mesmo repositório.
Nenhuma cópia parcial substituiu o checkout; nenhum trabalho anterior foi revertido.

O código ativo tinha seis cards de 142 px em uma linha, sem viewport, timing na linha 61 e
locks JAM apenas no personalizador. Não havia grade de 186 px nesse commit. Não se atribui
à captura antiga uma origem que não foi verificada. A causa estrutural nesta versão era
altura/coordenadas fixas sem espaço para rodapé próprio nem rolagem. A dança só tinha gatilhos
reais e dependia de áudio/MIDI: os seletores não ofereciam demonstração no silêncio.

## Implementação validada no Linux

- Cards medidos em 178 px, timing e JAM no rodapé com margem. Seis cards inteiros em
  1100×840 e 1000×760. Rack com viewport horizontal/vertical nas dimensões menores;
  inspector e teclado fora da área rolável. Mínimo 980×700, máximo 1680×1100.
- Locks existentes ligados aos novos botões; bypass/remoção/parâmetros/ordem sonora preservados.
  FREE/SYNC somente nos três DSP com timing. Os outros três não recebem timing fictício.
- DANÇA com PRÉVIA visual, pausa/retomada e parada. Nenhum note-on, MIDI ou áudio é gerado
  pela prévia, nenhum parâmetro sonoro muda. Off cancela a prévia. Reduzida remove gestos
  secundários, além de diminuir amplitude e frequência de atualização.
- Três coreografias com trajetórias distintas, rig de nove juntas, inércia, acentos,
  antecipação e recuperação. Braços, tronco e instrumento compartilham transformação;
  mãos e punhos usam os mesmos pontos. Keytar toca teclas e segura o braço; beat pads
  têm pontos separados de toque. Criaturas/itens existentes foram preservados.
- Transporte interno com pausa exata do motor/pose e rampa de saída/entrada de 64 samples.
  Stop limpa vozes/caudas em buffers preparados, sem alocar, bloqueios ou I/O, e volta ao trim.
  Não é controle remoto da DAW. O playhead não fornece uma distinção confiável pausa/stop;
  a transição host tocando → parado é tratada como stop. Não se declara teste no Logic.
- Oito designs de referência e duas famílias anteriores: dez escolhas reais de knobs, thumbnails e prévia interativa. `knobEdition` novo/append-only; ranges antigos intactos.
- Quatro criaturas de conceito com silhuetas próprias, somadas às três anteriores. Figurino separado em cabeça/camisa/jaqueta/calça/calçado/fones/patch/cordão/colar/brinco/óculos/instrumento; nenhum item opcional embutido na base. Encaixes e mãos usam coordenadas compartilhadas. Fedora/gorros são exclusivos; camisas/jaquetas e acessórios de categorias distintas podem combinar.
- Referências de turnaround, figurino e poses das quatro criaturas foram recebidas e vistas no Codex. Desenhos são interpretações vetoriais originais adequadas ao rig; não usam as pranchas como uma tela falsa/bitmap final. O detalhamento pictórico das referências não é idêntico ao render vetorial do plugin.
- Não existe hub funcional de XP no código ativo. Não foram criados XP ou desbloqueios falsos.

## Testes e evidência

Debian 13 Linux x86_64, GCC 14.2, CMake 3.31.6, JUCE 8.0.6, Release/Ninja, JUCE/Xvfb.
Build PLAY VST3/Standalone e Capture VST3 passou. CTest **11/11** passou.
pluginval 1.0.3 **nível 5, seed 42, ambos os VST3 passaram** (código de saída zero).

`RackDanceIntegration.h`: controles inteiros dentro dos cards, hit-testing real de todos os
locks, área inteira FREE/SYNC dentro do viewport após rolar, quatro dimensões de janela,
seis cards inteiros nos dois presets, capturas de janela inteira, prévia de três estilos sem
som/noteSerial, pausa exata de fase/pose, retomada e repouso, redução/off, MIDI real e playback,
pausa sem avançar sample e stop com retorno ao trim. Guard de alocação ativo no callback.
Escolha/aplicação/recall das dez famílias de knobs e isolamento de todos os parâmetros sonoros.
`ConceptIntegration.h`: sete criaturas × 12 slots com comparação real de pixels para equipar/remover,
28 misturas de coleções, hit-testing após rolar, bases sem itens/figurinos/performance/compacto,
recall de todos os slots e normalização antiga intacta. Os novos slots também passam pelos testes
existentes de presets visuais por escopo, rejeição de áudio e comparação de áudio bit-idêntico.
Testes existentes de importação, seis M4A AAC-LC/ALAC, MIDI, síntese, captura e estados preservados.
`SynthTests.cpp` verifica continuidade/repouso, diferenças de passos e remoção da articulação secundária.

Harness nativo ~2 s por fase; CPU do processo inteiro, incluindo DSP/teste, não apenas desenho:
performance Soul Fin com floral/franjas 25,9% de um núcleo; CORES 7,0%; preview Violet Curls com figurino 28,6%; captura 7,6%.
Zero layouts nessas quatro fases; zero personagem/deck atrás de CORES/captura.
Um repaint completo na mudança inicial da animação e um na troca de personagem/equipamento;
fase inicial de abertura 11,5%, quatro layouts e dois repaints completos de inicialização.
Animações limitadas a 24 Hz, 12 Hz reduzidas e paradas em Off; cenário estático em cache.
Isso não prevê CPU em Logic, Apple silicon ou hardware do usuário.

Capturas reais foram abertas e inspecionadas pela ferramenta no Linux: rack padrão/compacto,
DANÇA e novas famílias no personalizador/instrumento. Não se afirma que o cliente do usuário
exibe imagens de ferramentas. O exportador cria PNGs e HTML offline para entrega junto ao pacote.

## Publicação e validação externa

Todas as referências necessárias desta coleção foram recebidas. A publicação será feita na mesma branch/PR #1, mantendo rascunho, sem merge, release ou Windows. A validação Mac é separada destes testes Linux: consultar o workflow do novo commit, não reutilizar o artefato de 4f36345 como prova desta revisão.
Continuam exigindo Mac real: instalação/Gatekeeper, Retina nativo, áudio/MIDI físico, escuta/latência, comportamento do transporte/automação/recall em Logic e captura roteada em DAW.

## Comandos executados

Com `/workspace/work/build-env.sh` carregado e DISPLAY=:99/XDG_* apontando para o runtime local:

```sh
cmake --build /workspace/work/build -j2
python /workspace/work/run-ui-benchmark.py /workspace/work/deps/usr/bin/ctest --test-dir /workspace/work/build --output-on-failure
python /workspace/work/run-ui-benchmark.py /workspace/work/pluginval/pluginval --strictness-level 5 --random-seed 42 --validate /workspace/work/build/Rastro_artefacts/Release/VST3/RASTRO.vst3
python /workspace/work/run-ui-benchmark.py /workspace/work/pluginval/pluginval --strictness-level 5 --random-seed 42 --validate '/workspace/work/build/RastroCapture_artefacts/Release/VST3/RASTRO Capture.vst3'
python /workspace/work/run-ui-benchmark.py /workspace/work/build/rastro_ui_benchmark
python scripts/export_previews.py --build /workspace/work/build --before /workspace/work/rack-dance-baseline --before-commit 4f36345 --out docs/previews
git diff --check
```

Também foi executado clang-format --dry-run --Werror nos 22 arquivos C++/headers alterados.
O pacote offline local `/workspace/output/RASTRO-PLAY-previas.zip` tem 124 PNGs (122 atuais,
dois históricos), 127 entradas; SHA-256 de todos os PNGs e CRC de todo o ZIP conferidos.
É um arquivo local; não equivale a um novo link de artefato macOS.
