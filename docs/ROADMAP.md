# RASTRO PLAY — arquitetura e próximas etapas

Esta revisão implementa as seis etapas do instrumento sample-based: waveform/rack/XY, personagem e
customização, captura, synth, LOOP/relógios e busca aproximada de patches. RASTRO Capture é somente um
gravador complementar. Não há RASTRO FX, multisample ou motores independentes da fonte.

## Fronteiras atuais

- `EngineTypes.h`: sample estéreo imutável, picos multirresolução e Settings sem parâmetros visuais.
- `Engine.h` / `SynthVoice.h`: 16 vozes, reprodução, pitch, ADSR/LPF/LFO, MIDI expressivo e regiões.
- `EffectRack.h` / `CharacterEffects.h`: seis módulos, ordem real, bypass/mix suavizados e relógios.
- `Processor.*`: APVTS, MIDI com offsets, host BPM/PPQ, migração, loader e estado com áudio embutido.
- `CaptureRecorder.h`: buffer limitado/pré-alocado, sem locks/allocs no callback. `CaptureBridge.h`:
  codificação WAV/arquivo compartilhado no worker. `CaptureProcessor.cpp`: insert de passagem gravador.
- `PatchSearch.h`: worker limitado que renderiza o motor real e compara descritores; mutex apenas entre
  worker/interface. `PatchPanel.*`: aplicar/audição/A-B/variação/desfazer e arquivo completo `.rastro`.
- `Editor`, `SampleView`, `Performance`, `DanceMotion`, `Hardware`, `Customizer`, `InstrumentPanel`:
  interface/desenho; telemetria atômica do áudio, animação suspensa quando coberta/invisível.

Defaults de migração preservam o caminho antigo: synth/região LOOP novos desligados; relógios legados;
PLAY Y sem CRUMBLE. IDs/ranges não são redefinidos. Novos controles têm IDs adicionais.

## Antes de considerar uma versão estável

Validar instalação/auval/Logic, captura em instrumento + insert, exportações reais do Gravador,
Apple silicon, áudio/MIDI físicos, Retina, recall/bounce/automação, escuta/latência e sessões longas.
No Windows falta decoder M4A; o build declara a limitação, sem fingir importar AAC/ALAC.
O patch search inicial avalia somente 0,4 s/16 kHz, sem estimar raiz e sem gerar uma fonte independente.
Os números de CPU em Xvfb não substituem medições em DAW.

## Multisample e duas fontes — futuro

Mapa imutável de zonas por nota/velocity, política explícita de sobreposição/roubo/round-robin e orçamento
combinado de memória. Migrar o sample antigo para fonte A, sem mudar as sessões anteriores. A/B do painel
PATCHES é comparação de configurações, não duas fontes simultâneas nem Layer.

## Pitch/time independentes — futuro

FIXED PITCH preserva leitura original, não faz time-stretch. LOOP só repete. Sincronizar duração e altura
independentes exige outro mecanismo, testes de transientes/latência, memória e licenças antes de implementar.

## Rack e busca — possíveis evoluções

Escuta pode orientar algoritmos dedicados de chorus/flanger/saturação; eles não existem como módulos nesta
revisão. Qualquer mudança precisa preservar o DSP legado ou versioná-lo. Busca futura pode estudar métricas
mais perceptuais, fontes mais longas e raiz estimada, sempre com audição, orçamento e limites explícitos.
Analógico virtual, FM, convolução por IR e RASTRO FX permanecem projetos separados, não controles escondidos.
