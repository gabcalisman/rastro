# RASTRO FX — etapa 1

Insert de áudio independente do instrumento RASTRO PLAY e do gravador RASTRO Capture.
Esta etapa prepara somente o caminho de áudio, ganhos e bypass. Não contém sampler,
MIDI, gravação, link de captura ou rack de efeitos.

| Produto | Bundle ID | Código JUCE/AU | Tipo |
| --- | --- | --- | --- |
| PLAY (binário RASTRO, preservado) | `com.gabcalisman.rastro` | `Rstr` | instrumento |
| Capture (preservado) | `com.gabcalisman.rastro.capture` | `Rcpt` | insert gravador |
| FX | `com.gabcalisman.rastro.fx` | `Rfx1` | insert de áudio |

Fabricante: `GbCa`. Target CMake: `RastroFx`; nome do produto: `RASTRO FX`.
VST3 em Linux/Windows; VST3 e AU (`aufx Rfx1 GbCa`) no macOS. Sem Standalone nesta etapa.
Os workflows existentes também compilam os targets FX e executam sua suíte; os
pacotes históricos PLAY/Capture e seus validadores continuam com o escopo anterior,
sem incluir ou certificar o FX nesta etapa.
O FX tem seu próprio estado `RASTRO_FX_STATE`; não importa estados do PLAY ou Capture.

## Áudio e controles

- Entrada e saída correspondentes: mono → mono ou estéreo → estéreo, sem sidechain.
- `fxInputGain` / **Input Gain**: −24 a +24 dB, padrão 0 dB.
- `fxOutputGain` / **Output Gain**: −24 a +24 dB, padrão 0 dB.
- `fxBypass` / **Bypass**: padrão desligado; também exposto como bypass do host.
- Editor nativo genérico JUCE com os três parâmetros, automação e recall pelo host.
- Ganhos com rampa linear de 20 ms; bypass com transição linear de 10 ms entre
  o sinal com ganho e o sinal original. Mudanças durante uma rampa partem do valor atual.
  Os ganhos continuam atualizando durante bypass, mas não alteram o sinal seco final.
- Ganho unitário deixa as amostras intactas, inclusive silêncio, valores acima de
  0 dBFS e canais independentes. Não há clipping, limiter, dither ou processamento oculto.
  Float e double são suportados. Latência e cauda declaradas: zero.
- O callback não aloca buffers nem usa locks. Ganhos são combinados nesta etapa,
  pois não existe processamento entre entrada e saída. Ao adicionar efeitos, separar
  o ganho antes/depois do rack e manter um caminho seco para o bypass.

## Compilar e testar

JUCE 8.0.6 é obtido pelo CMake existente. São necessárias as dependências de
compilação JUCE da plataforma; no Linux, X11, ALSA, FreeType/fontconfig e OpenGL.
O editor web do PLAY continua opcional e não é necessário para o FX.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DRASTRO_INTEGRATION_TESTS=ON
cmake --build build --parallel 3
xvfb-run -a ctest --test-dir build --output-on-failure
```

Para compilar somente o novo plugin e seus testes:

```sh
cmake --build build --target RastroFx_VST3 rastro_fx_tests --parallel 3
ctest --test-dir build -R '^fx_processor$' --output-on-failure
```

O modo `RASTRO_CORE_ONLY=ON` continua limitado aos testes portáveis existentes;
para testar o processador FX real é necessário o build com JUCE.
Os testes FX verificam igualdade bit a bit em ganho unitário (mono/estéreo,
float/double, três taxas e vários tamanhos de bloco), ganhos em dB, rampas,
bypass do parâmetro e callback do host, reversão de transição, isolamento de
canais, layouts inválidos, latência/cauda e persistência/isolamento de estado.

Build Linux não comprova execução em DAW, AU/Logic, Windows ou macOS.
Para validação no Logic, compilar `RastroFx_AU`, instalar o bundle independente e
executar `auval -v aufx Rfx1 GbCa`, além de testar áudio, automação e recall na DAW.

## Validação no ambiente Linux

Base: `33a242deaca1dec1847cf795ab7787357df68006`, branch
`codex/rastro-fx-foundation`.

- Build Release completo com `RASTRO_INTEGRATION_TESTS=ON`: passou. Gerados
  PLAY Standalone/VST3, Capture VST3 e FX VST3, além dos executáveis de teste.
- Oito suítes portáveis, importação/gravação, captura/IPC e processador FX:
  **11 suítes passaram** na execução inicial.
- A suíte `processor` do PLAY inicialmente parou com erro X11 `BadAtom` no
  Xvfb sem gerenciador de janelas. Reexecutada com Xfwm4 no mesmo Xvfb:
  **passou**, sem alterações no código ou nos testes do PLAY. Total: **12 suítes
  aprovadas**, contando a reexecução da suíte gráfica.
- Metadados VST3 gerados confirmam classes de áudio distintas para PLAY, Capture
  e FX; os códigos existentes `Rstr` e `Rcpt` permanecem inalterados.
- Não executados: builds macOS/Windows, AU/auval, pluginval e testes em DAW/hardware.

Em Linux sem sessão gráfica, a suíte completa pode precisar de um gerenciador de
janelas além do Xvfb. A reexecução aprovada usou `dbus-run-session`, iniciou
`xfwm4 --compositor=off`, aguardou `_NET_SUPPORTING_WM_CHECK` via `xprop` e executou
`ctest --test-dir build -R '^processor$' --output-on-failure`. O teste FX não
precisa abrir uma janela.
