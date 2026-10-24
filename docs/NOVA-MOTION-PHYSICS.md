# NOVA — especificação de movimento físico e balanço

Este documento transforma o pedido de “dança com balanço de videogame” em critérios de implementação. Use as referências e os seis frames individuais como guia de atuação. Eles são storyboard; não os trate como sprites perfeitamente alinhados nem como animação pronta.

## Meta visual

A NOVA deve parecer uma personagem cartunesca com peso, ritmo e continuidade: antecipa o passo, transfere o peso entre os pés, acentua o groove e recupera a pose. Cabelo, roupa, acessórios e instrumento acompanham o movimento com atraso e amortecimento. Evite limitar a dança a pulo, bounce vertical ou oscilação no mesmo lugar.

## Rig e contatos

- Preserve a identidade oficial da NOVA: alien turquesa, cabelo roxo-escuro, olhos amarelos, roupa roxo/laranja/creme; exatamente dois braços e duas mãos.
- Reutilize o renderer/rig existente do PR #5 e adapte-o à NOVA. Não troque de personagem nem reescreva a interface inteira para alcançar o movimento.
- Use pivôs coerentes para root/pelve, tronco, pescoço/cabeça, ombros, cotovelos, pulsos, quadris, joelhos e tornozelos.
- Durante passos, fixe o pé de apoio ao chão e mova o corpo por transferência de peso. Evite que os pés deslizem.
- Mantenha as mãos presas a alvos coerentes no instrumento: uma toca as teclas/cordas e outra segura o braço ou corpo. Alça, keytar e instrumento acompanham o torso sem atravessar mãos ou corpo.

## Ciclo de dança

Implemente uma máquina de estados leve com transições suaves:
1. antecipação: joelhos flexionam e o peso se prepara;
2. passo: pé livre se move e aterrissa;
3. acento: quadril/ombros/tronco fazem um gesto curto e legível;
4. follow-through: cabelo, roupa, acessórios e instrumento continuam o movimento;
5. recuperação: o peso assenta e prepara o próximo ciclo.

Use os seis frames individuais como poses de referência. Interpole o rig e mantenha a cadência mesmo quando chegam acordes ou rajadas MIDI; não dispare um ciclo completo independente por nota.

## Balanço secundário

Use molas amortecidas determinísticas para cabelo, pontas da roupa, acessórios flexíveis, alça/instrumento e cabeça. Atualize usando delta time limitado e parâmetros de rig estáveis. A mola deve reagir a aceleração/rotação do corpo e voltar ao repouso sem tremor, overshoot excessivo ou oscilação infinita. Evite uma simulação física geral ou buscas pesadas.

## Relação com música e controles

- BPM do host guia a fase do groove quando disponível; em ausência de BPM válido, use tempo livre suave.
- Notas MIDI iniciam acentos; velocity controla intensidade. Agrupe rajadas e suavize a energia para evitar movimentos nervosos.
- Use energia/amplitude do áudio com envelope suavizado para dar energia geral; em silêncio a personagem repousa.
- PLAY XY e macros podem influenciar apenas os gestos/elementos visuais correspondentes ao seu mapeamento real.
- Reações de FX vêm dos valores efetivos e do bypass: GHOST violeta, CRUMBLE âmbar, BLOOM rosa, WOBBLE ciano, DUST dourado, CRUSH lavanda. Mais mix/intensidade dá sinal visual mais forte; bypass ou mix zero apaga o sinal. FX simultâneos misturam-se com moderação.
- Luzes são visuais, não alteram áudio, parâmetros nem estado do rack.

## Transporte, desempenho e acessibilidade

- Pause congela som e pose no ponto atual para continuar dali.
- Stop encerra o som e a animação, voltando ao início do trecho conforme o transporte existente.
- Faça a animação na thread da interface; não aloque, bloqueie nem execute física pesada na thread de áudio.
- Pause a animação se a interface estiver invisível. Respeite movimento reduzido e desligado.
- Use atualização limitada e estável; o plugin deve continuar leve em 30/60 fps.

## Testes de aceitação

- Verificar idle/silêncio, notas isoladas, acordes, rajadas, velocities diferentes e BPM variável.
- Conferir contato dos pés, transferência de peso, continuidade do keytar e exatamente duas mãos em todas as poses.
- Conferir recuperação sem tremor e resposta do cabelo/roupa com movimento reduzido/desligado.
- Conferir pausa congelando a pose, stop retornando ao início e ausência de animação no silêncio.
- Testar cada cor de FX isoladamente, bypass/mix zero e misturas simultâneas.
- Confirmar que mudar visual, dança ou luzes não altera o áudio renderizado, parâmetros, IDs, estado de sessão nem automações.
- Registrar o que foi implementado e testado. Não declarar animação final se houver apenas um concept, poses estáticas, placeholders ou uma oscilação de idle.
