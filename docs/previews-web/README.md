# PR #5 — primeira passagem do editor web

Estas são capturas da interface HTML/JS real do PR #5 em **Chromium 154 no Debian 13 x86_64**.
O bridge de navegador existente usa **telemetria e sample simulados**. Não são capturas do
plugin/WebKit no Mac, teste de áudio, Logic Pro nem o redesign final aprovado da NOVA.
O desenho da personagem ainda não foi adaptado às referências oficiais; a captura mostra
o estado implementado, inclusive essa diferença. As imagens de conceito ficam em
[`../referencias/rastro-play/`](../referencias/rastro-play/).

- [Estúdio — 1440 × 900, janela inteira](pr5-studio-standard.png)
- [Estúdio — 1100 × 760, janela inteira](pr5-studio-compact.png)
- [Edição do sample — janela inteira](pr5-sample.png)
- [Edição do sample — compacto](pr5-sample-compact.png)
- [Locker — janela inteira](pr5-locker.png)
- [Locker — compacto](pr5-locker-compact.png)

No download Mac, abra `Previews/Web-PR5/ABRIR-PREVIAS.html` por duplo clique.
As capturas do editor JUCE anterior ficam separadas em `Previews/Historical-JUCE/`.
O aparecimento das imagens no Codex no navegador não foi confirmado; os PNGs acima
podem ser abertos no GitHub com acesso ao repositório privado.

Reproduzir a captura: instale Playwright no ambiente de desenvolvimento e execute
`python3 scripts/capture_web_previews.py --chromium /usr/bin/chromium`.
Não há dependência nova no plugin. O script confere inicialização, troca real de telas,
erros JavaScript, imagens quebradas, hit areas dos seis bypasses, separação entre
rack/cenário/teclado e enquadramento horizontal da personagem no Locker em dois tamanhos.
Não valida MIDI/áudio, desenho final ou encaixe de todos os controles.
`capture-report.json` registra o resultado obtido.
