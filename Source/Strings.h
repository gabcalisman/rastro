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
    {"pt", "Português (BR)", "Português (BR)"},
    {"en", "English", "English"},
    {"load", "CARREGAR SOM", "LOAD SOUND"},
    {"empty", "Carregue WAV ou AIFF · toque pelo MIDI ou teclado abaixo",
     "Load WAV or AIFF · play via MIDI or the keyboard below"},
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
    {"rack", "02 / CAMINHO DO SOM · arraste os títulos", "02 / SIGNAL PATH · drag the headings"},
    {"perform", "03 / TOQUE A TEXTURA", "03 / PLAY THE TEXTURE"},
    {"sample", "01 / SUA MATÉRIA-PRIMA", "01 / YOUR RAW MATERIAL"},
    {"error",
     "Não foi possível abrir o áudio. Use WAV/AIFF válido, até 32 MB e 120 s. Limite de áudio "
     "carregado por sessão: 256 MB; reabra a instância para liberar.",
     "Could not open audio. Use valid WAV/AIFF up to 32 MB and 120 s. Session audio limit: 256 MB; "
     "reopen the instance to release it."},
    {"choose", "Escolha um som", "Choose a sound"},
    {"aboutBody",
     "RASTRO by Gab Calisman\nInstrumento de amostras · protótipo 0.1\n16 vozes · afinação "
     "cromática · rack modular\nGHOST: eco. CRUMBLE: repetidor de fragmentos. BLOOM: reverberação "
     "difusa simples.\nArraste títulos para reordenar. Remova e adicione módulos pelo botão +. "
     "Clique Aprender CC e mova um controle MIDI.\nPRESENÇA: ganho suave. RASTRO: retorno/cauda. "
     "MUTAÇÃO: variação dos fragmentos e intensidade de MUTAR.",
     "RASTRO by Gab Calisman\nSample instrument · prototype 0.1\n16 voices · chromatic pitch · "
     "modular rack\nGHOST: echo. CRUMBLE: fragment repeater. BLOOM: simple diffuse reverb.\nDrag "
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
