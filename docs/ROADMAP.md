# RASTRO PLAY — etapas do produto

## Etapa 1: instrumento visual (esta revisão local)

Sampler MIDI existente + rack existente + interface noturna + personalizador + Grooveiro vetorial.
Inclui PITCH MODE (REPITCH/FIXED PITCH) e M4A renderizado. Nenhum efeito novo ou multisample; instrumento continua sem entrada de áudio.

Arquitetura atual: `Engine` tem 16 vozes para um `Sample` imutável; `EffectRack` é separado do sampler;
`Processor` adapta MIDI/parâmetros/estado; `Editor`, `Performance`, `Hardware`, `Customizer` e `Visuals` desenham na thread da interface.
O personalizador usa parâmetros próprios, fora de `Settings` e dos algoritmos DSP. Suas peças não mudam o som.
Os ranges/IDs antigos, inclusive `controlStyle` (0–2), permanecem intactos; `knobDesign` (0–4) é adicional.
`knobDesign=0` usa o estilo legado; 1–4 escolhem Vintage/Amplifier/Synth/Console. Sessões sem os novos campos recebem defaults.

## Etapa 2: fontes e multisample — planejada, não implementada

- Duas fontes A/B com solo/mute, volume e pan independentes. Comparar A/B é diferente de Layer: Layer toca ambas.
- Uma fonte começa com um arquivo e pode evoluir para um instrumento com zonas de notas/velocity, afinação e trims por zona.
- Preservar importação WAV/AIFF/MP3/M4A offline e PITCH MODE independente, arrasto, marcadores e os cinco modos já existentes.
- Definir política de sobreposição de zonas, velocity, round-robin e roubo de voz antes de expandir o motor.

Ponto de integração futuro: o carregador publica um mapa imutável de zonas por fonte; note-on resolve a zona
fora de qualquer operação de carregamento, e a voz retém sua fonte e limites. Um mixer A/B recebe as vozes,
aplica volume/pan e alimenta o mesmo `EffectRack`. O estado deverá migrar o sample único antigo para a fonte A.
Definir orçamento de memória combinado e aposentadoria dos buffers fora da thread de áudio antes de implementar.
Não há A/B ou multisample escondido na interface desta revisão: os cartões continuam representando efeitos, não fontes.

## Time-stretch — possibilidade futura, não implementada

PITCH MODE atual separa REPITCH cromático de FIXED PITCH sem transposição, mantendo os modos de execução.
Pitch/time independentes exigem um novo mecanismo: avaliar qualidade de transientes e loops, latência,
polifonia, memória pré-alocada, sample rates, automação e licenças. Não adicionar um algoritmo pesado ou
conversão bloqueante ao callback. FIXED PITCH **não é time-stretch**. Definir testes de escuta/realtime antes
de escolher uma implementação. Mantê-lo separado de Slice e da futura escolha de zonas multisample.

## Etapa 3: rack — planejada, não implementada

Auditoria atual: GHOST = delay filtrado sincronizado; CRUMBLE = repetição de fragmentos;
BLOOM = cauda difusa; WOBBLE = atraso modulado; DUST = textura gerada seguindo a fonte;
CRUSH = redução de resolução/taxa. São seis módulos; a ordem visível já corresponde à ordem DSP.

- Estudar chorus/flanger e saturação dedicados sem duplicar ou renomear silenciosamente os atuais.
- Evoluir delay/reverb se os testes de escuta justificarem, preservando presets/sessões ou versionando os algoritmos.
- Matriz PLAY com atribuições configuráveis e visíveis; hoje X = profundidade WOBBLE, Y = DUST/CRUSH.
- Preservar automação, MIDI Learn, bypass/mix e reordenação; testar equivalência ordem UI/DSP.

## Etapa 4: IR e motores — possibilidades futuras

- Convolução por IR é um **efeito do rack**, não um motor de síntese. Importar/preparar IR fora do callback.
- Analógico virtual e FM seriam motores de fonte distintos; não recriações oficiais de marcas.
- A escolha do motor deverá viver na fonte A/B futura, mantendo o rack compartilhado.

## Fora desta rodada

Vídeo/memes/mobile, desenho livre e upload de imagens; contas/sincronização/XP e desbloqueios.
Áudio e importação continuarão offline, sem contas ou serviços de vídeo. Nenhum efeito essencial será bloqueado.
As peças atuais são vetoriais predefinidas, não um editor livre de posição/tamanho; novos modelos podem usar o
mesmo painel e IDs adicionais. Um futuro RASTRO FX poderá reutilizar o rack, mas não foi criado nesta etapa.

## Próximo passo recomendado

Avaliar esta interface e as combinações no Mac/Logic em tamanhos mínimo e padrão, com MIDI e áudio reais.
Revisar contraste, movimento reduzido, recuperação de sessão e consumo de CPU antes da Etapa 2.
Só depois especificar e testar a migração para mapas de zonas e A/B.
