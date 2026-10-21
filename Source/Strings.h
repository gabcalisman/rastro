#pragma once
#include <juce_core/juce_core.h>
namespace text
{
struct Entry
{
    const char *key;
    const char *pt;
    const char *en;
};
inline const Entry entries[]{
    {"oneShot", "Disparo único", "One Shot"},
    {"reverse", "Reverso", "Reverse"},
    {"pingPong", "Ping-Pong", "Ping-Pong"},
    {"slice", "Cortes MIDI", "MIDI Slices"},
    {"preview", "Ouvir trecho", "Preview"},
    {"stopPreview", "Parar prévia", "Stop preview"},
    {"waveHelp",
     "Arraste os triângulos laranja para recortar. Em Cortes MIDI, arraste as linhas verdes para "
     "editar os cortes. Notas MIDI são indicadas sob cada parte.",
     "Drag orange triangles to trim. In MIDI Slices mode, drag green lines to edit cuts. MIDI "
     "notes are shown below each slice."},
    {"sliceHelp",
     "Quantidade de cortes. Alterar aqui redistribui os cortes igualmente; arraste as linhas "
     "verdes para refinar. A nota raiz toca o primeiro corte.",
     "Slice count. Changing it here redistributes cuts equally; drag green lines to refine. The "
     "root note plays the first slice."},
    {"modeHelp",
     "Disparo/Reverso: uma passagem. Loop/Ping-Pong: repetem enquanto a nota está ativa. Cortes "
     "MIDI: raiz + índice do corte, sem transposição. One Shot ignora note-off; os outros modos "
     "liberam em até ~30 ms.",
     "One Shot/Reverse: one pass. Loop/Ping-Pong: repeat while the note is active. MIDI Slices: "
     "root + slice index, without transposition. One Shot ignores note-off; other modes release "
     "within ~30 ms."},
    {"dustHelp",
     "Do chiado aberto aos grãos e estalos filtrados. A textura acompanha o nível do sample; não "
     "gera ruído contínuo no silêncio.",
     "From airy hiss to filtered grain and crackle. Texture follows sample level; it does not "
     "generate continuous noise in silence."},
    {"wobbleHelp",
     "Atraso interpolado modulado por movimento lento e flutter. PLAY X multiplica a intensidade. "
     "Não muda a nota raiz.",
     "Interpolated delay modulated by slow motion and flutter. PLAY X scales depth. Does not "
     "change the root note."},
    {"crushHelp",
     "Combina quantização e retenção de amostras com suavização curta. PLAY Y multiplica a "
     "intensidade; o mix mantém o original disponível.",
     "Combines quantization and sample holding with short smoothing. PLAY Y scales intensity; mix "
     "keeps the original available."},
    {"mix", "Mix", "Mix"},
    {"wDepth", "Intensidade", "Depth"},
    {"wRate", "Velocidade", "Speed"},
    {"wMix", "Mix", "Mix"},
    {"dLevel", "Nível", "Level"},
    {"dCharacter", "Caráter", "Character"},
    {"dMix", "Mix", "Mix"},
    {"kAmount", "Intensidade", "Amount"},
    {"kMix", "Mix", "Mix"},
    {"wobble", "Afinação em movimento", "Moving pitch"},
    {"dust", "Textura que segue o som", "Sound-following texture"},
    {"crush", "Grão digital suavizado", "Smoothed digital grain"},
    {"playAxes", "X / movimento    ·    Y / transformação",
     "X / movement    ·    Y / transformation"},
    {"playHelp",
     "Arraste para tocar a textura. Duplo clique ou Home centraliza; setas ajustam. X: WOBBLE. Y: "
     "DUST + CRUSH. Os controles do rack continuam valendo.",
     "Drag to play texture. Double-click or Home centers; arrows adjust. X: WOBBLE. Y: DUST + "
     "CRUSH. Rack settings remain effective."},
    {"center", "Centralizar", "Center"},
    {"undoMutation", "Voltar variação", "Undo mutation"},
    {"animOff", "Animação: desligada", "Animation: off"},
    {"animReduced", "Animação: reduzida", "Animation: reduced"},
    {"animFull", "Animação: completa", "Animation: full"},
    {"animationHelp", "Reduz movimento e atualização visual; não altera o som.",
     "Reduces visual motion and refresh rate; does not alter sound."},
    {"alive", "VIVO", "ALIVE"},
    {"still", "Movimento desligado", "Motion off"},
    {"reduced", "Movimento reduzido", "Reduced motion"},
    {"reactive", "Seu som ganha corpo", "Your sound takes shape"},
    {"pt", "Português (BR)", "Português (BR)"},
    {"en", "English", "English"},
    {"load", "CARREGAR SOM", "LOAD SOUND"},
    {"empty", "Arraste WAV, AIFF ou MP3 aqui · ou use CARREGAR SOM",
     "Drop WAV, AIFF or MP3 here · or use LOAD SOUND"},
    {"root", "Nota raiz", "Root note"},
    {"start", "Início", "Start"},
    {"end", "Fim", "End"},
    {"loop", "Repetir", "Loop"},
    {"level", "Saída", "Output"},
    {"velocity", "Vel. → filtro", "Vel. → filter"},
    {"tempo", "Tempo manual", "Manual tempo"},
    {"presence", "PRESENÇA", "PRESENCE"},
    {"trail", "RASTRO", "TRAIL"},
    {"mutation", "MUTAÇÃO", "MUTATION"},
    {"drywet", "Original / rack", "Dry / rack"},
    {"gTime", "Divisão", "Division"},
    {"gFeedback", "Retorno", "Feedback"},
    {"gTone", "Tonalidade", "Tone"},
    {"gMix", "Mistura", "Mix"},
    {"cRate", "Ritmo", "Rate"},
    {"cVariation", "Fragmento", "Fragment"},
    {"cMix", "Mistura", "Mix"},
    {"bSize", "Espaço", "Size"},
    {"bTone", "Tonalidade", "Tone"},
    {"bDecay", "Cauda", "Decay"},
    {"bMix", "Mistura", "Mix"},
    {"mutate", "MUTAR", "MUTATE"},
    {"reset", "Restaurar efeitos", "Reset effects"},
    {"add", "+ Módulo", "+ Module"},
    {"about", "Sobre", "About"},
    {"learn", "Aprender CC", "Learn CC"},
    {"learning", "Envie um CC…", "Send a CC…"},
    {"on", "Ativo", "Active"},
    {"off", "Ignorado", "Bypassed"},
    {"remove", "Remover", "Remove"},
    {"ghost", "Repetições sincronizadas", "Tempo-synced repeats"},
    {"crumble", "Fragmentos rítmicos", "Rhythmic fragments"},
    {"bloom", "Cauda difusa · 4 linhas", "Diffuse tail · 4 delay lines"},
    {"rack", "03 / RACK · selecione ou arraste os títulos", "03 / RACK · select or drag headings"},
    {"perform", "02 / PLAY · TOQUE A TEXTURA", "02 / PLAY · PLAY THE TEXTURE"},
    {"sample", "01 / SUA MATÉRIA-PRIMA", "01 / YOUR RAW MATERIAL"},
    {"error",
     "Não foi possível abrir o áudio. Use WAV/AIFF/MP3 válido, até 32 MB e 120 s. Limite de áudio "
     "carregado por sessão: 256 MB; reabra a instância para liberar.",
     "Could not open audio. Use valid WAV/AIFF/MP3 up to 32 MB and 120 s. Session audio limit: 256 "
     "MB; "
     "reopen the instance to release it."},
    {"choose", "Escolha um som", "Choose a sound"},
    {"aboutBody",
     "RASTRO by Gab Calisman\nInstrumento de amostras · protótipo 0.1\n16 vozes · afinação "
     "cromática · rack modular\nGHOST: eco. CRUMBLE: repetidor de fragmentos. BLOOM: reverberação "
     "difusa simples. WOBBLE: fita modulada. DUST: textura ligada ao som. CRUSH: grão "
     "digital.\nPLAY: X move a fita; Y aumenta textura e grão. Duplo clique centraliza. Voltar "
     "variação desfaz MUTAR.\nArraste títulos para reordenar. Remova e adicione módulos pelo botão "
     "+. "
     "Clique Aprender CC e mova um controle MIDI.\nPRESENÇA: ganho suave. RASTRO: retorno/cauda. "
     "MUTAÇÃO: variação dos fragmentos e intensidade de MUTAR.",
     "RASTRO by Gab Calisman\nSample instrument · prototype 0.1\n16 voices · chromatic pitch · "
     "modular rack\nGHOST: echo. CRUMBLE: fragment repeater. BLOOM: simple diffuse reverb. WOBBLE: "
     "modulated tape. DUST: sound-following texture. CRUSH: digital grain.\nPLAY: X moves tape; Y "
     "increases texture and grain. Double-click centers. Undo mutation restores the previous "
     "variation.\nDrag "
     "headings to reorder. Remove and add modules using +. Click Learn CC, then move a MIDI "
     "control.\nPRESENCE: smooth gain. TRAIL: feedback/tail. MUTATION: fragment variation and "
     "MUTATE intensity."},
    {"help", "Ajuste; duplo clique restaura. Automação disponível no host.",
     "Adjust; double-click resets. Host automation available."},
    {"bypassHelp", "Compare o sinal com e sem este módulo.",
     "Compare the signal with and without this module."},
    {"mutateHelp", "Varia parâmetros com limites seguros; preserva amostra e volume.",
     "Varies parameters within safe limits; preserves sample and level."},
    {"range", "Fim menor que início: intervalo mínimo de uma amostra.",
     "End before start: minimum one-sample range."},
    {"removed", "Módulo removido · use + para adicionar", "Module removed · use + to add"},
    {"host", "Host", "Host"}};
inline juce::String get(const juce::String &key, int language)
{
    for (auto &e : entries)
        if (key == e.key)
            return juce::String::fromUTF8(language ? e.en : e.pt);
    return key;
}
} // namespace text
