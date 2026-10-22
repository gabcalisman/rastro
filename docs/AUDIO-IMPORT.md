# Importação offline — Gravador / Voice Memos

Esta revisão implementa M4A renderizado, preservando WAV/AIFF e MP3. Não depende de uma gravação
específica do usuário. Os fixtures próprios verificam codecs relevantes às exportações comprimidas
(AAC-LC) e sem perdas (ALAC); não são arquivos capturados pelo aplicativo Apple.

## Como usar

1. No Gravador do iPhone/Mac, compartilhe ou exporte a gravação para uma pasta local. Escolha
   **Comprimido/AAC** ou **Sem perdas/ALAC**, quando houver essas opções. Os nomes variam entre versões.
2. No RASTRO, use **CARREGAR SOM** e selecione o `.m4a`, ou arraste **um arquivo** para o plugin.
3. Confira o nome e a waveform. **Ouvir trecho** faz uma prévia; o teclado MIDI toca o sample normalmente.
4. Salve a sessão da DAW. Os bytes originais são embutidos no estado: não depende do caminho externo.

**QuickTime “Editável” não é suportado nesta etapa.** `.mov`/`.qt` são oferecidos no seletor somente
para mostrar a orientação, e seu drop exibe a mesma mensagem. Exporte novamente como áudio renderizado
`.m4a`; não basta trocar a extensão. Se necessário, abra no QuickTime Player e use
**Arquivo → Exportar Como → Somente Áudio**, ou renderize a mixagem no editor que consegue abrir a gravação.
Contêiner com marca QuickTime, vídeo ou mais de uma faixa de áudio também recebe essa orientação.
Um M4A deve representar uma única faixa de áudio renderizada, não uma sessão editável.

## Variantes efetivamente testadas

**Linux x86_64, FFmpeg 7.1.5, offline:**

| Contêiner | Codec | Canais | Frequência | Resolução |
| --- | --- | --- | --- | --- |
| M4A | AAC-LC | mono | 44,1 e 48 kHz | lossy; fonte de teste 16 bit |
| M4A | AAC-LC | estéreo | 48 kHz | lossy; fonte de teste 16 bit |
| M4A | Apple Lossless / ALAC | mono | 44,1 kHz | 16 bit |
| M4A | Apple Lossless / ALAC | mono | 48 kHz | 24 bit |
| M4A | Apple Lossless / ALAC | estéreo | 44,1 kHz | 16 bit |
| WAV / AIFF | PCM | mono e estéreo | 48 kHz | 16 bit |
| MP3 | MPEG Layer III | mono / estéreo | 44,1 / 48 kHz | 96 / 128 kb/s |

Os seis M4A têm codec/perfil/canais/taxa/resolução conferidos com ffprobe. Os testes do instrumento
verificam carga por arquivo, worker usado pelo seletor, drop assíncrono real na classe do editor,
PCM e waveform finitos, prévia/parada, MIDI, trim/modo/nome e restauração exata de PCM pelo mesmo backend.
Verificam arquivo truncado, átomo inválido, codec desconhecido, QuickTime inclusive renomeado e preservação
do sample anterior após erro. **O diálogo nativo do seletor e o arrasto físico do Finder não foram operados.**
WAV/AIFF são gerados em memória na suíte; MP3/M4A são fixtures pequenos versionáveis.

**macOS: implementado com CoreAudio do sistema; o resultado da compilação está no workflow do commit atual. A decodificação não é executada pelos quatro testes portáveis do CI.**
Não afirmar sucesso de AAC/ALAC no Mac ou no Logic a partir dos testes Linux. Rode a suíte de integração
no Mac com os mesmos fixtures. Não foram testados HE-AAC, HE-AACv2, xHE-AAC, DRM, áudio espacial/surround,
outros sample rates/profundidades ou metadados particulares de todas as versões do Gravador.
A identificação `mp4a` não certifica todos os perfis AAC; o decoder ainda pode recusar um perfil.

