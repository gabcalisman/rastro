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
    {"pitchHelp",
     "REPITCH: notas MIDI mudam altura e velocidade em relação à raiz. FIXED PITCH: todas as notas "
     "tocam na altura e velocidade originais. Slice mantém a seleção de cortes por notas. Efeitos "
     "continuam aplicados; não há time-stretch.",
     "REPITCH: MIDI notes change pitch and speed relative to root. FIXED PITCH: all notes play at "
     "original pitch and speed. Slice keeps note-based slice selection. Effects remain applied; no "
     "time-stretch."},
    {"deckHelp",
     "Bobinas indicam vozes em reprodução, inclusive prévia; param antes das caudas do rack. "
     "Ponteiro: pico real de saída em dBFS, sem calibração VU. Sem efeito tape adicionado.",
     "Reels indicate playing voices, including audition; they stop before effect tails. Needle: "
     "actual output peak in dBFS, not a calibrated VU meter. No tape effect is added."},
    {"importRead",
     "Não foi possível ler o arquivo. Verifique se ele foi exportado para uma pasta local.",
     "Could not read the file. Export it to a local folder first."},
    {"importCorrupt",
     "O arquivo está vazio, incompleto ou corrompido. Exporte uma nova cópia de áudio.",
     "The file is empty, incomplete or corrupt. Export a fresh audio copy."},
    {"importFormat", "Formato não suportado. Use WAV, AIFF, MP3 ou M4A renderizado (AAC/ALAC).",
     "Unsupported format. Use WAV, AIFF, MP3 or rendered M4A (AAC/ALAC)."},
    {"importCodec",
     "O contêiner M4A foi reconhecido, mas o codec não é compatível. Exporte como AAC ou Apple "
     "Lossless (ALAC), mono/estéreo.",
     "The M4A container was recognised, but its codec is incompatible. Export as AAC or Apple "
     "Lossless (ALAC), mono/stereo."},
    {"importMovie",
     "QuickTime Editável não é importado. No Gravador, compartilhe/exporte uma versão de áudio "
     "renderizada em .m4a (Comprimido/AAC ou Sem perdas/ALAC), em vez de Editável. Se sua versão "
     "não oferecer essas opções, exporte áudio pelo QuickTime Player ou renderize a mixagem no seu "
     "editor.",
     "Editable QuickTime is not imported. In Voice Memos, share/export rendered .m4a audio "
     "(Compressed/AAC or Lossless/ALAC) instead of Editable. If your version does not offer those "
     "choices, export audio in QuickTime Player or render the mix in your editor."},
    {"importEncoded", "Arquivo maior que 32 MB. Exporte ou recorte uma gravação menor.",
     "File exceeds 32 MB. Export or trim a smaller recording."},
    {"importDuration",
     "A gravação excede o limite de 120 segundos/16 milhões de frames. Exporte um trecho menor.",
     "Recording exceeds 120 seconds/16 million frames. Export a shorter section."},
    {"importMemory",
     "O limite de 256 MB de áudio desta instância foi atingido. Reabra a instância para liberar a "
     "memória antes de importar.",
     "This instance reached its 256 MB audio budget. Reopen the instance to release memory before "
     "importing."},
    {"importBackend",
     "O decoder local de M4A não está disponível. No Linux, instale FFmpeg no caminho configurado "
     "no build. No Mac, a leitura usa Core Audio do sistema.",
     "The local M4A decoder is unavailable. On Linux install FFmpeg at the configured build path. "
     "Mac decoding uses system Core Audio."},
    {"importDecode",
     "M4A AAC/ALAC reconhecido, mas não foi possível decodificar. O arquivo pode estar incompleto "
     "ou usar um perfil incompatível com o decoder local. Exporte outra cópia renderizada.",
     "AAC/ALAC M4A recognised, but decoding failed. The file may be incomplete or use a profile "
     "incompatible with the local decoder. Export another rendered copy."},
    {"importSuccess", "Áudio importado.", "Audio imported."},
    {"ghostShort", "Eco filtrado", "Filtered echo"},
    {"crumbleShort", "Fragmentos", "Fragments"},
    {"bloomShort", "Cauda difusa", "Diffuse tail"},
    {"wobbleShort", "Atraso modulado", "Modulated delay"},
    {"dustShort", "Textura dinâmica", "Dynamic texture"},
    {"crushShort", "Bits + taxa", "Bits + rate"},
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
    {"empty", "Arraste WAV, AIFF, MP3 ou M4A aqui · ou use CARREGAR SOM",
     "Drop WAV, AIFF, MP3 or M4A here · or use LOAD SOUND"},
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
    {"sample", "01 / SEU SAMPLE", "01 / YOUR SAMPLE"},
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
