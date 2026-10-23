/* RASTRO factory sounds. Every sound here is synthesised by this file (maths only: FM, plucked
   strings, detuned oscillators, filtered noise). No samples, presets or recordings from anyone else.
   A sound is rendered to a WAV in memory only when the user picks it, then loaded like any import.
   All sounds are tuned to C4 (MIDI 60), the plugin's default root. */
(function () {
  "use strict";
  const SR = 44100, C4 = 261.6256, TAU = Math.PI * 2;
  function rng(seed) { let s = seed >>> 0 || 1; return function () { s ^= s << 13; s >>>= 0; s ^= s >> 17; s ^= s << 5; s >>>= 0; return s / 4294967296; }; }
  function env(t, a, d, s, r, len) { // ADSR in seconds; len = note length before release
    let e = t < a ? t / a : t < a + d ? 1 - (1 - s) * (t - a) / d : s;
    if (t > len) e *= Math.max(0, 1 - (t - len) / r);
    return e;
  }
  function lp(buf, cutoffFn, q) { // resonant state-variable low-pass, cutoff may change per sample
    let low = 0, band = 0;
    for (let i = 0; i < buf.length; i++) {
      const f = 2 * Math.sin(Math.PI * Math.min(.45, cutoffFn(i / SR) / SR));
      low += f * band; const high = buf[i] - low - q * band; band += f * high; buf[i] = low;
    }
    return buf;
  }
  function hp(buf, fc) { let y = 0, x1 = 0; const a = 1 / (1 + TAU * fc / SR); for (let i = 0; i < buf.length; i++) { y = a * (y + buf[i] - x1); x1 = buf[i]; buf[i] = y; } return buf; }
  function norm(buf, peak) { let m = 0; for (let i = 0; i < buf.length; i++) m = Math.max(m, Math.abs(buf[i])); const g = m > 0 ? (peak || .89) / m : 0; for (let i = 0; i < buf.length; i++) buf[i] *= g; return buf; }
  function fade(buf) { const n = Math.min(400, buf.length); for (let i = 0; i < n; i++) buf[buf.length - 1 - i] *= i / n; return buf; }
  const saw = function (p) { return 2 * (p - Math.floor(p + .5)); }, sq = function (p, w) { return (p - Math.floor(p)) < (w || .5) ? 1 : -1; };
  function make(sec, fn) { const n = Math.round(sec * SR), b = new Float32Array(n); for (let i = 0; i < n; i++) b[i] = fn(i / SR, i); return b; }
  function ks(f, sec, bright, decay, seed) { // Karplus-Strong plucked string
    const r = rng(seed || 7), n = Math.round(sec * SR), p = Math.max(2, Math.round(SR / f)), line = new Float32Array(p), out = new Float32Array(n);
    for (let i = 0; i < p; i++) line[i] = (r() * 2 - 1);
    let idx = 0, last = 0;
    for (let i = 0; i < n; i++) { const cur = line[idx], nxt = line[(idx + 1) % p]; const v = (cur * bright + nxt * (1 - bright)) * decay; last = v; line[idx] = v; out[i] = cur; idx = (idx + 1) % p; }
    void last; return out;
  }
  function fm(sec, ratio, index, idxDecay, a, d, s, r, len, extra) {
    return make(sec, function (t) { const e = env(t, a, d, s, r, len), I = index * Math.exp(-t * idxDecay); const m = Math.sin(TAU * C4 * ratio * t) * I; return Math.sin(TAU * C4 * t + m) * e * (extra ? extra(t) : 1); });
  }
  function supersaw(sec, n, spread, a, d, s, r, len, seed) {
    const ph = [], rr = rng(seed || 3); for (let k = 0; k < n; k++) ph.push(rr());
    return make(sec, function (t) { let v = 0; for (let k = 0; k < n; k++) { const det = 1 + spread * (k - (n - 1) / 2) / n; v += saw(ph[k] + C4 * det * t); } return v / n * env(t, a, d, s, r, len); });
  }
  const SOUNDS = [
    // KEYS
    { id: "neon-keys", name: "Neon Keys", cat: "keys", tags: "Retro · Lush · Neon", sec: 3.2, make: function () { return fm(3.2, 1, 2.2, 2.6, .004, 2.2, .25, .6, 2.4, function (t) { return 1 + .12 * Math.sin(TAU * 4.5 * t); }); } },
    { id: "velvet-rhodes", name: "Velvet Rhodes", cat: "keys", tags: "Warm · Analog · Dreamy", sec: 3.4, make: function () { const a = fm(3.4, 1, 1.4, 3.5, .006, 2.6, .2, .7, 2.6), b = fm(3.4, 14, .25, 30, .001, .05, 0, .05, .05); for (let i = 0; i < a.length; i++) a[i] += b[i] * .12; return a; } },
    { id: "solar-piano", name: "Solar Piano", cat: "keys", tags: "Cinematic · Warm · Wide", sec: 3.6, make: function () { return make(3.6, function (t) { let v = 0; for (let h = 1; h <= 9; h++) v += Math.sin(TAU * C4 * h * (1 + .0006 * h * h) * t) / (h * 1.3) * Math.exp(-t * (1.1 + h * .45)); return v * Math.min(1, t / .003); }); } },
    { id: "funk-keys", name: "Funk Keys", cat: "keys", tags: "Clav · Bite · Short", sec: 1.4, make: function () { return hp(lp(make(1.4, function (t) { return sq(C4 * t, .22) * env(t, .002, .35, .15, .1, .5); }), function (t) { return 900 + 5200 * Math.exp(-t * 9); }, .35), 120); } },
    { id: "tokyo-mallets", name: "Tokyo Mallets", cat: "keys", tags: "Wood · Bright · Short", sec: 1.8, make: function () { return make(1.8, function (t) { return (Math.sin(TAU * C4 * t) * Math.exp(-t * 3.2) + .45 * Math.sin(TAU * C4 * 3.98 * t) * Math.exp(-t * 11) + .2 * Math.sin(TAU * C4 * 9.9 * t) * Math.exp(-t * 30)) * Math.min(1, t / .001); }); } },
    // PLUCKS
    { id: "orbit-pluck", name: "Orbit Pluck", cat: "plucks", tags: "Spacey · Digital · Bright", sec: 2.4, make: function () { return ks(C4, 2.4, .5, .996, 11); } },
    { id: "laser-harp", name: "Laser Harp", cat: "plucks", tags: "Bright · Sci-fi", sec: 2.2, make: function () { const a = ks(C4, 2.2, .8, .998, 5), b = ks(C4 * 2.003, 2.2, .7, .997, 9); for (let i = 0; i < a.length; i++) a[i] = a[i] + b[i] * .5; return a; } },
    { id: "glass-pluck", name: "Glass Pluck", cat: "plucks", tags: "Crystal · Soft", sec: 2.2, make: function () { return fm(2.2, 3.5, 1.6, 6, .002, 1.4, 0, .4, 1.5); } },
    // PADS
    { id: "cosmic-pad", name: "Cosmic Pad", cat: "pads", tags: "Wide · Slow · Night", sec: 4.5, make: function () { return lp(supersaw(4.5, 7, .018, .9, 1, .85, 1, 3.2, 21), function (t) { return 700 + 900 * (.5 + .5 * Math.sin(TAU * .25 * t)); }, .5); } },
    { id: "veludo-82", name: "Veludo 82", cat: "pads", tags: "Warm · 80s · Soft", sec: 4.5, make: function () { return lp(make(4.5, function (t) { return (saw(C4 * t) + saw(C4 * 1.004 * t + .3) + .5 * sq(C4 * .5 * t)) * env(t, .6, 1.2, .8, 1.2, 3.1) * (1 + .08 * Math.sin(TAU * 5 * t)); }), function () { return 1500; }, .7); } },
    { id: "nebula", name: "Nebula", cat: "pads", tags: "Air · Texture · Dark", sec: 5, make: function () { const r = rng(31), n = make(5, function () { return r() * 2 - 1; }); const out = new Float32Array(n.length); [1, 2, 3, 5].forEach(function (h, k) { let low = 0, band = 0; const f = 2 * Math.sin(Math.PI * C4 * h / SR); for (let i = 0; i < n.length; i++) { low += f * band; const high = n[i] - low - .012 * band; band += f * high; out[i] += band / (k + 1); } }); for (let i = 0; i < out.length; i++) out[i] *= env(i / SR, 1.2, 1, .9, 1.2, 3.6); return out; } },
    // LEADS
    { id: "neon-lead", name: "Neon Lead", cat: "leads", tags: "Square · Vibrato", sec: 2.4, make: function () { return lp(make(2.4, function (t) { const vib = 1 + .006 * Math.sin(TAU * 5.5 * t) * Math.min(1, t / .5); return sq(C4 * vib * t, .45) * env(t, .01, .3, .7, .3, 1.9); }), function () { return 3200; }, .4); } },
    { id: "sunset-saw", name: "Sunset Saw", cat: "leads", tags: "Supersaw · Big", sec: 2.6, make: function () { return lp(supersaw(2.6, 9, .03, .01, .4, .8, .4, 2, 4), function () { return 5000; }, .3); } },
    // BASS
    { id: "acid-drop", name: "Acid Drop", cat: "bass", tags: "Squelch · Resonant", sec: 1.6, make: function () { return lp(make(1.6, function (t) { return saw(C4 / 2 * t) * env(t, .003, .5, .6, .2, 1.2); }), function (t) { return 180 + 3000 * Math.exp(-t * 6); }, .12); } },
    { id: "sub-moon", name: "Sub Moon", cat: "bass", tags: "Sub · Round", sec: 2, make: function () { return make(2, function (t) { const v = Math.sin(TAU * C4 / 2 * t) * env(t, .005, .6, .8, .3, 1.6); return Math.tanh(v * 1.8); }); } },
    { id: "funk-bass", name: "Funk Bass", cat: "bass", tags: "Pluck · Groove", sec: 1.6, make: function () { return lp(ks(C4 / 2, 1.6, .35, .995, 17), function (t) { return 400 + 2500 * Math.exp(-t * 10); }, .5); } },
    // STRINGS
    { id: "string-machine", name: "String Machine", cat: "strings", tags: "Ensemble · 70s", sec: 4, make: function () { return lp(make(4, function (t) { let v = 0; for (let k = 0; k < 4; k++) v += saw(C4 * (1 + .004 * Math.sin(TAU * (.6 + k * .37) * t + k)) * t + k * .25); return v / 4 * env(t, .35, .8, .85, .9, 3); }), function () { return 2600; }, .6); } },
    // BELLS
    { id: "sino-de-vidro", name: "Sino de Vidro", cat: "bells", tags: "FM · Shimmer", sec: 4, make: function () { return fm(4, 3.5, 3, 1.2, .001, 3.6, 0, .3, 3.7); } },
    { id: "music-box", name: "Music Box", cat: "bells", tags: "Tiny · Sweet", sec: 2.4, make: function () { return make(2.4, function (t) { return (Math.sin(TAU * C4 * 2 * t) + .3 * Math.sin(TAU * C4 * 2 * 5.4 * t) * Math.exp(-t * 8)) * Math.exp(-t * 2.2) * Math.min(1, t / .001); }); } },
    // TEXTURES
    { id: "dusty-memories", name: "Dusty Memories", cat: "textures", tags: "Lo-Fi · Nostalgic · Soft", sec: 4, make: function () { const r = rng(77), a = fm(4, 1, .8, 1, .05, 1.5, .5, 1, 3); for (let i = 0; i < a.length; i++) { a[i] *= 1 + .05 * Math.sin(TAU * .7 * i / SR); a[i] += (r() < .0008 ? (r() * 2 - 1) * .8 : 0) + (r() * 2 - 1) * .015; } return lp(a, function () { return 3500; }, .7); } },
    { id: "tape-rain", name: "Tape Rain", cat: "textures", tags: "Noise · Ambience", sec: 4, make: function () { const r = rng(5); return lp(make(4, function (t) { return (r() * 2 - 1) * (.5 + .5 * Math.sin(TAU * .3 * t)) * env(t, .5, .5, .9, .8, 3.2); }), function (t) { return 1200 + 800 * Math.sin(TAU * .2 * t); }, .4); } },
    // PERCUSSION
    { id: "kick-808", name: "Kick 808", cat: "perc", tags: "Deep · Long", sec: 1.2, make: function () { let ph = 0; return make(1.2, function (t) { const f = 45 + 120 * Math.exp(-t * 28); ph += f / SR; return Math.tanh(Math.sin(TAU * ph) * 1.4) * Math.exp(-t * 3.2); }); } },
    { id: "snap", name: "Neon Snap", cat: "perc", tags: "Clap · Crisp", sec: .5, make: function () { const r = rng(9); return hp(make(.5, function (t) { const bursts = [0, .011, .022].reduce(function (a, o) { return a + (t >= o ? Math.exp(-(t - o) * 90) : 0); }, 0) + Math.exp(-t * 18) * .5; return (r() * 2 - 1) * bursts; }), 900); } },
    { id: "hat", name: "Cosmo Hat", cat: "perc", tags: "Metal · Tight", sec: .35, make: function () { const r = rng(12); return hp(make(.35, function (t) { let m = 0; [2, 3, 4.16, 5.43, 6.79, 8.21].forEach(function (k) { m += sq(C4 * k * 1.5 * t); }); return (m / 6 * .5 + (r() * 2 - 1) * .5) * Math.exp(-t * 40); }), 6000); } }
  ];
  const CATS = [["all", "ALL", "wave"], ["keys", "KEYS", "keys"], ["plucks", "PLUCKS", "pluck"], ["pads", "PADS", "pad"], ["leads", "LEADS", "lead"], ["bass", "BASS", "bass"], ["strings", "STRINGS", "strings"], ["bells", "BELLS", "bell"], ["textures", "TEXTURES", "texture"], ["perc", "PERCUSSION", "drum"]];
  function wav(buf) {
    const n = buf.length, bytes = new Uint8Array(44 + n * 2), dv = new DataView(bytes.buffer);
    const w = function (o, s) { for (let i = 0; i < s.length; i++) bytes[o + i] = s.charCodeAt(i); };
    w(0, "RIFF"); dv.setUint32(4, 36 + n * 2, true); w(8, "WAVE"); w(12, "fmt "); dv.setUint32(16, 16, true); dv.setUint16(20, 1, true); dv.setUint16(22, 1, true);
    dv.setUint32(24, SR, true); dv.setUint32(28, SR * 2, true); dv.setUint16(32, 2, true); dv.setUint16(34, 16, true); w(36, "data"); dv.setUint32(40, n * 2, true);
    for (let i = 0; i < n; i++) dv.setInt16(44 + i * 2, Math.max(-1, Math.min(1, buf[i])) * 32767, true);
    let bin = ""; const CH = 32768;
    for (let i = 0; i < bytes.length; i += CH) bin += String.fromCharCode.apply(null, bytes.subarray(i, i + CH));
    return btoa(bin);
  }
  const cache = {};
  function render(id) {
    if (cache[id]) return cache[id];
    const s = SOUNDS.filter(function (x) { return x.id === id; })[0]; if (!s) return null;
    cache[id] = wav(fade(norm(s.make())));
    return cache[id];
  }
  // Small peak picture for the list (computed once per sound, cheap).
  function peaks(id, n) {
    const s = SOUNDS.filter(function (x) { return x.id === id; })[0]; if (!s) return [];
    if (!s._p) { const b = norm(s.make()); const out = []; const step = Math.floor(b.length / 64); for (let i = 0; i < 64; i++) { let m = 0; for (let j = i * step; j < (i + 1) * step; j++) m = Math.max(m, Math.abs(b[j])); out.push(m); } s._p = out; }
    return s._p.slice(0, n || 64);
  }
  window.RastroFactory = { SOUNDS: SOUNDS, CATS: CATS, render: render, peaks: peaks };
})();