## Implementação, dependências e licenças

- `Source/RecordingImport.cpp`: código original de inspeção limitada de átomos ISO-BMFF e adaptadores.
  Não incorpora decoder proprietário nem copia código de outros plugins.
- **Mac:** JUCE 8.0.6 já registra `CoreAudioFormat`; usa os codecs locais AAC/ALAC da Apple.
  Nenhum executável FFmpeg é necessário no Mac. As condições de licença existentes do JUCE
  (AGPLv3 ou licença comercial aplicável) continuam valendo; o uso do framework do sistema não muda isso.
  Esta alteração não concede uma licença comercial para JUCE ou para o projeto.
- **Linux:** invoca um **FFmpeg instalado separadamente**, por array de argumentos, sem shell e sem rede.
  Caminho padrão `/usr/bin/ffmpeg`, configurável no build com
  `-DRASTRO_FFMPEG_EXECUTABLE=/caminho/absoluto/ffmpeg`. Acesso a protocolos limitado a arquivos locais;
  referências externas MOV não são habilitadas. Se faltar o executável, uma mensagem explica a dependência.
  O pacote deste ambiente é FFmpeg 7.1.5 Debian, um build GPL; o FFmpeg upstream usa LGPL 2.1+ com
  partes/opções GPL. **Não há cópia, link, download ou distribuição de FFmpeg pelo plugin/CMake.**
  Se alguém futuramente o redistribuir, deverá cumprir a licença e as obrigações de fonte desse build:
  https://ffmpeg.org/legal.html. A instalação inicial da dependência pode exigir rede; a importação não.
- **MP3:** continua usando minimp3 CC0 já vendorizado. Sua licença e revisão estão em
  `third_party/minimp3/NOTICE.md`; o caminho MP3 não exige FFmpeg.
- Fixtures sintéticos originais CC0 e geração em `tests/fixtures/README.md` e `scripts/generate_recordings.py`.
- Windows: M4A não é implementado por este adaptador. WAV/AIFF/MP3 permanecem nos caminhos anteriores.

## Limites e threads

Até **32 MB codificados, 120 s/16 milhões de frames**, orçamento retido **256 MB por instância**.
Falhas mantêm o sample/estado anteriores. Mensagens PT/EN distinguem leitura, formato, corrupção,
codec, QuickTime, limites e decoder ausente. Todos os imports pela UI passam pelo worker existente;
I/O, decoder, temporários e cálculo dos 512 picos ficam fora do callback de áudio. O callback continua
lendo fonte imutável por ponteiro atômico, sem novos locks/alocações.
A restauração de sessão decodifica na chamada não realtime do host, como o importador anterior.

No Linux o áudio é convertido para float estéreo a 48 kHz; mono é duplicado sem a atenuação automática
usual do downmix FFmpeg. No Mac, o adaptador preserva o sample rate nativo e duplica mono.
Não se promete PCM idêntico entre plataformas/decoders, nem compensação gapless de padding AAC.
Há temporários locais apagados ao terminar e timeout de 30 s no processo Linux; a thread de carga/fechamento
pode aguardar esse decoder, mas o processamento de áudio não o espera. Não são adicionados novos efeitos.

## Reproduzir os testes

```sh
# Requer FFmpeg/ffprobe somente para regenerar os exemplos; já estão no projeto.
python3 scripts/generate_recordings.py
cmake -S . -B build-check -DCMAKE_BUILD_TYPE=Release -DRASTRO_INTEGRATION_TESTS=ON
cmake --build build-check --config Release --parallel 2
ctest --test-dir build-check -C Release --output-on-failure
```

No Mac são necessários Xcode/Command Line Tools e CMake, conforme README. Na integração, execute em uma
sessão gráfica, pois a suíte também abre um peer de janela e renderiza a interface. No Linux sem desktop,
use Xvfb com window manager; este workspace tem um helper específico descrito em `UI-VALIDATION.md`.
