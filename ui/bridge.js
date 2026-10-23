/* RASTRO bridge: talks to the C++ editor through JUCE's native integration.
   Without JUCE (browser preview) a small simulated engine answers instead, clearly labelled. */
(function () {
  "use strict";
  const juce = window.__JUCE__ && window.__JUCE__.backend;
  const listeners = {};
  function on(event, fn) { (listeners[event] = listeners[event] || []).push(fn); }
  function fire(event, data) { (listeners[event] || []).forEach(function (fn) { fn(data); }); }

  if (juce) {
    let nextId = 1;
    const pending = new Map();
    juce.addEventListener("__juce__complete", function (e) {
      const p = pending.get(e.promiseId);
      if (p) { pending.delete(e.promiseId); p(e.result); }
    });
    ["tick", "params", "sample"].forEach(function (ev) {
      juce.addEventListener(ev, function (data) { fire(ev, data); });
    });
    window.Rastro = {
      simulated: false,
      on: on,
      call: function (name) {
        const args = Array.prototype.slice.call(arguments, 1);
        return new Promise(function (resolve) {
          const id = nextId++;
          pending.set(id, resolve);
          juce.emitEvent("__juce__invoke", { name: name, params: args, resultId: id });
        });
      }
    };
    return;
  }

  /* ------------------------------------------------------------------ */
  /* Simulated engine for browser previews. NOT the plugin's DSP.        */
  /* ------------------------------------------------------------------ */
  const P = {};
  function add(id, min, max, def, step) { P[id] = { id: id, v: def, min: min, max: max, def: def, step: step || 0 }; }
  [["root", 0, 127, 60, 1], ["start", 0, .999, 0], ["end", .001, 1, 1], ["loop", 0, 1, 0, 1], ["level", 0, 1, .7], ["velocity", 0, 1, .5],
   ["tempo", 30, 300, 120, 1], ["presence", 0, 1, .5], ["trail", 0, 1, .5], ["mutation", 0, 1, .3], ["drywet", 0, 1, 1], ["rackOrder", 0, 719, 0, 1],
   ["playX", 0, 1, .5], ["playY", 0, 1, .5], ["animation", 0, 2, 2, 1], ["playMode", 0, 4, 0, 1], ["sliceCount", 2, 16, 8, 1], ["pitchMode", 0, 1, 0, 1],
   ["regionLoop", 0, 1, 0, 1], ["loopRegionSet", 0, 1, 0, 1], ["loopStart", 0, .999, 0], ["loopEnd", .001, 1, 1], ["loopCrossfade", 0, 50, 5],
   ["synthEnable", 0, 1, 0, 1], ["ampAttack", .001, 4, .01], ["ampDecay", .001, 4, .2], ["ampSustain", 0, 1, .75], ["ampRelease", .001, 8, .2],
   ["filterCutoff", 20, 20000, 16000], ["filterResonance", 0, .95, .1], ["envAmount", -1, 1, 0], ["lfoRate", .05, 12, 1], ["lfoDepth", 0, 1, 0],
   ["lfoTarget", 0, 1, 0, 1], ["voiceMode", 0, 2, 0, 1], ["glide", 0, 2, 0], ["triggerMode", 0, 2, 0, 1], ["colourSet", 0, 8, 0, 1], ["avatarSpecies", 0, 2, 1, 1],
   ["gTime", 0, 1, .5], ["gFeedback", 0, 1, .5], ["gTone", 0, 1, .5], ["gMix", 0, 1, .25], ["cRate", 0, 1, .5], ["cVariation", 0, 1, .5], ["cMix", 0, 1, .25],
   ["bSize", 0, 1, .5], ["bTone", 0, 1, .5], ["bDecay", 0, 1, .5], ["bMix", 0, 1, .25], ["wDepth", 0, 1, .25], ["wRate", 0, 1, .3], ["wMix", 0, 1, .5],
   ["dLevel", 0, 1, .15], ["dCharacter", 0, 1, .5], ["dMix", 0, 1, .5], ["kAmount", 0, 1, .3], ["kMix", 0, 1, .3],
   ["gTiming", 0, 2, 2, 1], ["cTiming", 0, 2, 2, 1], ["wTiming", 0, 2, 1, 1], ["gDivision", 0, 3, 1, 1], ["cDivision", 0, 3, 1, 1], ["wDivision", 0, 3, 1, 1],
   ["gFreeMs", 5, 2000, 250], ["cFreeHz", .5, 40, 8], ["wFreeHz", .05, 12, 1.005]
  ].forEach(function (a) { add.apply(null, a); });
  ["g", "c", "b", "w", "d", "k"].forEach(function (m) { add(m + "On", 0, 1, 1, 1); add(m + "Present", 0, 1, 1, 1); });
  for (let i = 1; i < 16; i++) add("slice" + i, 0, 1, Math.min(1, i / 8));

  // A synthetic electric-piano-ish sample so the waveform is not empty in previews.
  const frames = 44100 * 2.4, peaksN = 2048;
  const L = [], R = [];
  for (let i = 0; i < peaksN; i++) {
    const t = i / peaksN, hit = [0, .25, .5, .62, .75].some(function (h) { return t >= h && t < h + .2; });
    const env = Math.exp(-((t % .25) * 9)) * (hit ? 1 : .25);
    const a = env * (.55 + .45 * Math.abs(Math.sin(i * .37) * Math.cos(i * .11)));
    L.push(-a, a); R.push(-a * .9, a * .92);
  }
  const sample = { loaded: true, name: "Velvet Keys (prévia)", frames: frames, rate: 44100, seconds: frames / 44100, stereo: true };
  const voices = [];
  let held = [0, 0, 0, 0], serial = 0, lastVel = 0, meter = 0, pos = 0, canUndo = false, previewing = false;
  let order = [0, 1, 2, 3, 4, 5];
  const changed = {};
  function setv(id, v) { if (!P[id]) return; P[id].v = Math.min(P[id].max, Math.max(P[id].min, v)); changed[id] = P[id].v; }
  function note(n, v, isOn) {
    const w = n >> 5, bit = 1 << (n & 31);
    if (isOn) { held[w] = (held[w] | bit) >>> 0; voices.push({ n: n, v: v, age: 0, rel: false }); serial++; lastVel = v; }
    else { held[w] = (held[w] & ~bit) >>> 0; voices.forEach(function (x) { if (x.n === n) x.rel = true; }); }
  }
  let last = performance.now(), beat = 0;
  setInterval(function () {
    const now = performance.now(), dt = (now - last) / 1000; last = now;
    beat = (beat + dt * P.tempo.v / 60) % 4;
    let energy = 0;
    for (let i = voices.length - 1; i >= 0; i--) {
      const x = voices[i]; x.age += dt;
      const a = P.ampAttack.v, d = P.ampDecay.v, s = P.ampSustain.v;
      let e = x.age < a ? x.age / a : Math.max(s, 1 - (x.age - a) / d * (1 - s));
      if (x.rel) { x.relAge = (x.relAge || 0) + dt; e *= Math.max(0, 1 - x.relAge / Math.max(.05, P.ampRelease.v)); }
      if (x.age > 6 || e <= 0.001) { voices.splice(i, 1); continue; }
      x.e = e; energy += e * x.v;
    }
    if (previewing) energy += .6;
    meter += (Math.min(1, energy * P.level.v) - meter) * .35;
    pos = voices.length || previewing ? (pos + dt / sample.seconds) % 1 : pos;
    const act = order.map(function () { return 0; });
    ["g", "c", "b", "w", "d", "k"].forEach(function (m, i) {
      const mixId = m + "Mix";
      act[i] = P[m + "On"].v > .5 && P[m + "Present"].v > .5 ? meter * P[mixId].v * .08 * (1 + Math.sin(now / (180 + i * 60)) * .3) : 0;
    });
    fire("tick", {
      meter: meter, act: act, env: voices.length ? voices[voices.length - 1].e || 0 : 0, wob: act[3] * 10, dust: act[4] * 10, crush: act[5] * 10, bloom: act[2] * 10,
      pos: pos, playing: voices.length > 0, preview: previewing, vel: lastVel, serial: serial, bpm: 0, beat: -1, hostPlaying: false,
      notes: held.slice(), canUndoJam: canUndo, order: order.slice()
    });
    if (Object.keys(changed).length) { fire("params", Object.assign({}, changed)); for (const k in changed) delete changed[k]; }
  }, 1000 / 60);

  const api = {
    ready: function () { return { params: Object.values(P).map(function (p) { return Object.assign({}, p); }), language: 0, sample: sample, canUndoJam: false, host: "Prévia no navegador", version: "preview" }; },
    setParam: function (id, v) { if (v !== undefined && v !== null) setv(id, v); return true; },
    note: function (n, v, isOn) { note(n, v, isOn); return true; },
    allNotesOff: function () { held = [0, 0, 0, 0]; voices.length = 0; return true; },
    importDialog: function () { return { ok: false, message: "Prévia no navegador: a importação funciona no plugin." }; },
    importData: function (name) { sample.name = name; fire("sample", sample); return { ok: true, message: "" }; },
    waveform: function () { return { l: L, r: R, stereo: true }; },
    playbackMode: function (m) { setv("playMode", m); setv("loop", m === 1 ? 1 : 0); return true; },
    slices: function (n) { n = Math.max(2, Math.min(16, n)); setv("sliceCount", n); for (let i = 1; i < 16; i++) setv("slice" + i, Math.min(1, i / n)); return true; },
    moveModule: function (from, to) { const m = order.splice(from, 1)[0]; order.splice(to, 0, m); return true; },
    jam: function () { ["gMix", "cMix", "bMix", "wMix", "dMix", "kMix", "gFeedback", "bSize", "wDepth"].forEach(function (id) { setv(id, Math.random() * .8 + .1); }); canUndo = true; return true; },
    undoJam: function () { canUndo = false; return false; },
    resetEffects: function () { return true; },
    preview: function (onOff) { previewing = !!onOff; if (onOff) setTimeout(function () { previewing = false; }, sample.seconds * 1000); return true; }
  };
  window.Rastro = {
    simulated: true,
    on: on,
    call: function (name) {
      const args = Array.prototype.slice.call(arguments, 1);
      return Promise.resolve(api[name] ? api[name].apply(null, args) : null);
    }
  };
})();
