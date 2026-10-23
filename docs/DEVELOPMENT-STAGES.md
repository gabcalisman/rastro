# RASTRO PLAY — revisão por etapas

## Estado inicial registrado

Branch `prepare-mac-testing`; HEAD inicial `f852c3487845ed6107baafd04019ce07057481ee`.
`git status --short` vazio antes das alterações. PR #1 já aberto em rascunho. Sem alterações antigas
não relacionadas a incorporar. README, arquitetura, testes e workflows inspecionados.

Já existiam: instrumento de uma fonte/16 vozes, Poly/Mono/Legato, ADSR/LPF/env/LFO, CC64/CC1/bend,
cinco modos, LOOP independente, PITCH MODE, seis efeitos reordenáveis, XY/MIDI Learn, importação
WAV/AIFF/MP3/M4A, Capture complementar/Standalone e Patch Search aproximado. O último Mac CI passou
com 10 suítes/pluginval; isso não comprova a revisão em andamento nem uso no Logic.

Duas referências de interface foram disponibilizadas no chat: quarto ilustrado/figurino musical e
quarto neon/monstrinho. Foram vistas; nenhum pixel dessas imagens será usado como arte do produto.

## Ordem e fronteiras

1. Inspeção/registro: realizado acima.
2. Interface funcional: sampler/room/ADSR, cenário e personagem originais, personalização real,
   prévias da árvore JUCE compilada. Validar antes de considerar concluída.
3. Synth/MIDI: auditar o motor existente e testar regressões; não adicionar motores independentes.
4. Rack: seis algoritmos existentes, cores e sinais visuais medidos; sem módulos de DSP fictícios.
5. JAM/Patch Lab: manter worker real; separar carregar som/visual, salvar visuais e bloqueios do JAM.
6. Captura: preservar fluxo real com insert no Logic, sem anunciar entrada direta no instrumento.
7. Validar conjunto, commit/push no PR existente em rascunho; CI/artifact Mac apenas. Sem release/merge.

Este documento é um registro de trabalho, não uma declaração de aprovação de etapas pendentes.
Os resultados finais e limitações serão registrados em UI-VALIDATION.md e no PR do commit entregue.
