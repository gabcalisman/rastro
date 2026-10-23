/* RASTRO Play — interface logic. Every control maps to a real plugin parameter or action.
   Screens follow Gab's approved references: LIMPO, PERFORM, PALCO, LOCKER, PATCH LAB, CAPTURA. */
(function () {
  "use strict";
  const R = window.Rastro, ART = window.RastroArt, ICO = window.RastroIcons, AL = window.RastroAlien, PUP = window.RastroPuppet, FAC = window.RastroFactory;
  const $ = function (s, el) { return (el || document).querySelector(s); };
  const $$ = function (s, el) { return Array.prototype.slice.call((el || document).querySelectorAll(s)); };
  const clamp = function (v, a, b) { return Math.min(b, Math.max(a, v)); };
  const NOTE = ["C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"];
  const noteName = function (n) { n = Math.round(n); return NOTE[((n % 12) + 12) % 12] + (Math.floor(n / 12) - 1); };
  const W0 = 1440, H0 = 900, NS = "http://www.w3.org/2000/svg";

  /* ================= state ================= */
  const P = {}, views = {}, busy = {};
  let T = { meter: 0, act: [0, 0, 0, 0, 0, 0], env: 0, pos: 0, playing: false, preview: false, vel: 0, serial: 0, bpm: 0, beat: -1, notes: [0, 0, 0, 0], search: {}, capture: {} };
  let order = [0, 1, 2, 3, 4, 5];
  let sample = { loaded: false, name: "", seconds: 0 };
  let peaks = null, S = 1, quietXP = false, lang = "pt";
  function v(id) { return P[id] ? P[id].v : 0; }
  function watch(id, fn) { (views[id] = views[id] || []).push(fn); if (P[id]) fn(P[id].v); return fn; }
  function apply(id, value) { if (!P[id]) return; P[id].v = value; if (!busy[id]) (views[id] || []).forEach(function (f) { f(value); }); }
  function setParam(id, value, phase) {
    if (!P[id]) return;
    value = clamp(value, P[id].min, P[id].max);
    if (P[id].step) value = Math.round(value / P[id].step) * P[id].step;
    P[id].v = value;
    (views[id] || []).forEach(function (f) { f(value); });
    R.call("setParam", id, value, phase || 0);
  }
  function gesture(id, value) { R.call("setParam", id, null, 1); setParam(id, value === undefined ? v(id) : value, 2); tweaked(id); }
  const VISUAL = /^(__|visual|avatar|scene|dance|animation|controlStyle|knobDesign|uiCompact|colourSet|cosmicArcade|language)|Colour$|JamLock$/;
  // Every real edit to the sound earns a little XP (rate-limited) and becomes an undo step.
  const tweakSeen = {}; let lastTweak = 0;
  function tweaked(id) {
    if (!P[id] || VISUAL.test(id)) return;
    history.mark();
    if (quietXP) return;
    const now = performance.now();
    if (now - lastTweak < 1500 || now - (tweakSeen[id] || -1e9) < 6000) return;
    lastTweak = now; tweakSeen[id] = now; game.award(1, "tweak");
  }
  function snapshot() { const s = {}; for (const id in P) if (!VISUAL.test(id)) s[id] = P[id].v; return s; }
  function restore(s) { quietXP = true; history.hold = true; for (const id in s) if (P[id] && Math.abs(P[id].v - s[id]) > 1e-6) gesture(id, s[id]); history.hold = false; quietXP = false; }
  // undo / redo of sound edits (snapshots taken when a gesture ends)
  const history = {
    past: [], future: [], cur: null, hold: false, t: 0,
    mark: function () { if (this.hold) return; clearTimeout(this.t); const self = this; this.t = setTimeout(function () { self.commit(); }, 350); },
    commit: function () { const s = snapshot(); if (this.cur && JSON.stringify(s) === JSON.stringify(this.cur)) return; if (this.cur) { this.past.push(this.cur); if (this.past.length > 60) this.past.shift(); } this.cur = s; this.future = []; this.ui(); },
    undo: function () { if (!this.past.length) return; this.future.push(this.cur); this.cur = this.past.pop(); restore(this.cur); this.ui(); toast("Desfeito."); },
    redo: function () { if (!this.future.length) return; this.past.push(this.cur); this.cur = this.future.pop(); restore(this.cur); this.ui(); toast("Refeito."); },
    ui: function () { $("#undoBtn").disabled = !this.past.length; $("#redoBtn").disabled = !this.future.length; }
  };
  $("#undoBtn").addEventListener("click", function () { history.undo(); });
  $("#redoBtn").addEventListener("click", function () { history.redo(); });

  /* ================= scaling, toast, screens ================= */
  function fit() {
    const w = window.innerWidth, h = window.innerHeight;
    S = Math.min(w / W0, h / H0);
    $("#app").style.transform = "translate(" + Math.max(0, (w - W0 * S) / 2) + "px," + Math.max(0, (h - H0 * S) / 2) + "px) scale(" + S + ")";
    sizeCanvas(); placeKey = "";
  }
  window.addEventListener("resize", fit);
  let toastTimer = 0;
  function toast(msg, ms) { const t = $("#toast"); t.textContent = msg; t.classList.add("show"); clearTimeout(toastTimer); toastTimer = setTimeout(function () { t.classList.remove("show"); }, ms || 2800); }
  function screen() { return document.body.dataset.screen; }
  function setScreen(sc) {
    const prev = screen();
    document.body.dataset.screen = sc;
    document.body.dataset.mode = sc === "palco" ? "palco" : sc === "captura" ? "captura" : "limpo";
    $$("#menu [data-go]").forEach(function (b) { b.classList.toggle("on", b.dataset.go === sc); });
    $$("#navTabs button").forEach(function (b) { b.classList.toggle("on", (b.dataset.nav === "locker" && sc === "locker") || (b.dataset.nav === "lab" && sc === "lab") || (sc === "perform" && b.dataset.nav === libTab)); });
    if (sc === "captura") openCapture(); else if (prev === "captura") closeCapture(true);
    if (sc === "locker") renderLocker();
    if (sc === "lab") renderLabScreen();
    if (sc === "perform") renderLibrary();
    $("#appBg").style.backgroundImage = sc === "locker" ? "url(" + roomFile() + ")" : "";
    renderRack(); placeKey = ""; syncMainTabs();
    if (sc === "palco") $("#jamPatch").textContent = $("#patchName").textContent;
    $("#jam").classList.toggle("on", sc === "palco");
    setTimeout(function () { sizeCanvas(); buildKeys(); }, 0);
  }
  function setEtab(t) {
    document.body.dataset.etab = t;
    syncMainTabs();
    if (t === "fx") renderFxDetail();
    setTimeout(function () { sizeCanvas(); }, 0);
  }
  // Main tabs: PLAY (sample) · SOM (instrument) · RACK (fx) · PATCHES (lab) · ESTÚDIO (locker)
  const MAIN = { play: ["limpo", "sample"], som: ["limpo", "instrument"], rack: ["limpo", "fx"], patches: ["lab"], estudio: ["locker"] };
  function currentMain() {
    const sc = screen(), et = document.body.dataset.etab;
    if (sc === "lab") return "patches"; if (sc === "locker") return "estudio";
    if (sc === "limpo") return et === "instrument" ? "som" : et === "fx" ? "rack" : "play";
    return "";
  }
  function syncMainTabs() { const m = currentMain(); $$("#editTabs button").forEach(function (b) { b.classList.toggle("on", b.dataset.main === m); }); }
  function setMain(m) { const t = MAIN[m]; if (!t) return; if (t[1]) setEtab(t[1]); if (screen() !== t[0]) setScreen(t[0]); syncMainTabs(); }
  $$("#editTabs button").forEach(function (b) { b.addEventListener("click", function () { setMain(b.dataset.main); }); });
  // menu
  const menu = $("#menu");
  function toggleMenu(e) { menu.hidden = !menu.hidden; if (e) e.stopPropagation(); }
  $("#menuBtn").addEventListener("click", toggleMenu); $("#palcoMenu").addEventListener("click", toggleMenu);
  document.addEventListener("click", function (e) { if (!e.target.closest("#menu")) menu.hidden = true; if (!e.target.closest("#addMenu")) $("#addMenu").hidden = true; });
  $$("#menu [data-go]").forEach(function (b) { b.addEventListener("click", function () { setScreen(b.dataset.go); menu.hidden = true; }); });
  $("#mImport").addEventListener("click", function () { menu.hidden = true; doImport(); });
  $("#mOpenSound").addEventListener("click", function () { menu.hidden = true; fileAction(false, true); });
  $("#mOpenFull").addEventListener("click", function () { menu.hidden = true; fileAction(false, false); });
  $("#mReact").addEventListener("click", function () { menu.hidden = true; lkCat = "react"; setScreen("locker"); });
  $("#mKeys").addEventListener("click", function () { look.kb = look.kb ? 0 : 1; applyKb(); saveLook(); });
  $("#mHero").addEventListener("click", function () { look.hero3d = look.hero3d === 0 ? 1 : 0; $("#mHeroState").textContent = look.hero3d ? "3D (beta)" : "pintado 2.5D"; saveLook(); applyLook(); });
  function applyKb() { document.body.classList.toggle("kb-on", !!look.kb); $("#mKeysState").textContent = look.kb ? "ligado" : "desligado"; setTimeout(buildKeys, 0); }

  /* ================= knobs ================= */
  const fmt = {
    pct: function (x) { return Math.round(x * 100) + "%"; },
    ms: function (x) { return x < 1 ? Math.round(x * 1000) + " ms" : x.toFixed(2) + " s"; },
    hz: function (x) { return x >= 1000 ? (x / 1000).toFixed(1) + " kHz" : Math.round(x) + " Hz"; },
    lfo: function (x) { return x.toFixed(2) + " Hz"; },
    note: function (x) { return noteName(x); },
    bip: function (x) { return (x > 0 ? "+" : "") + Math.round(x * 100) + "%"; },
    xms: function (x) { return Math.round(x) + " ms"; },
    st: function (x) { return Math.round(x) + " st"; },
    db: function (x) { return x <= .001 ? "−∞" : (20 * Math.log10(x / .7)).toFixed(1) + " dB"; }
  };
  function knobSvg(size) {
    const c = size / 2, r = size / 2 - 4, rf = r - 6, flute = rf * 2 * Math.PI / 28;
    return '<svg width="' + size + '" height="' + size + '" viewBox="0 0 ' + size + ' ' + size + '">' +
      '<circle cx="' + c + '" cy="' + c + '" r="' + r + '" fill="none" stroke="var(--knob-track)" stroke-width="4" stroke-dasharray="' + (r * Math.PI * 1.5) + ' 999" transform="rotate(135 ' + c + ' ' + c + ')" stroke-linecap="round"/>' +
      '<circle class="arc k-arc" cx="' + c + '" cy="' + c + '" r="' + r + '" fill="none" stroke-width="4" stroke-linecap="round" transform="rotate(135 ' + c + ' ' + c + ')" stroke-dasharray="0 999"/>' +
      '<circle class="k-skirt" cx="' + c + '" cy="' + c + '" r="' + (rf + 2.5) + '"/>' +
      '<circle class="k-ring" cx="' + c + '" cy="' + c + '" r="' + (rf + 1) + '" transform="rotate(-90 ' + c + ' ' + c + ')"/>' +
      '<circle class="k-face" cx="' + c + '" cy="' + c + '" r="' + rf + '"/>' +
      '<circle class="k-flute" cx="' + c + '" cy="' + c + '" r="' + (rf - 1.6) + '" stroke-dasharray="' + (flute * .5).toFixed(2) + ' ' + (flute * .5).toFixed(2) + '"/>' +
      '<circle class="k-cap" cx="' + c + '" cy="' + c + '" r="' + Math.max(2, r - 12) + '"/>' +
      '<g class="ptr"><line class="k-ptr" x1="' + c + '" y1="' + (c - rf * .35) + '" x2="' + c + '" y2="' + (c - rf + 3) + '"/><circle class="k-dot" cx="' + c + '" cy="' + (c - r + 10) + '" r="2.6"/></g></svg>';
  }
  // Gab's painted knobs: the picture turns with the value. Top-view knobs turn whole; 3/4-view knobs turn only
  // their cap (clipped to the cap ellipse and rotated in its plane), so the body keeps its perspective.
  const KIMG_HTML = '<span class="k-img" aria-hidden="true"><i></i><b><i></i></b></span>';
  function knob(parent, id, label, opts) {
    opts = opts || {};
    const size = opts.size || 42, r = size / 2 - 4, el = document.createElement("div");
    el.className = "knob"; el.title = opts.title || label;
    if (opts.color) el.style.setProperty("--kc", opts.color);
    el.style.setProperty("--ks", size + "px");
    el.innerHTML = '<span class="k-dial">' + knobSvg(size) + KIMG_HTML + '</span><div class="k-label">' + label + '</div><div class="k-value"></div>';
    parent.appendChild(el);
    const arc = $(".arc", el), ptr = $(".ptr", el), val = $(".k-value", el), circ = r * Math.PI * 1.5, c = size / 2;
    const p = function () { return P[id]; };
    const log = opts.curve === "log";
    function toT(x) { const q = p(); if (!q) return 0; if (log) { const lo = Math.max(q.min, 1e-4); return Math.log(Math.max(x, lo) / lo) / Math.log(q.max / lo); } return (x - q.min) / (q.max - q.min); }
    function fromT(t) { const q = p(); t = clamp(t, 0, 1); if (log) { const lo = Math.max(q.min, 1e-4); return lo * Math.pow(q.max / lo, t); } return q.min + t * (q.max - q.min); }
    function draw(x) {
      const t = clamp(toT(x), 0, 1);
      arc.setAttribute("stroke-dasharray", (t * circ) + " 999");
      ptr.setAttribute("transform", "rotate(" + (-135 + t * 270) + " " + c + " " + c + ")");
      el.style.setProperty("--kr", (-135 + t * 270).toFixed(1) + "deg");
      val.textContent = (opts.format || fmt.pct)(x);
    }
    if (opts.owned) draw.owned = opts.owned;
    watch(id, draw);
    let startY = 0, startT = 0;
    el.addEventListener("pointerdown", function (e) {
      if (!p()) return;
      e.preventDefault(); e.stopPropagation(); el.setPointerCapture(e.pointerId);
      startY = e.clientY; startT = toT(v(id)); busy[id] = true; el.classList.add("dragging");
      R.call("setParam", id, null, 1);
    });
    el.addEventListener("pointermove", function (e) {
      if (!busy[id] || !el.hasPointerCapture(e.pointerId)) return;
      setParam(id, fromT(startT + (startY - e.clientY) / S * (e.shiftKey ? 0.0012 : 0.005))); draw(P[id].v);
    });
    function end() { if (!busy[id]) return; busy[id] = false; el.classList.remove("dragging"); R.call("setParam", id, null, 2); tweaked(id); if (opts.onEnd) opts.onEnd(); }
    el.addEventListener("pointerup", end); el.addEventListener("pointercancel", end);
    el.addEventListener("dblclick", function () { if (p()) gesture(id, p().def); });
    el.addEventListener("wheel", function (e) { if (!p()) return; e.preventDefault(); setParam(id, fromT(toT(v(id)) - e.deltaY * 0.0008)); tweaked(id); }, { passive: false });
    return el;
  }
  function knobDefs() {
    const d = document.createElementNS(NS, "svg");
    d.setAttribute("width", 0); d.setAttribute("height", 0); d.style.position = "absolute";
    d.innerHTML = '<defs>' +
      '<radialGradient id="kdCream" cx=".35" cy=".3" r=".8"><stop offset="0" stop-color="#fffaf0"/><stop offset=".6" stop-color="#f1dfbd"/><stop offset="1" stop-color="#c9a77a"/></radialGradient>' +
      '<linearGradient id="kdChrome" x1="0" y1="0" x2="1" y2="1"><stop offset="0" stop-color="#ffffff"/><stop offset=".35" stop-color="#b9bdc7"/><stop offset=".55" stop-color="#eef0f4"/><stop offset="1" stop-color="#6e7380"/></linearGradient>' +
      '<radialGradient id="kdChromeCap" cx=".4" cy=".35" r=".7"><stop offset="0" stop-color="#ffffff" stop-opacity=".9"/><stop offset="1" stop-color="#9aa0ad" stop-opacity=".2"/></radialGradient>' +
      '<radialGradient id="kdRubber" cx=".4" cy=".3" r=".8"><stop offset="0" stop-color="#55505e"/><stop offset="1" stop-color="#1c1a22"/></radialGradient>' +
      '<radialGradient id="kdAmp" cx=".38" cy=".3" r=".85"><stop offset="0" stop-color="#ffc27a"/><stop offset=".55" stop-color="#f07a1f"/><stop offset="1" stop-color="#a8460c"/></radialGradient>' +
      '<radialGradient id="kdAmber" cx=".4" cy=".35" r=".8"><stop offset="0" stop-color="#ffd27a" stop-opacity=".95"/><stop offset=".6" stop-color="#e88a12" stop-opacity=".85"/><stop offset="1" stop-color="#7a3a05" stop-opacity=".95"/></radialGradient>' +
      '<radialGradient id="kdIce" cx=".4" cy=".35" r=".8"><stop offset="0" stop-color="#d6fbff"/><stop offset=".5" stop-color="#3fc8f0" stop-opacity=".85"/><stop offset="1" stop-color="#0b4a7a"/></radialGradient>' +
      '<radialGradient id="kdPurple" cx=".38" cy=".3" r=".85"><stop offset="0" stop-color="#d9b6ff"/><stop offset=".45" stop-color="#8a3cf0"/><stop offset="1" stop-color="#3b1173"/></radialGradient>' +
      '<radialGradient id="kdRed" cx=".38" cy=".3" r=".85"><stop offset="0" stop-color="#ff9a7a"/><stop offset=".5" stop-color="#e5341f"/><stop offset="1" stop-color="#7d1206"/></radialGradient>' +
      '<radialGradient id="kdBakelite" cx=".4" cy=".3" r=".85"><stop offset="0" stop-color="#7a4a35"/><stop offset=".6" stop-color="#3b1d14"/><stop offset="1" stop-color="#1b0b06"/></radialGradient>' +
      '<linearGradient id="kdTeal" x1="0" y1="0" x2="1" y2="1"><stop offset="0" stop-color="#9ff0ee"/><stop offset=".35" stop-color="#138a8f"/><stop offset=".6" stop-color="#5fd0cf"/><stop offset="1" stop-color="#0b4e57"/></linearGradient>' +
      '<radialGradient id="kdCosmos" cx=".45" cy=".4" r=".8"><stop offset="0" stop-color="#4b2c86"/><stop offset=".7" stop-color="#1d1040"/><stop offset="1" stop-color="#0b0618"/></radialGradient>' +
      '<radialGradient id="kdPearl" cx=".35" cy=".3" r=".9"><stop offset="0" stop-color="#ffffff"/><stop offset=".4" stop-color="#f3e3ff"/><stop offset=".7" stop-color="#cfe9ff"/><stop offset="1" stop-color="#e8c9a8"/></radialGradient>' +
      '<radialGradient id="kdNeon" cx=".4" cy=".35" r=".8"><stop offset="0" stop-color="#2a2236"/><stop offset="1" stop-color="#0c0814"/></radialGradient>' +
      '<linearGradient id="kdDual" x1="0" y1="0" x2="1" y2="1"><stop offset="0" stop-color="#ffa02e"/><stop offset="1" stop-color="#ff3fb4"/></linearGradient></defs>';
    document.body.appendChild(d);
  }

  /* ================= segmented controls ================= */
  function seg(selector, attr, read, write) {
    const buttons = $$(selector + " button");
    buttons.forEach(function (b) { b.addEventListener("click", function () { write(Number(b.dataset[attr])); }); });
    return function refresh() { const cur = read(); buttons.forEach(function (b) { b.classList.toggle("on", Number(b.dataset[attr]) === cur); }); };
  }
  function bindSeg(selector, attr, id) { watch(id, seg(selector, attr, function () { return Math.round(v(id)); }, function (x) { gesture(id, x); })); }
  function bindToggle(btn, id, after) {
    btn.addEventListener("click", function () { gesture(id, v(id) > .5 ? 0 : 1); if (after) after(); });
    watch(id, function (x) { btn.classList.toggle("on", x > .5); btn.setAttribute("aria-pressed", x > .5); });
  }

  /* ================= waveform ================= */
  const canvas = $("#wave"), ctx = canvas.getContext("2d"), waveBox = $("#waveBox");
  let view = [0, 1], cols = null, waveDirty = true, dragMarker = null, cue = -1, palette = {}, snap = true;
  function sizeCanvas() {
    if (!waveBox.clientWidth) return;
    const dpr = (window.devicePixelRatio || 1) * S;
    canvas.width = Math.round(waveBox.clientWidth * dpr); canvas.height = Math.round(waveBox.clientHeight * dpr);
    cols = null; waveDirty = true;
  }
  function mode() { const m = Math.round(v("playMode")); return m === 0 && v("loop") > .5 ? 1 : m; }
  function cut(i) { const n = Math.round(v("sliceCount")); if (i <= 0) return 0; if (i >= n) return 1; return v("slice" + i); }
  function cutAbs(i) { return v("start") + (v("end") - v("start")) * cut(i); }
  function buildCols() {
    if (!peaks || !peaks.l) { cols = null; return; }
    const Wd = canvas.width, chans = peaks.stereo && peaks.r ? [peaks.l, peaks.r] : [peaks.l];
    cols = chans.map(function (data) {
      const n = data.length / 2, out = new Float32Array(Wd * 2);
      for (let x = 0; x < Wd; x++) {
        const a = view[0] + (view[1] - view[0]) * x / Wd, b = view[0] + (view[1] - view[0]) * (x + 1) / Wd;
        let i0 = Math.floor(a * n), i1 = Math.max(i0 + 1, Math.ceil(b * n)), lo = 0, hi = 0;
        for (let i = i0; i < i1 && i < n; i++) { lo = Math.min(lo, data[i * 2]); hi = Math.max(hi, data[i * 2 + 1]); }
        out[x * 2] = lo; out[x * 2 + 1] = hi;
      }
      return out;
    });
  }
  function px(norm) { return (norm - view[0]) / (view[1] - view[0]) * canvas.width; }
  function normAt(clientX) { const r = canvas.getBoundingClientRect(); return view[0] + (view[1] - view[0]) * (clientX - r.left) / r.width; }
  function readPalette() { const cs = getComputedStyle(document.body); ["--wave", "--wave2", "--sel", "--loop", "--wave-bg", "--gold", "--cyan", "--violet", "--wave-off"].forEach(function (k) { palette[k] = cs.getPropertyValue(k).trim(); }); waveDirty = true; }
  const LOOPBAR = 22; // px (css) reserved under the wave for the loop bracket
  function drawWave() {
    const Wd = canvas.width, Hd = canvas.height, d = (window.devicePixelRatio || 1) * S, lb = LOOPBAR * d, wh = Hd - lb;
    if (!Wd) return;
    ctx.clearRect(0, 0, Wd, Hd); ctx.fillStyle = palette["--wave-bg"]; ctx.fillRect(0, 0, Wd, Hd);
    if (!sample.loaded) return;
    if (!cols) buildCols();
    const st = v("start"), en = v("end"), m = mode();
    ctx.fillStyle = palette["--sel"]; ctx.fillRect(px(st), 0, px(en) - px(st), wh);
    if (cols) {
      const lanes = cols.length, laneH = wh / lanes;
      cols.forEach(function (c, li) {
        const mid = laneH * li + laneH / 2, amp = laneH * .46;
        const g = ctx.createLinearGradient(0, mid - amp, 0, mid + amp); g.addColorStop(0, li ? palette["--wave2"] : palette["--wave"]); g.addColorStop(.5, "#e9b6ff"); g.addColorStop(1, li ? palette["--wave2"] : palette["--wave"]);
        for (let x = 0; x < Wd; x++) {
          const n = view[0] + (view[1] - view[0]) * x / Wd, inside = n >= st && n <= en;
          ctx.fillStyle = inside ? g : palette["--wave-off"];
          const y0 = mid - c[x * 2 + 1] * amp, y1 = mid - c[x * 2] * amp;
          ctx.fillRect(x, y0, 1, Math.max(1, y1 - y0));
        }
        if (lanes > 1) { ctx.fillStyle = "rgba(255,255,255,.45)"; ctx.font = (9 * d) + "px Menlo, monospace"; ctx.fillText(li ? "R" : "L", 40 * d, laneH * li + 12 * d); }
      });
    }
    // slice lines
    if (m === 4) { ctx.fillStyle = palette["--violet"]; for (let i = 1; i < Math.round(v("sliceCount")); i++) { const x = px(cutAbs(i)); ctx.fillRect(x - .75 * d, 0, 1.5 * d, wh); ctx.font = "600 " + (8.5 * d) + "px sans-serif"; ctx.fillText(noteName(v("root") + i), x + 3 * d, wh - 5 * d); } }
    // S / E markers with tags (reference look)
    const tag = function (x, color, label) {
      ctx.fillStyle = color; ctx.fillRect(x - 1 * d, 0, 2 * d, wh);
      const w = 15 * d, h = 16 * d; x = clamp(x, w / 2 + d, Wd - w / 2 - d); ctx.beginPath(); if (ctx.roundRect) ctx.roundRect(x - w / 2, 2 * d, w, h, 3 * d); else ctx.rect(x - w / 2, 2 * d, w, h); ctx.fill();
      ctx.fillStyle = "#1a0b24"; ctx.font = "700 " + (10 * d) + "px sans-serif"; ctx.textAlign = "center"; ctx.fillText(label, x, 14 * d); ctx.textAlign = "left";
    };
    tag(px(st), palette["--violet"], "S"); tag(px(en), palette["--gold"], "E");
    // loop bracket bar
    const ls = v("loopRegionSet") > .5 ? v("loopStart") : st, le = v("loopRegionSet") > .5 ? v("loopEnd") : en, on = v("regionLoop") > .5;
    const y = wh + lb / 2, xa = px(ls), xb = px(le);
    ctx.strokeStyle = on ? palette["--cyan"] : "rgba(255,255,255,.25)"; ctx.lineWidth = 2.5 * d; ctx.beginPath(); ctx.moveTo(xa, y); ctx.lineTo(xb, y); ctx.stroke();
    if (on) { ctx.fillStyle = palette["--loop"]; ctx.fillRect(xa, 0, xb - xa, wh); }
    [xa, xb].forEach(function (x) { ctx.fillStyle = on ? palette["--cyan"] : "rgba(255,255,255,.35)"; ctx.beginPath(); if (ctx.roundRect) ctx.roundRect(x - 7 * d, y - 8 * d, 14 * d, 16 * d, 3 * d); else ctx.rect(x - 7 * d, y - 8 * d, 14 * d, 16 * d); ctx.fill(); ctx.fillStyle = "#12081c"; ctx.fillRect(x - 2 * d, y - 4 * d, 4 * d, 4 * d); });
    if (cue >= 0) { ctx.fillStyle = "rgba(255,210,122,.8)"; for (let yy = 0; yy < wh; yy += 8 * d) ctx.fillRect(px(cue) - d, yy, 2 * d, 4 * d); }
    if (T.playing || T.preview) { ctx.fillStyle = "#fff"; ctx.fillRect(px(T.pos) - d, 0, 2 * d, wh); }
  }
  function secs(x) { return (x * (sample.seconds || 0)).toFixed(2) + "s"; }
  function markerInfo() {
    let t = "S " + secs(v("start")) + " · E " + secs(v("end"));
    if (v("regionLoop") > .5) t += " · LOOP " + secs(v("loopRegionSet") > .5 ? v("loopStart") : v("start")) + "–" + secs(v("loopRegionSet") > .5 ? v("loopEnd") : v("end"));
    $("#markerInfo").textContent = t;
    $("#waveLen").textContent = (sample.seconds || 0).toFixed(2) + "s";
    $("#cueInfo").textContent = cue >= 0 ? "▶ a partir de " + secs(cue) : "";
  }
  function markers(e) {
    const r = canvas.getBoundingClientRect(), yLocal = (e.clientY - r.top) / r.height * (r.height / S), inLoopBar = yLocal > r.height / S - LOOPBAR;
    const list = [];
    if (inLoopBar) list.push({ id: "loopStart", x: v("loopRegionSet") > .5 ? v("loopStart") : v("start") }, { id: "loopEnd", x: v("loopRegionSet") > .5 ? v("loopEnd") : v("end") });
    else { list.push({ id: "start", x: v("start") }, { id: "end", x: v("end") }); if (mode() === 4) for (let i = 1; i < Math.round(v("sliceCount")); i++) list.push({ id: "slice" + i, x: cutAbs(i), slice: i }); }
    return list;
  }
  function nearMarker(e) {
    const r = canvas.getBoundingClientRect(); let best = null, bd = 14 * S;
    markers(e).forEach(function (mk) { const dx = Math.abs((mk.x - view[0]) / (view[1] - view[0]) * r.width - (e.clientX - r.left)); if (dx < bd) { bd = dx; best = mk; } });
    return best;
  }
  // SNAP: move the marker to the quietest point nearby (uses the waveform overview, never the audio)
  function snapQuiet(x) {
    if (!snap || !peaks || !peaks.l) return x;
    const data = peaks.l, n = data.length / 2, i0 = Math.round(x * n), rad = Math.max(2, Math.round(n * (view[1] - view[0]) * .006));
    let best = i0, bv = 1e9;
    for (let i = Math.max(0, i0 - rad); i <= Math.min(n - 1, i0 + rad); i++) { const a = data[i * 2 + 1] - data[i * 2] + Math.abs(i - i0) / rad * .02; if (a < bv) { bv = a; best = i; } }
    return best / n;
  }
  waveBox.addEventListener("pointerdown", function (e) {
    if (e.target.closest(".wave-nav")) return;
    if (!sample.loaded) { doImport(); return; }
    const best = nearMarker(e);
    if (!best) { cue = clamp(normAt(e.clientX), v("start"), v("end") - .001); markerInfo(); waveDirty = true; if (T.preview) R.call("preview", true, cue); return; }
    dragMarker = best; busy[best.id] = true; waveBox.setPointerCapture(e.pointerId);
    R.call("setParam", best.id, null, 1);
    if (best.id.indexOf("loop") === 0 && v("loopRegionSet") < .5) { gesture("loopStart", v("start")); gesture("loopEnd", v("end")); gesture("loopRegionSet", 1); }
  });
  waveBox.addEventListener("pointermove", function (e) {
    if (!dragMarker) { waveBox.style.cursor = !sample.loaded ? "pointer" : nearMarker(e) ? "ew-resize" : "text"; return; }
    const id = dragMarker.id; let n = clamp(normAt(e.clientX), 0, 1);
    if (!dragMarker.slice) n = snapQuiet(n);
    let x = n;
    if (id === "start") x = Math.min(x, v("end") - .001);
    else if (id === "end") x = Math.max(x, v("start") + .001);
    else if (id === "loopStart") x = Math.min(x, v("loopEnd") - .001);
    else if (id === "loopEnd") x = Math.max(x, v("loopStart") + .001);
    else if (dragMarker.slice) { const i = dragMarker.slice; x = clamp((n - v("start")) / (v("end") - v("start")), cut(i - 1) + .0001, cut(i + 1) - .0001); }
    setParam(id, x); waveDirty = true; markerInfo();
  });
  function endMarker() { if (!dragMarker) return; busy[dragMarker.id] = false; R.call("setParam", dragMarker.id, null, 2); tweaked(dragMarker.id); dragMarker = null; }
  waveBox.addEventListener("pointerup", endMarker); waveBox.addEventListener("pointercancel", endMarker);
  function zoom(kind, at) {
    const span = view[1] - view[0], c = at === undefined ? (view[0] + view[1]) / 2 : at;
    if (kind === "in" || kind === "out") { const s2 = kind === "in" ? Math.max(.002, span / 1.8) : Math.min(1, span * 1.8); view = [c - (c - view[0]) / span * s2, 0]; view[1] = view[0] + s2; }
    if (kind === "all") view = [0, 1];
    if (kind === "sel") { const pad = (v("end") - v("start")) * .06; view = [v("start") - pad, v("end") + pad]; }
    if (kind === "start" || kind === "end") { const x = v(kind), s2 = Math.min(span, .12); view = [x - s2 / 2, x + s2 / 2]; }
    if (kind === "loop") { const a = v("loopRegionSet") > .5 ? v("loopStart") : v("start"), b = v("loopRegionSet") > .5 ? v("loopEnd") : v("end"), pad = (b - a) * .08; view = [a - pad, b + pad]; }
    if (view[0] < 0) { view[1] -= view[0]; view[0] = 0; } if (view[1] > 1) { view[0] -= view[1] - 1; view[1] = 1; }
    view[0] = Math.max(0, view[0]); cols = null; waveDirty = true;
  }
  function scroll(dir) { const span = view[1] - view[0]; view = [clamp(view[0] + dir * span * .5, 0, 1 - span), 0]; view[1] = view[0] + span; cols = null; waveDirty = true; }
  $$("[data-zoom]").forEach(function (b) { b.addEventListener("click", function () { zoom(b.dataset.zoom); }); });
  $("#wavePrev").addEventListener("click", function () { scroll(-1); }); $("#waveNext").addEventListener("click", function () { scroll(1); });
  $$("#markSeg button").forEach(function (b) {
    b.addEventListener("click", function () {
      if (b.dataset.mark === "snap") { snap = !snap; b.classList.toggle("on", snap); toast(snap ? "SNAP: os marcadores grudam no ponto mais silencioso por perto." : "SNAP desligado."); return; }
      zoom(b.dataset.mark);
    });
  });
  waveBox.addEventListener("wheel", function (e) {
    if (!sample.loaded) return; e.preventDefault();
    if (Math.abs(e.deltaX) > Math.abs(e.deltaY)) { const span = view[1] - view[0]; view = [clamp(view[0] + e.deltaX / 600 * span, 0, 1 - span), 0]; view[1] = view[0] + span; cols = null; waveDirty = true; }
    else zoom(e.deltaY < 0 ? "in" : "out", normAt(e.clientX));
  }, { passive: false });
  waveBox.addEventListener("keydown", function (e) {
    if (e.key === "+" || e.key === "=") zoom("in"); else if (e.key === "-") zoom("out"); else if (e.key === "Home") zoom("all"); else if (e.key === "f" || e.key === "F") zoom("sel"); else return;
    e.preventDefault(); e.stopPropagation();
  });

  /* ---- import ---- */
  function importResult(r, silent) {
    if (!r) return;
    if (r.ok) { if (!silent) toast("Sample carregado. Toque no teclado!"); game.award(silent ? 2 : 20, "import"); game.event("import"); }
    else toast(r.message || "Não foi possível importar.", 4500);
  }
  function doImport() { R.call("importDialog").then(function (r) { if (r && r.ok) { patchName = "Sample importado"; current = null; updatePill(); } importResult(r); }); }
  $("#libImport").addEventListener("click", doImport);
  ["dragenter", "dragover"].forEach(function (ev) { document.addEventListener(ev, function (e) { e.preventDefault(); waveBox.classList.add("dragover"); }); });
  ["dragleave", "drop"].forEach(function (ev) { document.addEventListener(ev, function (e) { e.preventDefault(); if (ev === "dragleave" && e.relatedTarget) return; waveBox.classList.remove("dragover"); }); });
  document.addEventListener("drop", function (e) {
    const f = e.dataTransfer && e.dataTransfer.files && e.dataTransfer.files[0];
    if (!f) return;
    if (f.size > 32 * 1024 * 1024) { toast("Arquivo acima de 32 MB."); return; }
    toast("Importando " + f.name + "…", 8000);
    const reader = new FileReader();
    reader.onload = function () { R.call("importData", f.name, String(reader.result).split(",")[1] || "").then(function (r) { if (r && r.ok) { patchName = f.name.replace(/\.[^.]+$/, ""); current = null; updatePill(); } importResult(r); }); };
    reader.readAsDataURL(f);
  });
  function onSample(info) {
    sample = info || sample;
    $("#sampleName").textContent = sample.loaded ? sample.name : "Arraste um áudio aqui";
    waveBox.classList.toggle("loaded", !!sample.loaded);
    peaks = null; cols = null; view = [0, 1]; cue = -1; waveDirty = true; markerInfo(); updatePill();
    if (sample.loaded) R.call("waveform", 4096).then(function (w) { peaks = w && w.l ? w : null; cols = null; waveDirty = true; });
  }

  /* ================= SOUND panels ================= */
  function setupSample() {
    const k = $("#sampleKnobs");
    knob(k, "root", "TUNE", { format: fmt.note, color: "#2fe0ff", title: "RAIZ: nota em que o sample toca na altura original" });
    knob(k, "velocity", "VEL→FILT", { color: "#2fe0ff", title: "Quanto a força da nota abre o filtro" });
    const xf = knob(k, "loopCrossfade", "XFADE", { format: fmt.xms, color: "#b06bff", title: "Crossfade na emenda do LOOP" });
    knob(k, "level", "VOLUME", { format: fmt.db, color: "#ffb340", title: "Volume de saída" });
    watch("regionLoop", function (x) { xf.classList.toggle("disabled", x < .5); });
    const refreshMode = seg("#modeSeg", "mode", mode, function (m) { R.call("playbackMode", m); P.playMode.v = m; P.loop.v = m === 1 ? 1 : 0; refreshMode(); slicesUi(); waveDirty = true; updatePill(); tweaked("playMode"); });
    watch("playMode", function () { refreshMode(); slicesUi(); waveDirty = true; }); watch("loop", refreshMode);
    bindToggle($("#loopBtn"), "regionLoop", function () { waveDirty = true; markerInfo(); });
    watch("regionLoop", function () { waveDirty = true; markerInfo(); });
    bindSeg("#pitchSeg", "pitch", "pitchMode");
    function slicesUi() { $("#sliceCount").textContent = Math.round(v("sliceCount")) + " cortes"; $("#sliceCtl").classList.toggle("off", mode() !== 4); keyboardSlices(); }
    watch("sliceCount", slicesUi);
    $$("[data-slices]").forEach(function (b) { b.addEventListener("click", function () { R.call("slices", Math.round(v("sliceCount")) + Number(b.dataset.slices)); tweaked("sliceCount"); }); });
    ["start", "end", "loopStart", "loopEnd", "loopRegionSet"].forEach(function (id) { watch(id, function () { waveDirty = true; markerInfo(); }); });
    for (let i = 1; i < 16; i++) watch("slice" + i, function () { waveDirty = true; });
  }
  const envSvg = $("#envSvg"), EW = 300, EH = 70, seg1 = 92, seg2 = 82, hold = 40, seg4 = 80;
  function tPos(x, max, w) { const lo = .001; return 6 + (w - 6) * Math.log(Math.max(x, lo) / lo) / Math.log(max / lo); }
  function tFrom(pxv, max, w) { const lo = .001; return lo * Math.pow(max / lo, clamp((pxv - 6) / (w - 6), 0, 1)); }
  function envLive() { return v("synthEnable") > .5 || Math.round(v("triggerMode")) === 2; }
  function drawEnv() {
    const a = tPos(v("ampAttack"), 4, seg1), d = tPos(v("ampDecay"), 4, seg2), s = v("ampSustain"), r = tPos(v("ampRelease"), 8, seg4);
    const y0 = EH - 6, top = 8, sy = y0 - (y0 - top) * s, live = envLive();
    const p1 = [4 + a, top], p2 = [p1[0] + d, sy], p3 = [p2[0] + hold, sy], p4 = [p3[0] + r, y0], col = live ? "#ffb340" : "rgba(255,255,255,.35)";
    envSvg.innerHTML = '<defs><linearGradient id="envG" x1="0" y1="0" x2="0" y2="1"><stop offset="0" stop-color="#ffb340" stop-opacity=".35"/><stop offset="1" stop-color="#ffb340" stop-opacity="0"/></linearGradient></defs>' +
      '<path d="M4 ' + y0 + ' L' + p1 + ' L' + p2 + ' L' + p3 + ' L' + p4 + ' Z" fill="' + (live ? "url(#envG)" : "rgba(255,255,255,.05)") + '"/>' +
      '<path d="M4 ' + y0 + ' L' + p1 + ' L' + p2 + ' L' + p3 + ' L' + p4 + '" fill="none" stroke="' + col + '" stroke-width="2.4" stroke-linejoin="round" vector-effect="non-scaling-stroke" style="filter:drop-shadow(0 0 3px ' + col + ')"/>' +
      [["A", p1], ["D", p2], ["S", p3], ["R", p4]].map(function (q) { return '<circle data-h="' + q[0] + '" cx="' + q[1][0] + '" cy="' + q[1][1] + '" r="4" fill="' + col + '" stroke="#1a0b24" stroke-width="1.5" vector-effect="non-scaling-stroke"/>'; }).join("");
    $("#envNote").textContent = live ? "" : "ADSR inativo: acenda o ponto (motor de synth) ou use o disparo GATE.";
    $$("#envKnobs .knob").forEach(function (kk) { kk.classList.toggle("disabled", !live); });
  }
  function setupEnv() {
    const k = $("#envKnobs");
    knob(k, "ampAttack", "A", { format: fmt.ms, curve: "log", size: 40, color: "#ffb340" });
    knob(k, "ampDecay", "D", { format: fmt.ms, curve: "log", size: 40, color: "#ff4fd8" });
    knob(k, "ampSustain", "S", { size: 40, color: "#b06bff", format: function (x) { return x <= .001 ? "−∞" : (20 * Math.log10(x)).toFixed(1) + " dB"; } });
    knob(k, "ampRelease", "R", { format: fmt.ms, curve: "log", size: 40, color: "#2fe0ff" });
    ["ampAttack", "ampDecay", "ampSustain", "ampRelease", "synthEnable", "triggerMode"].forEach(function (id) { watch(id, drawEnv); });
    bindToggle($("#synthBtn"), "synthEnable");
    let h = null; const ids = function () { return h === "A" ? ["ampAttack"] : h === "D" || h === "S" ? ["ampDecay", "ampSustain"] : ["ampRelease"]; };
    envSvg.addEventListener("pointerdown", function (e) { const t = e.target.getAttribute && e.target.getAttribute("data-h"); if (!t) return; h = t; envSvg.setPointerCapture(e.pointerId); ids().forEach(function (id) { R.call("setParam", id, null, 1); }); });
    envSvg.addEventListener("pointermove", function (e) {
      if (!h) return;
      const r = envSvg.getBoundingClientRect(), x = (e.clientX - r.left) / r.width * EW, y = (e.clientY - r.top) / r.height * EH, a = tPos(v("ampAttack"), 4, seg1);
      if (h === "A") setParam("ampAttack", tFrom(x - 4, 4, seg1));
      if (h === "D" || h === "S") { if (h === "D") setParam("ampDecay", tFrom(x - 4 - a, 4, seg2)); setParam("ampSustain", clamp((EH - 6 - y) / (EH - 14), 0, 1)); }
      if (h === "R") setParam("ampRelease", tFrom(x - 4 - a - tPos(v("ampDecay"), 4, seg2) - hold, 8, seg4));
      drawEnv();
    });
    envSvg.addEventListener("pointerup", function () { if (!h) return; ids().forEach(function (id) { R.call("setParam", id, null, 2); tweaked(id); }); h = null; });
  }
  function drawFilter() {
    const c = Math.log(Math.max(20, v("filterCutoff")) / 20) / Math.log(1000), q = v("filterResonance"), x = 8 + c * 210, live = v("synthEnable") > .5, col = live ? "#b06bff" : "rgba(255,255,255,.35)";
    $("#filterSvg").innerHTML = '<path d="M0 12 H' + (x - 24) + ' C' + (x - 8) + ' 12 ' + (x - 6) + ' ' + (12 - q * 10) + ' ' + x + ' ' + (12 - q * 9) + ' S' + (x + 20) + ' 44 ' + (x + 36) + ' 46 L240 46 L0 46 Z" fill="' + (live ? "rgba(176,107,255,.18)" : "rgba(255,255,255,.04)") + '"/>' +
      '<path d="M0 12 H' + (x - 24) + ' C' + (x - 8) + ' 12 ' + (x - 6) + ' ' + (12 - q * 10) + ' ' + x + ' ' + (12 - q * 9) + ' S' + (x + 20) + ' 44 ' + (x + 36) + ' 46" fill="none" stroke="' + col + '" stroke-width="2.6" vector-effect="non-scaling-stroke" style="filter:drop-shadow(0 0 3px ' + col + ')"/>';
  }
  function drawLfo() {
    const live = v("synthEnable") > .5 && v("lfoDepth") > 0, cyc = clamp(v("lfoRate") / 2, .5, 6), dep = .2 + v("lfoDepth") * .8, col = live ? "#2fe0ff" : "rgba(255,255,255,.35)";
    let d = "M0 23"; for (let i = 0; i <= 120; i++) { const t = i / 120; d += " L" + (t * 240).toFixed(1) + " " + (23 - Math.sin(t * cyc * Math.PI * 2) * 18 * dep).toFixed(1); }
    $("#lfoSvg").innerHTML = '<path d="' + d + '" fill="none" stroke="' + col + '" stroke-width="2.6" vector-effect="non-scaling-stroke" style="filter:drop-shadow(0 0 3px ' + col + ')"/>';
  }
  function setupSynth() {
    const f = $("#filterKnobs");
    knob(f, "filterCutoff", "CUTOFF", { format: fmt.hz, curve: "log", color: "#b06bff", size: 40 });
    knob(f, "filterResonance", "RES", { color: "#b06bff", size: 40, format: function (x) { return x.toFixed(2); } });
    knob(f, "envAmount", "ENV", { format: fmt.bip, color: "#b06bff", size: 40, title: "Quanto o envelope de filtro abre ou fecha" });
    ["filterCutoff", "filterResonance", "synthEnable"].forEach(function (id) { watch(id, drawFilter); });
    const l = $("#lfoKnobs");
    knob(l, "lfoRate", "RATE", { format: fmt.lfo, curve: "log", color: "#2fe0ff", size: 40 });
    knob(l, "lfoDepth", "DEPTH", { color: "#2fe0ff", size: 40 });
    ["lfoRate", "lfoDepth", "synthEnable"].forEach(function (id) { watch(id, drawLfo); });
    bindSeg("#lfoTargetSeg", "lfo", "lfoTarget");
    const flRefresh = seg("#flTabs", "fl", function () { return flPage; }, function (x) { flPage = x; flRefresh(); $$(".fl-page").forEach(function (pg) { pg.hidden = Number(pg.dataset.fl) !== x; }); });
    let flPage = 0; flRefresh();
    watch("synthEnable", function (x) { $$("#filterPanel .knob").forEach(function (kk) { kk.classList.toggle("disabled", x < .5); }); $("#filterPanel .lv").classList.toggle("on", x > .5); });
    bindSeg("#voiceSeg", "voice", "voiceMode"); bindSeg("#trigSeg", "trig", "triggerMode");
    const vk = $("#voiceKnobs");
    const g = knob(vk, "glide", "GLIDE", { size: 36, color: "#2fe0ff", format: function (x) { return Math.round(x * 1000) + " ms"; }, title: "Portamento em Mono/Legato" });
    knob(vk, "bendRange", "BEND", { size: 36, color: "#2fe0ff", format: fmt.st, title: "Alcance do pitch bend" });
    watch("voiceMode", function (x) { g.classList.toggle("disabled", Math.round(x) === 0); });
    ["voiceMode", "playMode", "loop", "regionLoop"].forEach(function (id) { watch(id, updatePill); });
    const m = $("#masterKnobs");
    knob(m, "drywet", "ORIG/RACK", { size: 36, color: "#ff4fd8", title: "Mistura entre o som original e o rack" });
  }

  /* ================= XY pad & macros ================= */
  function setupPad() {
    const pad = $("#pad"), dot = $("#padDot"), trail = $("#padTrail");
    let dragging = false;
    function place() {
      dot.style.left = (v("playX") * 100) + "%"; dot.style.top = ((1 - v("playY")) * 100) + "%";
      $("#xyXVal").textContent = "X " + v("playX").toFixed(2); $("#xyYVal").innerHTML = "Y<br>" + v("playY").toFixed(2);
      const pd = $("#palcoDot"); pd.style.left = (v("playX") * 100) + "%"; pd.style.top = ((1 - v("playY")) * 100) + "%";
    }
    function note() {
      const wob = v("wOn") > .5 && v("wPresent") > .5, deg = ["c", "d", "k"].some(function (m) { return v(m + "On") > .5 && v(m + "Present") > .5; }), msg = [];
      if (!wob) msg.push("X sem efeito: WOBBLE desligado."); if (!deg) msg.push("Y sem efeito: CRUMBLE, DUST e CRUSH desligados.");
      $("#padNote").textContent = msg.length ? msg.join(" ") : "X: estável → WOBBLE. Y: limpo → degradado (CRUMBLE, DUST, CRUSH). Duplo clique centraliza.";
      $("#padInfo").textContent = msg.length ? "destinos desligados" : "";
    }
    watch("playX", place); watch("playY", place);
    ["wOn", "wPresent", "cOn", "cPresent", "dOn", "dPresent", "kOn", "kPresent"].forEach(function (id) { watch(id, note); });
    function bindPad(el, withTrail) {
      function at(e) { const r = el.getBoundingClientRect(); setParam("playX", clamp((e.clientX - r.left) / r.width, 0, 1)); setParam("playY", clamp(1 - (e.clientY - r.top) / r.height, 0, 1)); place(); }
      el.addEventListener("pointerdown", function (e) { dragging = true; busy.playX = busy.playY = true; el.setPointerCapture(e.pointerId); R.call("setParam", "playX", null, 1); R.call("setParam", "playY", null, 1); at(e); });
      el.addEventListener("pointermove", function (e) {
        if (!dragging) return; at(e);
        if (!withTrail) return;
        const s = document.createElement("span"); s.style.left = dot.style.left; s.style.top = dot.style.top; trail.appendChild(s);
        if (s.animate) s.animate([{ opacity: .9, transform: "scale(1)" }, { opacity: 0, transform: "scale(.2)" }], { duration: 600 }).onfinish = function () { s.remove(); }; else setTimeout(function () { s.remove(); }, 600);
      });
      function end() { if (!dragging) return; dragging = false; busy.playX = busy.playY = false; R.call("setParam", "playX", null, 2); R.call("setParam", "playY", null, 2); tweaked("playX"); }
      el.addEventListener("pointerup", end); el.addEventListener("pointercancel", end);
      el.addEventListener("dblclick", function () { gesture("playX", .5); gesture("playY", .5); });
    }
    bindPad(pad, true); bindPad($("#palcoPad"), false);
    const m = $("#macroKnobs");
    knob(m, "presence", "PRESENÇA", { size: 50, color: "#ffb340" });
    knob(m, "trail", "RASTRO", { size: 50, color: "#ff4fd8" });
    knob(m, "mutation", "VARIAÇÃO", { size: 50, color: "#2fe0ff", title: "Quanto o VARIAR FX muda os efeitos; também deixa o CRUMBLE mais imprevisível" });
    knob(m, "drywet", "MIX", { size: 50, color: "#b06bff", title: "Mistura entre o som original e o rack" });
    knob($("#palcoK1"), "presence", "PRESENÇA", { size: 46, color: "#2fe0ff" });
    knob($("#palcoK2"), "trail", "RASTRO", { size: 46, color: "#ffb340" });
    $("#palcoIc1").innerHTML = ICO.modViz(3); $("#palcoIc2").innerHTML = ICO.modViz(4);
  }

  /* ================= rack ================= */
  const MOD = [
    { k: "g", name: "Ghost", sub: "Delay · Echo", say: "Delay: repete o som como um eco que vai escurecendo.", sayEn: "Echoes the sound, each repeat darker.", color: "#b06bff", ctl: ["T", "gFeedback", "gTone"], labels: ["TEMPO", "FEEDBACK", "TOM"], slot: 0, look: "rastros violeta" },
    { k: "c", name: "Crumble", sub: "Stutter · Granular", say: "Stutter: picota o som em pedacinhos que se repetem.", sayEn: "Chops the sound into repeating crumbs.", color: "#ffa02e", ctl: ["T", "cVariation"], labels: ["TAXA", "VARIAÇÃO"], slot: 1, look: "fragmentos âmbar" },
    { k: "b", name: "Bloom", sub: "Reverb", say: "Reverb: abre uma cauda, como um espaço. DIFUSÃO alisa as repetições.", sayEn: "Reverb: opens a tail, like a room. DIFFUSION smooths the echoes.", color: "#ff4fd8", ctl: ["bSize", "bTone", "bDecay", "bDiffusion"], labels: ["TAMANHO", "TOM", "DECAY", "DIFUSÃO"], look: "aura rosa" },
    { k: "w", name: "Wobble", sub: "Chorus · Vibrato", say: "Chorus/vibrato: a afinação balança como fita gasta. LARGURA abre o estéreo.", sayEn: "Chorus/vibrato: the pitch wobbles like worn tape. WIDTH opens the stereo.", color: "#2fe0ff", ctl: ["wDepth", "T", "wWidth"], labels: ["PROFUND.", "TAXA", "LARGURA"], slot: 2, look: "contorno ciano" },
    { k: "d", name: "Dust", sub: "Noise · Vinil", say: "Noise: adiciona poeira e chiado que seguem o som.", sayEn: "Adds dust and hiss that follow the sound.", color: "#ffd23f", ctl: ["dLevel", "dCharacter"], labels: ["NÍVEL", "CARÁTER"], look: "partículas douradas" },
    { k: "k", name: "Crush", sub: "Bitcrusher", say: "Bitcrusher: reduz bits e taxa, som digital e áspero.", sayEn: "Lowers bits and rate: gritty digital sound.", color: "#8a4dff", ctl: ["kAmount"], labels: ["INTENS."], look: "pixels roxos" }
  ];
  const timeIds = [["gTime", "gFreeMs", "gDivision"], ["cRate", "cFreeHz", "cDivision"], ["wRate", "wFreeHz", "wDivision"]], timingIds = ["gTiming", "cTiming", "wTiming"], DIV = ["1/4", "1/8", "1/8.", "1/8T"];
  function timeParam(mi) { const slot = MOD[mi].slot; return timeIds[slot][Math.round(v(timingIds[slot]))]; }
  function formatFor(id) { if (/Division$/.test(id)) return function (x) { return DIV[Math.round(x)] || ""; }; if (id === "gFreeMs") return fmt.xms; if (/FreeHz$/.test(id)) return fmt.lfo; return fmt.pct; }
  function hexA(hex, a) { const n = parseInt(hex.slice(1), 16); return "rgba(" + (n >> 16 & 255) + "," + (n >> 8 & 255) + "," + (n & 255) + "," + a + ")"; }
  let cards = [], selMod = 0;
  function dropOwned(tag) { for (const k in views) views[k] = views[k].filter(function (f) { return f.owned !== tag; }); }
  function modOn(i) { return v(MOD[i].k + "On") > .5 && v(MOD[i].k + "Present") > .5; }
  function renderRack() {
    dropOwned("rack"); cards = [];
    const list = $("#rackList"); list.innerHTML = "";
    const sc = screen();
    if (sc !== "limpo" && sc !== "perform") return;
    const tiles = sc === "limpo";
    list.className = "rack " + (tiles ? "tiles" : "rows");
    order.forEach(function (mi, pos) {
      const M = MOD[mi];
      if (v(M.k + "Present") < .5) return;
      const card = document.createElement("div");
      card.className = "mod" + (selMod === mi && tiles && document.body.dataset.etab === "fx" ? " sel" : "");
      card.style.setProperty("--mc", M.color); card.style.setProperty("--mcb", hexA(M.color, .55)); card.style.setProperty("--mcg", hexA(M.color, .35)); card.style.setProperty("--mcg2", hexA(M.color, .12));
      card.dataset.pos = pos; card.title = M.say + "\n" + M.sayEn + "\nNo personagem: " + M.look;
      card.innerHTML = (tiles ? '<i class="grip" data-ic="drag"></i><i class="grip b" data-ic="drag"></i>' : '<i class="grip" data-ic="drag"></i>') + ICO.modIcon(mi) +
        '<div class="mod-text"><div class="mod-name">' + M.name + '</div><div class="mod-sub">' + M.sub + '</div></div>' + ICO.modViz(mi) +
        '<button class="pwr" data-ic="power" title="Ligar / bypass" aria-label="Ligar ' + M.name + '"></button>' + (tiles ? "" : '<button class="chev" data-ic="down" title="Abrir controles"></button>') + '<div class="mod-meter"></div>';
      list.appendChild(card);
      ICO.hydrate(card);
      const power = $(".pwr", card), onId = M.k + "On";
      const pw = function (x) { card.classList.toggle("on", x > .5); card.classList.toggle("off", x < .5); }; pw.owned = "rack"; watch(onId, pw);
      power.addEventListener("click", function (e) { e.stopPropagation(); gesture(onId, v(onId) > .5 ? 0 : 1); toast(M.name + (v(onId) > .5 ? " ligado. No personagem: " + M.look + "." : " em bypass.")); });
      card.addEventListener("click", function (e) { if (e.target.closest(".pwr")) return; if (moved) return; selMod = mi; if (screen() === "limpo") setEtab("fx"); else { setScreen("limpo"); setEtab("fx"); } renderRack(); });
      let moved = false;
      dragCard(card, pos, function (m) { moved = m; });
      cards.push({ el: card, mi: mi, meter: $(".mod-meter", card), viz: $(".mod-viz", card) });
    });
    $("#addModule").disabled = MOD.every(function (M) { return v(M.k + "Present") > .5; });
    renderLegend();
  }
  function renderFxDetail() {
    dropOwned("fx");
    const M = MOD[selMod], body = $("#fxBody"); body.innerHTML = "";
    $("#fxTitle").innerHTML = ""; $("#fxTitle").insertAdjacentHTML("beforeend", '<span style="color:' + M.color + ';text-shadow:0 0 8px ' + M.color + '">' + M.name.toUpperCase() + '</span> <small class="fx-kind">· ' + M.sub + '</small>');
    $("#fxSay").textContent = M.say;
    const row = document.createElement("div"); row.className = "knob-row"; body.appendChild(row);
    M.ctl.forEach(function (c, i) { const id = c === "T" ? timeParam(selMod) : c; knob(row, id, M.labels[i], { size: 52, color: M.color, format: formatFor(id), owned: "fx" }); });
    knob(row, M.k + "Mix", "MIX", { size: 52, color: M.color, owned: "fx" });
    const tools = document.createElement("div"); tools.className = "fx-tools"; body.appendChild(tools);
    const on = document.createElement("button"); on.className = "chip-btn"; tools.appendChild(on);
    const onf = function (x) { on.textContent = x > .5 ? "LIGADO" : "BYPASS"; on.classList.toggle("on", x > .5); }; onf.owned = "fx"; watch(M.k + "On", onf);
    on.addEventListener("click", function () { gesture(M.k + "On", v(M.k + "On") > .5 ? 0 : 1); });
    if (M.slot !== undefined) {
      const tid = timingIds[M.slot];
      [[1, "FREE"], [2, "SYNC"]].forEach(function (o) { const b = document.createElement("button"); b.className = "chip-btn"; b.textContent = o[1]; const f = function (x) { b.classList.toggle("on", Math.round(x) === o[0]); }; f.owned = "fx"; watch(tid, f); b.addEventListener("click", function () { gesture(tid, o[0]); renderFxDetail(); }); tools.appendChild(b); });
    }
    const lid = M.k + "JamLock";
    if (P[lid]) { const b = document.createElement("button"); b.className = "chip-btn"; b.title = "Travado: o VARIAR FX não mexe neste módulo"; const f = function (x) { b.textContent = x > .5 ? "🔒 VARIAR TRAVADO" : "VARIAR LIVRE"; b.classList.toggle("on", x > .5); }; f.owned = "fx"; watch(lid, f); b.addEventListener("click", function () { gesture(lid, v(lid) > .5 ? 0 : 1); }); tools.appendChild(b); }
    const rm = document.createElement("button"); rm.className = "chip-btn"; rm.textContent = "REMOVER DO RACK"; rm.addEventListener("click", function () { gesture(M.k + "Present", 0); renderRack(); }); tools.appendChild(rm);
    const look2 = document.createElement("span"); look2.className = "hint"; look2.style.marginLeft = "auto"; look2.textContent = "no personagem: " + M.look; tools.appendChild(look2);
  }
  function dragCard(card, pos, setMoved) {
    let sx = 0, sy = 0, moved = false, dragging = false;
    card.addEventListener("pointerdown", function (e) { if (e.target.closest(".knob, .pwr, .chev")) return; sx = e.clientX; sy = e.clientY; moved = false; setMoved(false); dragging = true; card.setPointerCapture(e.pointerId); });
    card.addEventListener("pointermove", function (e) {
      if (!dragging) return;
      const dx = (e.clientX - sx) / S, dy = (e.clientY - sy) / S;
      if (Math.abs(dy) + Math.abs(dx) > 6) { moved = true; setMoved(true); card.classList.add("dragging"); }
      if (moved) card.style.transform = "translate(" + dx + "px," + dy + "px)";
    });
    card.addEventListener("pointerup", function (e) {
      if (!dragging) return; dragging = false; card.style.transform = ""; card.classList.remove("dragging");
      if (!moved) return;
      setTimeout(function () { setMoved(false); }, 0);
      const visible = cards.map(function (c) { return c.el; });
      let target = visible.length - 1, best = 1e9;
      visible.forEach(function (el, i) { const r = el.getBoundingClientRect(), d = Math.hypot(e.clientX - (r.left + r.width / 2), e.clientY - (r.top + r.height / 2)); if (d < best) { best = d; target = i; } });
      const toPos = Number(visible[target].dataset.pos);
      if (toPos !== pos) { R.call("moveModule", pos, toPos); const m = order.splice(pos, 1)[0]; order.splice(toPos, 0, m); renderRack(); game.award(3, "rack"); game.event("reorder"); toast(v("rackParallel") > .5 ? "Ordem trocada. Em PARALLEL a ordem não muda o som." : "Nova ordem: o som passa pelos módulos nessa sequência."); }
    });
  }
  $("#addModule").addEventListener("click", function (e) {
    const m = $("#addMenu"); m.innerHTML = "";
    MOD.forEach(function (M) { if (v(M.k + "Present") > .5) return; const b = document.createElement("button"); b.innerHTML = "<b>+ " + M.name + "</b>"; b.addEventListener("click", function () { gesture(M.k + "Present", 1); m.hidden = true; renderRack(); }); m.appendChild(b); });
    const r = e.currentTarget.getBoundingClientRect(), ar = $("#app").getBoundingClientRect();
    m.style.left = ((r.left - ar.left) / S - 180) + "px"; m.style.top = ((r.bottom - ar.top) / S + 6) + "px"; m.style.right = "auto"; m.hidden = false; e.stopPropagation();
  });
  $("#resetFx").addEventListener("click", function () { R.call("resetEffects"); toast("Efeitos de volta ao padrão (desligados)."); });
  function setupTopology() {
    const sw = $("#parallelSw");
    sw.addEventListener("change", function () { gesture("rackParallel", sw.checked ? 1 : 0); if (sw.checked) game.event("parallel"); toast(sw.checked ? "PARALLEL: cada módulo recebe o som original; as diferenças são somadas." : "SERIAL: um módulo alimenta o próximo; a ordem muda o som."); });
    watch("rackParallel", function (x) { sw.checked = x > .5; $("#topoHint").textContent = x > .5 ? "PARALLEL: a ordem não muda o som." : "SERIAL: a ordem é a ordem do som."; });
  }
  let legendEls = [];
  function renderLegend() {
    const box = $("#fxLegend"); box.innerHTML = screen() === "perform" ? "<b>EFFECT COLOR MAP</b>" : "";
    legendEls = MOD.map(function (m, i) { const s = document.createElement("span"); s.title = m.name + ": " + m.look; s.innerHTML = '<i style="color:' + m.color + '"></i>' + m.name.toUpperCase(); box.appendChild(s); return s; });
  }

  /* ================= keyboard ================= */
  let base = 48;
  const keyEls = {}, kb = $("#keyboard");
  function buildKeys() {
    if (!kb.clientWidth) return;
    kb.innerHTML = "";
    for (const k in keyEls) delete keyEls[k];
    const whites = [], Wd = kb.clientWidth, n1 = base + 48;
    for (let n = base; n < n1; n++) if ([1, 3, 6, 8, 10].indexOf(n % 12) < 0) whites.push(n);
    const ww = Wd / whites.length;
    whites.forEach(function (n, i) { const k = document.createElement("div"); k.className = "key w"; k.style.left = (i * ww) + "px"; k.style.width = ww + "px"; k.dataset.n = n; if (n % 12 === 0) k.innerHTML = "<span>" + noteName(n) + "</span>"; kb.appendChild(k); keyEls[n] = k; });
    for (let n = base; n < n1; n++) if ([1, 3, 6, 8, 10].indexOf(n % 12) >= 0) { const i = whites.indexOf(n - 1), k = document.createElement("div"); k.className = "key b"; k.style.left = ((i + 1) * ww - ww * .3) + "px"; k.style.width = (ww * .6) + "px"; k.dataset.n = n; kb.appendChild(k); keyEls[n] = k; }
    $("#octLabel").textContent = noteName(base);
    keyboardSlices();
  }
  function keyboardSlices() { const sl = mode() === 4, r = Math.round(v("root")), n = Math.round(v("sliceCount")); for (const k in keyEls) keyEls[k].classList.toggle("slice", sl && k >= r && k < r + n); }
  const down = {};
  function play(n, vel, on) { if (on) { if (down[n]) return; down[n] = true; } else { if (!down[n]) return; delete down[n]; } R.call("note", n, vel, on); }
  let mouseNote = -1;
  function keyAt(e) { let el = document.elementFromPoint(e.clientX, e.clientY); if (el && !el.classList.contains("key") && el.parentNode && el.parentNode.classList && el.parentNode.classList.contains("key")) el = el.parentNode; return el && el.classList.contains("key") ? Number(el.dataset.n) : -1; }
  function velAt(e, n) { const el = keyEls[n]; if (!el) return .8; const r = el.getBoundingClientRect(); return clamp(.35 + (e.clientY - r.top) / r.height * .65, .2, 1); }
  kb.addEventListener("pointerdown", function (e) { kb.setPointerCapture(e.pointerId); const n = keyAt(e); if (n >= 0) { mouseNote = n; play(n, velAt(e, n), true); } });
  kb.addEventListener("pointermove", function (e) { if (mouseNote < 0) return; const n = keyAt(e); if (n >= 0 && n !== mouseNote) { play(mouseNote, 0, false); mouseNote = n; play(n, velAt(e, n), true); } });
  function kbUp() { if (mouseNote >= 0) play(mouseNote, 0, false); mouseNote = -1; }
  kb.addEventListener("pointerup", kbUp); kb.addEventListener("pointercancel", kbUp);
  $("#octDown").addEventListener("click", function () { base = Math.max(12, base - 12); buildKeys(); });
  $("#octUp").addEventListener("click", function () { base = Math.min(72, base + 12); buildKeys(); });
  const KEYMAP = { a: 0, w: 1, s: 2, e: 3, d: 4, f: 5, t: 6, g: 7, y: 8, h: 9, u: 10, j: 11, k: 12, o: 13, l: 14 }, keyNote = {};
  document.addEventListener("keydown", function (e) {
    if (e.metaKey || e.ctrlKey || e.altKey || e.repeat || (e.target && /INPUT|TEXTAREA/.test(e.target.tagName))) return;
    const k = e.key.toLowerCase();
    if (k === "z") { base = Math.max(12, base - 12); buildKeys(); return; }
    if (k === "x") { base = Math.min(72, base + 12); buildKeys(); return; }
    if (k === " " && sample.loaded) { e.preventDefault(); if (T.preview) $("#tPause").click(); else $("#tPlay").click(); return; }
    if (KEYMAP[k] === undefined) return;
    const n = base + 12 + KEYMAP[k]; keyNote[k] = n; play(n, .85, true);
  });
  document.addEventListener("keyup", function (e) { const k = e.key.toLowerCase(); if (keyNote[k] !== undefined) { play(keyNote[k], 0, false); delete keyNote[k]; } });
  window.addEventListener("blur", function () { Object.keys(down).forEach(function (n) { play(Number(n), 0, false); }); });

  /* ================= library (factory sounds, my patches, takes, favorites) ================= */
  let libTab = "sons", libCat = "all", current = null, userPatches = [], takes = [];
  function favs() { return game.state.favs || (game.state.favs = []); }
  function isFav(key) { return favs().indexOf(key) >= 0; }
  function toggleFav(key) { const f = favs(), i = f.indexOf(key); if (i >= 0) f.splice(i, 1); else f.push(key); game.save(); renderLibrary(); updatePill(); }
  function libItems() {
    const q = ($("#libSearch").value || "").toLowerCase();
    let items = [];
    if (libTab === "sons" || libTab === "favs") items = items.concat(FAC.SOUNDS.map(function (s) { return { key: "f:" + s.id, name: s.name, cat: s.cat, tags: s.tags, kind: "factory", id: s.id }; }));
    if (libTab === "patches" || libTab === "favs") items = items.concat(userPatches.map(function (p) { return { key: "p:" + p.path, name: p.name, cat: "mine", tags: p.folder, kind: "patch", path: p.path }; }));
    if (libTab === "favs") items = items.filter(function (it) { return isFav(it.key); });
    if (libTab === "sons" && libCat !== "all") items = items.filter(function (it) { return it.cat === libCat; });
    if (q) items = items.filter(function (it) { return (it.name + " " + it.tags).toLowerCase().indexOf(q) >= 0; });
    return items;
  }
  function renderLibrary() {
    $$("#navTabs button").forEach(function (b) { b.classList.toggle("on", screen() === "perform" && b.dataset.nav === libTab); });
    const cats = $("#libCats"); cats.innerHTML = "";
    if (libTab === "sons") {
      FAC.CATS.forEach(function (c) {
        const n = c[0] === "all" ? FAC.SOUNDS.length : FAC.SOUNDS.filter(function (s) { return s.cat === c[0]; }).length;
        const li = document.createElement("li"); li.className = libCat === c[0] ? "on" : ""; li.innerHTML = "<span>" + c[1] + "</span><small>" + n + "</small>";
        li.addEventListener("click", function () { libCat = c[0]; renderLibrary(); }); cats.appendChild(li);
      });
    } else {
      const li = document.createElement("li"); li.className = "on"; li.innerHTML = "<span>" + (libTab === "patches" ? "MEUS PATCHES" : "FAVORITOS") + "</span><small>" + libItems().length + "</small>"; cats.appendChild(li);
      const hint = document.createElement("li"); hint.className = "lib-empty"; hint.style.cursor = "default";
      hint.textContent = libTab === "patches" ? "Pasta: Música › RASTRO › Patches. O SAVE salva lá." : "Toque no coração para guardar aqui."; cats.appendChild(hint);
    }
    const list = $("#libList"); list.innerHTML = "";
    const items = libItems();
    if (!items.length) { list.innerHTML = '<li class="lib-empty" style="height:auto;cursor:default">' + (libTab === "patches" ? "Nenhum patch salvo ainda. Use SAVE: o patch leva o áudio dentro e aparece aqui." : libTab === "favs" ? "Nenhum favorito ainda." : "Nada encontrado.") + "</li>"; return; }
    items.forEach(function (it) {
      const li = document.createElement("li"); li.className = current && current.key === it.key ? "on" : "";
      li.innerHTML = '<span class="pl" data-ic="play"></span><b>' + it.name + '</b><span class="bars" data-ic="wave"></span><button class="hrt' + (isFav(it.key) ? " on" : "") + '" data-ic="' + (isFav(it.key) ? "heartFill" : "heart") + '" aria-label="Favoritar"></button>';
      ICO.hydrate(li);
      li.addEventListener("click", function (e) { if (e.target.closest(".hrt")) { toggleFav(it.key); return; } loadItem(it); });
      list.appendChild(li);
    });
  }
  $("#libSearch").addEventListener("input", renderLibrary);
  function refreshUserPatches() { return R.call("libraryList").then(function (l) { userPatches = l || []; if (screen() === "perform") renderLibrary(); if (screen() === "lab") renderLabLib(); }); }
  function loadItem(it) {
    if (it.kind === "factory") {
      toast("Carregando " + it.name + "…", 1500);
      setTimeout(function () {
        const b64 = FAC.render(it.id);
        R.call("importData", it.name + ".wav", b64).then(function (r) {
          if (r && r.ok) { current = it; patchName = it.name; updatePill(); renderLibrary(); if (screen() === "lab") renderLabScreen(); setTimeout(function () { audition(); }, 120); importResult(r, true); }
          else importResult(r);
        });
      }, 20);
    } else if (it.kind === "patch") {
      R.call("libraryLoad", it.path, true).then(function (ok) { if (ok) { current = it; patchName = it.name; updatePill(); renderLibrary(); toast("Patch aberto: " + it.name + " (só o som)."); setTimeout(audition, 200); } else toast("Não consegui abrir esse patch."); });
    }
  }
  function audition() { const root = Math.round(v("root")); R.call("note", root, .8, true); setTimeout(function () { R.call("note", root, 0, false); }, 600); }
  function stepPreset(dir) {
    const items = libItems().length ? libItems() : FAC.SOUNDS.map(function (s) { return { key: "f:" + s.id, name: s.name, kind: "factory", id: s.id, cat: s.cat, tags: s.tags }; });
    let i = current ? items.map(function (x) { return x.key; }).indexOf(current.key) : -1;
    i = (i + dir + items.length) % items.length; loadItem(items[i]);
  }
  $("#presetPrev").addEventListener("click", function () { stepPreset(-1); });
  $("#presetNext").addEventListener("click", function () { stepPreset(1); });
  $("#presetFav").addEventListener("click", function () { if (current) toggleFav(current.key); else toast("Escolha um som da biblioteca ou salve o patch para favoritar."); });
  $("#sampleFav").addEventListener("click", function () { $("#presetFav").click(); });
  $$("#navTabs button").forEach(function (b) {
    b.addEventListener("click", function () {
      const n = b.dataset.nav;
      if (n === "locker") { setScreen("locker"); return; }
      if (n === "lab") { setScreen("lab"); return; }
      libTab = n; if (n === "patches") refreshUserPatches();
      if (screen() !== "perform") setScreen("perform"); else renderLibrary();
    });
  });

  /* ================= patch name, A/B, files, dice ================= */
  let patchName = "Novo patch";
  const MODES = ["One Shot", "Loop", "Reverse", "Ping-Pong", "Slice"], VOICES = ["Poly", "Mono", "Legato"];
  const ab = { cur: "A", A: null, B: null, nameA: "", nameB: "" };
  function updatePill() {
    $("#patchName").textContent = patchName + (ab.cur === "B" ? " · B" : "");
    $("#patchSub").textContent = (sample.loaded ? sample.name : "sem sample") + " · " + MODES[mode()] + " · " + VOICES[Math.round(v("voiceMode"))] + (v("regionLoop") > .5 ? " · LOOP" : "");
    const fv = current && isFav(current.key);
    $("#presetFav").classList.toggle("on", !!fv); $("#presetFav").innerHTML = ICO.I[fv ? "heartFill" : "heart"];
  }
  function switchAB(to) {
    if (to === ab.cur) return;
    ab[ab.cur] = snapshot(); ab["name" + ab.cur] = patchName;
    if (!ab[to]) { ab[to] = snapshot(); ab["name" + to] = patchName; } else { restore(ab[to]); patchName = ab["name" + to] || patchName; }
    ab.cur = to; $("#abA").classList.toggle("on", to === "A"); $("#abB").classList.toggle("on", to === "B"); updatePill(); renderCompare();
    toast(to === "B" && !ab.seen ? "B começou como cópia de A. Mexa à vontade e alterne para comparar." : "Ouvindo " + to + ".");
    if (to === "B") ab.seen = true;
  }
  $("#abA").addEventListener("click", function () { switchAB("A"); });
  $("#abB").addEventListener("click", function () { switchAB("B"); });
  $("#abSwap").addEventListener("click", function () { switchAB(ab.cur === "A" ? "B" : "A"); });
  $("#abCompare").addEventListener("click", function () { switchAB(ab.cur === "A" ? "B" : "A"); setTimeout(audition, 80); });
  function fileAction(save, soundOnly) {
    R.call("patchFile", save, soundOnly).then(function (r) {
      if (!r) return;
      if (!r.ok) { toast("Falha no arquivo (limite 64 MB)."); $("#fileStatus").textContent = "Falha no arquivo."; return; }
      patchName = r.name || patchName; updatePill();
      const msg = save ? "Patch salvo: " + r.name + " (na sua biblioteca)" : "Patch aberto: " + r.name + (soundOnly ? " (somente som)" : " (completo)");
      toast(msg); $("#fileStatus").textContent = msg; if (save) { game.award(10, "save"); refreshUserPatches(); }
    });
  }
  $("#saveBtn").addEventListener("click", function () { fileAction(true); });
  $("#patchSave").addEventListener("click", function () { fileAction(true); });
  $("#patchOpenSound").addEventListener("click", function () { fileAction(false, true); });
  $("#patchOpenFull").addEventListener("click", function () { fileAction(false, false); });
  const DICE_IDS = ["synthEnable", "ampAttack", "ampDecay", "ampSustain", "ampRelease", "filterCutoff", "filterResonance", "envAmount", "lfoRate", "lfoDepth", "lfoTarget", "voiceMode", "glide",
    "gOn", "cOn", "bOn", "wOn", "dOn", "kOn", "gMix", "cMix", "bMix", "wMix", "dMix", "kMix", "gFeedback", "gTone", "cVariation", "bSize", "bTone", "bDecay", "wDepth", "dLevel", "dCharacter", "kAmount", "rackParallel"];
  let diceArmed = false;
  function rollDice() {
    if (!diceArmed) { if (ab.cur !== "A") switchAB("A"); ab.A = snapshot(); ab.nameA = patchName; }
    const rnd = Math.random, pick = function (a) { return a[(rnd() * a.length) | 0]; };
    const val = {
      synthEnable: rnd() < .8 ? 1 : 0, ampAttack: Math.pow(rnd(), 3) * 1.2 + .001, ampDecay: .05 + rnd() * 1.6, ampSustain: rnd(), ampRelease: .05 + Math.pow(rnd(), 2) * 3,
      filterCutoff: 200 * Math.pow(80, rnd()), filterResonance: rnd() * .8, envAmount: rnd() * 1.6 - .6, lfoRate: .1 * Math.pow(80, rnd()), lfoDepth: rnd() < .5 ? 0 : rnd() * .6, lfoTarget: rnd() < .7 ? 0 : 1,
      voiceMode: pick([0, 0, 0, 1, 2]), glide: rnd() < .6 ? 0 : rnd() * .4, rackParallel: rnd() < .3 ? 1 : 0
    };
    ["g", "c", "b", "w", "d", "k"].forEach(function (m) { val[m + "On"] = rnd() < .5 ? 1 : 0; val[m + "Mix"] = .15 + rnd() * .6; });
    ["gFeedback", "gTone", "cVariation", "bSize", "bTone", "bDecay", "wDepth", "dLevel", "dCharacter", "kAmount"].forEach(function (id) { val[id] = .1 + rnd() * .8; });
    ab.B = null; ab.cur = "B"; $("#abA").classList.remove("on"); $("#abB").classList.add("on");
    quietXP = true; history.hold = true; DICE_IDS.forEach(function (id) { if (P[id] && val[id] !== undefined) gesture(id, val[id]); }); history.hold = false; quietXP = false; history.mark();
    diceArmed = true; $("#diceKeep").disabled = false; $("#diceBack").disabled = false;
    patchName = "Dado #" + (1 + (Math.random() * 999 | 0)); ab.nameB = patchName; updatePill(); renderCompare();
    audition();
    toast("Patch novo tocando em B; o anterior está em A. Compare, FICAR para manter ou VOLTAR.");
    motion.nodV += 4; popup("🎲", 240, 190, "#b57bff");
  }
  $("#dice").addEventListener("click", function () { if (!sample.loaded) { toast("Importe ou escolha um som primeiro."); return; } rollDice(); });
  $("#diceKeep").addEventListener("click", function () { ab.A = snapshot(); ab.nameA = patchName; ab.B = null; ab.cur = "A"; $("#abA").classList.add("on"); $("#abB").classList.remove("on"); diceArmed = false; $("#diceKeep").disabled = true; $("#diceBack").disabled = true; updatePill(); toast("Ficou com o patch do dado. Use SAVE para guardar."); game.award(4, "dice"); game.event("diceKeep"); });
  $("#diceBack").addEventListener("click", function () { if (ab.A) restore(ab.A); patchName = ab.nameA || "Novo patch"; ab.B = null; ab.cur = "A"; $("#abA").classList.add("on"); $("#abB").classList.remove("on"); diceArmed = false; $("#diceKeep").disabled = true; $("#diceBack").disabled = true; updatePill(); toast("Voltou ao patch de antes do dado."); });

  /* ================= PATCH LAB ================= */
  let labDir = 0, labRev = -1, labPicked = -1, labCat = "all", labRes = null;
  const labDirRefresh = seg("#labDir", "dir", function () { return labDir; }, function (x) { labDir = x; labDirRefresh(); });
  labDirRefresh();
  function artFor(name, hue) {
    let h = 0; for (let i = 0; i < name.length; i++) h = (h * 31 + name.charCodeAt(i)) >>> 0;
    const a = hue === undefined ? h % 360 : hue, b = (a + 60 + h % 90) % 360;
    return "background:radial-gradient(circle at " + (20 + h % 60) + "% " + (30 + h % 40) + "%, hsla(" + b + ",95%,65%,.9), transparent 45%), linear-gradient(135deg, hsl(" + a + ",70%,22%), hsl(" + ((a + 40) % 360) + ",80%,40%))";
  }
  function waveSvg(id, color) {
    const p = id ? FAC.peaks(id, 48) : Array.from({ length: 48 }, function (_, i) { return .3 + .5 * Math.abs(Math.sin(i * .7)); });
    let d = ""; p.forEach(function (a, i) { d += "M" + (i * 2 + 1) + " " + (8 - a * 7).toFixed(1) + "V" + (8 + a * 7).toFixed(1); });
    return '<svg class="wv" viewBox="0 0 96 16" preserveAspectRatio="none"><path d="' + d + '" stroke="' + (color || "#ff6fe0") + '" stroke-width="1.2"/></svg>';
  }
  function renderLabLib() {
    const cats = $("#labCats"); cats.innerHTML = "";
    const all = [["all", "ALL", "wave"]].concat(FAC.CATS.slice(1)).concat([["mine", "MEUS PATCHES", "folder"], ["favs", "FAVORITES", "heart"]]);
    all.forEach(function (c) {
      const n = c[0] === "all" ? FAC.SOUNDS.length + userPatches.length : c[0] === "mine" ? userPatches.length : c[0] === "favs" ? favs().length : FAC.SOUNDS.filter(function (s) { return s.cat === c[0]; }).length;
      const li = document.createElement("li"); li.className = labCat === c[0] ? "on" : ""; li.innerHTML = '<i data-ic="' + c[2] + '"></i><span>' + c[1] + "</span><small>" + n + "</small>"; ICO.hydrate(li);
      li.addEventListener("click", function () { labCat = c[0]; renderLabLib(); }); cats.appendChild(li);
    });
    const q = ($("#labSearch").value || "").toLowerCase();
    let items = FAC.SOUNDS.map(function (s) { return { key: "f:" + s.id, name: s.name, cat: s.cat, tags: s.tags, kind: "factory", id: s.id }; }).concat(userPatches.map(function (p) { return { key: "p:" + p.path, name: p.name, cat: "mine", tags: "Meu patch", kind: "patch", path: p.path }; }));
    if (labCat === "favs") items = items.filter(function (it) { return isFav(it.key); }); else if (labCat !== "all") items = items.filter(function (it) { return it.cat === labCat; });
    if (q) items = items.filter(function (it) { return (it.name + " " + it.tags).toLowerCase().indexOf(q) >= 0; });
    const box = $("#labLib"); box.innerHTML = "";
    items.forEach(function (it) {
      const b = document.createElement("div"); b.className = "lcard" + (current && current.key === it.key ? " on" : ""); b.setAttribute("role", "button");
      b.innerHTML = '<div class="art" style="' + artFor(it.name) + '"><i data-ic="play"></i></div><div><b>' + it.name + '</b><small>' + it.tags + '</small>' + waveSvg(it.id) + '</div><button class="hrt' + (isFav(it.key) ? " on" : "") + '" data-ic="' + (isFav(it.key) ? "heartFill" : "heart") + '"></button>';
      ICO.hydrate(b);
      b.addEventListener("click", function (e) { if (e.target.closest(".hrt")) { toggleFav(it.key); renderLabLib(); return; } loadItem(it); });
      box.appendChild(b);
    });
    if (!items.length) box.innerHTML = '<p class="lib-empty">Nada aqui ainda.</p>';
  }
  $("#labSearch").addEventListener("input", renderLabLib);
  let labAlien = null;
  function renderLabScreen() {
    renderLabLib();
    $("#labParent").innerHTML = '<small>PARENT PATCH</small><b>' + patchName + '</b><div class="art" style="' + artFor(patchName) + '"></div>' + waveSvg(current && current.id, "#ff6fe0");
    if (!labAlien) { labAlien = PUP.create($("#labAlien"), "pads"); labAlien.root.setAttribute("transform", "translate(-75 -300) scale(.42)"); }
    renderCompare();
    R.call("patchResults").then(renderLab);
  }
  $("#labGo").addEventListener("click", function () {
    if (!sample.loaded) { toast("Escolha um som ou importe um sample primeiro."); return; }
    R.call("patchSearch", labDir === 1).then(function (ok) { if (!ok) toast("A busca já está rodando ou o trecho é curto demais."); });
    labPicked = -1; $("#labResults").innerHTML = ""; $("#labBubble").textContent = "Procurando sons novos…";
  });
  $("#labVar").addEventListener("click", function () {
    if (!sample.loaded) { toast("Escolha um som primeiro."); return; }
    R.call("jam").then(function (can) { $("#undoJam").disabled = !can; });
    toast("Variação criada nos efeitos ligados. Desfazer em MACROS (DESFAZER VARIAÇÃO)."); $("#labBubble").textContent = "Gostou dessa variação?";
  });
  $("#labPause").addEventListener("click", function () { R.call("patchSearchPause"); });
  $("#labCancel").addEventListener("click", function () { R.call("patchSearchCancel"); });
  const STATUS = ["Pronto.", "Procurando…", "Pronto! Clique numa opção para ouvir.", "Busca cancelada. Seu patch atual foi mantido.", "O trecho está silencioso. Escolha outro trecho.", "Falha na busca."];
  function labTick(s) {
    if (!s || s.status === undefined) return;
    $("#labBar").style.width = Math.round((s.busy || s.status === 2 ? s.progress : 0) * 100) + "%";
    $("#labPause").disabled = !s.busy; $("#labCancel").disabled = !s.busy; $("#labGo").disabled = !!s.busy;
    $("#labPause").textContent = s.paused ? "RETOMAR" : "PAUSAR";
    $("#labStatus").textContent = s.busy ? (s.paused ? "Pausado · " : "Gerando variações… ") + Math.round(s.progress * 100) + "%" : (sample.loaded ? STATUS[s.status] || "" : "Escolha um som primeiro.");
    if (s.revision !== labRev) { labRev = s.revision; R.call("patchResults").then(renderLab); }
  }
  const LABN = ["Lo-Fi Neon", "Space Drift", "Analog Haze", "Pixel Dreams", "Velvet Orbit", "Cosmic Dust"], LABC = ["#ff4fd8", "#2fe0ff", "#ffa02e", "#b06bff", "#ff4fd8", "#2fe0ff"];
  function miniPatch(vals, color) {
    const a = 4 + Math.min(1, Math.log(Math.max(vals[1], .001) / .001) / Math.log(4000)) * 60, d = Math.min(1, Math.log(Math.max(vals[2], .001) / .001) / Math.log(4000)) * 60, s = vals[3], r = Math.min(1, Math.log(Math.max(vals[4], .001) / .001) / Math.log(8000)) * 60;
    const y0 = 64, top = 8, sy = y0 - (y0 - top) * s, p1 = [a, top], p2 = [a + d, sy], p3 = [a + d + 40, sy], p4 = [a + d + 40 + r, y0];
    return '<svg class="env" viewBox="0 0 240 70" preserveAspectRatio="none"><path d="M4 64 L' + p1 + ' L' + p2 + ' L' + p3 + ' L' + p4 + '" fill="none" stroke="' + color + '" stroke-width="2.4" vector-effect="non-scaling-stroke" style="filter:drop-shadow(0 0 3px ' + color + ')"/></svg>';
  }
  function renderLab(res) {
    labRes = res;
    const box = $("#labResults"); box.innerHTML = "";
    const tree = $("#labTree"); tree.innerHTML = "";
    if (!res || !res.results || !res.results.length) return;
    if (!res.valid) { $("#labStatus").textContent = "O sample mudou desde a busca. Gere de novo."; return; }
    $("#labBubble").textContent = "Escolha uma e compare!";
    res.results.forEach(function (r, i) {
      const col = LABC[i % 6], x = (i % 3) * 200 + 100;
      if (i < 3) tree.insertAdjacentHTML("beforeend", '<path d="M300 0 C300 30 ' + x + ' 20 ' + x + ' 56" stroke="' + col + '" style="filter:drop-shadow(0 0 3px ' + col + ')"/>');
      const b = document.createElement("button"); b.className = "lab-card" + (labPicked === i ? " on" : ""); b.style.setProperty("--lc", col);
      b.innerHTML = "<b>" + LABN[i % 6] + "</b><div class='art' style='" + artFor(LABN[i % 6] + i) + "'></div>" + miniPatch(r.values, col) + "<small>distância " + r.distance.toFixed(3) + (r.values[0] > .5 ? " · synth" : "") + "</small>";
      b.addEventListener("click", function () {
        if (labPicked < 0) { ab.A = snapshot(); ab.nameA = patchName; }
        labPicked = i; R.call("patchApply", r.values).then(function () { ab.B = null; ab.cur = "B"; $("#abA").classList.remove("on"); $("#abB").classList.add("on"); ab.B = snapshot(); patchName = LABN[i % 6]; ab.nameB = patchName; updatePill(); renderCompare(); audition(); });
        $$(".lab-card").forEach(function (c, j) { c.classList.toggle("on", j === i); });
        toast(LABN[i % 6] + " aplicado em B. O patch anterior está em A."); game.award(4, "lab"); game.event("lab");
      });
      box.appendChild(b);
    });
  }
  function renderCompare() {
    const fill = function (which, snap) {
      $("#ab" + which + "Name").textContent = ab["name" + which] || (which === ab.cur ? patchName : "—");
      $("#ab" + which + "Art").setAttribute("style", artFor((ab["name" + which] || patchName) + which));
      const s = snap || (which === ab.cur ? snapshot() : null), ul = $("#ab" + which + "List");
      if (!s) { ul.innerHTML = "<li>vazio</li>"; return; }
      ul.innerHTML = "<li>Filter <b>" + fmt.hz(s.filterCutoff) + "</b></li><li>Resonance <b>" + s.filterResonance.toFixed(2) + "</b></li><li>Movement <b>" + Math.round(s.lfoDepth * 100) + "%</b></li><li>Effect Mix <b>" + Math.round(s.drywet * 100) + "%</b></li>";
    };
    fill("A", ab.cur === "A" ? null : ab.A); fill("B", ab.cur === "B" ? null : ab.B);
  }
  $("#labFav").addEventListener("click", function () { $("#presetFav").click(); $("#labFav").classList.toggle("on", !!(current && isFav(current.key))); });

  /* ================= capture ================= */
  let capOpen = false, capTimer = 0;
  const CAPSTATE = ["pronto", "gravando", "salvando", "pronto", "exportando", "iniciando"];
  $("#captureBtn").addEventListener("click", function () { setScreen("captura"); });
  function openCapture() {
    $("#captureSheet").hidden = false; capOpen = true;
    R.call("captureInfo").then(function (info) { info = info || {}; $("#capDirect").hidden = !info.direct; $("#capSettings").hidden = !info.canOpenSettings; });
    pollCapture(); clearInterval(capTimer); capTimer = setInterval(pollCapture, 1000);
  }
  function closeCapture(fromScreen) { $("#captureSheet").hidden = true; capOpen = false; clearInterval(capTimer); if (!fromScreen && screen() === "captura") setScreen("limpo"); }
  $("#capSettings").addEventListener("click", function () { R.call("captureSettings"); });
  $("#capRec").addEventListener("click", function () { R.call("captureRecord").then(function (ok) { if (ok) { patchName = "Captura"; current = null; updatePill(); toast("Gravação carregada como sample. Toque no teclado!"); game.award(15, "capture"); game.event("capture"); closeCapture(); } }); });
  $("#capStop").addEventListener("click", function () { R.call("captureStop"); });
  $("#capMon").addEventListener("click", function () { const on = !$("#capMon").classList.contains("on"); R.call("captureMonitor", on); $("#capMon").classList.toggle("on", on); if (on) toast("Monitor ligado (−20 dB). Use fones."); });
  function pollCapture() {
    if (!capOpen) return;
    R.call("capturePoll").then(function (r) {
      if (!r) return;
      const links = $("#capLinks"); links.innerHTML = "";
      if (!r.links.length) links.innerHTML = '<div class="fine">Nenhum RASTRO Capture encontrado. Confira os passos acima.</div>';
      r.links.forEach(function (l) {
        const row = document.createElement("div"); row.className = "cap-item"; const rec = l.state === 1;
        row.innerHTML = '<div class="grow">Insert ' + l.id + '<small>' + (rec ? "gravando · " + (l.frames / (l.rate || 48000)).toFixed(1) + " s" : CAPSTATE[l.state] || "") + " · " + (l.flow ? "áudio chegando" : "sem áudio chegando") + (l.outcomeText ? " · " + l.outcomeText : "") + '</small></div>';
        const b = document.createElement("button"); b.className = "chip-btn"; b.textContent = rec ? "■ PARAR" : "● GRAVAR";
        b.addEventListener("click", function () { R.call("captureRemote", l.id, !rec).then(function (ok) { if (!ok) toast("Não consegui falar com o insert."); }); });
        row.appendChild(b); links.appendChild(row);
      });
      const files = $("#capFiles"); files.innerHTML = "";
      if (!r.files.length) files.innerHTML = '<div class="fine">Nenhuma tomada ainda.</div>';
      r.files.slice(0, 8).forEach(function (f) {
        const row = document.createElement("div"); row.className = "cap-item";
        row.innerHTML = '<div class="grow">' + f.name + '<small>' + new Date(f.time).toLocaleString() + '</small></div>';
        const b = document.createElement("button"); b.className = "chip-btn on"; b.textContent = "USAR";
        b.addEventListener("click", function () { R.call("captureLoad", f.path).then(function (res) { importResult(res); if (res && res.ok) { patchName = f.name.replace(/\.[^.]+$/, ""); current = null; updatePill(); closeCapture(); } }); });
        row.appendChild(b); files.appendChild(row);
      });
    });
  }
  function capTick(c) {
    if (!capOpen || !c) return;
    $("#capMeter").style.width = Math.min(100, (c.peak || 0) * 100) + "%";
    $("#capTime").textContent = (c.seconds || 0).toFixed(1).replace(".", ",") + " s";
    const rec = c.state === 1; $("#capRec").classList.toggle("live", rec); $("#capRec").textContent = rec ? "● GRAVANDO" : "● GRAVAR";
    $("#capStatus").textContent = rec ? "Gravando… clique em PARAR para usar a tomada (máx. 30 s). Ela substitui o sample atual." : c.outcome === 2 ? "Gravação vazia: nenhum áudio chegou." : c.outcome === 3 ? "Chegou áudio, mas em silêncio: confira entrada e ganho." : "";
  }
  $$(".sheet").forEach(function (s) { s.addEventListener("click", function (e) { if (e.target === s || e.target.hasAttribute("data-close")) { if (s.id === "captureSheet") closeCapture(); else s.hidden = true; } }); });

  /* ================= stage & character ================= */
  const CHARS = [
    { key: "fuzz", name: "Fuzz", species: 0 }, { key: "soul", name: "Soul", species: 1 }, { key: "lua", name: "Lua", species: 2 },
    { key: "nova", name: "Nova", species: 3 }, { key: "sapo", name: "Sapo", species: 5 }, { key: "fuzz2", name: "Fuzz Street", species: 4, level: 3 }
  ];
  const SPECIES = { 0: "fuzz", 1: "soul", 2: "lua", 3: "nova", 4: "fuzz2", 5: "sapo", 6: "alien" };
  const DEFAULT_STYLE = { bounce: 1, sway: 1, nod: 1, dip: 1, lean: .3, hop: 0, nodHit: 1, jitter: 0 };
  let charKind = "alien", rig = {}, charSvg = null, sceneKind = "", sc = {}, puppet = null;
  const motion = { squash: 0, squashV: 0, strike: 0, look: 0, energy: 0, shake: 0, shakeT: 9, alive: 0, lastSound: -1e9, nod: 0, nodV: 0, dark: 0, lfoPhase: 0, peak: .05, hit: false, pitch: .5, pad: 0, x: 240, y: 412, rot: 0 };
  const look = { hero3d: 1, pose: "disco", dance: "groove", sync: 1, bpm: 110, strip: 1, knob: "kn_00", glow: "auto", react: 1, kb: 0 };
  let lookTimer = 0;
  function saveLook() { clearTimeout(lookTimer); lookTimer = setTimeout(function () { R.call("lookSet", JSON.stringify(look)); }, 300); }
  function readLook(json) {
    try {
      const o = JSON.parse(json || "null");
      if (o) {
        if (PUP.LOOKS[o.pose]) look.pose = o.pose; else if (PUP.ALIAS[o.pose]) look.pose = PUP.ALIAS[o.pose];
        if (AL.DANCES.some(function (d) { return d.id === o.dance; })) look.dance = o.dance;
        look.sync = o.sync === 0 ? 0 : 1; look.bpm = clamp(Math.round(o.bpm || 110), 60, 180); look.strip = o.strip === 0 ? 0 : 1;
        if (typeof o.knob === "number") look.knob = OLD_KNOB[clamp(Math.round(o.knob), 0, 23)];
        else if (typeof o.knob === "string" && KNOBS.some(function (k) { return k.id === o.knob; })) look.knob = o.knob;
        if (AL.GLOW[o.glow]) look.glow = o.glow;
        if (typeof o.react === "number") look.react = clamp(o.react, 0, 1.5);
        look.kb = o.kb ? 1 : 0;
        look.hero3d = o.hero3d === 0 ? 0 : 1;
      }
    } catch (e) { /* keep defaults */ }
  }
  function applyLook() {
    document.body.classList.toggle("strip-off", !look.strip); applyKnobClass(); applyKb();
    if (charSvg) buildCharacter();
    if (screen() === "locker") renderLocker();
  }
  function setT(el, t) { if (el) el.setAttribute("transform", t); }
  function setA(el, k, val) { if (el) el.setAttribute(k, val); }
  function roomFile() { const k = ART.rooms[Math.round(v("sceneRoom"))] || "neon"; return "room_" + k + ".jpg"; }
  function buildStage() {
    sceneKind = ART.rooms[Math.round(v("sceneRoom"))] || "neon";
    $("#stageArt").innerHTML = ART.scene(sceneKind);
    charSvg = $("#stageArt svg"); sc = { scale: ART.roomScale(sceneKind), ox: ART.roomOffset(sceneKind) };
    ["pxSky", "pxCity", "fg", "spot", "vu", "bloomVeil", "darkVeil", "neon", "room", "winGlow", "arcadeGlow", "bedGlow", "neonStrip", "lampGlow", "lamp2Glow", "globeGlow",
     "coneL1", "coneL2", "coneR1", "coneR2", "reel1", "reel2", "blob1", "blob2", "blob3"].forEach(function (id) { sc[id] = charSvg.getElementById(id); });
    sc.bulbs = $$(".bulb", charSvg); sc.wins = $$(".win", charSvg); sc.stars = $$(".star", charSvg); sc.sways = $$(".sway", charSvg); sc.spk = $$(".spkGlow", charSvg);
    // effect layers drawn around the character (behind and in front)
    const layer = $("#charLayer", charSvg);
    const behind = document.createElementNS(NS, "g"); behind.id = "fxBehind"; layer.parentNode.insertBefore(behind, layer);
    const front = document.createElementNS(NS, "g"); front.id = "fxFront"; layer.parentNode.insertBefore(front, layer.nextSibling);
    sc.fxBehind = behind; sc.fxFront = front;
    sc.world = charSvg.getElementById("world"); cam = null;
    if (sc.world) buildWorld();
    buildCharacter();
  }
  // Living stage (Estúdio Palco): collect the lights placed on the painting and make the floating bokeh layer.
  let cam = null;
  function buildWorld() {
    const q = function (sel) { return $$(sel, charSvg); };
    sc.lampGs = q(".lampG").map(function (el, i) { return { el: el, ph: i * 1.7, fl: 0 }; });
    sc.twk = q(".twk").map(function (el) { return { el: el, ph: Math.random() * 6.28, sp: .25 + Math.random() * 1.1, on: Math.random() }; });
    sc.strips = q(".stripG"); sc.bars = q(".barG"); sc.dots = q(".dotG"); sc.ringEls = q("#platRings ellipse");
    ["platRings", "platPool", "cones", "coneA", "coneB", "gGlow0", "gGlow1", "gCone0", "gCone1", "gCone2", "bokeh", "charRefl"].forEach(function (id) { sc[id] = charSvg.getElementById(id); });
    sc.planet = $(".planetG", charSvg); sc.lava = $(".lavaG", charSvg);
    sc.bk = [];
    for (let i = 0; i < 16; i++) {
      const c = document.createElementNS(NS, "circle"), r = 3 + Math.random() * 9;
      c.setAttribute("r", r.toFixed(1)); c.setAttribute("fill", ["#ffb340", "#ff4fd8", "#2fe0ff", "#b06bff"][i % 4]); c.setAttribute("opacity", "0");
      sc.bokeh.appendChild(c);
      sc.bk.push({ el: c, x: Math.random() * 520 - 20, y: Math.random() * 452, z: .4 + Math.random() * 1.2, vy: -(3 + Math.random() * 8), ph: Math.random() * 6.28, a: .08 + Math.random() * .18 });
    }
  }
  // Game camera for the living stage: frames the platform and the character for the current panel size,
  // breathes slowly, leans toward the mouse and pushes in a touch on the beat. Never shows past the painting.
  function stageCam(now, groove, onBeat, react, amp) {
    const st = $("#stage"), W = st.clientWidth, H = st.clientHeight;
    if (!W || !H) return cam || { tx: 0, ty: 0, s: .5 };
    const k = Math.max(W / 480, H / 452), visW = W / k, visH = H / k, vx0 = 240 - visW / 2, vy0 = 226 - visH / 2;
    const scr = screen(), bottomPad = scr === "palco" && look.strip ? 118 / k : 0, topPad = 0;
    const S = ART.stage, yTop = 150, yBot = 805;
    let s = (visH - bottomPad - topPad) / (yBot - yTop);
    s = Math.max(s, visW / (S.w - 60), visH / (S.h - 40));
    const life = 1 + .006 * groove * onBeat * react * amp + jamBurst * .01; // the room stays still: no drifting painting
    s *= 1.03 * life;
    let tx = 240 - S.feet[0] * s;
    let ty = vy0 + topPad - yTop * s;
    if (scr === "locker") {
      // fitting room: the character stands between the side panels, about 450 px tall on screen, feet near y 640
      // ESTÚDIO: the character stands in the left frame (centre x 361 px), about 620 px tall, feet near y 850
      s = Math.max(620 / (520 * k), visW / (S.w - 60), visH / (S.h - 40)) * life;
      tx = 240 + (361 - 720) / k - S.feet[0] * s; ty = vy0 + 850 / k - S.feet[1] * s;
    }
    tx = Math.min(vx0, Math.max(vx0 + visW - S.w * s, tx)); ty = Math.min(vy0, Math.max(vy0 + visH - S.h * s, ty));
    cam = { tx: tx, ty: ty, s: s, vx0: vx0, vy0: vy0, visW: visW, visH: visH };
    return cam;
  }
  function stepWorld(now, dt, o) {
    const g = o.glow, react = o.react, beat = o.beat, onBeat = o.onBeat, groove = o.groove, e = o.energy, alive = o.alive;
    const C = cam; sc.world.setAttribute("transform", "translate(" + C.tx.toFixed(2) + " " + C.ty.toFixed(2) + ") scale(" + C.s.toFixed(4) + ")");
    // colour of the stage lights = the sound's glow colour
    sc.gGlow0.setAttribute("stop-color", g); sc.gGlow1.setAttribute("stop-color", g);
    sc.gCone1.setAttribute("stop-color", g); sc.gCone2.setAttribute("stop-color", g);
    sc.platRings.setAttribute("stroke", g);
    // platform: rings chase around with the beat, pool of light under the feet follows the envelope
    const chase = beat * 86;
    sc.ringEls.forEach(function (r, i) { if (i === 0) r.setAttribute("stroke-dashoffset", (-chase).toFixed(1)); if (i === 2) r.setAttribute("stroke-dashoffset", (chase * .7).toFixed(1)); r.setAttribute("opacity", (i % 2 ? .45 + onBeat * groove * .5 : .7).toFixed(2)); });
    sc.platRings.setAttribute("opacity", ((.18 + e * .55 + o.env * .25) * react).toFixed(2));
    sc.platPool.setAttribute("opacity", ((.12 + o.env * .5 + onBeat * groove * .25) * react).toFixed(2));
    sc.cones.setAttribute("opacity", ((e * .7 + o.bloom * .3) * react).toFixed(2));
    const sw = Math.sin(beat * Math.PI * .5) * 7 * groove;
    sc.coneA.setAttribute("transform", "rotate(" + (-6 + sw).toFixed(2) + " 590 -20)"); sc.coneB.setAttribute("transform", "rotate(" + (5 - sw).toFixed(2) + " 1080 -20)");
    // front light bars pulse on the beat; side dots chase
    sc.bars.forEach(function (b) { b.setAttribute("fill", b.dataset.k === "c" ? mixHex("#2fe0ff", g, .4) : mixHex("#ffb340", g, .25)); b.setAttribute("opacity", ((.1 + onBeat * groove * .55 + o.env * .25) * react).toFixed(2)); });
    const step = Math.floor(beat * 2) % 3;
    sc.dots.forEach(function (d, i) { d.setAttribute("opacity", (((i % 3) === step ? .2 + groove * .6 : .05) * react).toFixed(2)); });
    // room lights: lamps breathe and flicker a little, the lava lamp follows the LFO, speakers kick
    sc.lampGs.forEach(function (l, i) { if (Math.random() < .004) l.fl = 1; l.fl *= Math.exp(-dt * 9); l.el.setAttribute("opacity", ((.16 + .06 * Math.sin(now / 900 + l.ph) + o.cut * .18 + e * .2 + (i % 2 ? onBeat : 0) * groove * .18 - l.fl * .12) * react).toFixed(2)); });
    if (sc.lava) sc.lava.setAttribute("opacity", ((.25 + Math.abs(o.lfo) * .55 + o.res * .2) * react).toFixed(2));
    if (sc.planet) sc.planet.setAttribute("opacity", ((.12 + o.bloom * .5 + .08 * Math.sin(now / 1700)) * react).toFixed(2));
    sc.strips.forEach(function (p, i) { p.setAttribute("opacity", ((.06 + o.res * .4 * (.5 + .5 * Math.sin(now / 70 + i)) + groove * .14 + (i === step ? onBeat * groove * .2 : 0)) * react).toFixed(2)); });
    sc.spk.forEach(function (c) { const cx = c.getAttribute("cx"), cy = c.getAttribute("cy"); c.setAttribute("opacity", ((e * .6 + onBeat * groove * .5) * react).toFixed(2)); c.setAttribute("transform", "translate(" + cx + " " + cy + ") scale(" + (.75 + e * .35 + onBeat * groove * .35).toFixed(2) + ") translate(" + (-cx) + " " + (-cy) + ")"); });
    sc.twk.forEach(function (t) { t.ph += dt * t.sp; const on = .5 + .5 * Math.sin(t.ph * 2.3 + t.on * 6); t.el.setAttribute("opacity", ((.08 + on * on * .55 * (.35 + .65 * alive)) * Math.min(1, react)).toFixed(2)); });
    // bokeh in front of the scene: drifts up, moves faster than the room when the mouse moves (depth)
    sc.bk.forEach(function (b) {
      b.y += b.vy * dt * b.z; b.ph += dt; if (b.y < -20) { b.y = 470; b.x = Math.random() * 520 - 20; }
      b.el.setAttribute("cx", (b.x - mx * 40 * b.z).toFixed(1)); b.el.setAttribute("cy", (b.y - my * 14 * b.z).toFixed(1));
      b.el.setAttribute("opacity", (b.a * (.3 + .7 * e) * (.6 + .4 * Math.sin(b.ph * 1.3)) * react).toFixed(3));
    });
  }
  // ---------- 3D hero (PR #3 prototype). Falls back to the painted puppet when WebGL is missing. ----------
  let hero = null, heroLayer = null, heroTrail = null, heroRefl = null, heroFailed = false;
  function heroWanted() { return look.hero3d !== 0 && !heroFailed && charKind === "alien" && window.RastroHero3D; }
  function setupHero() {
    if (!heroWanted()) { if (heroLayer) heroLayer.hidden = true; if (rig.pos) rig.pos.style.visibility = ""; return; }
    if (!heroLayer) {
      heroLayer = document.createElement("div"); heroLayer.id = "heroLayer"; $("#stageArt").after(heroLayer);
      heroTrail = document.createElement("canvas"); heroTrail.className = "hero-trail"; heroLayer.appendChild(heroTrail);
      heroRefl = document.createElement("canvas"); heroRefl.className = "hero-refl"; heroLayer.appendChild(heroRefl);
      hero = window.RastroHero3D.create(heroLayer);
      if (!hero) { heroFailed = true; heroLayer.remove(); heroLayer = null; toast("3D indisponível neste computador: usando o personagem pintado."); return; }
    }
    heroLayer.hidden = false;
    hero.setOutfit(look.pose);
    if (rig.pos) rig.pos.style.visibility = "hidden"; // the painted puppet keeps running invisibly (positions for effects)
    fx.ghosts.forEach(function (g) { g.style.display = "none"; });
    if (sc.charRefl) sc.charRefl.style.display = "none";
  }
  function heroScreen(x, y) {
    // stage-local CSS pixels of a point in the character layer (the world camera moves it)
    const layer = rig.pos.parentNode, m = layer.getScreenCTM(), st = $("#stage"), r = st.getBoundingClientRect(), k = r.width / st.clientWidth;
    return { x: (m.a * x + m.c * y + m.e - r.left) / k, y: (m.b * x + m.d * y + m.f - r.top) / k, unit: m.a / k };
  }
  function drawHero(state, dt, X0, Y0, s, ghostAct, react) {
    const p = heroScreen(X0, Y0), heightPx = 488 * s * p.unit;
    const h = hero.heightFor(heightPx), w = h * .82;
    hero.setSize(w, h);
    const f = hero.feetPixel(), left = p.x - f[0], top = p.y - f[1];
    hero.canvas.style.left = left.toFixed(1) + "px"; hero.canvas.style.top = top.toFixed(1) + "px";
    hero.update(state, dt); hero.render();
    const cw = hero.canvas.width, ch = hero.canvas.height;
    // reflection on the glossy platform (flipped copy, faded)
    if (heroRefl.width !== cw || heroRefl.height !== ch) { heroRefl.width = cw; heroRefl.height = ch; heroTrail.width = cw; heroTrail.height = ch; }
    [heroRefl, heroTrail].forEach(function (c) { c.style.width = w + "px"; c.style.height = h + "px"; c.style.left = left.toFixed(1) + "px"; });
    heroTrail.style.top = top.toFixed(1) + "px";
    heroRefl.style.top = (2 * p.y - top - h).toFixed(1) + "px";
    const rc = heroRefl.getContext("2d"); rc.clearRect(0, 0, cw, ch); rc.globalAlpha = .26; rc.drawImage(hero.canvas, 0, 0);
    // GHOST: violet afterimages that trail behind the moving body (strength = real GHOST activity)
    const tc = heroTrail.getContext("2d");
    if (ghostAct > .02) {
      tc.globalCompositeOperation = "destination-out"; tc.fillStyle = "rgba(0,0,0," + (.08 + (1 - v("gFeedback")) * .14).toFixed(3) + ")"; tc.fillRect(0, 0, cw, ch);
      tc.globalCompositeOperation = "source-over"; tc.globalAlpha = clamp(ghostAct * react * .55, 0, .6); tc.drawImage(hero.canvas, -ghostAct * 10 * (window.devicePixelRatio || 1), 0); tc.globalAlpha = 1;
      tc.globalCompositeOperation = "source-atop"; tc.fillStyle = "rgba(176,107,255,.75)"; tc.fillRect(0, 0, cw, ch); tc.globalCompositeOperation = "source-over";
      heroTrail.dataset.on = "1";
    } else if (heroTrail.dataset.on) { tc.clearRect(0, 0, cw, ch); heroTrail.dataset.on = ""; }
  }
  function buildCharacter() {
    const layer = $("#charLayer", charSvg);
    puppet = null;
    if (charKind === "alien") {
      layer.innerHTML = '<g id="charPos"></g>';
      const pos = layer.querySelector("#charPos");
      puppet = PUP.create(pos, look.pose);
      rig = { puppet: true, pos: pos, pads: [], style: DEFAULT_STYLE };
      setupHero();
    } else {
      const C = ART.characters[charKind]();
      layer.innerHTML = '<g id="charPos" transform="translate(240 412)"><g transform="scale(.86)">' + C.svg + '</g></g>';
      rig = { painted: !!C.painted, pivot: C.pivot || [0, C.anchors.head[1] + 40], style: C.style || DEFAULT_STYLE, anchors: C.anchors };
      ["c-root", "c-head", "c-hair", "c-eyes", "c-armL", "c-armR", "c-inst", "c-torso", "c-legL", "c-legR", "c-mouth", "c-mouthOpen"].forEach(function (id) { rig[id] = layer.querySelector("#" + id); });
      rig.pos = layer.querySelector("#charPos"); rig.pupils = $$(".c-pupil", layer);
      rig.keyGlow = layer.querySelector(".keyGlow"); rig.keySpot = layer.querySelector(".keySpot"); rig.pads = $$(".padLit", layer).map(function (el) { return { el: el, v: 0 }; });
      rig.keyRx = rig.keyGlow ? Number(rig.keyGlow.getAttribute("rx")) : 0; rig.keyCx = rig.keyGlow ? Number(rig.keyGlow.getAttribute("cx")) : 0; rig.flash = 0;
    }
    buildFx();
  }
  // Effect visuals (references from Gab): GHOST violet afterimages, CRUMBLE amber shards, BLOOM pink aura/lotus,
  // WOBBLE cyan contour, DUST golden particles, CRUSH purple pixel blocks. Intensity = real module activity.
  const fx = { ghosts: [], hist: [], shards: [], dust: [], pix: [], bloom: null, pulse: null };
  function buildFx() {
    const b = sc.fxBehind, f = sc.fxFront; if (!b) return;
    b.innerHTML = ""; f.innerHTML = ""; fx.ghosts = []; fx.shards = []; fx.dust = []; fx.pix = []; fx.hist = [];
    const g = document.createElementNS(NS, "g"); g.innerHTML =
      '<defs><radialGradient id="bloomG"><stop offset="0" stop-color="#ff4fd8" stop-opacity=".75"/><stop offset=".55" stop-color="#ff4fd8" stop-opacity=".25"/><stop offset="1" stop-color="#ff4fd8" stop-opacity="0"/></radialGradient></defs>' +
      '<g id="bloomFx" opacity="0"><ellipse rx="150" ry="170" fill="url(#bloomG)"/>' + Array.from({ length: 7 }, function (_, i) { const a = -90 + (i - 3) * 26; return '<path transform="rotate(' + a + ')" d="M0 0 C-26 -50 -20 -120 0 -150 C20 -120 26 -50 0 0Z" fill="none" stroke="#ff4fd8" stroke-width="3" opacity=".8" style="filter:drop-shadow(0 0 6px #ff4fd8)"/>'; }).join("") + '</g>';
    b.appendChild(g); fx.bloom = g.querySelector("#bloomFx");
    // ghost afterimages = violet silhouettes of the puppet (reuse its glow mask: cheap, no filter)
    if (puppet) for (let i = 0; i < 4; i++) {
      const gg = document.createElementNS(NS, "g"); gg.setAttribute("opacity", "0");
      gg.innerHTML = '<rect width="' + puppet.L.w + '" height="700" fill="#b06bff" mask="url(#' + puppet.id + 'mG)"/>';
      b.appendChild(gg); fx.ghosts.push(gg);
    }
    fx.front = f;
  }
  let mouseX = 0, mouseY = 0, mx = 0, my = 0;
  $("#stage").addEventListener("pointermove", function (e) { const r = e.currentTarget.getBoundingClientRect(); mouseX = (e.clientX - r.left) / r.width - .5; mouseY = (e.clientY - r.top) / r.height - .5; });
  $("#stage").addEventListener("pointerleave", function () { mouseX = mouseY = 0; });
  $("#stage").addEventListener("pointerdown", function (e) { if (e.target.closest("button, .palco-bar")) return; motion.hit = true; spawnNotes(240, 250, 2, .6); });
  function spawnNotes(x, y, count, vel) {
    return; // floating note/star glyphs removed (Gab: they looked cheap)
    if (animMode < 2 || !charSvg || paused) return;
    const g = $("#notesP", charSvg), glyphs = ["♪", "♫", "✦"];
    if (!g) return;
    let ks = 1;
    if (sc.world && cam) { x = (x - cam.tx) / cam.s; y = (y - cam.ty) / cam.s; ks = 1 / cam.s; }
    for (let i = 0; i < count; i++) {
      const t = document.createElementNS(NS, "text");
      t.textContent = glyphs[(Math.random() * glyphs.length) | 0];
      t.setAttribute("x", x + (Math.random() - .5) * 60 * ks); t.setAttribute("y", y); t.setAttribute("font-size", (12 + vel * 14) * ks);
      t.setAttribute("fill", ["#ffd27a", "#ff4fd8", "#2fe0ff", "#b06bff"][(Math.random() * 4) | 0]); t.setAttribute("font-weight", "700"); t.setAttribute("style", "filter:drop-shadow(0 0 4px currentColor)");
      g.appendChild(t);
      const dx = (Math.random() - .5) * 140 * ks, dy = (-80 - Math.random() * 90) * ks, rot = (Math.random() - .5) * 80;
      if (t.animate) t.animate([{ transform: "translate(0,0) rotate(0deg)", opacity: 1 }, { transform: "translate(" + dx + "px," + dy + "px) rotate(" + rot + "deg)", opacity: 0 }], { duration: 1000 + Math.random() * 500, easing: "cubic-bezier(.2,.7,.3,1)" }).onfinish = function () { t.remove(); };
      else setTimeout(function () { t.remove(); }, 1300);
    }
  }
  function spawn(list, max, make) { if (list.length < max) list.push(make()); }
  function stepParticles(dt, k, crumble, dust, crush, cx, cy, sz) {
    const f = fx.front; if (!f) return;
    // CRUMBLE: amber shards breaking off the outline
    if (crumble > .03 && Math.random() < crumble * 9 * dt * k) spawn(fx.shards, 40, function () {
      const p = document.createElementNS(NS, "path"), a = Math.random() * Math.PI * 2, r = sz * (.25 + Math.random() * .2);
      p.setAttribute("d", "M0 -" + (4 + Math.random() * 6) + " L" + (3 + Math.random() * 5) + " 4 L-" + (3 + Math.random() * 4) + " 3Z"); p.setAttribute("fill", Math.random() < .5 ? "#ffa02e" : "#ffd27a"); p.setAttribute("style", "filter:drop-shadow(0 0 3px #ffa02e)");
      f.appendChild(p); return { el: p, x: cx + Math.cos(a) * r * .6, y: cy + Math.sin(a) * r, vx: Math.cos(a) * 40, vy: Math.sin(a) * 30 - 10, rot: Math.random() * 360, vr: (Math.random() - .5) * 400, life: .8 + Math.random() * .7 };
    });
    // DUST: golden particles drifting
    if (dust > .03 && Math.random() < dust * 22 * dt * k) spawn(fx.dust, 70, function () {
      const c = document.createElementNS(NS, "circle"); c.setAttribute("r", (Math.random() * 1.6 + .5).toFixed(1)); c.setAttribute("fill", Math.random() < .6 ? "#ffd27a" : "#ffa02e"); c.setAttribute("style", "filter:drop-shadow(0 0 2px #ffd23f)");
      f.appendChild(c); return { el: c, x: cx + (Math.random() - .5) * sz * .9, y: cy + (Math.random() - .6) * sz, vx: (Math.random() - .5) * 14, vy: -6 - Math.random() * 16, rot: 0, vr: 0, life: 1.5 + Math.random() * 1.5 };
    });
    // CRUSH: purple pixel blocks blinking around the body
    if (crush > .03 && Math.random() < crush * 14 * dt * k) spawn(fx.pix, 36, function () {
      const r = document.createElementNS(NS, "rect"), s = 4 + Math.round(Math.random() * 3) * 3; r.setAttribute("width", s); r.setAttribute("height", s); r.setAttribute("fill", Math.random() < .5 ? "#8a4dff" : "#c79bff"); r.setAttribute("style", "filter:drop-shadow(0 0 3px #8a4dff)");
      f.appendChild(r); const x = cx + (Math.random() < .5 ? -1 : 1) * sz * (.2 + Math.random() * .3); return { el: r, x: Math.round(x / 4) * 4, y: Math.round((cy + (Math.random() - .5) * sz) / 4) * 4, vx: 0, vy: 0, rot: 0, vr: 0, life: .25 + Math.random() * .5, pix: true };
    });
    [fx.shards, fx.dust, fx.pix].forEach(function (list) {
      for (let i = list.length - 1; i >= 0; i--) {
        const d = list[i]; d.life -= dt; if (d.life <= 0) { d.el.remove(); list.splice(i, 1); continue; }
        d.x += d.vx * dt; d.y += d.vy * dt; d.vy += (list === fx.shards ? 60 : 0) * dt; d.rot += d.vr * dt;
        d.el.setAttribute("transform", "translate(" + d.x.toFixed(1) + " " + d.y.toFixed(1) + ") rotate(" + d.rot.toFixed(0) + ")");
        d.el.setAttribute("opacity", Math.min(1, d.life * 2).toFixed(2));
      }
    });
  }

  /* ================= game layer: XP unlocks looks only, never sound ================= */
  // [file, name, kind, level, cap ellipse for 3/4 view (cx, cy, rx, ry in the 160-px picture) or null]
  const KNOBS = [
    ["kn_00", "Neon âmbar", "anel aceso", 1], ["kn_13", "Violeta puro", "anel aceso", 1], ["k2_00", "Marfim canelado", "vintage", 1, [80, 45, 55, 38]], ["k2_01", "Console preto", "estúdio", 1, [80, 45, 52, 38]],
    ["k1_07", "Marfim clássico", "vintage", 1, [80, 69, 58, 46]], ["k1_04", "Anel ciano", "brilho", 1], ["kn_02", "Anel violeta", "neon", 1], ["k2_03", "Borracha", "fosco", 1, [80, 55, 60, 48]],
    ["kn_01", "Segmentos rosa", "neon", 2], ["kn_06", "Ciano fino", "neon", 2], ["kn_10", "Ciano forte", "neon", 2], ["k2_02", "Chrome escovado", "metal", 2, [80, 66, 64, 56]],
    ["k2_04", "Vermelho 70s", "canelado", 2, [80, 45, 58, 40]], ["k2_11", "Ponteiro vermelho", "estúdio", 2, [80, 38, 52, 36]], ["k1_02", "Console 4 asas", "futurista", 2], ["k1_05", "Lilás serrilhado", "metal", 2, [80, 52, 57, 40]],
    ["k1_11", "Ciano aceso", "vidro", 2, [80, 55, 52, 35]], ["kn_03", "Marcas ciano", "neon", 3], ["kn_08", "Serrilha roxa", "neon", 3], ["kn_11", "Pontos rosa", "LEDs", 3],
    ["k1_03", "Marfim parafusado", "vintage", 3], ["k1_10", "Âmbar vidro", "translúcido", 3, [80, 60, 60, 38]], ["k1_14", "Engrenagem", "metal", 3], ["k2_06", "Creme âmbar", "vintage", 3, [80, 52, 62, 45]],
    ["k2_08", "Roxo gloss", "serrilhado", 3, [80, 48, 55, 38]], ["kn_04", "Anel duplo", "laranja", 4], ["kn_07", "Riscos magenta", "neon", 4], ["kn_15", "Âmbar duplo", "neon", 4],
    ["k1_01", "Bolha cósmica", "líquido", 4], ["k1_09", "Madeira & metal", "estúdio", 4, [80, 57, 52, 37]], ["k2_05", "Baquelite", "marrom", 4, [80, 45, 55, 40]], ["k2_07", "Teal anodizado", "metal", 4, [80, 58, 62, 48]],
    ["kn_05", "Gema violeta", "facetado", 5], ["kn_12", "LEDs âmbar", "neon", 5], ["k1_00", "Gema âmbar", "facetado", 5], ["k1_12", "Grade neon", "pixels", 5, [80, 48, 58, 32]],
    ["k2_09", "Cristal ciano", "vidro", 5, [80, 45, 58, 40]], ["k2_10", "Âmbar translúcido", "vidro", 5, [80, 40, 55, 38]], ["kn_09", "Pôr do sol", "laranja → rosa", 6], ["kn_14", "Facetas azuis", "neon", 6],
    ["k1_08", "Pérola", "iridescente", 6], ["k1_13", "Cósmico", "órbitas", 6], ["k1_15", "Bronze orgânico", "escultura", 7]
  ].map(function (k) { return { id: k[0], name: k[1], kind: k[2], level: k[3], cap: k[4] || null }; });
  // looks saved before the painted knobs used the index of the old vector families
  const OLD_KNOB = ["k2_00", "k2_02", "k1_12", "k2_01", "k2_10", "k1_10", "k2_09", "k2_08", "k2_04", "k2_05", "k2_07", "k1_13", "k1_09", "k1_08", "kn_00", "kn_01", "kn_02", "kn_03", "kn_04", "kn_05", "kn_11", "kn_10", "kn_09", "kn_07"];
  function knobById(id) { for (let i = 0; i < KNOBS.length; i++) if (KNOBS[i].id === id) return KNOBS[i]; return KNOBS[0]; }
  function knobVars(target, k) {
    const st = target.style; st.setProperty("--kimg", "url(knob_" + k.id + ".png)");
    if (k.cap) { const c = k.cap; st.setProperty("--ccx", (c[0] / 1.6) + "%"); st.setProperty("--ccy", (c[1] / 1.6) + "%"); st.setProperty("--crx", (c[2] / 1.6) + "%"); st.setProperty("--cry", (c[3] / 1.6) + "%"); st.setProperty("--csy", (c[3] / c[2]).toFixed(3)); st.setProperty("--csyi", (c[2] / c[3]).toFixed(3)); }
    target.classList.toggle("kpersp", !!k.cap);
  }
  function applyKnobClass() { document.body.classList.add("kimg"); knobVars(document.body, knobById(look.knob)); }
  const PALETTES = [
    { id: 8, name: "Orbit Night", cls: "", level: 1, sw: ["#b06bff", "#2fe0ff", "#ff4fd8", "#f5ecff", "#12071c"] },
    { id: 0, name: "Solar Dusk", cls: "pal-dusk", level: 1, sw: ["#ff7a3d", "#ffb340", "#ff4f7a", "#ffe7c4", "#1a0a12"] },
    { id: 2, name: "Cosmic Blue", cls: "pal-cosmic", level: 2, sw: ["#2fe0ff", "#6f8bff", "#9ff3ff", "#eaf4ff", "#050b1e"] },
    { id: 3, name: "Vinho", cls: "pal-wine", level: 3, sw: ["#ff6f9a", "#ffb347", "#d58bff", "#fff1e4", "#16050c"] },
    { id: 1, name: "Sunrise", cls: "pal-sunrise", level: 2, sw: ["#ff8a3d", "#ffd27a", "#8a3cf0", "#2b170c", "#f2e2cf"] }
  ];
  function paletteFor(x) { x = Math.round(x); for (let i = 0; i < PALETTES.length; i++) if (PALETTES[i].id === x) return PALETTES[i]; return [2, 5, 6, 8].indexOf(x) >= 0 ? PALETTES[0] : PALETTES[1]; }
  const CHALLENGES = [
    { id: "voice", name: "Da gravação ao patch", text: "Capture ou importe um som e aplique uma opção do Patch Lab.", xp: 40 },
    { id: "parallel", name: "Textura em paralelo", text: "PARALLEL com DUST e BLOOM ligados; toque 8 notas.", xp: 30 },
    { id: "order", name: "A ordem importa", text: "Em SERIAL, troque a ordem de dois módulos e toque 4 notas.", xp: 25 },
    { id: "degrade", name: "Desce pro degradado", text: "Com CRUSH ou CRUMBLE ligados, leve o Y acima de 80% tocando.", xp: 25 },
    { id: "dice", name: "Dado da sorte", text: "Gere um patch com RANDOMIZE e fique com ele.", xp: 20 },
    { id: "loop", name: "Loop costurado", text: "LOOP com crossfade acima de 10 ms; segure uma nota 2 s.", xp: 25 }
  ];
  const game = {
    state: { xp: 0, level: 1, on: 1, looks: [], done: {}, seen: {}, favs: [], lang: "pt" }, combo: 0, lastNote: 0, flags: {}, notesSince: {},
    load: function (json) {
      let s = null;
      try { s = JSON.parse(json || "null"); } catch (e) { s = null; }
      if (!s) { try { s = JSON.parse(localStorage.getItem("rastro.game") || "null"); } catch (e) { s = null; } }
      if (s) { this.state.lang = s.lang || "pt"; this.state.xp = s.xp || 0; this.state.level = Math.max(1, s.level || 1); this.state.on = s.on === 0 ? 0 : 1; this.state.looks = s.looks || []; this.state.done = s.done || {}; this.state.seen = s.seen || {}; this.state.favs = s.favs || []; }
    },
    save: function () { clearTimeout(this._t); const st = this.state; this._t = setTimeout(function () { R.call("progressSet", JSON.stringify(st)); }, 400); },
    need: function (lvl) { return 60 + lvl * 40; },
    xpTo: function (lvl) { let n = this.need(this.state.level) - this.state.xp; for (let l = this.state.level + 1; l < lvl; l++) n += this.need(l); return Math.max(0, Math.ceil(n)); },
    unlocksAt: function (lvl) {
      const out = [];
      for (const k in PUP.LOOKS) if (PUP.LOOKS[k].level === lvl) out.push("visual " + PUP.LOOKS[k].name);
      AL.DANCES.forEach(function (d) { if (d.level === lvl) out.push("dança " + d.pt); });
      KNOBS.forEach(function (k) { if (k.level === lvl) out.push("knob " + k.name); });
      PALETTES.forEach(function (p) { if (p.level === lvl) out.push("cores " + p.name); });
      CHARS.forEach(function (c) { if (c.level === lvl) out.push(c.name); });
      return out;
    },
    nextLevelWithItems: function () { for (let l = this.state.level + 1; l < 12; l++) if (this.unlocksAt(l).length) return l; return 0; },
    award: function (xp, why) {
      if (!this.state.on) return;
      this.state.xp += xp;
      while (this.state.xp >= this.need(this.state.level)) {
        this.state.xp -= this.need(this.state.level); this.state.level++;
        popup("NÍVEL " + this.state.level + "!", 240, 150, "#ffd27a");
        const got = this.unlocksAt(this.state.level);
        toast(got.length ? "Liberou: " + got.join(", ") + ". Veja no LOCKER." : "Nível " + this.state.level + "!", 5000);
      }
      this.save(); this.hud();
      if (why && why !== "note" && why !== "tweak") popup("+" + xp + " XP", 240, 300, "#2fe0ff");
    },
    hud: function () {
      const need = this.need(this.state.level), nl = this.nextLevelWithItems();
      $("#lvlBadge").textContent = "NV " + this.state.level;
      $("#xpFill").style.width = Math.min(100, this.state.xp / need * 100) + "%";
      $("#nextReward").textContent = nl ? this.unlocksAt(nl)[0] + " · " + this.xpTo(nl) + " XP" : "tudo liberado!";
      document.body.classList.toggle("game-off", !this.state.on);
      $("#lkLvl").textContent = this.state.level; $("#lkBar").style.width = Math.min(100, this.state.xp / need * 100) + "%"; $("#lkXp").textContent = Math.floor(this.state.xp) + " / " + need + " XP";
    },
    note: function (vel, now) {
      if (!this.state.on) return;
      const gap = now - this.lastNote; this.lastNote = now;
      this.combo = gap < 1400 ? this.combo + 1 : 1;
      let xp = 1;
      if (T.hostPlaying && T.beat >= 0) { const ph = (T.beat * 2) % 1; if (ph < .12 || ph > .88) { xp += 2; if (Math.random() < .5) popup("NO TEMPO!", 150 + Math.random() * 180, 210, "#2fe0ff"); } }
      if (this.combo % 8 === 0) { const words = ["GROOVY!", "QUE RASTRO!", "ON FIRE!", "LENDÁRIO!"]; popup(words[Math.min(3, this.combo / 8 - 1 | 0)] + " ×" + this.combo, 240, 170, "#ffb340"); xp += 4; }
      this.award(xp, "note");
      const c = $(".hud-combo"); $("#comboNum").textContent = this.combo; c.classList.add("bump"); setTimeout(function () { c.classList.remove("bump"); }, 90);
      for (const k in this.notesSince) this.notesSince[k]++;
      this.check();
    },
    tick: function (now) { if (this.combo && now - this.lastNote > 2200) { this.combo = 0; $("#comboNum").textContent = "0"; } },
    event: function (name) { this.flags[name] = performance.now(); if (name === "reorder" || name === "parallel") this.notesSince[name] = 0; this.check(); },
    complete: function (id) {
      if (this.state.done[id] || !this.state.on) return;
      const c = CHALLENGES.filter(function (x) { return x.id === id; })[0];
      this.state.done[id] = Date.now(); popup("DESAFIO!", 240, 200, "#ffd27a"); toast("Desafio concluído: " + c.name + " (+" + c.xp + " XP)", 4000);
      this.award(c.xp, "challenge"); if (screen() === "locker") renderLocker();
    },
    check: function () {
      const f = this.flags;
      if ((f.import || f.capture) && f.lab) this.complete("voice");
      if (v("rackParallel") > .5 && modOn(4) && modOn(2) && (this.notesSince.parallel || 0) >= 8) this.complete("parallel");
      if (v("rackParallel") < .5 && f.reorder && (this.notesSince.reorder || 0) >= 4) this.complete("order");
      if (f.diceKeep) this.complete("dice");
      if (v("playY") > .8 && (modOn(5) || modOn(1)) && anyNoteHeld()) this.complete("degrade");
      if (v("regionLoop") > .5 && v("loopCrossfade") > 10 && heldFor > 2) this.complete("loop");
    }
  };
  let heldFor = 0;
  function anyNoteHeld() { for (let w = 0; w < 4; w++) if (T.notes[w]) return true; return false; }
  function popup(text, x, y, color) {
    if (!game.state.on || paused || !$("#stage").clientWidth) return;
    const st = $("#stage"), sx = st.clientWidth / 480;
    const p = document.createElement("div"); p.className = "pop"; p.textContent = text; p.style.left = (x * sx - 60) + "px"; p.style.top = y + "px"; p.style.color = color || "#fff";
    $("#popups").appendChild(p); setTimeout(function () { p.remove(); }, 950);
  }

  /* ================= LOCKER ================= */
  let animMode = 2, lkCat = "looks", lockerPup = null, instFilter = "all";
  // ESTÚDIO (Locker). WARDROBE = complete looks; MIX & MATCH = one slot at a time.
  // Real today: outfits (complete painted looks), instruments, knobs, palettes, stage, dance, glow, reactivity.
  // Hair, headwear, eyes and audio need the character art in separate layers: shown as EM BREVE, never as working.
  const LK_CATS = [["hair", "Hair", "face", "em breve"], ["headwear", "Headwear", "hat", "em breve"], ["eyewear", "Eyes", "glasses", "em breve"], ["audio", "Audio", "phones", "em breve"],
    ["looks", "Outfit", "jacket", "visual completo"], ["inst", "Instruments", "guitar", "pose e gestos"], ["knobs", "Knobs", "knob", "todos os knobs reais"], ["palettes", "Palette", "palette", "cores da interface"],
    ["stage", "Stage", "room", "onde ele toca"], ["dance", "Dança", "dance", "estilos e tempo"], ["glow", "Brilho", "glow", "cor da luz"], ["react", "Reatividade", "spark", "intensidade visual"],
    ["guests", "Convidados", "face", "outros personagens"], ["saved", "Meus looks", "look", "salvos"], ["challenges", "Desafios", "star", "XP opcional"]];
  const OUTFIT_ART = { keytar: "it_o_varsity", guitar: "it_o_utility", bass: "it_o_disco", pads: "it_o_holo" };
  const INST_ART = { keytar: "it_keytar", guitar: "it_guitar", bass: "it_bass", pads: "it_pads" };
  const INST_NAME = { keytar: "Keytar", guitar: "Guitarra estrela", bass: "Baixo", pads: "Controlador", none: "Sem instrumento" };
  const SOON = {
    hair: [["it_hr_flame", "Retro Wave"], ["it_hr_curls", "Curl Cloud"], ["it_hr_spikes", "Chrome Quiff"], ["it_hr_orbit", "Orbit Locs"]],
    headwear: [["it_h_beanie_night", "Gorro Noite"], ["it_h_cap_sunset", "Boné Sunset"], ["it_h_bucket_violet", "Bucket Violeta"], ["it_h_beanie", "Gorro Estrela"], ["it_h_bucket", "Bucket Swirl"], ["it_h_aviator", "Aviador"], ["it_h_fedora", "Fedora Psicodélico"], ["it_h_visor", "Viseira Neon"], ["it_h_cap", "Boné Cyber"]],
    audio: [["it_a_inear_cyan", "In-ear Ciano"], ["it_a_inear_sun", "In-ear Sunset"], ["it_a_inear_violet", "In-ear Violeta"], ["it_a_orbit", "Neon Orbit"], ["it_a_studio", "Studio Chrome"], ["it_a_retro", "Retro Groove"], ["it_a_velocity", "Velocity Ice"]],
    eyewear: [["it_e_visor_sun", "Visor Sunset"], ["it_e_round_gold", "Redondo Dourado"], ["it_e_rect_violet", "Retângulo Violeta"], ["it_e_visor", "Visor Violeta"], ["it_e_round", "Redondo Âmbar"], ["it_e_tri", "Triângulo Cyber"], ["it_e_ice", "Ski Ice"], ["it_e_sport", "Sport Sunset"], ["it_e_cat", "Gatinho Neon"]],
    looks: [["it_o_neon", "Jaqueta Neon"], ["it_o_silver", "Bomber Prata"], ["it_o_swirl", "Camisa Swirl"]]
  };
  let lkMode = "wardrobe";
  let lockerBuilt = false;
  function buildLocker() { $("#lockerSvg").innerHTML = ""; lockerBuilt = true; }
  function flash(el) { el.style.boxShadow = "0 0 26px rgba(255,179,64,.8)"; setTimeout(function () { el.style.boxShadow = ""; }, 600); }
  function lockerLines() { /* the slot circles were replaced by the RASTRO summary card */ }
  function itemCard(opts) {
    const b = document.createElement("button"); b.className = "it" + (opts.worn ? " worn" : "") + (opts.locked ? " locked" : "");
    b.innerHTML = '<div class="thumb">' + opts.thumb + '</div><b>' + opts.name + '</b><small>' + opts.note + '</small>' + (opts.worn ? '<span class="badge" data-ic="check"></span><span class="eq">EQUIPPED</span>' : "") + (opts.locked ? '<span class="lock" data-ic="lock"></span>' : "");
    ICO.hydrate(b); b.addEventListener("click", opts.click); return b;
  }
  function lockedNote(lvl) { return "🔒 nível " + lvl + " · " + game.xpTo(lvl) + " XP"; }
  function soonCard(file, name) {
    const b = itemCard({ thumb: '<img src="' + file + '.jpg" alt="">', name: name, note: "precisa da arte em partes", worn: false, locked: false,
      click: function () { toast("Em breve: para vestir " + name + " o personagem precisa vir em partes separadas (cabeça, tronco, braços). A peça ainda não funciona, por isso está marcada."); } });
    b.classList.add("soon"); b.insertAdjacentHTML("beforeend", '<span class="soon-tag">EM BREVE</span>'); return b;
  }
  function lookThumb(k) { const L = PUP.LOOKS[k]; return L.outfit ? '<img src="' + L.img + '.png" alt="" class="full">' : OUTFIT_ART[k] ? '<img src="' + OUTFIT_ART[k] + '.jpg" alt="">' : '<img src="' + L.img + '.png" alt="" class="full">'; }
  function wearLook(k) { const L = PUP.LOOKS[k]; if (L.level > game.state.level) { toast("Libera no nível " + L.level + " (faltam " + game.xpTo(L.level) + " XP)."); return; } look.pose = k; if (charKind !== "alien") gesture("avatarCharacter", 6); saveLook(); applyLook(); }
  function renderWardrobe() {
    const w = $("#lkWardrobe"); w.innerHTML = "";
    Object.keys(PUP.LOOKS).forEach(function (k) {
      const L = PUP.LOOKS[k], locked = L.level > game.state.level, on = look.pose === k && charKind === "alien";
      const b = document.createElement("button"); b.className = "wcard" + (on ? " on" : "") + (locked ? " locked" : "");
      b.innerHTML = '<div class="wc-head"><b>' + L.name.split(" · ")[0] + '</b><span class="wc-xp">' + (locked ? "🔒 " + game.xpTo(L.level) + " XP" : "NV " + L.level) + '</span></div>' +
        '<div class="wc-art"><img src="' + L.img + '.png" alt=""></div><small>' + (L.inst === "none" ? "sem instrumento" : INST_NAME[L.inst]) + '</small>' + (on ? '<span class="badge" data-ic="check"></span>' : "");
      ICO.hydrate(b); b.addEventListener("click", function () { wearLook(k); }); w.appendChild(b);
    });
  }
  function renderSummary() {
    const L = PUP.LOOKS[look.pose], box = $("#lkSumList"), dn = AL.DANCES.filter(function (d) { return d.id === look.dance; })[0];
    const rows = [["looks", "jacket", "OUTFIT", L.name.split(" · ")[0]], ["inst", "guitar", "INSTRUMENTO", INST_NAME[L.inst]], ["knobs", "knob", "KNOBS", knobById(look.knob).name],
      ["stage", "room", "STAGE", ART.roomNames[Math.round(v("sceneRoom"))] || "—"], ["dance", "dance", "DANÇA", dn ? dn.pt : "—"]];
    box.innerHTML = "";
    rows.forEach(function (r) {
      const b = document.createElement("button"); b.className = "sum-row" + (lkMode === "mix" && lkCat === r[0] ? " on" : "");
      b.innerHTML = '<i data-ic="' + r[1] + '"></i><span><small>' + r[2] + '</small><b>' + r[3] + '</b></span><em data-ic="right"></em>';
      ICO.hydrate(b); b.addEventListener("click", function () { lkMode = "mix"; lkCat = r[0]; renderLocker(); }); box.appendChild(b);
    });
  }
  function renderLocker() {
    if (!lockerBuilt) buildLocker();
    game.hud();
    document.body.dataset.lkm = lkMode;
    $$("#lkModeSeg button").forEach(function (b) { b.classList.toggle("on", b.dataset.lkm === lkMode); });
    $("#lkSayT").textContent = lkMode === "wardrobe" ? "LOOKS COMPLETOS" : "MIX & MATCH / ITEM SLOTS";
    $("#lkSayS").textContent = lkMode === "wardrobe" ? "Escolha um visual inteiro. Toque notas para ver ele dançar." : "Uma peça de cada vez. Só o visual muda, nunca o som.";
    renderSummary();
    if (lkMode === "wardrobe") { renderWardrobe(); return; }
    const cats = $("#lkCats"); cats.innerHTML = "";
    LK_CATS.forEach(function (c) {
      const b = document.createElement("button"); b.className = lkCat === c[0] ? "on" : "";
      b.innerHTML = '<i data-ic="' + c[2] + '"></i><span>' + c[1] + '<small>' + c[3] + '</small></span>'; ICO.hydrate(b);
      b.addEventListener("click", function () { lkCat = c[0]; renderLocker(); }); cats.appendChild(b);
    });
    const grid = $("#itemGrid"); grid.innerHTML = ""; grid.className = "item-grid three";
    const title = LK_CATS.filter(function (c) { return c[0] === lkCat; })[0];
    $("#slotTitle").textContent = title ? title[1].toUpperCase() : ""; $("#slotNote").textContent = "";
    if (lkCat === "hair" || lkCat === "headwear" || lkCat === "audio" || lkCat === "eyewear") {
      $("#slotNote").textContent = "em breve: precisa do personagem em camadas";
      SOON[lkCat].forEach(function (it) { grid.appendChild(soonCard(it[0], it[1])); });
    } else if (lkCat === "knobs") {
      $("#slotNote").textContent = "muda todos os knobs reais"; grid.className = "item-grid knobs"; renderKnobs(grid);
    } else if (lkCat === "palettes") {
      $("#slotNote").textContent = "tema da interface inteira"; grid.className = "item-grid one"; renderPalettes(grid);
    } else if (lkCat === "challenges") {
      $("#slotNote").textContent = "opcional · XP só libera visual"; grid.className = "item-grid one"; renderChallenges(grid, 99);
    } else if (lkCat === "stage") {
      $("#slotNote").textContent = "o quarto também reage ao som";
      ART.rooms.forEach(function (rk, i) {
        grid.appendChild(itemCard({ thumb: '<div style="width:100%;height:100%;border-radius:8px;background:url(room_' + rk + '.jpg) center/cover"></div>', name: ART.roomNames[i], note: i === 5 ? "plataforma e luzes vivas" : "quarto pintado", worn: Math.round(v("sceneRoom")) === i, locked: false,
          click: function () { gesture("sceneRoom", i); setTimeout(renderLocker, 50); } }));
      });
    } else if (lkCat === "looks") {
      $("#slotNote").textContent = "nesta fase a roupa vem com a pose pintada";
      Object.keys(PUP.LOOKS).forEach(function (k) {
        const L = PUP.LOOKS[k], locked = L.level > game.state.level;
        grid.appendChild(itemCard({ thumb: lookThumb(k), name: L.name.split(" · ")[0], note: locked ? lockedNote(L.level) : look.pose === k ? "equipado" : "liberado", worn: look.pose === k && charKind === "alien", locked: locked,
          click: function () { if (locked) { toast("Libera no nível " + L.level + " (faltam " + game.xpTo(L.level) + " XP)."); return; } look.pose = k; if (charKind !== "alien") gesture("avatarCharacter", 6); saveLook(); applyLook(); } }));
      });
      SOON.looks.forEach(function (it) { grid.appendChild(soonCard(it[0], it[1])); });
    } else if (lkCat === "inst") {
      $("#slotNote").textContent = "cada instrumento tem pose e gestos próprios"; renderInst(grid);
    } else if (lkCat === "dance") {
      AL.DANCES.forEach(function (d) {
        const locked = d.level > game.state.level;
        grid.appendChild(itemCard({ thumb: '<span data-ic="dance" style="width:56px;height:56px;color:' + (look.dance === d.id ? "var(--gold)" : "var(--violet)") + '"></span>', name: d.pt, note: locked ? lockedNote(d.level) : d.tip, worn: look.dance === d.id, locked: locked,
          click: function () { if (locked) { toast("Libera no nível " + d.level + "."); return; } look.dance = d.id; saveLook(); renderLocker(); } }));
      });
      const opts = document.createElement("div"); opts.className = "dance-opts"; opts.style.gridColumn = "1 / -1";
      opts.innerHTML = '<div class="c-row"><span>Intensidade</span><div class="seg seg-small" id="danceAmtSeg"><button data-da="0">Parado</button><button data-da="1">Groove</button><button data-da="2">Expressivo</button></div></div>' +
        '<div class="c-row"><span>Tempo</span><div class="seg seg-small" id="danceSyncSeg"><button data-sync="1" title="Segue o pulso do Logic (ou o TEMPO do plugin com o Logic parado)">SYNC</button><button data-sync="0" title="BPM próprio">FREE</button></div></div>' +
        '<div class="c-row"><span>BPM livre</span><div class="bpm-ctl"><button class="chip-btn" id="bpmDown">−</button><span id="bpmVal" class="mono"></span><button class="chip-btn" id="bpmUp">+</button></div></div><p class="hint">Notas e velocity viram acentos; agudo ou grave não muda a velocidade. Sem som, ele volta ao repouso.</p>';
      grid.appendChild(opts);
      dropOwned("lk"); const f = seg("#danceAmtSeg", "da", function () { return Math.round(v("danceAmount")); }, function (x) { gesture("danceAmount", x); }); f.owned = "lk"; watch("danceAmount", f);
      const syncR = seg("#danceSyncSeg", "sync", function () { return look.sync; }, function (x) { look.sync = x; saveLook(); syncR(); }); syncR();
      $("#bpmVal").textContent = look.bpm + " BPM";
      $("#bpmDown").addEventListener("click", function () { look.bpm = clamp(look.bpm - 2, 60, 180); $("#bpmVal").textContent = look.bpm + " BPM"; saveLook(); });
      $("#bpmUp").addEventListener("click", function () { look.bpm = clamp(look.bpm + 2, 60, 180); $("#bpmVal").textContent = look.bpm + " BPM"; saveLook(); });
    } else if (lkCat === "glow") {
      AL.ITEMS.glow.forEach(function (it) {
        const locked = it[3] > game.state.level, col = AL.GLOW[it[0]];
        grid.appendChild(itemCard({ thumb: it[0] === "auto" ? '<div style="width:70px;height:70px;border-radius:50%;background:conic-gradient(#b06bff,#ffa02e,#ff4fd8,#2fe0ff,#ffd23f,#8a4dff,#b06bff);filter:blur(1px);box-shadow:0 0 18px #ff4fd8"></div>' : '<div style="width:70px;height:70px;border-radius:50%;background:radial-gradient(' + col + ',transparent 70%);box-shadow:0 0 24px ' + col + '"></div>',
          name: it[1], note: locked ? lockedNote(it[3]) : it[0] === "auto" ? "mistura as cores dos efeitos que soam" : "cor de base fixa", worn: look.glow === it[0], locked: locked,
          click: function () { if (locked) return; look.glow = it[0]; saveLook(); renderLocker(); } }));
      });
    } else if (lkCat === "react") {
      const box = document.createElement("div"); box.style.gridColumn = "1 / -1";
      box.innerHTML = '<p class="hint">VISUAL REACTIVITY: quanto as luzes e o movimento reagem ao som. Só muda a apresentação, nunca o áudio.</p><div class="knob-row" id="reactKnob"></div><div class="c-row" style="margin-top:12px"><span>Animação</span><div class="seg seg-small" id="animSeg2"><button data-anim="2">Completa</button><button data-anim="1">Reduzida</button><button data-anim="0">Off</button></div></div>';
      grid.appendChild(box); reactKnob($("#reactKnob"), 64);
      dropOwned("lk"); const f = seg("#animSeg2", "anim", function () { return Math.round(v("animation")); }, function (x) { gesture("animation", x); }); f.owned = "lk"; watch("animation", f);
    } else if (lkCat === "guests") {
      [{ key: "alien", name: "Alien (principal)", species: 6 }].concat(CHARS).forEach(function (c) {
        const locked = c.level && game.state.level < c.level, ps = ART.paintedSize[c.key];
        const thumb = c.key === "alien" ? '<img src="' + PUP.LOOKS[look.pose].img + '.png" alt="">' : ps ? '<span class="pc" style="position:relative;height:100%;aspect-ratio:' + ps[0] + '/760;display:block"><img src="' + c.key + '_body.png" alt="" style="position:absolute;inset:0;width:100%;height:100%"><img src="' + c.key + '_head.png" alt="" style="position:absolute;left:0;top:0;width:100%;height:' + (ps[1] / 760 * 100).toFixed(2) + '%"></span>' : "";
        grid.appendChild(itemCard({ thumb: thumb, name: c.name, note: locked ? lockedNote(c.level) : c.key === "alien" ? "customizável" : "visual fixo", worn: charKind === c.key, locked: locked, click: function () { if (locked) return; gesture("avatarCharacter", c.species); } }));
      });
    } else if (lkCat === "saved") {
      const save = document.createElement("button"); save.className = "gold-btn"; save.style.gridColumn = "1 / -1"; save.textContent = "SALVAR LOOK ATUAL";
      save.addEventListener("click", function () { $("#lookSave").click(); }); grid.appendChild(save);
      game.state.looks.forEach(function (lk, i) {
        const L = PUP.LOOKS[lk.pose] || PUP.LOOKS[PUP.ALIAS[lk.pose]] || PUP.LOOKS.disco;
        const b = itemCard({ thumb: '<img src="' + L.img + '.png" alt="">', name: lk.name, note: (lk.dance || "") + " · clique para vestir", worn: false, locked: false, click: function () { look.pose = PUP.LOOKS[lk.pose] ? lk.pose : PUP.ALIAS[lk.pose] || look.pose; if (lk.dance) look.dance = lk.dance; if (lk.glow) look.glow = lk.glow; if (typeof lk.knob === "number") look.knob = OLD_KNOB[lk.knob] || look.knob; else if (lk.knob) look.knob = lk.knob; saveLook(); applyLook(); toast("Look vestido: " + lk.name); } });
        const del = document.createElement("span"); del.className = "eq"; del.style.cursor = "pointer"; del.textContent = "APAGAR"; del.addEventListener("click", function (e) { e.stopPropagation(); game.state.looks.splice(i, 1); game.save(); renderLocker(); });
        b.appendChild(del); grid.appendChild(b);
      });
    }
  }
  function reactKnob(parent, size) {
    // VISUAL REACTIVITY lives in the look (presentation only). A tiny fake-param object lets the knob widget drive it.
    P.__react = { id: "__react", v: look.react, min: 0, max: 1.5, def: 1, step: 0 };
    dropOwned("react");
    const k = knob(parent, "__react", "VISUAL REACTIVITY", { size: size, color: "#ff4fd8", owned: "react", format: function (x) { return Math.round(x * 100) + "%"; } });
    k.addEventListener("pointerup", function () { look.react = P.__react.v; saveLook(); });
    k.addEventListener("wheel", function () { look.react = P.__react.v; saveLook(); });
    k.addEventListener("dblclick", function () { look.react = 1; P.__react.v = 1; saveLook(); });
    return k;
  }
  function renderInst(grid) {
    const chips = document.createElement("div"); chips.className = "chips"; chips.style.gridColumn = "1 / -1"; grid.appendChild(chips);
    [["all", "ALL"], ["keytar", "KEYTARS"], ["guitar", "GUITARS"], ["bass", "BASS"], ["pads", "SYNTHS"]].forEach(function (c) {
      const b = document.createElement("button"); b.className = instFilter === c[0] ? "on" : ""; b.textContent = c[1]; b.addEventListener("click", function () { instFilter = c[0]; renderLocker(); }); chips.appendChild(b);
    });
    Object.keys(PUP.LOOKS).forEach(function (k) {
      const L = PUP.LOOKS[k]; if (L.inst === "none" || (instFilter !== "all" && L.inst !== instFilter)) return;
      const locked = L.level > game.state.level, name = { keytar: "Keytar", guitar: "Guitarra estrela", bass: "Baixo", pads: "Controlador" }[L.inst] + (k === "disco" ? " 70s" : "");
      grid.appendChild(itemCard({ thumb: '<img src="' + INST_ART[L.inst] + '.jpg" alt="">', name: name, note: locked ? lockedNote(L.level) : "pose e gestos próprios", worn: look.pose === k && charKind === "alien", locked: locked,
        click: function () { wearLook(k); } }));
    });
  }
  function knobPreview(k) {
    const d = document.createElement("div"); d.className = "kp knob kp-img"; d.style.setProperty("--ks", "64px"); d.style.setProperty("--kr", "40deg");
    d.innerHTML = KIMG_HTML; knobVars(d, k); return d;
  }
  function renderKnobs(kg) {
    KNOBS.forEach(function (k) {
      const locked = k.level > game.state.level, on = look.knob === k.id;
      const b = document.createElement("button"); b.className = "kchoice" + (on ? " on" : "") + (locked ? " locked" : "");
      b.appendChild(knobPreview(k));
      b.insertAdjacentHTML("beforeend", '<b>' + k.name + '</b><small>' + (locked ? "🔒 nível " + k.level : on ? "EQUIPPED" : k.kind) + '</small>' + (on ? '<span class="badge" data-ic="check"></span>' : ""));
      ICO.hydrate(b);
      b.addEventListener("click", function () { if (locked) { toast("Libera no nível " + k.level + " (faltam " + game.xpTo(k.level) + " XP)."); return; } look.knob = k.id; applyKnobClass(); saveLook(); renderLocker(); });
      kg.appendChild(b);
    });
  }
  function renderPalettes(pg) {
    const cur = paletteFor(v("colourSet"));
    PALETTES.forEach(function (p) {
      const locked = p.level > game.state.level;
      const b = document.createElement("button"); b.className = "pal-row" + (cur === p ? " on" : "") + (locked ? " locked" : "");
      b.innerHTML = p.sw.map(function (c) { return '<i style="background:' + c + ';box-shadow:0 0 8px ' + c + '"></i>'; }).join("") + '<b>' + p.name + (locked ? " 🔒" : "") + '</b>';
      b.addEventListener("click", function () { if (locked) { toast("Libera no nível " + p.level + "."); return; } gesture("colourSet", p.id); });
      pg.appendChild(b);
    });
  }
  function renderChallenges(list, n) {
    CHALLENGES.forEach(function (c) {
      const done = !!game.state.done[c.id];
      const d = document.createElement("div"); d.className = "ch" + (done ? " done" : ""); d.innerHTML = '<div style="flex:1"><b>' + c.name + (done ? " ✓" : "") + '</b>' + c.text + '</div><span class="xpv">+' + c.xp + ' XP</span>'; list.appendChild(d);
    });
  }
  function setupLocker() {
    $("#lookUndo").hidden = true;
    $("#lookRandom").addEventListener("click", function () {
      const ok = Object.keys(PUP.LOOKS).filter(function (k) { return PUP.LOOKS[k].level <= game.state.level; });
      const ds = AL.DANCES.filter(function (d) { return d.level <= game.state.level; });
      look.pose = ok[(Math.random() * ok.length) | 0]; look.dance = ds[(Math.random() * ds.length) | 0].id; saveLook(); applyLook();
    });
    $("#lookSave").addEventListener("click", function () {
      if (game.state.looks.length >= 12) { toast("Limite de 12 looks. Apague um para salvar outro."); return; }
      game.state.looks.push({ name: "Look " + (game.state.looks.length + 1), pose: look.pose, dance: look.dance, glow: look.glow, knob: look.knob }); game.save(); toast("Look salvo em Meus looks.");
      if (screen() === "locker") renderLocker();
    });
    $$("#lkModeSeg button").forEach(function (b) { b.addEventListener("click", function () { lkMode = b.dataset.lkm; renderLocker(); }); });
  }
  function setupOptions() {
    watch("sceneRoom", function (x) { if (charSvg && sceneKind !== ART.rooms[Math.round(x)]) buildStage(); });
    watch("colourSet", function (x) {
      const p = paletteFor(x);
      PALETTES.forEach(function (q) { if (q.cls) document.body.classList.remove(q.cls); });
      if (p.cls) document.body.classList.add(p.cls);
      readPalette(); drawEnv(); drawFilter(); drawLfo(); if (screen() === "locker") renderLocker();
    });
    watch("avatarCharacter", function (x) {
      const k = SPECIES[Math.round(x)] || "alien";
      if (k !== charKind) { charKind = k; if (charSvg) buildCharacter(); }
      if (screen() === "locker") renderLocker();
    });
    bindSeg("#animSeg", "anim", "animation");
    watch("animation", function (x) { animMode = Math.round(x); document.body.classList.toggle("anim-off", animMode === 0); });
    const gameRefresh = seg("#gameSeg", "game", function () { return game.state.on ? 1 : 0; }, function (x) { game.state.on = x; game.save(); game.hud(); gameRefresh(); });
    gameRefresh();
    // JAM = performance mode (character, patch, keyboard, essential controls). Same state, nothing is lost.
    let beforeJam = "limpo";
    $("#jam").addEventListener("click", function () {
      if (screen() === "palco") { setScreen(beforeJam === "palco" ? "limpo" : beforeJam); return; }
      beforeJam = screen(); setScreen("palco");
    });
    $("#jamExit").addEventListener("click", function () { setScreen(beforeJam === "palco" ? "limpo" : beforeJam); });
    // VARIAR FX (formerly the JAM button): new values only on the effects that are on; undo keeps working
    $("#varyFx").addEventListener("click", function () {
      R.call("jam").then(function (can) { $("#undoJam").disabled = !can; });
      jamBurst = 1; motion.hit = true; spawnNotes(240, 220, 6, .9); game.award(5, "jam"); popup("VARIOU!", 240, 190, "#ffb340");
    });
    $("#undoJam").addEventListener("click", function () { R.call("undoJam").then(function (can) { $("#undoJam").disabled = !can; }); toast("Voltou à combinação anterior à variação."); });
  }

  /* ================= transport, language ================= */
  let paused = false, jamBurst = 0;
  $("#stripBtn").addEventListener("click", function () { look.strip = look.strip ? 0 : 1; document.body.classList.toggle("strip-off", !look.strip); saveLook(); });
  function setPaused(p) { paused = p; document.body.classList.toggle("paused", p); $("#pausedBadge").hidden = !p; }
  $("#tPlay").addEventListener("click", function () {
    if (!sample.loaded) { toast("Importe ou escolha um som primeiro."); return; }
    setPaused(false); R.call("preview", true, cue >= 0 ? cue : v("start"));
  });
  $("#tPause").addEventListener("click", function () {
    if (paused) { $("#tPlay").click(); return; }
    if (T.preview) { cue = T.pos; R.call("preview", false); markerInfo(); waveDirty = true; }
    R.call("allNotesOff"); setPaused(true);
  });
  $("#tStop").addEventListener("click", function () {
    R.call("preview", false); R.call("allNotesOff");
    if (T.capture && T.capture.state === 1) R.call("captureStop");
    cue = -1; markerInfo(); waveDirty = true; setPaused(false);
    motion.alive = 0; motion.lastSound = -1e9; motion.energy = 0;
    if (puppet) puppet.pose();
  });
  function setLang(l) {
    lang = l; document.body.classList.toggle("lang-en", l === "en"); document.documentElement.lang = l === "en" ? "en" : "pt-BR";
    const EN = { "nav.sons": "SAMPLES", "nav.patches": "INSTRUMENTS", "nav.favs": "FAVORITES", save: "SAVE" }, PT = { "nav.sons": "SONS", "nav.patches": "PATCHES", "nav.favs": "FAVORITOS", save: "SALVAR" };
    $$("[data-i18n]").forEach(function (el) { const t = (l === "en" ? EN : PT)[el.dataset.i18n]; if (t) el.textContent = t; });
    $("#langState").textContent = l === "en" ? "English" : "Português";
  }
  $("#langBtn").addEventListener("click", function () { setLang(lang === "pt" ? "en" : "pt"); game.state.lang = lang; game.save(); });

  /* ================= frame loop ================= */
  let lastSerial = -1, lastFrame = performance.now(), midiFlash = 0, beatClock = 0, lastNoteAt = -1e9;
  function noteHeld(n) { return ((T.notes[n >> 5] >>> 0) & (1 << (n & 31))) !== 0; }
  function hexRgb(h) { const n = parseInt(h.slice(1), 16); return [n >> 16 & 255, n >> 8 & 255, n & 255]; }
  function rgbHex(c) { return "#" + c.map(function (x) { const h = Math.round(clamp(x, 0, 255)).toString(16); return h.length < 2 ? "0" + h : h; }).join(""); }
  function mixHex(a, b, t) { const x = hexRgb(a), y = hexRgb(b); return rgbHex([0, 1, 2].map(function (k) { return x[k] + (y[k] - x[k]) * t; })); }
  function glowColour(base, acts) {
    let sum = 0; const mix = [0, 0, 0];
    acts.forEach(function (a, i) { if (a <= 0) return; const c = hexRgb(MOD[i].color); sum += a; for (let k = 0; k < 3; k++) mix[k] += c[k] * a; });
    if (sum < .02) return base;
    const b = hexRgb(base), t = clamp(sum * 1.6, 0, 1);
    return rgbHex([0, 1, 2].map(function (k) { return b[k] + (mix[k] / sum - b[k]) * t; }));
  }
  let placeKey = "", placeVal = { y: 400, s: .5 };
  function charPlace() {
    if (sc.world) return { x: ART.stage.feet[0], y: ART.stage.feet[1] + 2, s: 520 / 700, world: true };
    const st = $("#stage"), W = st.clientWidth, H = st.clientHeight, key = W + "x" + H;
    if (key === placeKey || !W || !H) return placeVal;
    const k = Math.max(W / 480, H / 452), visH = H / k, bottom = 226 + visH / 2;
    const palco = screen() === "palco";
    placeKey = key; placeVal = { y: bottom - (palco ? 118 : 14) / k - 4, s: clamp(visH * (palco ? .66 : .8) / 700, .25, .62) };
    return placeVal;
  }
  function frame(now) {
    const dt = Math.min(.05, (now - lastFrame) / 1000); lastFrame = now;
    if (document.hidden) { requestAnimationFrame(frame); return; } // nothing to draw while the window is hidden
    const react = look.react;
    const amp = (animMode === 2 ? 1 : animMode === 1 ? .5 : 0) * Math.min(1, react);
    const ease = function (rate) { return 1 - Math.exp(-rate * dt); };
    if (T.serial !== lastSerial) {
      if (lastSerial >= 0) {
        if (paused) setPaused(false);
        const vel = T.vel || .7;
        lastNoteAt = now; motion.hit = true; midiFlash = 1; motion.shake = vel * 2 * amp; motion.shakeT = 0;
        let lastN = -1; for (let n = 0; n < 128; n++) if (noteHeld(n)) lastN = n;
        if (lastN < 0) lastN = 60;
        motion.pitch = clamp((lastN - 36) / 48, 0, 1); motion.pad = ((lastN % 16) + 16) % 16; motion.vel = vel;
        if (rig.pads && rig.pads.length) { const p = rig.pads[motion.pad]; if (p) p.v = .6 + vel * .4; }
        rig.flash = 1; rig.flashPos = ((lastN - base) / 48);
        spawnNotes(240 + (Math.random() - .5) * 60, 200, 1 + Math.round(vel * 2), vel);
        game.note(vel, now);
      }
      lastSerial = T.serial;
    }
    game.tick(now);
    $("#midiDot") && $("#midiDot").classList.toggle("on", midiFlash > .1); midiFlash *= .85;
    $("#masterMeter").style.width = Math.min(100, T.meter * 100) + "%";
    let anyHeld = false; for (let w = 0; w < 4; w++) if (T.notes[w]) anyHeld = true;
    heldFor = anyHeld ? heldFor + dt : 0; if (anyHeld && heldFor > 2 && heldFor - dt <= 2) game.check();
    const acts = MOD.map(function (m, i) { return modOn(i) ? clamp((T.act[i] || 0) * 14, 0, 1) : 0; });
    legendEls.forEach(function (el, i) { el.classList.toggle("on", modOn(i)); });
    cards.forEach(function (c) { const a = acts[c.mi]; c.meter.style.width = (a * 80) + "%"; if (c.viz && animMode) c.viz.style.opacity = modOn(c.mi) ? (.55 + a * .45).toFixed(2) : ""; });
    for (const n in keyEls) keyEls[n].classList.toggle("on", noteHeld(Number(n)));
    $("#tPlay").classList.toggle("on", !!T.preview);
    if (waveDirty || T.playing || T.preview) { drawWave(); waveDirty = false; }
    if (paused) { requestAnimationFrame(frame); return; } // Pause freezes every movement where it is
    if (anyHeld || T.playing || T.preview) motion.lastSound = now;
    const aliveTarget = now - motion.lastSound < 900 ? 1 : 0;
    motion.alive += (aliveTarget - motion.alive) * ease(aliveTarget ? 6 : 1.4);
    motion.peak = Math.max(.02, Math.max(T.meter, motion.peak * Math.exp(-dt / 6)));
    const alive = motion.alive, meterN = clamp(T.meter / motion.peak, 0, 1);
    const energyT = alive * (.6 + .4 * meterN);
    motion.energy += (energyT - motion.energy) * ease(energyT > motion.energy ? 5 : 1.5);
    const syncHost = look.sync && T.hostPlaying && T.beat >= 0;
    const bpm = look.sync ? (T.bpm > 0 ? T.bpm : v("tempo")) : look.bpm, amtSel = Math.round(v("danceAmount"));
    if (syncHost) { let d = T.beat - (beatClock % 4); if (d < -2) d += 4; if (d > 2) d -= 4; beatClock += d; } else beatClock += dt * bpm / 60;
    if (beatClock > 4096) beatClock -= 4096;
    const phase = beatClock % 1, onBeat = Math.pow(1 - phase, 4), groove = motion.energy > .02 ? motion.energy : 0;
    const style = look.dance === "disco" ? 1 : look.dance === "half" ? 2 : 0;
    let lookTarget = mouseX * .8;
    for (let n = 0; n < 128; n++) if (noteHeld(n)) { lookTarget = clamp((n - 60) / 24, -1, 1); break; }
    motion.look += (lookTarget - motion.look) * ease(5);
    mx += (mouseX - mx) * ease(4); my += (mouseY - my) * ease(4);
    const synthOn = v("synthEnable") > .5, cutT = clamp(Math.log(Math.max(20, v("filterCutoff")) / 20) / Math.log(1000), 0, 1);
    motion.dark += ((synthOn ? (1 - cutT) * .45 : 0) - motion.dark) * ease(5);
    motion.lfoPhase = (motion.lfoPhase + dt * v("lfoRate") * Math.PI * 2) % (Math.PI * 2);
    const lfo = synthOn ? Math.sin(motion.lfoPhase) * v("lfoDepth") : 0, res = synthOn ? v("filterResonance") : 0;
    const wobOn = modOn(3), degOn = modOn(1) || modOn(4) || modOn(5);
    const camWob = (wobOn ? Math.max(0, v("playX") - .5) * 2 : 0) * alive, degrade = (degOn ? Math.max(0, v("playY") - .5) * 2 : 0) * alive;
    const ghostAct = acts[0] * alive, crumbleAct = acts[1] * alive, bloom = acts[2], wob = acts[3] * alive, dst = acts[4] * alive, crush = acts[5] * alive;
    motion.shakeT += dt; const shake = motion.shake * Math.exp(-motion.shakeT * 9) * Math.sin(motion.shakeT * 55);
    jamBurst = Math.max(0, jamBurst - dt * 1.5);
    const sc0 = screen(), stageOn = sc0 === "palco" || sc0 === "limpo" || sc0 === "perform" || sc0 === "locker";
    // light colour: effect colours (or fixed base), warmed and dimmed by a low filter cutoff, pulsed by the LFO
    let glow = look.glow === "auto" ? glowColour(AL.GLOW.cyan, acts.map(function (a) { return a * alive; })) : AL.GLOW[look.glow];
    if (synthOn) glow = mixHex(glow, "#ff8a3d", (1 - cutT) * .55);
    const envL = clamp(T.env || 0, 0, 1) * alive; // real envelope telemetry: follows attack, sustain and release
    const glowAmt = clamp((motion.energy * .35 + envL * .55 + (motion.vel || 0) * midiFlash * .3 + Math.abs(lfo) * .25 + jamBurst * .5) * (synthOn ? .55 + cutT * .45 : 1) * react, 0, 1);
    if (stageOn && charSvg && animMode > 0) {
      const L0 = charPlace();
      if (puppet) {
        const s = L0.s, L = puppet.L;
        const X0 = L0.x || 240;
        let ax = X0 - L.hip[0] * s + Math.sin(now / 260) * (wob + camWob * .4) * 5 * amp, ay = L0.y - L.feet * s;
        if (crush > .25) { const q = 2 + crush * 5; ax = Math.round(ax / q) * q; ay = Math.round(ay / q) * q; }
        rig.pos.setAttribute("transform", "translate(" + ax.toFixed(2) + " " + ay.toFixed(2) + ") scale(" + s.toFixed(4) + ")");
        puppet.update({ alive: sc0 === "locker" ? Math.max(alive, .35) : alive, intensity: Math.max(sc0 === "locker" ? .45 : 0, [0, .8, 1.15][amtSel] * clamp(motion.energy / .6, 0, 1.25)) * Math.min(1.25, react), beats: beatClock, style: look.dance, playing: anyHeld || (T.playing && now - lastNoteAt < 700),
          pitch: motion.pitch, pad: motion.pad, hit: motion.hit && amp > 0, vel: motion.vel || .7, look: motion.look, amp: amp, glow: glow, glowAmt: glowAmt, bloom: bloom * react, wob: wob * react, wobRate: v("wRate") * 4 }, dt);
        if (hero && heroLayer && !heroLayer.hidden)
          drawHero({ alive: sc0 === "locker" ? Math.max(alive, .35) : alive, intensity: Math.max(sc0 === "locker" ? .45 : 0, [0, .8, 1.15][amtSel] * clamp(motion.energy / .6, 0, 1.25)) * Math.min(1.25, react), beats: beatClock, style: look.dance,
            playing: anyHeld || (T.playing && now - lastNoteAt < 700), pitch: motion.pitch, hit: motion.hit && amp > 0, vel: motion.vel || .7, look: motion.look * .8 + mx * .4, amp: amp, glow: glow, glowAmt: glowAmt, bloom: bloom * react, wob: wob * react, crush: crush }, dt, X0, L0.y, s, ghostAct, react);
        // centre of the body in stage units (for particles and auras)
        const cx = X0, cy = L0.y - 330 * s, sz = 700 * s;
        if (fx.bloom) { const bs = (.7 + bloom * .55 + (v("presence") - .5) * .2) * sz / 330; fx.bloom.setAttribute("transform", "translate(" + cx + " " + (cy + 10 * s) + ") scale(" + bs.toFixed(3) + ")"); bloomV += ((bloom * alive) - bloomV) * ease(bloom * alive > bloomV ? 6 : 1 / (.3 + v("bDecay") * 2.5)); fx.bloom.setAttribute("opacity", (bloomV * .9 * react).toFixed(2)); }
        // GHOST: afterimages follow the body with a delay; spacing from the RASTRO macro, strength from feedback
        fx.hist.unshift(puppet.body.getAttribute("transform")); if (fx.hist.length > 40) fx.hist.pop();
        const spacing = Math.round(3 + v("trail") * 6);
        fx.ghosts.forEach(function (g, i) {
          const t = fx.hist[Math.min(fx.hist.length - 1, (i + 1) * spacing)] || "";
          g.setAttribute("transform", "translate(" + (ax - (i + 1) * 9 * ghostAct) + " " + ay + ") scale(" + s.toFixed(4) + ") " + t);
          g.setAttribute("opacity", Math.max(0, ghostAct * react * (.55 - i * .12) * (.6 + v("gFeedback") * .6)).toFixed(2));
        });
        stepParticles(dt, react, crumbleAct, dst, crush, cx, cy, sz);
        motion.hit = false;
      } else if (rig.pos) {
        motion.hit = false;
        const bounce = Math.abs(Math.sin(beatClock * Math.PI)), sway = Math.sin(beatClock * Math.PI * .5) * .8, dance = groove * amp * [0, .75, 1.3][amtSel];
        const tx = 240 + sway * 8 * dance, ty = 412 - bounce * 8 * dance;
        motion.x += (tx - motion.x) * ease(10); motion.y += (ty - motion.y) * ease(14);
        rig.pos.setAttribute("transform", "translate(" + motion.x.toFixed(2) + " " + motion.y.toFixed(2) + ") rotate(" + (sway * 2 * dance).toFixed(2) + ")");
        if (rig["c-head"]) setT(rig["c-head"], "rotate(" + (Math.sin(beatClock * Math.PI * 2 + .6) * 3.5 * dance + motion.look * 2.5).toFixed(2) + " " + rig.pivot[0] + " " + rig.pivot[1] + ")");
      }
      const camRot = Math.sin(now / 320) * camWob * 1.4 * amp;
      // notes no longer shake the whole painting; only WOBBLE + pad X (a real effect) sways the camera
      $("#stageArt").style.transform = camRot ? "rotate(" + camRot.toFixed(2) + "deg)" : "";
      if (sc.world) { stageCam(now, groove, onBeat, react, amp); stepWorld(now, dt, { glow: glow, react: react, beat: beatClock, onBeat: onBeat, groove: groove, energy: motion.energy, alive: alive, env: envL, bloom: bloomV, lfo: lfo, res: res, cut: synthOn ? cutT : .5 }); }
      setT(sc.room, "translate(" + sc.ox.toFixed(1) + " -18) scale(" + sc.scale.toFixed(4) + ")");
      sc.spk.forEach(function (g) { const cx = g.getAttribute("cx"), cy = g.getAttribute("cy"); g.setAttribute("opacity", ((motion.energy * .7 + onBeat * groove * .4) * react).toFixed(2)); g.setAttribute("transform", "translate(" + cx + " " + cy + ") scale(" + (.8 + motion.energy * .4 + onBeat * groove * .25).toFixed(2) + ") translate(" + (-cx) + " " + (-cy) + ")"); });
      setA(sc.bloomVeil, "opacity", (bloomV * (sc.world ? .1 : .25) * react).toFixed(3));
      setA(sc.darkVeil, "opacity", motion.dark.toFixed(3));
      setA(sc.lampGlow, "opacity", ((.15 + cutT * (synthOn ? .5 : .25) + onBeat * groove * .25) * react).toFixed(2));
      setA(sc.lamp2Glow, "opacity", ((.2 + motion.energy * .5 + res * .3) * react).toFixed(2));
      setA(sc.globeGlow, "opacity", ((.2 + Math.abs(lfo) * .7 + bloom * .4) * react).toFixed(2));
      setA(sc.winGlow, "opacity", ((bloom * .35 + groove * .08) * react).toFixed(2));
      setA(sc.arcadeGlow, "opacity", (Math.random() < (crush + crumbleAct) * .4 ? .6 : .12 + onBeat * groove * .3).toFixed(2));
      setA(sc.bedGlow, "opacity", ((groove * .8 * onBeat + envL * .25) * react).toFixed(2));
      setA(sc.neonStrip, "opacity", ((.1 + res * .5 * (.5 + .5 * Math.sin(now / 70)) + groove * .15) * react).toFixed(2));
      $("#fx-pixels").style.opacity = clamp(degrade * .45 * amp, 0, 1).toFixed(2);
      if ((crumbleAct > .05 || degrade > .3) && animMode === 2 && Math.random() < Math.max(crumbleAct, degrade * .5) * .35) {
        const s = document.createElement("span"); s.style.left = (30 + Math.random() * 40) + "%"; s.style.top = (Math.random() * 85) + "%"; s.style.width = (10 + Math.random() * 50) + "px"; s.style.height = (4 + Math.random() * 14) + "px";
        $("#fx-crumble").appendChild(s); setTimeout(function () { s.remove(); }, 90 + Math.random() * 120);
      }
    } else if (stageOn && puppet && animMode === 0) {
      // animation off: still place everything correctly, lights steady
      const L0 = charPlace(), s = L0.s, L = puppet.L;
      rig.pos.setAttribute("transform", "translate(" + ((L0.x || 240) - L.hip[0] * s).toFixed(2) + " " + (L0.y - L.feet * s).toFixed(2) + ") scale(" + s.toFixed(4) + ")");
      if (sc.world) { stageCam(now, 0, 0, react, 0); stepWorld(now, 0, { glow: glow, react: Math.max(.6, react), beat: 0, onBeat: 0, groove: 0, energy: .3, alive: 0, env: 0, bloom: 0, lfo: 0, res: 0, cut: .5 }); }
      puppet.update({ alive: 0, amp: 0, glow: glow, glowAmt: .2 }, dt);
      if (hero && heroLayer && !heroLayer.hidden) drawHero({ alive: 0, amp: 0, glow: glow, glowAmt: .2 }, dt, L0.x || 240, L0.y, s, 0, react);
    }
    if (sc0 === "locker") lockerLines();
    if (labAlien && sc0 === "lab") labAlien.update({ alive: Math.max(alive, .3), intensity: .5, beats: beatClock, style: "groove", amp: amp || .3, glow: glow, glowAmt: .4 }, dt);
    requestAnimationFrame(frame);
  }
  let bloomV = 0;

  /* ================= boot ================= */
  // Gab's own logo artwork (RASTRO + PLAY + by Gab Calisman), transparent PNG
  const LOGO_IMG = '<img src="logo.png" alt="RASTRO Play by Gab Calisman" draggable="false">';
  function boot(init) {
    (init.params || []).forEach(function (p) { P[p.id] = p; });
    readLook(init.look); charKind = SPECIES[Math.round(v("avatarCharacter"))] || "alien";
    ICO.hydrate(); $("#logo").innerHTML = LOGO_IMG; $("#palcoLogo").innerHTML = LOGO_IMG;
    knobDefs(); setupOptions(); setupLocker();
    buildStage(); setupTopology();
    setupSample(); setupEnv(); setupSynth(); setupPad();
    readPalette(); setEtab("sample"); setScreen("limpo"); fit(); buildKeys(); game.hud(); onSample(init.sample); updatePill(); applyLook(); history.commit();
    if (game.state.lang === "en") setLang("en"); else setLang("pt");
    $("#mHeroState").textContent = look.hero3d ? "3D (beta)" : "pintado 2.5D";
    refreshUserPatches();
    $("#undoJam").disabled = !init.canUndoJam;
    new MutationObserver(function () { $("#jamPatch").textContent = $("#patchName").textContent; }).observe($("#patchName"), { childList: true, characterData: true, subtree: true });
    if (R.simulated) toast("Prévia no navegador com motor simulado. No plugin, tudo usa o áudio real.", 4200);
    requestAnimationFrame(frame);
  }
  R.on("tick", function (t) {
    const lr = T.lookRev; T = Object.assign(T, t);
    if (lr !== undefined && t.lookRev !== undefined && t.lookRev !== lr) R.call("lookGet").then(function (j) { if (j) { readLook(j); applyLook(); } });
    if (t.order) { const changed = t.order.join() !== order.join(); order = t.order.slice(); if (changed) renderRack(); }
    $("#undoJam").disabled = !t.canUndoJam;
    labTick(t.search); capTick(t.capture);
  });
  R.on("params", function (changes) {
    let rack = false;
    for (const id in changes) { apply(id, changes[id]); if (/Present$|Timing$/.test(id)) rack = true; }
    if (rack) { renderRack(); if (document.body.dataset.etab === "fx") renderFxDetail(); }
  });
  R.on("sample", onSample);
  R.call("ready").then(function (init) {
    Promise.all([R.call("lookGet"), R.call("progressGet")]).then(function (r) { init.look = r[0]; game.load(r[1]); boot(init); });
  });
})();
