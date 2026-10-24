# RASTRO FX / capturas

## Interface web (macOS)

Renderizadas no Chromium a partir de `ui-fx/` com o **motor simulado** do `fxbridge.js`
(sem áudio real). Mostram o layout, os materiais e as interações; não são capturas do Logic.
No plugin, os mesmos arquivos rodam no WebView do macOS e leem o áudio real.

- [Tela principal: cadeia, palco, prateleira e modulação](fx-web-main.jpg)
- [Modo paralelo](fx-web-parallel.jpg)
- [Arrastando um efeito da prateleira para a cadeia](fx-web-drag.jpg)
- [Cadeia vazia: o cabo atravessa limpo](fx-web-empty.jpg)

## Editor nativo (fallback em Linux/Windows e nos testes)

Geradas pela suíte `fx_processor` no Linux/JUCE, com parâmetros e componentes reais.

- [Janela 1180×840, rack + pad](fx-editor-1180.png)
- [Módulos novos + LFO/follower](fx-editor-new-modules.png)
- [Panner editável + modulação avançada](fx-editor-swing.png)

Os testes também verificam 980×720 e 1440×1000. Os workflows nativos incluem as capturas do
próprio run em `Previews/FX` no ZIP.
