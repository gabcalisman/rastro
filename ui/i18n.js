// RASTRO PLAY — interface language (Português / English). Visual only.
// The interface is written in Portuguese; in English every visible text node and tooltip is looked up
// here (exact text, then a few patterns). Untranslated text simply stays in Portuguese.
(function () {
  "use strict";
  const D = {
    // header / tabs / common
    "Novo patch": "New patch", "ESTÚDIO": "STUDIO", "SAMPLE": "SAMPLE", "Solte o áudio para importar": "Drop the audio to import",
    "Comece aqui": "Start here", "Arraste um WAV, AIFF, MP3 ou M4A, escolha um som na biblioteca, ou capture pelo menu.": "Drag a WAV, AIFF, MP3 or M4A, pick a sound from the library, or capture from the menu.",
    "Arraste um áudio aqui": "Drag an audio file here", "CAPTURA": "CAPTURE", "XY PERFORMANCE PAD": "XY PERFORMANCE PAD", "PERFORMANCE": "PERFORMANCE",
    "destinos desligados": "targets off", "DEGRADADO": "DEGRADED", "LIMPO": "CLEAN", "ESTÁVEL": "STABLE",
    "prévia interna · não controla o Logic": "internal preview · does not control Logic", "SAÍDA": "OUTPUT", "VARIAR FX": "VARY FX", "FICAR": "KEEP", "VOLTAR": "BACK",
    "PRESENÇA": "PRESENCE", "RASTRO": "TRAIL", "VARIAÇÃO": "VARIATION", "Prévia no navegador com motor simulado. No plugin, tudo usa o áudio real.": "Browser preview with a simulated engine. In the plugin everything uses real audio.",
    "LIGAR SYNTH": "SYNTH ON?", "SYNTH LIGADO": "SYNTH ON", "DESLIGADO": "OFF",
    "Para o ADSR moldar o som, clique em LIGAR SYNTH (ou use o disparo GATE em MIDI / VOZ).": "For the ADSR to shape the sound, turn the SYNTH on (or use the GATE trigger in VOICE).",
    "PRÓPRIO": "OWN", "SEGUE AMP": "FOLLOWS AMP", "DESTINO": "TARGET", "DISPARO": "TRIGGER", "POR MODO": "PER MODE", "Noise · Vinil": "Noise · Vinyl",
    "TOM": "TONE", "TEMPO": "TIME", "TAXA": "RATE", "PROFUND.": "DEPTH", "LARGURA": "WIDTH", "TAMANHO": "SIZE", "DIFUSÃO": "DIFFUSION", "NÍVEL": "LEVEL", "CARÁTER": "CHARACTER", "INTENS.": "AMOUNT",
    "LIGADO": "ON", "VARIAR LIVRE": "VARY FREE", "🔒 VARIAR TRAVADO": "🔒 VARY LOCKED", "REMOVER DO RACK": "REMOVE FROM RACK", "EFEITO": "EFFECT",
    "Delay: repete o som como um eco que vai escurecendo.": "Delay: repeats the sound as an echo that gets darker.",
    "Stutter: picota o som em pedacinhos que se repetem.": "Stutter: chops the sound into repeating crumbs.",
    "Reverb: abre uma cauda, como um espaço. DIFUSÃO alisa as repetições.": "Reverb: opens a tail, like a room. DIFFUSION smooths the echoes.",
    "Chorus/vibrato: a afinação balança como fita gasta. LARGURA abre o estéreo.": "Chorus/vibrato: the pitch wobbles like worn tape. WIDTH opens the stereo.",
    "Noise: adiciona poeira e chiado que seguem o som.": "Noise: adds dust and hiss that follow the sound.",
    "Bitcrusher: reduz bits e taxa, som digital e áspero.": "Bitcrusher: lowers bits and rate, a gritty digital sound.",
    "rastros violeta": "violet trails", "fragmentos âmbar": "amber fragments", "aura rosa": "pink aura", "contorno ciano": "cyan outline", "partículas douradas": "golden particles", "pixels roxos": "purple pixels",
    // patch lab
    "FONTE DO SOM": "SOUND SOURCE", "PATCHES SUGERIDOS": "SUGGESTED PATCHES", "clique num patch para ouvir em B": "click a patch to hear it in B",
    "Escolha um som e clique em GERAR PATCHES.": "Pick a sound and click GENERATE PATCHES.", "BUSCA DE PATCHES": "PATCH SEARCH", "DIREÇÃO": "DIRECTION",
    "MAIS FIEL": "CLOSER", "MAIS TRANSFORMADO": "MORE TRANSFORMED", "GERAR PATCHES": "GENERATE PATCHES", "VARIAR A PARTIR DO PATCH": "VARY FROM THE PATCH", "VARIAR": "VARY",
    "PAUSAR": "PAUSE", "RETOMAR": "RESUME", "CANCELAR": "CANCEL", "Pronto.": "Ready.", "SALVAR PATCH": "SAVE PATCH", "FAMÍLIA DE PATCHES": "PATCH FAMILY",
    "o patch atual e as variações geradas a partir dele": "the current patch and the variations made from it", "BIBLIOTECA": "LIBRARY", "FECHAR": "CLOSE",
    "Buscar sons e patches…": "Search sounds and patches…", "Procurando…": "Searching…", "Pronto! Clique numa opção para ouvir.": "Done! Click an option to listen.",
    "Busca cancelada. Seu patch atual foi mantido.": "Search cancelled. Your current patch was kept.", "Escolha um som primeiro.": "Pick a sound first.",
    "O trecho está silencioso. Escolha outro trecho.": "The selection is silent. Pick another part.", "Falha na busca.": "Search failed.",
    "ABRIR · SÓ SOM": "OPEN · SOUND ONLY", "ABRIR · COMPLETO": "OPEN · FULL",
    // estúdio
    "Desbloqueie visuais tocando, criando e explorando. Eles nunca mudam o seu som.": "Unlock visuals by playing, creating and exploring. They never change your sound.",
    "Cabelo": "Hair", "Chapéu": "Hat", "Fones": "Headphones", "Óculos": "Glasses", "Instrumento": "Instrument", "Traço de luz": "Light stroke", "Cor da luz": "Light colour",
    "Meus visuais": "My visuals", "Cores": "Colours", "Reatividade": "Reactivity", "Desafios": "Challenges",
    "CABELO": "HAIR", "CHAPÉU": "HAT", "FONES": "HEADPHONES", "ÓCULOS": "GLASSES", "INSTRUMENTO": "INSTRUMENT", "TRAÇO DE LUZ": "LIGHT STROKE", "COR DA LUZ": "LIGHT COLOUR",
    "MEUS VISUAIS": "MY VISUALS", "CORES": "COLOURS", "REATIVIDADE": "REACTIVITY", "DESAFIOS": "CHALLENGES", "PRODUCER": "PRODUCER",
    "liga / desliga": "on / off", "peça": "item", "salvos": "saved", "todos os knobs reais": "every real knob", "cores da interface": "interface colours", "intensidade visual": "visual intensity", "XP opcional": "optional XP",
    "troca só esta parte · XP só libera visual": "changes only this part · XP only unlocks visuals", "um por vez · só visual": "one at a time · visual only",
    "as mãos tocam nele no ritmo das notas": "the hands play it with your notes", "de onde vem a cor da luz": "where the light colour comes from",
    "equipado": "equipped", "liberado": "unlocked", "Nenhum": "None", "Crista": "Crest", "Coquinhos": "Space buns", "Raspado": "Buzz cut", "Longo": "Long",
    "Boné": "Cap", "Gorro": "Beanie", "Coroa de luz": "Light crown", "Retrô": "Retro", "No pescoço": "Around the neck", "Redondo": "Round", "Estrela": "Star",
    "Teclado": "Keyboard", "Toca-discos": "Turntables", "Synth modular": "Modular synth", "Fita dupla": "Double ribbon", "Pontilhado": "Dotted", "Partículas": "Particles", "Vetor fino": "Thin vector",
    "Segue os efeitos": "Follows the effects", "Paleta da interface": "Interface palette", "Dourado": "Gold", "Gelo": "Ice", "Arco-íris por nota": "Rainbow per note",
    "VISUAL ATUAL": "CURRENT VISUAL", "SALVAR VISUAL": "SAVE VISUAL", "VISUAL ALEATÓRIO": "RANDOM VISUAL",
    "o producer de luz aparece no centro de todas as telas": "the light producer appears in the centre of every screen", "Producer de luz": "Light producer",
    "contorno feito do seu som": "an outline made of your sound", "Só as ondas": "Waves only", "anéis, sem personagem": "rings, no character",
    "visuais que você salvou (SALVAR VISUAL)": "visuals you saved (SAVE VISUAL)", "NENHUM AINDA": "NONE YET", "Monte o seu producer e clique em SALVAR VISUAL.": "Build your producer and click SAVE VISUAL.",
    "clique para usar": "click to use", "muda todos os knobs reais": "changes every real knob", "tema da interface inteira": "theme of the whole interface",
    "CRIAR MINHA PALETA": "CREATE MY PALETTE", "Fundo": "Background", "Painéis": "Panels", "Destaque": "Accent", "Destaque 2": "Accent 2", "Texto": "Text",
    "USAR MINHA PALETA": "USE MY PALETTE", "EM USO": "IN USE", "opcional · XP só libera visual": "optional · XP only unlocks visuals",
    "Vinho": "Wine", "Pôr do sol": "Sunset", "Pérola": "Pearl", "Cósmico": "Cosmic", "Engrenagem": "Gear", "Borracha": "Rubber", "fosco": "matte", "anel aceso": "lit ring", "estúdio": "studio", "brilho": "glow",
    "Intensidade": "Intensity", "Parado": "Still", "Expressivo": "Expressive", "BPM livre": "free BPM",
    // menu
    "TELAS": "SCREENS", "sample, synth e rack": "sample, synth and rack", "biblioteca, X/Y, macros": "library, X/Y, macros", "PALCO": "STAGE",
    "producer de luz, knobs e cores": "light producer, knobs and colours", "gerar e comparar patches": "generate and compare patches", "gravar uma fonte": "record a source",
    "ARQUIVO": "FILE", "Importar áudio…": "Import audio…", "Abrir patch · só som…": "Open patch · sound only…", "Abrir patch · completo…": "Open patch · full…",
    "OPÇÕES": "OPTIONS", "luzes e movimento": "lights and motion", "Teclado na tela": "On-screen keyboard", "desligado": "off", "ligado": "on", "Idioma": "Language",
    "Animação": "Animation", "Completa": "Full", "Reduzida": "Reduced", "Jogo": "Game", "Ligado": "On", "Modo PRO": "PRO mode",
    // tooltips
    "Salvar patch .rastro com o áudio embutido": "Save a .rastro patch with the audio inside", "Desfazer a última mudança no som": "Undo the last sound change", "Refazer": "Redo",
    "RANDOMIZE: sorteia um patch novo (envelope, filtro, LFO, voz e efeitos). O atual fica em A; o novo toca em B.": "RANDOMIZE: rolls a new patch (envelope, filter, LFO, voice and effects). The current one stays in A; the new one plays in B.",
    "Menu: telas, arquivos e opções": "Menu: screens, files and options", "Carregar, arrastar ou capturar áudio; waveform; modo de reprodução; teclado": "Load, drag or capture audio; waveform; playback mode; keyboard",
    "ADSR, filtro, LFO, polifonia, pitch e performance": "ADSR, filter, LFO, polyphony, pitch and performance", "Cadeia de efeitos: a ordem é a ordem do som": "Effect chain: the order is the order of the sound",
    "Salvar, carregar, comparar e organizar sons": "Save, load, compare and organise sounds", "Personagem, guarda-roupa, instrumentos, knobs, cenário e cores": "Light producer, knobs and colours",
    "JAM: modo de performance (personagem, patch, teclado e controles essenciais)": "JAM: performance mode (visual, patch, keyboard and essential controls)",
    "SNAP: os marcadores grudam em cruzamentos de zero": "SNAP: markers stick to zero crossings", "Mostrar e ajustar o início": "Show and adjust the start", "Mostrar e ajustar o fim": "Show and adjust the end",
    "Mostrar a região de LOOP": "Show the LOOP region", "Afastar (−)": "Zoom out (−)", "Aproximar (+)": "Zoom in (+)", "Voltar à seleção": "Back to the selection", "Sample inteiro": "Whole sample",
    "CAPTURA: gravar voz, guitarra ou o que chegar na entrada e tocar no teclado": "CAPTURE: record voice, guitar or whatever reaches the input and play it on the keyboard",
    "TUNE (raiz): nota em que o sample toca na altura original": "TUNE (root): the note where the sample plays at its original pitch", "Quanto a força da nota abre o filtro": "How much note velocity opens the filter",
    "Tocar a prévia do sample ou retomar": "Play the sample preview or resume", "Pausar a prévia e congelar a animação": "Pause the preview and freeze the animation", "Parar e voltar ao início": "Stop and go back to the start",
    "Volume de saída": "Output volume", "VARIAR FX: sorteia valores novos só nos efeitos ligados": "VARY FX: new values only on the effects that are on", "Desfaz a última variação": "Undo the last variation",
    "Ficar com o patch sorteado (RANDOMIZE)": "Keep the rolled patch (RANDOMIZE)", "Voltar ao patch de antes do RANDOMIZE": "Go back to the patch before RANDOMIZE",
    "Quanto o VARIAR FX muda os efeitos; também deixa o CRUMBLE mais imprevisível": "How much VARY FX changes the effects; also makes CRUMBLE less predictable",
    "Mistura entre o som original e o rack": "Mix between the original sound and the rack", "Adicionar módulo removido": "Add a removed module", "Volta os efeitos aos valores padrão (desligados)": "Resets the effects to defaults (off)",
    "PARALLEL: cada módulo processa o som original e as diferenças são somadas. SERIAL: um alimenta o outro, e a ordem muda o som.": "PARALLEL: each module processes the original sound and the differences are added. SERIAL: one feeds the next, and the order changes the sound.",
    "Ligar / bypass": "On / bypass", "Toca o trecho até o fim, mesmo soltando a tecla": "Plays to the end even after the key is released", "Repete o trecho": "Repeats the selection", "De trás para frente": "Backwards",
    "Vai e volta": "Back and forth", "Cada nota toca um corte": "Each note plays a slice", "LOOP independente do modo: repete a região de loop sem mudar o modo": "LOOP independent of the mode: repeats the loop region without changing the mode",
    "GATE: soltar a tecla inicia o release do envelope (desligado = cada modo decide)": "GATE: releasing the key starts the envelope release (off = each mode decides)", "Crossfade na emenda do LOOP": "Crossfade at the LOOP seam",
    "Passa-baixa: tira os agudos": "Low-pass: removes the highs", "Passa-alta: tira os graves": "High-pass: removes the lows", "Passa-banda: só a faixa do cutoff": "Band-pass: only the band around the cutoff",
    "Notch: corta a faixa do cutoff": "Notch: cuts the band around the cutoff", "12 dB por oitava (suave)": "12 dB per octave (gentle)", "24 dB por oitava (forte)": "24 dB per octave (steep)",
    "DRIVE: saturação antes do filtro, com oversampling (sem aliasing)": "DRIVE: saturation before the filter, oversampled (no aliasing)", "ADSR próprio do filtro": "The filter's own ADSR",
    "Usa a forma do AMP ENVELOPE (como nas versões antigas)": "Uses the AMP ENVELOPE shape (as in older versions)", "Quanto o envelope abre (+) ou fecha (−) o filtro, ou muda a altura": "How much the envelope opens (+) or closes (−) the filter, or changes the pitch",
    "O envelope abre/fecha o filtro": "The envelope opens/closes the filter", "O envelope muda a altura": "The envelope changes the pitch", "Senoide": "Sine", "Triângulo": "Triangle", "Quadrada": "Square", "Rampa": "Saw",
    "Aleatório em degraus (sample & hold)": "Random steps (sample & hold)", "Quanto o LFO move o destino (CUTOFF ou PITCH)": "How much the LFO moves its target (CUTOFF or PITCH)",
    "FADE IN: o LFO entra aos poucos depois de cada nota": "FADE IN: the LFO comes in gradually after each note", "LFO no volume (tremolo)": "LFO to volume (tremolo)", "LFO no pan (som andando entre L e R)": "LFO to pan (sound moving between L and R)",
    "FREE: velocidade em Hz": "FREE: speed in Hz", "SYNC: segue o BPM do Logic": "SYNC: follows the Logic BPM", "REPITCH: altura e velocidade mudam com a nota": "REPITCH: pitch and speed change with the note",
    "FIXED: mantém altura e velocidade": "FIXED: keeps pitch and speed", "Alcance do pitch bend": "Pitch bend range", "Portamento em Mono/Legato": "Portamento in Mono/Legato", "Comportamento de cada modo": "Each mode's behaviour",
    "Ignora o soltar da tecla": "Ignores the key release", "Soltar a tecla inicia o release": "Releasing the key starts the release",
    "HQ: leitura do sample em alta qualidade (sem aliasing nas notas agudas) e DRIVE com oversampling. Sessões antigas abrem com HQ desligado para soar igual a antes.": "HQ: high-quality sample playback (no aliasing on high notes) and oversampled DRIVE. Older sessions open with HQ off so they sound exactly as before.",
    "Travado: o VARIAR FX não mexe neste módulo": "Locked: VARY FX leaves this module alone", "Abrir a biblioteca de sons e patches": "Open the sound and patch library", "CAPTURA: gravar e tocar no teclado": "CAPTURE: record and play on the keyboard",
    "TUNE (raiz)": "TUNE (root)", "Ouvir A (patch anterior)": "Listen to A (previous patch)", "Ouvir B (sugestão escolhida)": "Listen to B (chosen suggestion)", "Alterna A e B para comparar": "Switch A and B to compare",
    "Mais fiel: próximo ao original": "Closer: near the original", "Mais transformado": "More transformed", "Sorteia valores novos nos efeitos ligados (desfazer em MACROS)": "New values on the effects that are on (undo in MACROS)",
    "Favoritar": "Favourite", "Trocar o cenário do palco (só visual)": "Change the stage (visual only)", "Sorteia dança e cenário (o que existe para a arte original)": "Rolls a random visual from the unlocked items",
    "Segue o pulso do Logic (ou o TEMPO do plugin com o Logic parado)": "Follows the Logic pulse (or the plugin TEMPO with Logic stopped)", "BPM próprio": "Own BPM",
    // toasts
    "Look sorteado entre os itens liberados.": "Random visual from the unlocked items.", "Visual sorteado entre os itens liberados.": "Random visual from the unlocked items.",
    "Look salvo em Meus looks.": "Visual saved in My visuals.", "Escolha um som ou importe um sample primeiro.": "Pick a sound or import a sample first."
  };
  const PAT = [
    [/^🔒 nível (\d+)(.*)$/, function (m) { return "🔒 level " + m[1] + m[2]; }],
    [/^nível (\d+)$/, function (m) { return "level " + m[1]; }],
    [/^NV (\d+)$/, function (m) { return "LV " + m[1]; }],
    [/^contraste ok \((.*)\)$/, function (m) { return "contrast ok (" + m[1] + ")"; }],
    [/^texto pouco legível \((.*?), mínimo 4,5\) — ajustamos o texto automaticamente$/, function (m) { return "low text contrast (" + m[1] + ", minimum 4.5) — text adjusted automatically"; }],
    [/^No personagem: (.*)$/, function (m) { return "In the visual: " + (D[m[1]] || m[1]); }],
    [/^no personagem: (.*)$/, function (m) { return "in the visual: " + (D[m[1]] || m[1]); }],
    [/^AMOUNT = MIX do (\w+) \(quanto do efeito entra no som\)$/, function (m) { return "AMOUNT = " + m[1] + " MIX (how much of the effect is heard)"; }],
    [/^(.*) \(prévia\)$/, function (m) { return m[1] + " (preview)"; }],
    [/^Libera no nível (\d+) \(faltam (\d+) XP\)\.(.*)$/, function (m) { return "Unlocks at level " + m[1] + " (" + m[2] + " XP to go)." + (m[3] ? " Visual only." : ""); }],
    [/^Libera no nível (\d+)\.$/, function (m) { return "Unlocks at level " + m[1] + "."; }],
    [/^Visual aplicado: (.*)$/, function (m) { return "Visual applied: " + m[1]; }],
    [/^([^:]{2,20}): (.+)$/, function (m) { return (D[m[1]] || m[1]) + ": " + (D[m[2]] || m[2]); }]
  ];
  function t(s) {
    if (!s) return s;
    const k = s.trim(); if (D[k]) return s.replace(k, D[k]);
    for (let i = 0; i < PAT.length; i++) { const m = k.match(PAT[i][0]); if (m) { const r = PAT[i][1](m); if (r !== k) return s.replace(k, r); } }
    return s;
  }
  const origText = new WeakMap(), origTitle = new WeakMap(), origPh = new WeakMap();
  let lang = "pt";
  function apply(root) {
    root = root || document.body;
    const w = document.createTreeWalker(root, NodeFilter.SHOW_TEXT); let n;
    while ((n = w.nextNode())) {
      const p = n.parentElement; if (!p || p.tagName === "SCRIPT" || p.tagName === "STYLE") continue;
      const cur = n.nodeValue; if (!/[A-Za-zÀ-ú]/.test(cur)) continue;
      let o = origText.get(n);
      if (lang === "pt") { if (o !== undefined && cur !== o && cur === t(o)) n.nodeValue = o; else origText.set(n, cur); continue; }
      if (o === undefined || (cur !== o && cur !== t(o))) { o = cur; origText.set(n, o); } // text changed by the app
      const want = t(o);
      if (cur !== want) n.nodeValue = want;
    }
    root.querySelectorAll("[title]").forEach(function (el) {
      let o = origTitle.get(el); const cur = el.title;
      if (lang === "pt") { if (o !== undefined && cur !== o && cur === t(o)) el.title = o; else origTitle.set(el, cur); return; }
      if (o === undefined || (cur !== o && cur !== t(o))) { o = cur; origTitle.set(el, o); }
      const want = t(o); if (cur !== want) el.title = want;
    });
    root.querySelectorAll("input[placeholder]").forEach(function (el) {
      let o = origPh.get(el); if (o === undefined) { o = el.placeholder; origPh.set(el, o); }
      el.placeholder = lang === "en" ? t(o) : o;
    });
  }
  let timer = 0, obs = null;
  function setLang(l) {
    lang = l === "en" ? "en" : "pt"; apply(document.body);
    if (lang === "pt") { if (obs) { obs.disconnect(); obs = null; } return; }
    if (!obs) { obs = new MutationObserver(function () { if (timer) return; timer = setTimeout(function () { timer = 0; if (!obs) return; obs.disconnect(); apply(document.body); obs.observe(document.body, { childList: true, subtree: true, characterData: true, attributes: true, attributeFilter: ["title"] }); }, 80); });
      obs.observe(document.body, { childList: true, subtree: true, characterData: true, attributes: true, attributeFilter: ["title"] }); }
  }
  window.RastroI18n = { setLang: setLang, t: function (s) { return lang === "en" ? t(s) : s; }, get lang() { return lang; } };
})();
