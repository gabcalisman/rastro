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
    {"timingHelp",
     "FREE usa ms/Hz; SYNC usa 1/4, 1/8, colcheia pontuada ou tercina no BPM do host (ou tempo "
     "MANUAL se ausente). CRUMBLE e o LFO principal do WOBBLE seguem o transporte quando há PPQ; "
     "flutter fica livre. * preserva o modo legado. Faixa de tempo 30–300 BPM.",
     "FREE uses ms/Hz; SYNC uses quarter, eighth, dotted eighth or triplet at host BPM (or MANUAL "
     "tempo if absent). CRUMBLE and WOBBLE's main LFO follow transport when PPQ exists; flutter "
     "remains free. * preserves legacy behavior. Tempo range 30–300 BPM."},
    {"gFreeMs", "Eco · tempo (ms)", "Echo · time (ms)"},
    {"cFreeHz", "Fragmentos · Hz", "Fragments · Hz"},
    {"wFreeHz", "Wobble · Hz", "Wobble · Hz"},
    {"gDivision", "Eco · divisão", "Echo · division"},
    {"cDivision", "Fragmentos · divisão", "Fragments · division"},
    {"wDivision", "Wobble · divisão", "Wobble · division"},
    {"lfoTiming", "LFO · SYNC / FREE", "LFO · SYNC / FREE"},
    {"lfoDivision", "LFO · divisão SYNC", "LFO · SYNC division"},
    {"lfoTimingHelp",
     "FREE usa Hz. SYNC segue BPM e PPQ do host, com fallback manual. Legado conserva o seletor de "
     "relógio anterior.",
     "FREE uses Hz. SYNC follows host BPM/PPQ with manual fallback. Legacy preserves the previous "
     "clock selector."},
    {"lfoDivisionHelp", "Semínima, colcheia, colcheia pontuada ou tercina. Ative SYNC para usar.",
     "Quarter, eighth, dotted eighth or triplet. Enable SYNC to use."},
    {"xyHelp",
     "X multiplica a intensidade do WOBBLE; Y transforma CRUMBLE, DUST e CRUSH ativos. Centro "
     "mantém valores individuais. Não liga módulos em bypass. Sessões antigas mantêm o mapa antigo "
     "de Y (sem CRUMBLE).",
     "X scales WOBBLE depth; Y transforms active CRUMBLE, DUST and CRUSH. Centre preserves "
     "individual values. Never enables bypassed modules. Old sessions retain legacy Y mapping "
     "(without CRUMBLE)."},
    {"xyOff", "BYPASS / MIX 0", "BYPASS / MIX 0"},
    {"clean", "LIMPO", "CLEAN"},
    {"degraded", "DEGRADADO", "DEGRADED"},
    {"stable", "ESTÁVEL", "STABLE"},
    {"ready", "PRONTO", "READY"},
    {"randomHelp",
     "JAM: variação limitada dos efeitos ativos e desbloqueados; preserva sample, raiz, trims e modo. Desfazer "
     "variação recupera o anterior.",
     "JAM: bounded variation of active, unlocked effects; preserves sample, root, trims and mode. "
     "Undo variation restores the previous sound."},
    {"regionLoopHelp",
     "LOOP independente não troca o modo. Vale para novas notas. Região própria ou seleção; em "
     "Slice é limitada ao corte. One Shot repete após soltar; Gate/Hold libera pelo ADSR. Loop "
     "legado mantém seu note-off. Parada do host/All Sound Off encerra.",
     "Independent LOOP does not change mode. Applies to new notes. Own region or selection; Slice "
     "clips to its slice. One Shot repeats after key-up; Gate/Hold releases via ADSR. Legacy Loop "
     "keeps its note-off. Host stop/All Sound Off ends voices."},
    {"loopRegionSet", "Usar região própria do LOOP", "Use own LOOP region"},
    {"loopStart", "Loop · início (0–1)", "Loop · start (0–1)"},
    {"loopEnd", "Loop · fim (0–1)", "Loop · end (0–1)"},
    {"loopCrossfade", "Loop · crossfade", "Loop · crossfade"},
    {"loopStartHelp",
     "Posição normalizada do arquivo; habilite região própria. Também editável na waveform em "
     "LOOP.",
     "Normalized file position; enable own region. Also editable in waveform LOOP view."},
    {"loopEndHelp",
     "Fim da região; nunca amplia o trim/corte realmente tocado. Sem interseção, usa o corte "
     "inteiro.",
     "Region end; never enlarges the played trim/slice. With no intersection uses the whole "
     "slice."},
    {"loopCrossfadeHelp",
     "0–50 ms, limitado a 25% da região, no LOOP independente. Avanço/reverso usam crossfade "
     "linear com sobreposição; Ping-Pong usa reflexão interpolada contínua.",
     "0–50 ms, capped at 25% of region, for independent LOOP. Forward/reverse use linear overlap "
     "crossfade; Ping-Pong uses continuous interpolated reflection."},
    {"synthEnable", "Ativar SAMPLE SYNTH (novas vozes)", "Enable SAMPLE SYNTH (new voices)"},
    {"synthHelp",
     "ADSR e filtro ressonante por voz. Desligado preserva o som legado; Gate/Hold sempre usa "
     "ADSR. Vale para novas notas.",
     "Per-voice ADSR and resonant filter. Off preserves legacy audio; Gate/Hold always uses ADSR. "
     "Applies to new notes."},
    {"ampAttack", "Ataque", "Attack"},
    {"ampDecay", "Decaimento", "Decay"},
    {"ampSustain", "Sustain", "Sustain"},
    {"ampRelease", "Release", "Release"},
    {"filterCutoff", "Cutoff · low-pass", "Cutoff · low-pass"},
    {"filterResonance", "Ressonância", "Resonance"},
    {"envAmount", "Envelope · intensidade", "Envelope · amount"},
    {"envTarget", "Envelope →", "Envelope →"},
    {"lfoRate", "LFO · velocidade", "LFO · rate"},
    {"lfoDepth", "LFO · intensidade", "LFO · depth"},
    {"lfoTarget", "LFO →", "LFO →"},
    {"lfoSync", "LFO · relógio", "LFO · clock"},
    {"voiceMode", "Vozes", "Voices"},
    {"triggerMode", "Disparo / Gate", "Trigger / Gate"},
    {"glide", "Glide", "Glide"},
    {"bendRange", "Bend · semitons", "Bend · semitones"},
    {"ampAttackHelp", "Tempo de subida da amplitude por voz, em segundos.",
     "Per-voice amplitude attack time in seconds."},
    {"ampDecayHelp", "Tempo de 1 até o nível sustain.", "Time from 1 down to sustain level."},
    {"ampSustainHelp", "Nível mantido enquanto o gate está aberto e existe áudio na região.",
     "Held level while gate is open and audio remains in the region."},
    {"ampReleaseHelp",
     "Gate/Hold: note-off inicia esta liberação; pedal CC64 adia o release. One Shot ignora "
     "note-off.",
     "Gate/Hold: note-off starts release; CC64 pedal defers release. One Shot ignores note-off."},
    {"filterCutoffHelp", "Low-pass ressonante original de dois polos por voz; ative SAMPLE SYNTH.",
     "Original per-voice two-pole resonant low-pass; enable SAMPLE SYNTH."},
    {"filterResonanceHelp", "Realce na frequência de corte; limitado abaixo da auto-oscilação.",
     "Emphasis at cutoff; bounded below self-oscillation."},
    {"envAmountHelp",
     "Cutoff: até ±4 oitavas. Pitch: até ±12 semitons. Envelope de amplitude segue independente.",
     "Cutoff: up to ±4 octaves. Pitch: up to ±12 semitones. Amplitude envelope remains "
     "independent."},
    {"envTargetHelp", "Destino do envelope adicional: cutoff ou pitch.",
     "Additional envelope target: cutoff or pitch."},
    {"lfoRateHelp", "0,05–12 Hz quando livre. Sincronizado usa BPM do host ou tempo manual.",
     "0.05–12 Hz when free. Sync uses host BPM or manual tempo."},
    {"lfoDepthHelp",
     "Cutoff: até ±2 oitavas; pitch: até ±2 semitons. Mod wheel CC1 acrescenta intensidade, com "
     "limite.",
     "Cutoff: up to ±2 octaves; pitch: up to ±2 semitones. Mod wheel CC1 adds bounded depth."},
    {"lfoTargetHelp", "LFO senoidal em cutoff ou pitch; não é um novo efeito do rack.",
     "Sine LFO on cutoff or pitch; this is not a new rack effect."},
    {"lfoSyncHelp", "Livre ou uma oscilação por semínima, colcheia ou mínima.",
     "Free or one cycle per quarter, eighth or half note."},
    {"voiceModeHelp",
     "Poly: até 16 vozes. Mono: reinicia o envelope. Legato: mantém envelope/posição com notas "
     "sobrepostas. Última tecla tem prioridade.",
     "Poly: up to 16 voices. Mono retriggers envelope. Legato retains envelope/position for "
     "overlapping notes. Last-note priority."},
    {"triggerModeHelp",
     "Legado mantém note-off por modo. One Shot ignora note-off. Gate/Hold inicia o release ao "
     "soltar; sem loop, a borda do áudio ainda encerra a voz.",
     "Legacy preserves mode-specific note-off. One Shot ignores note-off. Gate/Hold releases on "
     "key-up; without looping the audio boundary still ends the voice."},
    {"glideHelp", "Portamento em Mono/Legato, em segundos; Poly ignora este controle.",
     "Portamento in Mono/Legato, in seconds; ignored in Poly."},
    {"bendRangeHelp", "Alcance do pitch bend MIDI nos dois pitch modes; zero desliga bend.",
     "MIDI pitch-bend range in both pitch modes; zero disables bend."},
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
    {"playAxes", "X: ESTÁVEL → WOBBLE · Y: LIMPO → DEGRADADO",
     "X: STABLE → WOBBLE · Y: CLEAN → DEGRADED"},
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
    {"mutation", "JAM", "JAM"},
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
    {"mutate", "JAM", "JAM"},
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
    {"mutateHelp",
     "Varia efeitos ativos aleatoriamente. Preserva sample, raiz, trims e modo. Desfazer variação "
     "recupera o som anterior.",
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
