# NOVA — especificação de movimento físico e balanço

Esta especificação orienta o trabalho em andamento no PR #2. Use o pacote visual da NOVA e as seis poses individuais como direção de atuação. As imagens são storyboard; não são sprites perfeitamente alinhados nem animação pronta.

## Meta visual

A NOVA deve parecer uma personagem cartunesca com peso, ritmo e continuidade: antecipa o passo, transfere o peso entre os pés, acentua o groove e recupera a pose. Cabelo, roupa, acessórios e instrumento acompanham o movimento com atraso e amortecimento. Evite limitar a dança a pulo, bounce vertical ou oscilação no mesmo lugar.

## Rig e contatos

- Preserve a identidade oficial da NOVA: alien turquesa, cabelo roxo-escuro, olhos amarelos, roupa roxo/laranja/creme; exatamente dois braços e duas mãos.
- Adapte o rig/renderer do PR #2; não troque de personagem nem reescreva a interface inteira para alcançar o movimento.
- Use pivôs coerentes para root/pelve, tronco, pescoço/cabeça, ombros, cotovelos, pulsos, quadris, joelhos e tornozelos.
- Durante passos, fixe o pé de apoio ao chão e mova o corpo por transferência de peso. Evite que os pés deslizem.
- Mantenha as mãos presas a alvos coerentes no instrumento: uma toca teclas/cordas e outra segura o braço/corpo. Alça e instrumento acompanham o torso sem atravessar mãos ou corpo.

## Ciclo de dança

Implemente transições suaves entre:
1. antecipação: joelhos flexionam e o peso se prepara;
2. passo: pé livre se move e aterrissa;
3. acento: quadril, ombros e tronco fazem um gesto curto e legível;
4. follow-through: cabelo, roupa, acessórios e instrumento continuam o movimento;
5. recuperação: o peso assenta e prepara o próximo ciclo.

Use os seis frames como poses de referência. Interpole o rig; não alterne seis imagens estáticas nem dispare um ciclo completo independente por nota.

## Balanço secundário

Use molas amortecidas determinísticas para cabelo, pontas da roupa, acessórios flexíveis, alça/instrumento e cabeça. Atualize por delta time limitado. As molas reagem à aceleração/rotação do corpo e voltam ao repouso sem tremor, overshoot excessivo ou oscilação infinita. Evite simulação pesada.

## Relação com música e efeitos

- BPM do host guia a fase do groove quando disponível; sem BPM válido, use tempo livre suave.
- Notas MIDI iniciam acentos; velocity controla intensidade. Agrupe rajadas e suavize a energia para evitar movimentos nervosos.
- Use energia/amplitude do áudio com envelope suavizado. Em silêncio, a NOVA repousa.
- PLAY XY e macros influenciam apenas gestos/elementos visuais correspondentes ao mapeamento real.
- Reações dos FX usam valores efetivos e bypass: GHOST violeta, CRUMBLE âmbar, BLOOM rosa, WOBBLE ciano, DUST dourado, CRUSH lavanda. Mais mix/intensidade dá sinal visual mais forte; bypass ou mix zero apaga o sinal. Misture cores com moderação.
- Luzes são visuais: não alteram áudio, parâmetros nem estado do rack.

## Transporte e desempenho

- Pause congela som e pose no ponto atual para continuar dali.
- Stop encerra som e animação, voltando ao início do trecho conforme o transporte existente.
- Faça a animação na thread da interface; não aloque, bloqueie nem execute física pesada na thread de áudio.
- Pause a animação se a interface estiver invisível. Respeite movimento reduzido/desligado.
- Use atualização limitada e estável; mantenha o plugin leve.

## Testes de aceitação

- Verificar idle/silêncio, notas isoladas, acordes, rajadas, velocities diferentes e BPM variável.
- Conferir contato dos pés, transferência de peso, continuidade do instrumento e exatamente duas mãos.
- Conferir recuperação sem tremor e resposta de cabelo/roupa com movimento reduzido/desligado.
- Conferir Pause congelando a pose, Stop retornando ao início e ausência de dança no silêncio.
- Testar cada cor de FX isoladamente, bypass/mix zero e misturas simultâneas.
- Confirmar que visual/animação/luzes não alteram áudio renderizado, parâmetros, IDs, sessão ou automações.
- Relatar exatamente o que foi implementado/testado. Não declarar animação final se houver apenas concept, poses estáticas, placeholders ou idle bob.
