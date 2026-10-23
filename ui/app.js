/* RASTRO Play — interface logic. Every control maps to a real plugin parameter or action. */
(function () {
  "use strict";
  const R = window.Rastro, ART = window.RastroArt;
  const $ = function (s, el) { return (el || document).querySelector(s); };
  const $$ = function (s, el) { return Array.prototype.slice.call((el || document).querySelectorAll(s)); };
  const clamp = function (v, a, b) { return Math.min(b, Math.max(a, v)); };
  const NOTE = ["C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"];
  const noteName = function (n) { n = Math.round(n); return NOTE[((n % 12) + 12) % 12] + (Math.floor(n / 12) - 1); };
  const W0 = 1440, H0 = 900;

  /* ================= state ================= */
  const P = {}, views = {}, busy = {};
  let T = { meter: 0, act: [0, 0, 0, 0, 0, 0], env: 0, pos: 0, playing: false, preview: false, vel: 0, serial: 0, bpm: 0, beat: -1, notes: [0, 0, 0, 0], search: {}, capture: {} };
  let order = [0, 1, 2, 3, 4, 5];
  let sample = { loaded: false, name: "", seconds: 0 };
  let peaks = null, S = 1, quietXP = false;
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
  // Every real edit to the sound earns a little XP (rate-limited so it rewards exploring, not spamming).
  const tweakSeen = {}; let lastTweak = 0;
  function tweaked(id) {
    if (quietXP || !P[id] || /^(visual|avatar|scene|dance|animation|knobDesign|colourSet)/.test(id)) return;
    const now = performance.now();
    if (now - lastTweak < 1500 || now - (tweakSeen[id] || -1e9) < 6000) return;
    lastTweak = now; tweakSeen[id] = now; game.award(1, "tweak");
  }

  /* ================= scaling, toast, tabs ================= */
  function fit() {
    const w = window.innerWidth, h = window.innerHeight;
    S = Math.min(w / W0, h / H0);
    $("#app").style.transform = "translate(" + Math.max(0, (w - W0 * S) / 2) + "px," + Math.max(0, (h - H0 * S) / 2) + "px) scale(" + S + ")";
    sizeCanvas();
  }
  window.addEventListener("resize", fit);
  let toastTimer = 0;
  function toast(msg, ms) { const t = $("#toast"); t.textContent = msg; t.classList.add("show"); clearTimeout(toastTimer); toastTimer = setTimeout(function () { t.classList.remove("show"); }, ms || 2800); }
  function setTab(tab) {
    document.body.dataset.tab = tab;
    $$(".tabs button").forEach(function (b) { b.classList.toggle("on", b.dataset.tab === tab); });
    renderRack(); if (tab === "character") renderCustom();
    setTimeout(function () { sizeCanvas(); buildKeys(); }, 0);
  }
  $$(".tabs button").forEach(function (b) { b.addEventListener("click", function () { setTab(b.dataset.tab); }); });
  $$("[data-goto]").forEach(function (b) { b.addEventListener("click", function () { setTab(b.dataset.goto); }); });

  /* ================= knobs ================= */
  const fmt = {
    pct: function (x) { return Math.round(x * 100) + "%"; },
    ms: function (x) { return x < 1 ? Math.round(x * 1000) + " ms" : x.toFixed(2) + " s"; },
    hz: function (x) { return x >= 1000 ? (x / 1000).toFixed(1) + " kHz" : Math.round(x) + " Hz"; },
    lfo: function (x) { return x.toFixed(2) + " Hz"; },
    note: function (x) { return noteName(x); },
    bip: function (x) { return (x > 0 ? "+" : "") + Math.round(x * 100) + "%"; },
    xms: function (x) { return Math.round(x) + " ms"; },
    st: function (x) { return Math.round(x) + " st"; }
  };
  function knobSvg(size) {
    const c = size / 2, r = size / 2 - 4;
    return '<svg width="' + size + '" height="' + size + '" viewBox="0 0 ' + size + ' ' + size + '">' +
      '<circle cx="' + c + '" cy="' + c + '" r="' + r + '" fill="none" stroke="var(--knob-track)" stroke-width="5" stroke-dasharray="' + (r * Math.PI * 1.5) + ' 999" transform="rotate(135 ' + c + ' ' + c + ')" stroke-linecap="round"/>' +
      '<circle class="arc k-arc" cx="' + c + '" cy="' + c + '" r="' + r + '" fill="none" stroke-width="5" stroke-linecap="round" transform="rotate(135 ' + c + ' ' + c + ')" stroke-dasharray="0 999"/>' +
      '<circle class="k-face" cx="' + c + '" cy="' + c + '" r="' + (r - 6) + '"/><circle class="k-cap" cx="' + c + '" cy="' + c + '" r="' + Math.max(2, r - 12) + '"/>' +
      '<line class="ptr k-ptr" x1="' + c + '" y1="' + (c - 2) + '" x2="' + c + '" y2="' + (c - r + 9) + '"/></svg>';
  }
  function knob(parent, id, label, opts) {
    opts = opts || {};
    const size = opts.size || 42, r = size / 2 - 4, el = document.createElement("div");
    el.className = "knob"; el.title = opts.title || label;
    if (opts.color) el.style.setProperty("--kc", opts.color);
    el.innerHTML = knobSvg(size) + '<div class="k-label">' + label + '</div><div class="k-value"></div>';
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
    el.addEventListener("wheel", function (e) { if (!p()) return; e.preventDefault(); setParam(id, fromT(toT(v(id)) - e.deltaY * 0.0008)); }, { passive: false });
    return el;
  }
  function knobDefs() {
    const d = document.createElementNS("http://www.w3.org/2000/svg", "svg");
    d.setAttribute("width", 0); d.setAttribute("height", 0); d.style.position = "absolute";
    d.innerHTML = '<defs>' +
      '<radialGradient id="kdCream" cx=".35" cy=".3" r=".8"><stop offset="0" stop-color="#fffaf0"/><stop offset=".6" stop-color="#f1dfbd"/><stop offset="1" stop-color="#c9a77a"/></radialGradient>' +
      '<linearGradient id="kdChrome" x1="0" y1="0" x2="1" y2="1"><stop offset="0" stop-color="#ffffff"/><stop offset=".35" stop-color="#b9bdc7"/><stop offset=".55" stop-color="#eef0f4"/><stop offset="1" stop-color="#6e7380"/></linearGradient>' +
      '<radialGradient id="kdChromeCap" cx=".4" cy=".35" r=".7"><stop offset="0" stop-color="#ffffff" stop-opacity=".9"/><stop offset="1" stop-color="#9aa0ad" stop-opacity=".2"/></radialGradient>' +
      '<radialGradient id="kdRubber" cx=".4" cy=".3" r=".8"><stop offset="0" stop-color="#55505e"/><stop offset="1" stop-color="#1c1a22"/></radialGradient>' +
      '<linearGradient id="kdWood" x1="0" y1="0" x2="1" y2="0"><stop offset="0" stop-color="#8a4b25"/><stop offset=".3" stop-color="#b8693a"/><stop offset=".55" stop-color="#7c3f1c"/><stop offset=".8" stop-color="#a95c30"/><stop offset="1" stop-color="#6b3418"/></linearGradient></defs>';
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
  let view = [0, 1], cols = null, waveDirty = true, dragMarker = null, cue = -1, palette = {};
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
  function readPalette() { const cs = getComputedStyle(document.body); ["--wave", "--wave2", "--sel", "--loop", "--wave-bg", "--accent", "--teal", "--violet"].forEach(function (k) { palette[k] = cs.getPropertyValue(k).trim(); }); waveDirty = true; }
  function drawWave() {
    const Wd = canvas.width, Hd = canvas.height, d = (window.devicePixelRatio || 1) * S;
    if (!Wd) return;
    ctx.clearRect(0, 0, Wd, Hd); ctx.fillStyle = palette["--wave-bg"]; ctx.fillRect(0, 0, Wd, Hd);
    if (!sample.loaded) return;
    if (!cols) buildCols();
    const st = v("start"), en = v("end"), m = mode();
    ctx.fillStyle = palette["--sel"]; ctx.fillRect(px(st), 0, px(en) - px(st), Hd);
    if (v("regionLoop") > .5) { const a = Math.max(st, v("loopStart")), b = Math.min(en, v("loopEnd")); ctx.fillStyle = palette["--loop"]; ctx.fillRect(px(a), Hd * .08, px(b) - px(a), Hd * .84); }
    if (cols) {
      const lanes = cols.length, laneH = Hd / lanes;
      cols.forEach(function (c, li) {
        const mid = laneH * li + laneH / 2, amp = laneH * .44;
        for (let x = 0; x < Wd; x++) {
          const n = view[0] + (view[1] - view[0]) * x / Wd, inside = n >= st && n <= en;
          ctx.fillStyle = inside ? (li ? palette["--wave2"] : palette["--wave"]) : "rgba(255,255,255,.18)";
          const y0 = mid - c[x * 2 + 1] * amp, y1 = mid - c[x * 2] * amp;
          ctx.fillRect(x, y0, 1, Math.max(1, y1 - y0));
        }
        if (lanes > 1) { ctx.fillStyle = "rgba(255,255,255,.55)"; ctx.font = (9 * d) + "px Menlo, monospace"; ctx.fillText(li ? "R" : "L", 5 * d, laneH * li + 12 * d); }
      });
    }
    const handle = function (x, color, top, label) {
      ctx.fillStyle = color; ctx.fillRect(x - 1.5 * d, 0, 3 * d, Hd);
      ctx.beginPath(); const y = top ? 0 : Hd, s = top ? 1 : -1;
      ctx.moveTo(x - 7 * d, y); ctx.lineTo(x + 7 * d, y); ctx.lineTo(x, y + s * 10 * d); ctx.closePath(); ctx.fill();
      if (label) { ctx.font = "bold " + (9 * d) + "px Menlo, monospace"; ctx.fillText(label, x + 4 * d, top ? 20 * d : Hd - 6 * d); }
    };
    if (m === 4) for (let i = 1; i < Math.round(v("sliceCount")); i++) handle(px(cutAbs(i)), palette["--violet"], false, noteName(v("root") + i));
    if (v("regionLoop") > .5) { handle(px(v("loopStart")), palette["--teal"], false, "L"); handle(px(v("loopEnd")), palette["--teal"], false, "L"); }
    handle(px(st), palette["--accent"], true, "IN"); handle(px(en), palette["--accent"], true, "OUT");
    if (cue >= 0) { ctx.fillStyle = "rgba(255,255,255,.75)"; for (let y = 0; y < Hd; y += 8 * d) ctx.fillRect(px(cue) - d, y, 2 * d, 4 * d); }
    if (T.playing || T.preview) { ctx.fillStyle = "#fff"; ctx.fillRect(px(T.pos) - d, 0, 2.5 * d, Hd); }
  }
  function ms(x) { return Math.round(x * (sample.seconds || 0) * 1000) + " ms"; }
  function markerInfo() {
    let t = "IN " + ms(v("start")) + " · OUT " + ms(v("end"));
    if (v("regionLoop") > .5) t += " · LOOP " + ms(v("loopStart")) + "–" + ms(v("loopEnd"));
    $("#markerInfo").textContent = t;
    $("#cueInfo").textContent = cue >= 0 ? "tocar a partir de " + ms(cue) : "clique na onda para escolher de onde tocar";
  }
  function markers() {
    const list = [{ id: "start", x: v("start") }, { id: "end", x: v("end") }];
    if (v("regionLoop") > .5) list.push({ id: "loopStart", x: v("loopStart") }, { id: "loopEnd", x: v("loopEnd") });
    if (mode() === 4) for (let i = 1; i < Math.round(v("sliceCount")); i++) list.push({ id: "slice" + i, x: cutAbs(i), slice: i });
    return list;
  }
  function nearMarker(e) {
    const r = canvas.getBoundingClientRect(); let best = null, bd = 12 * S;
    markers().forEach(function (mk) { const dx = Math.abs((mk.x - view[0]) / (view[1] - view[0]) * r.width - (e.clientX - r.left)); if (dx < bd) { bd = dx; best = mk; } });
    return best;
  }
  waveBox.addEventListener("pointerdown", function (e) {
    if (!sample.loaded) { doImport(); return; }
    const best = nearMarker(e);
    if (!best) { cue = clamp(normAt(e.clientX), v("start"), v("end") - .001); markerInfo(); waveDirty = true; if (T.preview) R.call("preview", true, cue); return; }
    dragMarker = best; busy[best.id] = true; waveBox.setPointerCapture(e.pointerId);
    R.call("setParam", best.id, null, 1);
    if (best.id.indexOf("loop") === 0 && v("loopRegionSet") < .5) gesture("loopRegionSet", 1);
  });
  waveBox.addEventListener("pointermove", function (e) {
    if (!dragMarker) { waveBox.style.cursor = !sample.loaded ? "pointer" : nearMarker(e) ? "ew-resize" : "text"; return; }
    const id = dragMarker.id, n = clamp(normAt(e.clientX), 0, 1);
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
    if (view[0] < 0) { view[1] -= view[0]; view[0] = 0; } if (view[1] > 1) { view[0] -= view[1] - 1; view[1] = 1; }
    view[0] = Math.max(0, view[0]); cols = null; waveDirty = true;
  }
  $$("[data-zoom]").forEach(function (b) { b.addEventListener("click", function () { zoom(b.dataset.zoom); }); });
  waveBox.addEventListener("wheel", function (e) {
    if (!sample.loaded) return; e.preventDefault();
    if (Math.abs(e.deltaX) > Math.abs(e.deltaY)) { const span = view[1] - view[0]; view = [clamp(view[0] + e.deltaX / 600 * span, 0, 1 - span), 0]; view[1] = view[0] + span; cols = null; waveDirty = true; }
    else zoom(e.deltaY < 0 ? "in" : "out", normAt(e.clientX));
  }, { passive: false });
  waveBox.addEventListener("keydown", function (e) {
    if (e.key === "+" || e.key === "=") zoom("in"); else if (e.key === "-") zoom("out"); else if (e.key === "Home") zoom("all"); else if (e.key === "f" || e.key === "F") zoom("sel"); else return;
    e.preventDefault(); e.stopPropagation();
  });
  /* transport: play from the cue point, pause keeps the position, stop returns to the trim start */
  $("#tPlay").addEventListener("click", function () { if (!sample.loaded) { toast("Importe ou capture um sample primeiro."); return; } R.call("preview", true, cue >= 0 ? cue : v("start")); });
  $("#tPause").addEventListener("click", function () { if (T.preview) { cue = T.pos; R.call("preview", false); markerInfo(); waveDirty = true; } });
  $("#tStop").addEventListener("click", function () { R.call("preview", false); cue = -1; markerInfo(); waveDirty = true; });

  /* ---- import ---- */
  function importResult(r) {
    if (!r) return;
    if (r.ok) { toast("Sample carregado. Toque no teclado!"); game.award(20, "import"); }
    else toast(r.message || "Não foi possível importar.", 4500);
  }
  function doImport() { R.call("importDialog").then(importResult); }
  $("#importBtn").addEventListener("click", doImport);
  ["dragenter", "dragover"].forEach(function (ev) { document.addEventListener(ev, function (e) { e.preventDefault(); waveBox.classList.add("dragover"); }); });
  ["dragleave", "drop"].forEach(function (ev) { document.addEventListener(ev, function (e) { e.preventDefault(); if (ev === "dragleave" && e.relatedTarget) return; waveBox.classList.remove("dragover"); }); });
  document.addEventListener("drop", function (e) {
    const f = e.dataTransfer && e.dataTransfer.files && e.dataTransfer.files[0];
    if (!f) return;
    if (f.size > 32 * 1024 * 1024) { toast("Arquivo acima de 32 MB."); return; }
    toast("Importando " + f.name + "…", 8000);
    const reader = new FileReader();
    reader.onload = function () { R.call("importData", f.name, String(reader.result).split(",")[1] || "").then(importResult); };
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
    knob(k, "root", "RAIZ", { format: fmt.note, title: "Nota em que o sample toca na altura original" });
    knob(k, "velocity", "VEL→FILTRO", { title: "Quanto a força da nota abre o filtro" });
    const xf = knob(k, "loopCrossfade", "XFADE", { format: fmt.xms, color: "var(--teal)", title: "Crossfade na emenda do LOOP" });
    watch("regionLoop", function (x) { xf.classList.toggle("disabled", x < .5); });
    const refreshMode = seg("#modeSeg", "mode", mode, function (m) { R.call("playbackMode", m); P.playMode.v = m; P.loop.v = m === 1 ? 1 : 0; refreshMode(); slicesUi(); waveDirty = true; updatePill(); tweaked("playMode"); });
    watch("playMode", function () { refreshMode(); slicesUi(); waveDirty = true; }); watch("loop", refreshMode);
    bindToggle($("#loopBtn"), "regionLoop", function () { waveDirty = true; markerInfo(); });
    watch("regionLoop", function () { waveDirty = true; markerInfo(); });
    bindSeg("#pitchSeg", "pitch", "pitchMode");
    function slicesUi() { $("#sliceCount").textContent = Math.round(v("sliceCount")) + " cortes"; $("#sliceCtl").classList.toggle("off", mode() !== 4); keyboardSlices(); }
    watch("sliceCount", slicesUi);
    $$("[data-slices]").forEach(function (b) { b.addEventListener("click", function () { R.call("slices", Math.round(v("sliceCount")) + Number(b.dataset.slices)); tweaked("sliceCount"); }); });
    ["start", "end", "loopStart", "loopEnd"].forEach(function (id) { watch(id, function () { waveDirty = true; markerInfo(); }); });
    for (let i = 1; i < 16; i++) watch("slice" + i, function () { waveDirty = true; });
  }
  const envSvg = $("#envSvg"), EW = 300, EH = 70, seg1 = 92, seg2 = 82, hold = 40, seg4 = 80;
  function tPos(x, max, w) { const lo = .001; return 6 + (w - 6) * Math.log(Math.max(x, lo) / lo) / Math.log(max / lo); }
  function tFrom(pxv, max, w) { const lo = .001; return lo * Math.pow(max / lo, clamp((pxv - 6) / (w - 6), 0, 1)); }
  function envLive() { return v("synthEnable") > .5 || Math.round(v("triggerMode")) === 2; }
  function drawEnv() {
    const a = tPos(v("ampAttack"), 4, seg1), d = tPos(v("ampDecay"), 4, seg2), s = v("ampSustain"), r = tPos(v("ampRelease"), 8, seg4);
    const y0 = EH - 6, top = 8, sy = y0 - (y0 - top) * s, live = envLive();
    const p1 = [4 + a, top], p2 = [p1[0] + d, sy], p3 = [p2[0] + hold, sy], p4 = [p3[0] + r, y0];
    envSvg.innerHTML = '<path d="M4 ' + y0 + ' L' + p1 + ' L' + p2 + ' L' + p3 + ' L' + p4 + ' Z" fill="' + (live ? "rgba(240,122,58,.3)" : "rgba(255,255,255,.08)") + '"/>' +
      '<path d="M4 ' + y0 + ' L' + p1 + ' L' + p2 + ' L' + p3 + ' L' + p4 + '" fill="none" stroke="' + (live ? "var(--wave)" : "rgba(255,255,255,.4)") + '" stroke-width="2.5" stroke-linejoin="round" vector-effect="non-scaling-stroke"/>' +
      [["A", p1], ["D", p2], ["R", p4]].map(function (q) { return '<circle data-h="' + q[0] + '" cx="' + q[1][0] + '" cy="' + q[1][1] + '" r="5" fill="#fff" stroke="#17101f" stroke-width="2" vector-effect="non-scaling-stroke"/>'; }).join("");
    $("#envNote").textContent = live ? "Arraste os pontos ou gire os knobs." : "ADSR inativo: ligue SYNTH ou use disparo Gate.";
    $$("#envKnobs .knob").forEach(function (kk) { kk.classList.toggle("disabled", !live); });
  }
  function setupEnv() {
    const k = $("#envKnobs");
    knob(k, "ampAttack", "ATTACK", { format: fmt.ms, curve: "log", size: 40 });
    knob(k, "ampDecay", "DECAY", { format: fmt.ms, curve: "log", size: 40 });
    knob(k, "ampSustain", "SUSTAIN", { size: 40 });
    knob(k, "ampRelease", "RELEASE", { format: fmt.ms, curve: "log", size: 40 });
    ["ampAttack", "ampDecay", "ampSustain", "ampRelease", "synthEnable", "triggerMode"].forEach(function (id) { watch(id, drawEnv); });
    bindToggle($("#synthBtn"), "synthEnable");
    let h = null; const ids = function () { return h === "A" ? ["ampAttack"] : h === "D" ? ["ampDecay", "ampSustain"] : ["ampRelease"]; };
    envSvg.addEventListener("pointerdown", function (e) { const t = e.target.getAttribute && e.target.getAttribute("data-h"); if (!t) return; h = t; envSvg.setPointerCapture(e.pointerId); ids().forEach(function (id) { R.call("setParam", id, null, 1); }); });
    envSvg.addEventListener("pointermove", function (e) {
      if (!h) return;
      const r = envSvg.getBoundingClientRect(), x = (e.clientX - r.left) / r.width * EW, y = (e.clientY - r.top) / r.height * EH, a = tPos(v("ampAttack"), 4, seg1);
      if (h === "A") setParam("ampAttack", tFrom(x - 4, 4, seg1));
      if (h === "D") { setParam("ampDecay", tFrom(x - 4 - a, 4, seg2)); setParam("ampSustain", clamp((EH - 6 - y) / (EH - 14), 0, 1)); }
      if (h === "R") setParam("ampRelease", tFrom(x - 4 - a - tPos(v("ampDecay"), 4, seg2) - hold, 8, seg4));
      drawEnv();
    });
    envSvg.addEventListener("pointerup", function () { if (!h) return; ids().forEach(function (id) { R.call("setParam", id, null, 2); tweaked(id); }); h = null; });
  }
  function drawFilter() {
    const c = Math.log(Math.max(20, v("filterCutoff")) / 20) / Math.log(1000), q = v("filterResonance"), x = 8 + c * 224, live = v("synthEnable") > .5;
    $("#filterSvg").innerHTML = '<path d="M0 14 H' + (x - 24) + ' C' + (x - 8) + ' 14 ' + (x - 6) + ' ' + (14 - q * 12) + ' ' + x + ' ' + (14 - q * 10) + ' S' + (x + 22) + ' 44 ' + (x + 40) + ' 46 L240 46" fill="none" stroke="' + (live ? "var(--teal)" : "rgba(255,255,255,.35)") + '" stroke-width="3" vector-effect="non-scaling-stroke"/>';
  }
  function drawLfo() {
    const live = v("synthEnable") > .5 && v("lfoDepth") > 0, cyc = clamp(v("lfoRate") / 2, .5, 6), dep = .2 + v("lfoDepth") * .8;
    let d = "M0 23"; for (let i = 0; i <= 120; i++) { const t = i / 120; d += " L" + (t * 240).toFixed(1) + " " + (23 - Math.sin(t * cyc * Math.PI * 2) * 18 * dep).toFixed(1); }
    $("#lfoSvg").innerHTML = '<path d="' + d + '" fill="none" stroke="' + (live ? "var(--violet)" : "rgba(255,255,255,.35)") + '" stroke-width="3" vector-effect="non-scaling-stroke"/>';
  }
  function setupSynth() {
    const f = $("#filterKnobs");
    knob(f, "filterCutoff", "CUTOFF", { format: fmt.hz, curve: "log", color: "var(--teal)", size: 40 });
    knob(f, "filterResonance", "RESSON.", { color: "var(--teal)", size: 40 });
    knob(f, "envAmount", "ENV →", { format: fmt.bip, color: "var(--teal)", size: 40, title: "Quanto o envelope de filtro abre ou fecha" });
    ["filterCutoff", "filterResonance", "synthEnable"].forEach(function (id) { watch(id, drawFilter); });
    const l = $("#lfoKnobs");
    knob(l, "lfoRate", "RATE", { format: fmt.lfo, curve: "log", color: "var(--violet)", size: 40 });
    knob(l, "lfoDepth", "DEPTH", { color: "var(--violet)", size: 40 });
    ["lfoRate", "lfoDepth", "synthEnable"].forEach(function (id) { watch(id, drawLfo); });
    bindSeg("#lfoTargetSeg", "lfo", "lfoTarget");
    watch("synthEnable", function (x) { $$("#filterPanel .knob, #lfoPanel .knob").forEach(function (kk) { kk.classList.toggle("disabled", x < .5); }); });
    bindSeg("#voiceSeg", "voice", "voiceMode"); bindSeg("#trigSeg", "trig", "triggerMode");
    const vk = $("#voiceKnobs");
    const g = knob(vk, "glide", "GLIDE", { size: 36, format: function (x) { return Math.round(x * 1000) + " ms"; }, title: "Portamento em Mono/Legato" });
    knob(vk, "bendRange", "BEND", { size: 36, format: fmt.st, title: "Alcance do pitch bend" });
    watch("voiceMode", function (x) { g.classList.toggle("disabled", Math.round(x) === 0); });
    ["voiceMode", "playMode", "loop", "regionLoop"].forEach(function (id) { watch(id, updatePill); });
    const m = $("#masterKnobs");
    knob(m, "level", "SAÍDA", { size: 44 });
    knob(m, "drywet", "ORIG/RACK", { size: 44, title: "Mistura entre o som original e o rack" });
  }

  /* ================= PLAY pad & macros ================= */
  function setupPad() {
    const pad = $("#pad"), dot = $("#padDot"), trail = $("#padTrail");
    let dragging = false;
    function place() { dot.style.left = (v("playX") * 100) + "%"; dot.style.top = ((1 - v("playY")) * 100) + "%"; $("#padInfo").textContent = "X " + Math.round(v("playX") * 100) + "% · Y " + Math.round(v("playY") * 100) + "%"; }
    function note() {
      const wob = v("wOn") > .5 && v("wPresent") > .5, deg = ["c", "d", "k"].some(function (m) { return v(m + "On") > .5 && v(m + "Present") > .5; }), msg = [];
      if (!wob) msg.push("X sem efeito: WOBBLE desligado."); if (!deg) msg.push("Y sem efeito: CRUMBLE, DUST e CRUSH desligados.");
      $("#padNote").textContent = msg.length ? msg.join(" ") : "X leva de estável a WOBBLE. Y leva de limpo a degradado (CRUMBLE, DUST, CRUSH). O palco acompanha. Duplo clique centraliza.";
    }
    watch("playX", place); watch("playY", place);
    ["wOn", "wPresent", "cOn", "cPresent", "dOn", "dPresent", "kOn", "kPresent"].forEach(function (id) { watch(id, note); });
    function at(e) { const r = pad.getBoundingClientRect(); setParam("playX", clamp((e.clientX - r.left) / r.width, 0, 1)); setParam("playY", clamp(1 - (e.clientY - r.top) / r.height, 0, 1)); place(); }
    pad.addEventListener("pointerdown", function (e) { dragging = true; busy.playX = busy.playY = true; pad.setPointerCapture(e.pointerId); R.call("setParam", "playX", null, 1); R.call("setParam", "playY", null, 1); at(e); });
    pad.addEventListener("pointermove", function (e) {
      if (!dragging) return; at(e);
      const s = document.createElement("span"); s.style.left = dot.style.left; s.style.top = dot.style.top; trail.appendChild(s);
      if (s.animate) s.animate([{ opacity: .9, transform: "scale(1)" }, { opacity: 0, transform: "scale(.2)" }], { duration: 600 }).onfinish = function () { s.remove(); }; else setTimeout(function () { s.remove(); }, 600);
    });
    function end() { if (!dragging) return; dragging = false; busy.playX = busy.playY = false; R.call("setParam", "playX", null, 2); R.call("setParam", "playY", null, 2); tweaked("playX"); }
    pad.addEventListener("pointerup", end); pad.addEventListener("pointercancel", end);
    pad.addEventListener("dblclick", function () { gesture("playX", .5); gesture("playY", .5); });
    const m = $("#macroKnobs");
    knob(m, "presence", "PRESENÇA", { size: 46 });
    knob(m, "trail", "RASTRO", { size: 46, color: "var(--violet)" });
    knob(m, "mutation", "JAM %", { size: 46, title: "Quanto o JAM varia" });
  }

  /* ================= rack ================= */
  const MOD = [
    { k: "g", name: "Ghost", desc: "Eco filtrado", color: "#9ad8ff", ctl: ["T", "gFeedback", "gTone"], labels: ["TEMPO", "FEEDBACK", "TOM"], slot: 0 },
    { k: "c", name: "Crumble", desc: "Fragmentos", color: "#f2b45a", ctl: ["T", "cVariation"], labels: ["TAXA", "VARIAÇÃO"], slot: 1 },
    { k: "b", name: "Bloom", desc: "Cauda difusa", color: "#ff9ad5", ctl: ["bSize", "bTone", "bDecay"], labels: ["TAMANHO", "TOM", "DECAY"] },
    { k: "w", name: "Wobble", desc: "Afinação viva", color: "#5fe3d6", ctl: ["wDepth", "T"], labels: ["PROFUND.", "TAXA"], slot: 2 },
    { k: "d", name: "Dust", desc: "Textura viva", color: "#cbbcff", ctl: ["dLevel", "dCharacter"], labels: ["NÍVEL", "CARÁTER"] },
    { k: "k", name: "Crush", desc: "Bits + taxa", color: "#ffd23f", ctl: ["kAmount"], labels: ["INTENS."] }
  ];
  const timeIds = [["gTime", "gFreeMs", "gDivision"], ["cRate", "cFreeHz", "cDivision"], ["wRate", "wFreeHz", "wDivision"]], timingIds = ["gTiming", "cTiming", "wTiming"], DIV = ["1/4", "1/8", "1/8.", "1/8T"];
  function timeParam(mi) { const slot = MOD[mi].slot; return timeIds[slot][Math.round(v(timingIds[slot]))]; }
  function formatFor(id) { if (/Division$/.test(id)) return function (x) { return DIV[Math.round(x)] || ""; }; if (id === "gFreeMs") return fmt.xms; if (/FreeHz$/.test(id)) return fmt.lfo; return fmt.pct; }
  let cards = [];
  function dropOwned(tag) { for (const k in views) views[k] = views[k].filter(function (f) { return f.owned !== tag; }); }
  function renderRack() {
    dropOwned("rack"); cards = [];
    const mini = $("#rackMini"), full = $("#rackFull"); mini.innerHTML = ""; full.innerHTML = "";
    const tab = document.body.dataset.tab;
    if (tab !== "sound" && tab !== "rack") return;
    const big = tab === "rack";
    order.forEach(function (mi, pos) {
      const M = MOD[mi];
      if (v(M.k + "Present") < .5) return;
      const card = document.createElement("div");
      card.className = "card" + (big ? " big" : ""); card.style.setProperty("--mc", M.color); card.dataset.pos = pos;
      card.innerHTML = '<div class="card-main">' + ART.icon(mi) + '<div><div class="card-title">' + M.name + '</div><div class="card-desc">' + M.desc + '</div></div><div class="card-tools"><button class="power" title="Ativar ou bypass" aria-label="Ativar ' + M.name + '"></button>' + (big ? '<button class="x" title="Remover do rack" aria-label="Remover ' + M.name + '">✕</button>' : "") + '</div></div>' + (big ? '<div class="knob-row"></div><div class="big-foot"><div class="timing"></div><button class="lock" title="Bloqueado: o JAM não mexe neste módulo">🔒 JAM</button></div>' : "") + '<div class="card-meter"></div>';
      (big ? full : mini).appendChild(card);
      const tools = $(".card-tools", card);
      if (!big) {
        const holder = document.createElement("div"); holder.style.display = "flex"; tools.insertBefore(holder, tools.firstChild);
        const firstId = M.ctl[0] === "T" ? timeParam(mi) : M.ctl[0];
        knob(holder, firstId, M.labels[0], { size: 32, color: "var(--line)", format: formatFor(firstId), owned: "rack" });
        knob(holder, M.k + "Mix", "MIX", { size: 32, color: "var(--line)", owned: "rack" });
      } else {
        const row = $(".knob-row", card);
        M.ctl.forEach(function (c, i) { const id = c === "T" ? timeParam(mi) : c; knob(row, id, M.labels[i], { size: 50, color: "var(--line)", format: formatFor(id), owned: "rack" }); });
        knob(row, M.k + "Mix", "MIX", { size: 50, color: "var(--line)", owned: "rack" });
        if (M.slot !== undefined) {
          const tm = $(".timing", card), tid = timingIds[M.slot];
          [[1, "FREE"], [2, "SYNC"]].forEach(function (o) { const b = document.createElement("button"); b.textContent = o[1]; b.classList.toggle("on", Math.round(v(tid)) === o[0]); b.addEventListener("click", function () { gesture(tid, o[0]); renderRack(); }); tm.appendChild(b); });
        }
        const lock = $(".lock", card), lid = M.k + "JamLock";
        if (P[lid]) { const lf = function (x) { lock.classList.toggle("on", x > .5); }; lf.owned = "rack"; watch(lid, lf); lock.addEventListener("click", function () { gesture(lid, v(lid) > .5 ? 0 : 1); }); } else lock.hidden = true;
        $(".x", card).addEventListener("click", function (e) { e.stopPropagation(); gesture(M.k + "Present", 0); renderRack(); });
      }
      const power = $(".power", card), onId = M.k + "On";
      const pw = function (x) { power.classList.toggle("on", x > .5); card.classList.toggle("off", x < .5); }; pw.owned = "rack"; watch(onId, pw);
      power.addEventListener("click", function (e) { e.stopPropagation(); gesture(onId, v(onId) > .5 ? 0 : 1); });
      dragCard(card, $(".card-main", card), pos, big);
      cards.push({ el: card, mi: mi, art: $(".card-art", card), meter: $(".card-meter", card) });
    });
    $("#addModule").disabled = MOD.every(function (M) { return v(M.k + "Present") > .5; });
  }
  function dragCard(card, handle, pos, big) {
    let sx = 0, sy = 0, moved = false, dragging = false;
    handle.addEventListener("pointerdown", function (e) { if (e.target.closest(".knob, button")) return; sx = e.clientX; sy = e.clientY; moved = false; dragging = true; handle.setPointerCapture(e.pointerId); });
    handle.addEventListener("pointermove", function (e) {
      if (!dragging) return;
      const dx = (e.clientX - sx) / S, dy = (e.clientY - sy) / S;
      if (Math.abs(dy) + Math.abs(dx) > 5) { moved = true; card.classList.add("dragging"); }
      if (moved) card.style.transform = "translate(" + (big ? dx : 0) + "px," + dy + "px) scale(1.03) rotate(-1.2deg)";
    });
    handle.addEventListener("pointerup", function (e) {
      if (!dragging) return; dragging = false; card.style.transform = ""; card.classList.remove("dragging");
      if (!moved) return;
      const visible = cards.map(function (c) { return c.el; });
      let target = visible.length - 1, best = 1e9;
      visible.forEach(function (el, i) { const r = el.getBoundingClientRect(), d = Math.hypot(e.clientX - (r.left + r.width / 2), e.clientY - (r.top + r.height / 2)); if (d < best) { best = d; target = i; } });
      const toPos = Number(visible[target].dataset.pos);
      if (toPos !== pos) { R.call("moveModule", pos, toPos); const m = order.splice(pos, 1)[0]; order.splice(toPos, 0, m); renderRack(); game.award(3, "rack"); toast("Nova ordem: o som passa pelos cartões nessa sequência."); }
    });
  }
  $("#addModule").addEventListener("click", function (e) {
    const menu = $("#addMenu"); menu.innerHTML = "";
    MOD.forEach(function (M) { if (v(M.k + "Present") > .5) return; const b = document.createElement("button"); b.textContent = "+ " + M.name; b.addEventListener("click", function () { gesture(M.k + "Present", 1); menu.hidden = true; renderRack(); }); menu.appendChild(b); });
    menu.style.left = "760px"; menu.style.top = "130px"; menu.hidden = false; e.stopPropagation();
  });
  document.addEventListener("click", function (e) { if (!e.target.closest("#addMenu")) $("#addMenu").hidden = true; });
  $("#resetFx").addEventListener("click", function () { R.call("resetEffects"); toast("Efeitos restaurados."); });

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

  /* ================= patch name, A/B, files, Patch Lab ================= */
  let patchName = "Novo patch";
  const MODES = ["One Shot", "Loop", "Reverse", "Ping-Pong", "Slice"], VOICES = ["Poly", "Mono", "Legato"];
  const VISUAL = /^(visual|avatar|scene|dance|animation|controlStyle|knobDesign|uiCompact|colourSet|cosmicArcade)|Colour$|JamLock$/;
  const ab = { cur: "A", A: null, B: null };
  function updatePill() {
    $("#patchName").textContent = patchName + (ab.cur === "B" ? " · B" : "");
    $("#patchSub").textContent = (sample.loaded ? sample.name : "sem sample") + " · " + MODES[mode()] + " · " + VOICES[Math.round(v("voiceMode"))] + (v("regionLoop") > .5 ? " · LOOP" : "");
  }
  function snapshot() { const s = {}; for (const id in P) if (!VISUAL.test(id)) s[id] = P[id].v; return s; }
  function restore(s) { quietXP = true; for (const id in s) if (P[id] && Math.abs(P[id].v - s[id]) > 1e-6) gesture(id, s[id]); quietXP = false; }
  function switchAB(to) {
    if (to === ab.cur) return;
    ab[ab.cur] = snapshot();
    if (!ab[to]) ab[to] = snapshot(); else restore(ab[to]);
    ab.cur = to; $("#abA").classList.toggle("on", to === "A"); $("#abB").classList.toggle("on", to === "B"); updatePill();
    toast(to === "B" && !ab.seen ? "B começou como cópia de A. Mexa à vontade e alterne para comparar." : "Ouvindo " + to + ".");
    if (to === "B") ab.seen = true;
  }
  $("#abA").addEventListener("click", function () { switchAB("A"); });
  $("#abB").addEventListener("click", function () { switchAB("B"); });
  function fileAction(save, soundOnly) {
    R.call("patchFile", save, soundOnly).then(function (r) {
      if (!r) return;
      if (!r.ok) { toast("Falha no arquivo (limite 64 MB)."); $("#fileStatus").textContent = "Falha no arquivo."; return; }
      patchName = r.name || patchName; updatePill();
      const msg = save ? "Patch salvo: " + r.name : "Patch aberto: " + r.name + (soundOnly ? " (somente som)" : " (completo)");
      toast(msg); $("#fileStatus").textContent = msg; if (save) game.award(10, "save");
    });
  }
  $("#saveBtn").addEventListener("click", function () { fileAction(true); });
  $("#patchSave").addEventListener("click", function () { fileAction(true); });
  $("#patchOpenSound").addEventListener("click", function () { fileAction(false, true); });
  $("#patchOpenFull").addEventListener("click", function () { fileAction(false, false); });
  let labDir = 0, labRev = -1, labPicked = -1;
  const labDirRefresh = seg("#labDir", "dir", function () { return labDir; }, function (x) { labDir = x; labDirRefresh(); });
  labDirRefresh();
  $("#labGo").addEventListener("click", function () {
    if (!sample.loaded) { toast("Importe ou capture um sample primeiro."); return; }
    R.call("patchSearch", labDir === 1).then(function (ok) { if (!ok) toast("A busca já está rodando ou o trecho é curto demais."); });
    labPicked = -1; $("#labResults").innerHTML = "";
  });
  $("#labPause").addEventListener("click", function () { R.call("patchSearchPause"); });
  $("#labCancel").addEventListener("click", function () { R.call("patchSearchCancel"); });
  const STATUS = ["Pronto.", "Procurando…", "Pronto! Clique numa opção para aplicar e ouvir.", "Busca cancelada. Seu patch atual foi mantido.", "O trecho está silencioso. Escolha outro trecho.", "Falha na busca."];
  function labTick(s) {
    if (!s || s.status === undefined) return;
    $("#labBar").style.width = Math.round((s.busy || s.status === 2 ? s.progress : 0) * 100) + "%";
    $("#labPause").disabled = !s.busy; $("#labCancel").disabled = !s.busy; $("#labGo").disabled = !!s.busy;
    $("#labPause").textContent = s.paused ? "RETOMAR" : "PAUSAR";
    $("#labStatus").textContent = s.busy ? (s.paused ? "Pausado · " : "Procurando · ") + Math.round(s.progress * 100) + "%" : (sample.loaded ? STATUS[s.status] || "" : "Importe ou capture um sample primeiro.");
    if (s.revision !== labRev) { labRev = s.revision; R.call("patchResults").then(renderLab); }
  }
  function miniPatch(vals) {
    const a = 4 + Math.min(1, Math.log(Math.max(vals[1], .001) / .001) / Math.log(4000)) * 60, d = Math.min(1, Math.log(Math.max(vals[2], .001) / .001) / Math.log(4000)) * 60, s = vals[3], r = Math.min(1, Math.log(Math.max(vals[4], .001) / .001) / Math.log(8000)) * 60;
    const y0 = 64, top = 8, sy = y0 - (y0 - top) * s, p1 = [a, top], p2 = [a + d, sy], p3 = [a + d + 40, sy], p4 = [a + d + 40 + r, y0];
    const c = 10 + Math.log(Math.max(20, vals[5]) / 20) / Math.log(1000) * 220;
    return '<svg viewBox="0 0 240 70" preserveAspectRatio="none"><path d="M4 64 L' + p1 + ' L' + p2 + ' L' + p3 + ' L' + p4 + ' Z" fill="rgba(240,122,58,.35)" stroke="var(--wave)" stroke-width="2" vector-effect="non-scaling-stroke"/><line x1="' + c + '" y1="0" x2="' + c + '" y2="70" stroke="var(--teal)" stroke-width="2" stroke-dasharray="4 3" vector-effect="non-scaling-stroke"/></svg>';
  }
  function renderLab(res) {
    const box = $("#labResults"); box.innerHTML = "";
    if (!res || !res.results || !res.results.length) return;
    if (!res.valid) { $("#labStatus").textContent = "O sample mudou desde a busca. Gere de novo."; return; }
    res.results.forEach(function (r, i) {
      const b = document.createElement("button"); b.className = "lab-card" + (labPicked === i ? " on" : "");
      b.innerHTML = "<b>OPÇÃO " + (i + 1) + "</b>" + miniPatch(r.values) + "<small>Δ " + r.distance.toFixed(3) + (r.values[0] > .5 ? " · SYNTH" : "") + "</small>";
      b.addEventListener("click", function () {
        if (labPicked < 0) { ab.A = snapshot(); }
        labPicked = i; R.call("patchApply", r.values).then(function () { const root = Math.round(v("root")); R.call("note", root, .85, true); setTimeout(function () { R.call("note", root, 0, false); }, 450); });
        $$(".lab-card").forEach(function (c, j) { c.classList.toggle("on", j === i); });
        patchName = "Patch Lab · opção " + (i + 1); updatePill(); toast("Opção " + (i + 1) + " aplicada. O patch anterior está em A."); game.award(4, "lab");
      });
      box.appendChild(b);
    });
  }

  /* ================= capture ================= */
  let capOpen = false, capTimer = 0;
  const CAPSTATE = ["pronto", "gravando", "salvando", "pronto", "exportando", "iniciando"];
  $("#captureBtn").addEventListener("click", function () {
    $("#captureSheet").hidden = false; capOpen = true;
    R.call("captureInfo").then(function (info) { info = info || {}; $("#capDirect").hidden = !info.direct; $("#capSettings").hidden = !info.canOpenSettings; });
    pollCapture(); clearInterval(capTimer); capTimer = setInterval(pollCapture, 1000);
  });
  function closeCapture() { $("#captureSheet").hidden = true; capOpen = false; clearInterval(capTimer); }
  $("#capSettings").addEventListener("click", function () { R.call("captureSettings"); });
  $("#capRec").addEventListener("click", function () { R.call("captureRecord").then(function (ok) { if (ok) { toast("Gravação carregada como sample. Toque no teclado!"); game.award(15, "capture"); closeCapture(); } }); });
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
        const b = document.createElement("button"); b.className = "btn btn-small " + (rec ? "" : "btn-rec"); b.textContent = rec ? "■ PARAR" : "● GRAVAR";
        b.addEventListener("click", function () { R.call("captureRemote", l.id, !rec).then(function (ok) { if (!ok) toast("Não consegui falar com o insert."); }); });
        row.appendChild(b); links.appendChild(row);
      });
      const files = $("#capFiles"); files.innerHTML = "";
      if (!r.files.length) files.innerHTML = '<div class="fine">Nenhuma tomada ainda.</div>';
      r.files.slice(0, 8).forEach(function (f) {
        const row = document.createElement("div"); row.className = "cap-item";
        row.innerHTML = '<div class="grow">' + f.name + '<small>' + new Date(f.time).toLocaleString() + '</small></div>';
        const b = document.createElement("button"); b.className = "btn btn-small btn-hot"; b.textContent = "USAR";
        b.addEventListener("click", function () { R.call("captureLoad", f.path).then(function (res) { importResult(res); if (res && res.ok) closeCapture(); }); });
        row.appendChild(b); files.appendChild(row);
      });
    });
  }
  function capTick(c) {
    if (!capOpen || !c) return;
    $("#capMeter").style.width = Math.min(100, (c.peak || 0) * 100) + "%";
    $("#capTime").textContent = (c.seconds || 0).toFixed(1).replace(".", ",") + " s";
    const rec = c.state === 1; $("#capRec").classList.toggle("live", rec); $("#capRec").textContent = rec ? "● GRAVANDO" : "● GRAVAR";
    $("#capStatus").textContent = rec ? "Gravando… clique em PARAR para usar a tomada (máx. 30 s)." : c.outcome === 2 ? "Gravação vazia: nenhum áudio chegou." : c.outcome === 3 ? "Chegou áudio, mas em silêncio: confira entrada e ganho." : "";
  }
  $$(".sheet").forEach(function (s) { s.addEventListener("click", function (e) { if (e.target === s || e.target.hasAttribute("data-close")) { s.hidden = true; if (s.id === "captureSheet") closeCapture(); } }); });

  /* ================= stage & character ================= */
  const CHARS = [
    { key: "fuzz", name: "Fuzz", note: "arte pintada", species: 0, img: "fuzz_body.png" },
    { key: "soul", name: "Soul", note: "arte pintada", species: 1, img: "soul_body.png" },
    { key: "lua", name: "Lua", note: "arte pintada", species: 2, img: "lua_body.png" },
    { key: "nova", name: "Nova", note: "arte pintada", species: 3, img: "nova_body.png" },
    { key: "sapo", name: "Sapo", note: "pads de beat", species: 5, img: "sapo_body.png" },
    { key: "fuzz2", name: "Fuzz Street", note: "visual extra", species: 4, img: "fuzz2_body.png", level: 3 }
  ];
  const SPECIES = { 0: "fuzz", 1: "soul", 2: "lua", 3: "nova", 4: "fuzz2", 5: "sapo" };
  const DEFAULT_STYLE = { bounce: 1, sway: 1, nod: 1, dip: 1, lean: .3, hop: 0, nodHit: 1, jitter: 0 };
  let charKind = "fuzz", rig = {}, anchors = null, charSvg = null, sceneKind = "", sc = {};
  const motion = { squash: 0, squashV: 0, strike: 0, look: 0, blink: 0, nextBlink: 2, energy: 0, shake: 0, shakeT: 9, hair: 0, hairV: 0, alive: 0, lastSound: -1e9, x: 240, y: 412, rot: 0, nod: 0, nodV: 0, dark: 0, lfoPhase: 0, lean: 0, leanV: 0, hop: 0, hopV: 0 };
  function setT(el, t) { if (el) el.setAttribute("transform", t); }
  function setA(el, k, val) { if (el) el.setAttribute(k, val); }
  function buildStage() {
    sceneKind = ART.rooms[Math.round(v("sceneRoom"))] || "sunset";
    $("#stageArt").innerHTML = ART.scene(sceneKind);
    charSvg = $("#stageArt svg"); sc = { scale: ART.roomScale(sceneKind), ox: ART.roomOffset(sceneKind) };
    ["pxSky", "pxCity", "fg", "spot", "vu", "bloomVeil", "darkVeil", "neon", "room", "winGlow", "arcadeGlow", "bedGlow", "neonStrip", "lampGlow", "lamp2Glow", "globeGlow",
     "coneL1", "coneL2", "coneR1", "coneR2", "reel1", "reel2", "blob1", "blob2", "blob3"].forEach(function (id) { sc[id] = charSvg.getElementById(id); });
    sc.bulbs = $$(".bulb", charSvg); sc.wins = $$(".win", charSvg); sc.stars = $$(".star", charSvg); sc.sways = $$(".sway", charSvg).concat($$("#leaves .sway")); sc.spk = $$(".spkGlow", charSvg);
    buildCharacter();
  }
  function buildCharacter() {
    const C = ART.characters[charKind]();
    anchors = C.anchors;
    const layer = $("#charLayer", charSvg);
    layer.innerHTML = '<g id="charPos" transform="translate(240 412)"><g transform="scale(' + (.86) + ')">' + C.svg + '</g></g>';
    rig = { painted: !!C.painted, pivot: C.pivot || [0, C.anchors.head[1] + 40], style: C.style || DEFAULT_STYLE };
    ["c-root", "c-head", "c-hair", "c-eyes", "c-armL", "c-armR", "c-inst", "c-torso", "c-legL", "c-legR", "c-mouth", "c-mouthOpen"].forEach(function (id) { rig[id] = layer.querySelector("#" + id); });
    rig.pos = layer.querySelector("#charPos"); rig.pupils = $$(".c-pupil", layer);
    rig.keyGlow = layer.querySelector(".keyGlow"); rig.keySpot = layer.querySelector(".keySpot"); rig.pads = $$(".padLit", layer).map(function (el) { return { el: el, v: 0 }; });
    rig.keyRx = rig.keyGlow ? Number(rig.keyGlow.getAttribute("rx")) : 0; rig.keyCx = rig.keyGlow ? Number(rig.keyGlow.getAttribute("cx")) : 0; rig.flash = 0;
    wearItems();
    const gl = $("#ghosts", charSvg); gl.innerHTML = "";
    rig.ghosts = [0, 1, 2, 3].map(function () { const g = document.createElementNS("http://www.w3.org/2000/svg", "g"); g.setAttribute("class", "ghost"); g.setAttribute("opacity", "0"); gl.appendChild(g); return { el: g }; });
    rig.ghostClock = 0;
  }
  function wearItems() {
    if (!charSvg) return;
    $$(".acc", charSvg).forEach(function (n) { n.remove(); });
    (game.state.worn || []).forEach(function (id) {
      const g = document.createElementNS("http://www.w3.org/2000/svg", "g");
      g.setAttribute("class", "acc"); g.innerHTML = ART.accessory(id, anchors);
      if (id === "halo") rig["c-head"].insertBefore(g, rig["c-head"].firstChild);
      else if (ART.behind[id]) rig["c-root"].insertBefore(g, rig["c-root"].firstChild);
      else if (id === "chain") rig["c-torso"].appendChild(g);
      else rig["c-head"].appendChild(g);
    });
  }
  let mouseX = 0, mouseY = 0, mx = 0, my = 0;
  $("#stage").addEventListener("pointermove", function (e) { const r = e.currentTarget.getBoundingClientRect(); mouseX = (e.clientX - r.left) / r.width - .5; mouseY = (e.clientY - r.top) / r.height - .5; });
  $("#stage").addEventListener("pointerleave", function () { mouseX = mouseY = 0; });
  $("#stage").addEventListener("pointerdown", function () { motion.squashV -= .25; motion.nodV += 3; spawnNotes(240, 250, 2, .6); });
  function spawnNotes(x, y, count, vel) {
    if (animMode < 2 || !charSvg) return;
    const g = $("#notesP", charSvg), glyphs = ["♪", "♫", "★", "✦"];
    for (let i = 0; i < count; i++) {
      const t = document.createElementNS("http://www.w3.org/2000/svg", "text");
      t.textContent = glyphs[(Math.random() * glyphs.length) | 0];
      t.setAttribute("x", x + (Math.random() - .5) * 40); t.setAttribute("y", y); t.setAttribute("font-size", 14 + vel * 16);
      t.setAttribute("fill", ["#ffd23f", "#ff7a3d", "#39e3d2", "#ff8ad1"][(Math.random() * 4) | 0]); t.setAttribute("stroke", "#17101f"); t.setAttribute("stroke-width", "1.5"); t.setAttribute("font-weight", "900");
      g.appendChild(t);
      const dx = (Math.random() - .5) * 140, dy = -80 - Math.random() * 90, rot = (Math.random() - .5) * 80;
      if (t.animate) t.animate([{ transform: "translate(0,0) rotate(0deg)", opacity: 1 }, { transform: "translate(" + dx + "px," + dy + "px) rotate(" + rot + "deg)", opacity: 0 }], { duration: 1000 + Math.random() * 500, easing: "cubic-bezier(.2,.7,.3,1)" }).onfinish = function () { t.remove(); };
      else setTimeout(function () { t.remove(); }, 1300);
    }
  }
  const dust = [];
  function stepDust(amount, dt) {
    const g = charSvg && $("#dustP", charSvg); if (!g) return;
    if (animMode >= 1 && Math.random() < amount * 2.5 * dt * 60 && dust.length < 60) {
      const c = document.createElementNS("http://www.w3.org/2000/svg", "circle");
      c.setAttribute("r", (Math.random() * 1.8 + .6).toFixed(1)); c.setAttribute("fill", Math.random() < .5 ? "#fff6dc" : "#17101f"); g.appendChild(c);
      dust.push({ el: c, x: Math.random() * 480, y: Math.random() * 300 + 40, vx: (Math.random() - .5) * 12, vy: 8 + Math.random() * 14, life: 2 + Math.random() * 2 });
    }
    for (let i = dust.length - 1; i >= 0; i--) { const d = dust[i]; d.life -= dt; d.x += d.vx * dt; d.y += d.vy * dt; if (d.life <= 0) { d.el.remove(); dust.splice(i, 1); continue; } d.el.setAttribute("cx", d.x.toFixed(1)); d.el.setAttribute("cy", d.y.toFixed(1)); d.el.setAttribute("opacity", Math.min(1, d.life).toFixed(2)); }
  }
  function crumbleGlitch(amount) {
    if (animMode < 2 || Math.random() > amount * .5) return;
    const s = document.createElement("span");
    s.style.left = (Math.random() * 90) + "%"; s.style.top = (Math.random() * 85) + "%"; s.style.width = (10 + Math.random() * 60) + "px"; s.style.height = (4 + Math.random() * 18) + "px";
    $("#fx-crumble").appendChild(s); setTimeout(function () { s.remove(); }, 90 + Math.random() * 120);
  }

  /* ================= game layer ================= */
  const game = {
    state: { xp: 0, level: 1, worn: [], on: 1 }, combo: 0, lastNote: 0,
    load: function () { try { const s = JSON.parse(localStorage.getItem("rastro.game") || "null"); if (s) this.state = Object.assign(this.state, s); } catch (e) { /* storage unavailable: session only */ } },
    save: function () { try { localStorage.setItem("rastro.game", JSON.stringify(this.state)); } catch (e) { /* ignore */ } },
    need: function (lvl) { return 60 + lvl * 40; },
    next: function () { return ART.items.filter(function (it) { return it.level > game.state.level; })[0]; },
    award: function (xp, why) {
      if (!this.state.on) return;
      this.state.xp += xp;
      while (this.state.xp >= this.need(this.state.level)) {
        this.state.xp -= this.need(this.state.level); this.state.level++;
        const item = ART.items.filter(function (it) { return it.level === game.state.level; })[0];
        popup("NÍVEL " + this.state.level + "!", 240, 150, "#ffd23f");
        const ch = CHARS.filter(function (c) { return c.level === game.state.level; })[0];
        toast(ch ? "Desbloqueou o visual " + ch.name + "! Escolha na aba CHARACTER." : item ? "Desbloqueou: " + item.name + "! Vista na aba CHARACTER." : "Nível " + this.state.level + "!", 4000);
      }
      this.save(); this.hud();
      if (why && why !== "note" && why !== "tweak") popup("+" + xp + " XP", 240, 300, "#39e3d2");
    },
    hud: function () {
      const need = this.need(this.state.level), nx = this.next();
      $("#lvlBadge").textContent = "NV " + this.state.level;
      $("#xpFill").style.width = Math.min(100, this.state.xp / need * 100) + "%";
      $("#nextReward").textContent = nx ? "próximo: " + nx.name + " · " + Math.ceil(need - this.state.xp) + " XP" : "coleção completa!";
      $("#collectionCount").textContent = ART.items.filter(function (it) { return it.level <= game.state.level; }).length + "/" + ART.items.length;
      document.body.classList.toggle("game-off", !this.state.on);
      if (document.body.dataset.tab === "character") renderReward();
    },
    note: function (vel, now) {
      if (!this.state.on) return;
      const gap = now - this.lastNote; this.lastNote = now;
      this.combo = gap < 1400 ? this.combo + 1 : 1;
      let xp = 1;
      if (T.hostPlaying && T.beat >= 0) { const ph = (T.beat * 2) % 1; if (ph < .12 || ph > .88) { xp += 2; if (Math.random() < .5) popup("NO TEMPO!", 150 + Math.random() * 180, 210, "#39e3d2"); } }
      if (this.combo % 8 === 0) { const words = ["GROOVY!", "QUE RASTRO!", "ON FIRE!", "LENDÁRIO!"]; popup(words[Math.min(3, this.combo / 8 - 1 | 0)] + " ×" + this.combo, 240, 170, "#ff7a3d"); xp += 4; }
      this.award(xp, "note");
      const c = $(".hud-combo"); $("#comboNum").textContent = this.combo; c.classList.add("bump"); setTimeout(function () { c.classList.remove("bump"); }, 90);
    },
    tick: function (now) { if (this.combo && now - this.lastNote > 2200) { this.combo = 0; $("#comboNum").textContent = "0"; } }
  };
  function popup(text, x, y, color) {
    if (!game.state.on) return;
    const st = $("#stage"), sx = st.clientWidth / 480;
    const p = document.createElement("div"); p.className = "pop"; p.textContent = text; p.style.left = (x * sx - 60) + "px"; p.style.top = y + "px"; p.style.color = color || "#fff";
    $("#popups").appendChild(p); setTimeout(function () { p.remove(); }, 950);
  }
  const fakeAnchors = { head: [0, 0], headW: 60, chest: [0, 0], back: [0, -10], top: [0, -6] };
  function renderReward() {
    const nx = game.next(), need = game.need(game.state.level);
    $("#rewardBox").innerHTML = nx ? '<svg viewBox="-70 -60 140 110">' + ART.accessory(nx.id, fakeAnchors) + '</svg><div style="flex:1"><b>Próximo prêmio: ' + nx.name + '</b><div class="fine" style="margin:2px 0">Nível ' + nx.level + ' · faltam ' + Math.ceil(need - game.state.xp) + ' XP. Toque, faça combos, acerte o tempo, mexa no som, use o Patch Lab e capture sons.</div><div class="bar"><div style="width:' + Math.min(100, game.state.xp / need * 100) + '%"></div></div></div>' : "<b>Coleção completa!</b>";
    const grid = $("#collectionGrid"); grid.innerHTML = "";
    ART.items.forEach(function (it) {
      const ok = it.level <= game.state.level, worn = game.state.worn.indexOf(it.id) >= 0;
      const b = document.createElement("button"); b.className = "item" + (ok ? "" : " locked") + (worn ? " worn" : ""); b.disabled = !ok;
      b.innerHTML = '<svg viewBox="-70 -60 140 110">' + ART.accessory(it.id, fakeAnchors) + '</svg>' + (ok ? it.name + (worn ? " ✓" : "") : "🔒 Nível " + it.level);
      b.addEventListener("click", function () { const i = game.state.worn.indexOf(it.id); if (i >= 0) game.state.worn.splice(i, 1); else game.state.worn.push(it.id); game.save(); wearItems(); renderReward(); });
      grid.appendChild(b);
    });
  }
  $("#collectionBtn").addEventListener("click", function () { setTab("character"); setTimeout(function () { $("#rewardBox").scrollIntoView({ behavior: "smooth" }); }, 50); });

  /* ================= CHARACTER tab ================= */
  let animMode = 2;
  const KNOBS = [["Creme", "baquelite"], ["Cromado", "metal"], ["Neon", "brilho"], ["Borracha", "macio"], ["Madeira", "anos 70"]];
  function renderCustom() {
    const cg = $("#charChoices"); cg.innerHTML = "";
    CHARS.forEach(function (c) {
      const locked = c.level && game.state.level < c.level;
      const b = document.createElement("button"); b.className = "choice" + (c.key === charKind ? " on" : "") + (locked ? " locked" : ""); b.disabled = !!c.disabled || locked;
      let art;
      const ps = ART.paintedSize[c.key];
      if (c.img && ps) art = '<span class="pc" style="aspect-ratio:' + ps[0] + '/760"><img src="' + c.key + '_body.png" alt=""><img src="' + c.key + '_head.png" alt="" style="height:' + (ps[1] / 760 * 100).toFixed(2) + '%"></span>';
      else if (c.disabled) art = '<svg viewBox="-60 -60 120 120"><circle r="44" fill="var(--knob-track)"/><text y="22" text-anchor="middle" font-size="60" font-weight="900" fill="var(--muted)">?</text></svg>';
      else art = '<svg viewBox="-90 -340 180 350">' + ART.characters[c.key]().svg.replace(/ id="[^"]+"/g, "") + '</svg>';
      b.innerHTML = art + '<b>' + c.name + '</b><small>' + (locked ? "🔒 nível " + c.level : c.note) + '</small>';
      if (!c.disabled) b.addEventListener("click", function () { gesture("avatarCharacter", c.species); });
      cg.appendChild(b);
    });
    const kg = $("#knobChoices"); kg.innerHTML = "";
    KNOBS.forEach(function (k, i) {
      const b = document.createElement("button"); b.className = "choice kd-" + i + (Math.round(v("knobDesign")) === i ? " on" : "");
      b.innerHTML = '<span style="display:block;height:60px">' + knobSvg(56).replace('stroke-dasharray="0 999"', 'stroke-dasharray="' + (24 * Math.PI) + ' 999"').replace('class="ptr k-ptr"', 'class="ptr k-ptr" transform="rotate(60 28 28)"') + '</span><b>' + k[0] + '</b><small>' + k[1] + '</small>';
      b.addEventListener("click", function () { gesture("knobDesign", i); });
      kg.appendChild(b);
    });
    $$("#sceneChoices .choice").forEach(function (b) { b.classList.toggle("on", Number(b.dataset.room) === Math.round(v("sceneRoom"))); });
    renderReward();
  }
  function setupCustom() {
    $$("#sceneChoices .choice").forEach(function (b) { b.addEventListener("click", function () { gesture("sceneRoom", Number(b.dataset.room)); }); });
    watch("sceneRoom", function (x) { if (charSvg && sceneKind !== ART.rooms[Math.round(x)]) buildStage(); if (document.body.dataset.tab === "character") renderCustom(); });
    seg("#themeSeg", "theme", function () { return 0; }, function (x) { gesture("colourSet", x ? 8 : 0); });
    watch("colourSet", function (x) {
      const night = [2, 5, 6, 8].indexOf(Math.round(x)) >= 0;
      document.body.classList.remove("theme-night", "theme-sunset"); document.body.classList.add("theme-" + (night ? "night" : "sunset"));
      $$("#themeSeg button").forEach(function (b) { b.classList.toggle("on", (Number(b.dataset.theme) === 1) === night); });
      readPalette();
    });
    watch("avatarCharacter", function (x) {
      const k = SPECIES[Math.round(x)] || "fuzz";
      if (k !== charKind || !rig.pos) { charKind = k; if (charSvg) buildCharacter(); }
      if (document.body.dataset.tab === "character") renderCustom();
    });
    watch("knobDesign", function (x) { for (let i = 0; i < 5; i++) document.body.classList.toggle("kd-" + i, Math.round(x) === i); if (document.body.dataset.tab === "character") renderCustom(); });
    bindSeg("#danceStyleSeg", "ds", "danceStyle"); bindSeg("#danceAmtSeg", "da", "danceAmount");
    bindSeg("#animSeg", "anim", "animation");
    watch("animation", function (x) { animMode = Math.round(x); document.body.classList.toggle("anim-off", animMode === 0); });
    const gameRefresh = seg("#gameSeg", "game", function () { return game.state.on ? 1 : 0; }, function (x) { game.state.on = x; game.save(); game.hud(); gameRefresh(); });
    gameRefresh();
  }
  function setupHeader() {
    $("#jam").addEventListener("click", function () {
      R.call("jam").then(function (can) { $("#undoJam").disabled = !can; });
      motion.squashV -= .3; motion.nodV += 4; spawnNotes(240, 220, 6, .9); game.award(5, "jam"); popup("JAM!", 240, 190, "#ff4fa3");
    });
    $("#undoJam").addEventListener("click", function () { R.call("undoJam").then(function (can) { $("#undoJam").disabled = !can; }); toast("Voltou à combinação anterior."); });
  }

  /* ================= frame loop (all motion is smoothed and frame-rate independent) ================= */
  let lastSerial = -1, lastFrame = performance.now(), midiFlash = 0, beatClock = 0;
  function noteHeld(n) { return ((T.notes[n >> 5] >>> 0) & (1 << (n & 31))) !== 0; }
  function frame(now) {
    const dt = Math.min(.05, (now - lastFrame) / 1000); lastFrame = now;
    const amp = animMode === 2 ? 1 : animMode === 1 ? .5 : 0;
    const ease = function (rate) { return 1 - Math.exp(-rate * dt); };
    if (T.serial !== lastSerial) {
      if (lastSerial >= 0) {
        const vel = T.vel || .7;
        const st = rig.style || DEFAULT_STYLE;
        motion.squashV -= (.35 + vel * .45) * amp * st.dip; motion.strike = 1; motion.shake = vel * 2.5 * amp; motion.shakeT = 0; motion.nodV += (2 + vel * 4) * amp * st.nodHit; midiFlash = 1;
        motion.leanV += vel * vel * 26 * amp * st.lean; motion.hopV += (6 + vel * 10) * amp * st.hop;
        // light the instrument: keys glow where the note sits, pads light one cell per note
        let lastN = -1; for (let n = 0; n < 128; n++) if (noteHeld(n)) lastN = n;
        if (lastN < 0) lastN = 60;
        rig.flash = 1; rig.flashPos = ((lastN - base) / 48);
        if (rig.pads.length) { const p = rig.pads[(lastN % 16 + 16) % 16]; if (p) p.v = .6 + vel * .4; }
        spawnNotes(240 + (Math.random() - .5) * 60, 250, 1 + Math.round(vel * 3), vel);
        game.note(vel, now);
      }
      lastSerial = T.serial;
    }
    game.tick(now);
    $("#midiDot").classList.toggle("on", midiFlash > .1); midiFlash *= .85;
    $("#masterMeter").style.width = Math.min(100, T.meter * 100) + "%";
    // only real sound moves the stage
    let anyHeld = false; for (let w = 0; w < 4; w++) if (T.notes[w]) anyHeld = true;
    if (anyHeld || T.playing || T.preview) motion.lastSound = now;
    const aliveTarget = now - motion.lastSound < 900 ? 1 : 0;
    motion.alive += (aliveTarget - motion.alive) * ease(aliveTarget ? 10 : 2.5);
    const alive = motion.alive, meterT = clamp(T.meter * 1.6, 0, 1) * alive;
    motion.energy += (meterT - motion.energy) * ease(meterT > motion.energy ? 14 : 4);
    const bpm = T.bpm > 0 ? T.bpm : v("tempo"), style = Math.round(v("danceStyle")), amtSel = Math.round(v("danceAmount"));
    if (T.beat >= 0 && style !== 2) beatClock = T.beat; else beatClock = (beatClock + dt * bpm / 60 * (style === 2 ? .5 : 1)) % 4;
    const phase = beatClock % 1, onBeat = Math.pow(1 - phase, 4), groove = motion.energy > .02 ? motion.energy : 0;
    const dance = groove * amp * [0, .75, 1.3][amtSel];
    motion.squashV += (-motion.squash * 90 - motion.squashV * 15) * dt; motion.squash = clamp(motion.squash + motion.squashV * dt, -.06, .06);
    motion.nodV += (-motion.nod * 70 - motion.nodV * 13) * dt; motion.nod += motion.nodV * dt;
    motion.strike = Math.max(0, motion.strike - dt * 5);
    motion.leanV += (-motion.lean * 60 - motion.leanV * 11) * dt; motion.lean += motion.leanV * dt;
    motion.hopV += (-motion.hop * 160 - motion.hopV * 9) * dt; motion.hop = Math.max(-2, motion.hop + motion.hopV * dt);
    const ST = rig.style || DEFAULT_STYLE;
    motion.shakeT += dt; const shake = motion.shake * Math.exp(-motion.shakeT * 9) * Math.sin(motion.shakeT * 55);
    const bounce = style === 1 ? Math.abs(Math.sin(beatClock * Math.PI)) * .6 : Math.abs(Math.sin(beatClock * Math.PI));
    const sway = style === 1 ? Math.sin(beatClock * Math.PI) * 1.8 : Math.sin(beatClock * Math.PI * .5) * .8;
    const breathe = Math.sin(now / 1100) * .006;
    const act = function (i) { return clamp((T.act[i] || 0) * 14, 0, 1) * alive; };
    const ghostAct = act(0), crumbleAct = act(1), bloom = act(2), wob = act(3), dst = act(4), crush = act(5);
    motion.nextBlink -= dt; if (motion.nextBlink <= 0) { motion.blink = 1; motion.nextBlink = 2 + Math.random() * 3; }
    motion.blink = Math.max(0, motion.blink - dt * 7);
    let lookTarget = mouseX * .8;
    for (let n = 0; n < 128; n++) if (noteHeld(n)) { lookTarget = clamp((n - base - 24) / 24, -1, 1); break; }
    motion.look += (lookTarget - motion.look) * ease(6);
    mx += (mouseX - mx) * ease(4); my += (mouseY - my) * ease(4);
    const synthOn = v("synthEnable") > .5, cutT = clamp(Math.log(Math.max(20, v("filterCutoff")) / 20) / Math.log(1000), 0, 1);
    motion.dark += ((synthOn ? (1 - cutT) * .5 : 0) - motion.dark) * ease(5);
    motion.lfoPhase = (motion.lfoPhase + dt * v("lfoRate") * Math.PI * 2) % (Math.PI * 2);
    const lfo = synthOn ? Math.sin(motion.lfoPhase) * v("lfoDepth") * alive : 0, res = synthOn ? v("filterResonance") : 0;
    const wobOn = v("wOn") > .5 && v("wPresent") > .5, degOn = ["c", "d", "k"].some(function (m) { return v(m + "On") > .5 && v(m + "Present") > .5; });
    const camWob = (wobOn ? Math.max(0, v("playX") - .5) * 2 : 0) * alive, degrade = (degOn ? Math.max(0, v("playY") - .5) * 2 : 0) * alive;
    const stageOn = document.body.dataset.tab === "sound" || document.body.dataset.tab === "character";
    if (rig.pos && animMode > 0 && stageOn) {
      const jit = ST.jitter ? (Math.sin(now / 47) * Math.sin(now / 131)) * ST.jitter * dance * 2 : 0;
      const tx = 240 + sway * 8 * dance * ST.sway + jit + Math.sin(now / 260) * (wob + camWob * .4) * 5 * amp, ty = 412 - bounce * 8 * dance * ST.bounce - Math.max(0, motion.hop);
      motion.x += (tx - motion.x) * ease(ST.smooth ? 7 : 16); motion.y += (ty - motion.y) * ease(ST.smooth ? 10 : 22);
      let x = motion.x, y = motion.y;
      if (crush > .25) { const q = 2 + crush * 5; x = Math.round(x / q) * q; y = Math.round(y / q) * q; }
      const sq = motion.squash + breathe, sx = 1 - sq * .5, sy = 1 + sq;
      const trot = sway * 2.2 * dance * ST.sway + Math.sin(now / 300) * (wob + camWob * .5) * 4 * amp + lfo * 2 - motion.lean;
      motion.rot += (trot - motion.rot) * ease(10);
      rig.pos.setAttribute("transform", "translate(" + x.toFixed(2) + " " + y.toFixed(2) + ") rotate(" + motion.rot.toFixed(2) + ") scale(" + sx.toFixed(4) + " " + sy.toFixed(4) + ")");
      const pv = rig.pivot, nodBeat = Math.sin(beatClock * Math.PI * 2 + .6) * (rig.painted ? 3.5 : 5) * dance * ST.nod;
      setT(rig["c-head"], "rotate(" + (nodBeat + motion.look * (rig.painted ? 2.5 : 4) - motion.nod).toFixed(2) + " " + pv[0].toFixed(1) + " " + pv[1].toFixed(1) + ")");
      const hy = anchors.head[1];
      motion.hairV += ((-sway * 10 * dance - bounce * 5 * dance) - motion.hair) * 40 * dt; motion.hairV *= Math.exp(-dt * 8); motion.hair += motion.hairV * dt;
      setT(rig["c-hair"], "rotate(" + motion.hair.toFixed(2) + " 0 " + (hy - 20) + ")");
      const ey = hy + 4, bl = 1 - motion.blink * .9;
      setT(rig["c-eyes"], "translate(0 " + ey + ") scale(1 " + bl.toFixed(2) + ") translate(0 " + (-ey) + ")");
      rig.pupils.forEach(function (p) { p.setAttribute("transform", "translate(" + (motion.look * 4).toFixed(1) + " " + (my * 3).toFixed(1) + ")"); });
      const open = clamp(T.env * 1.2, 0, 1) * (T.playing ? 1 : 0);
      setA(rig["c-mouthOpen"], "ry", (open * 8).toFixed(1)); setA(rig["c-mouth"], "opacity", open > .15 ? 0 : 1);
      setT(rig["c-armL"], "translate(" + (motion.look * 10).toFixed(1) + " " + (motion.strike * 5).toFixed(1) + ")");
      setT(rig["c-armR"], "rotate(" + (Math.sin(beatClock * Math.PI * 2) * 4 * dance).toFixed(2) + " 40 " + (anchors.chest[1] - 20) + ")");
      if (rig["c-inst"]) rig["c-inst"].setAttribute("transform", rig["c-inst"].getAttribute("transform").replace(/rotate\([^)]*\)/, "rotate(" + (-24 + motion.strike * 3 + sway * 1.5 * dance).toFixed(2) + ")"));
      const knee = bounce * 5 * dance;
      setT(rig["c-legL"], "rotate(" + (-knee).toFixed(2) + " -20 -124)"); setT(rig["c-legR"], "rotate(" + knee.toFixed(2) + " 20 -124)");
      if (rig.painted) setT(rig["c-torso"], "translate(0 " + (motion.strike * 1.5).toFixed(2) + ")");
      rig.ghostClock += dt;
      if (ghostAct > .04 && amp > 0 && rig.ghostClock > .09) {
        rig.ghostClock = 0;
        const last = rig.ghosts.pop(); rig.ghosts.unshift(last);
        const snap = rig.pos.cloneNode(true); snap.removeAttribute("id"); $$("[id]", snap).forEach(function (n) { n.removeAttribute("id"); });
        last.el.innerHTML = ""; last.el.appendChild(snap);
      }
      rig.ghosts.forEach(function (gh, i) { gh.el.setAttribute("opacity", Math.max(0, ghostAct * amp * (.5 - i * .11)).toFixed(2)); gh.el.setAttribute("transform", "translate(" + (-(i + 1) * 16 * ghostAct).toFixed(1) + " " + (-(i + 1) * 3 * ghostAct).toFixed(1) + ")"); });
      rig.flash = Math.max(0, rig.flash - dt * 4);
      if (rig.keyGlow) {
        const held = anyHeld ? .45 : 0, f = Math.max(rig.flash, held * alive);
        rig.keyGlow.setAttribute("opacity", (f * .85).toFixed(2));
        rig.keySpot.setAttribute("opacity", (rig.flash * .9).toFixed(2));
        rig.keySpot.setAttribute("cx", (rig.keyCx + clamp((rig.flashPos || .5) - .5, -.5, .5) * rig.keyRx * 1.6).toFixed(1));
      }
      rig.pads.forEach(function (p) { p.v = Math.max(0, p.v - dt * 3.5); p.el.setAttribute("opacity", (p.v * .9).toFixed(2)); });
    }
    if (charSvg && animMode > 0 && stageOn) {
      const camRot = Math.sin(now / 320) * camWob * 1.4 * amp;
      $("#stageArt").style.transform = "translate(" + shake.toFixed(2) + "px," + (shake * .6).toFixed(2) + "px) rotate(" + camRot.toFixed(2) + "deg)";
      setT(sc.pxSky, "translate(" + (-mx * 10).toFixed(1) + " " + (-my * 6).toFixed(1) + ")");
      setT(sc.pxCity, "translate(" + (-mx * 18).toFixed(1) + " " + (-my * 4).toFixed(1) + ")");
      setT(sc.fg, "translate(" + (mx * 16).toFixed(1) + " " + (my * 6).toFixed(1) + ")");
      setT(sc.room, "translate(" + (sc.ox - mx * 110).toFixed(1) + " " + (-18 - my * 10).toFixed(1) + ") scale(" + sc.scale.toFixed(4) + ")");
      const cone = 1 + motion.energy * .3 * amp + onBeat * motion.energy * .15;
      ["coneL1", "coneL2", "coneR1", "coneR2"].forEach(function (id) { setT(sc[id], "scale(" + cone.toFixed(3) + ")"); });
      sc.spk.forEach(function (g) { const cx = g.getAttribute("cx"), cy = g.getAttribute("cy"); g.setAttribute("opacity", (motion.energy * .8 + onBeat * groove * .4).toFixed(2)); g.setAttribute("transform", "translate(" + cx + " " + cy + ") scale(" + (.8 + motion.energy * .45 + onBeat * groove * .25).toFixed(2) + ") translate(" + (-cx) + " " + (-cy) + ")"); });
      setA(sc.spot, "opacity", (.25 + motion.energy * .8 + motion.strike * .25).toFixed(2));
      const lit = Math.floor(beatClock * 2) % 3;
      sc.bulbs.forEach(function (b, i) { b.classList.toggle("lit", groove > .03 && i % 3 === lit); });
      const spin = (now / 1000) * (T.playing || T.preview ? 220 : 0);
      setT(sc.reel1, "translate(44 207) rotate(" + spin.toFixed(1) + ")"); setT(sc.reel2, "translate(80 207) rotate(" + spin.toFixed(1) + ")");
      setA(sc.vu, "width", (motion.energy * 64).toFixed(1));
      const lava = now / 1000 * (.4 + bloom * 2.4);
      ["blob1", "blob2", "blob3"].forEach(function (id, i) { setT(sc[id], "translate(" + (Math.sin(lava * (1 + i * .3) + i) * 5).toFixed(1) + " " + (Math.sin(lava * .7 + i * 2) * 14).toFixed(1) + ")"); });
      setA(sc.bloomVeil, "opacity", (bloom * .22 * amp).toFixed(3));
      setA(sc.darkVeil, "opacity", motion.dark.toFixed(3));
      if (sc.neon) sc.neon.style.opacity = (.75 + bloom * .25 + onBeat * groove * .2 + res * Math.abs(Math.sin(now / 90)) * .2).toFixed(2);
      setA(sc.lampGlow, "opacity", (.15 + cutT * (synthOn ? .5 : .25) + onBeat * groove * .25).toFixed(2));
      setA(sc.lamp2Glow, "opacity", (.2 + motion.energy * .5 + res * .3).toFixed(2));
      setA(sc.globeGlow, "opacity", (.2 + Math.abs(lfo) * .7 + bloom * .4).toFixed(2));
      setA(sc.winGlow, "opacity", (bloom * .35 + groove * .08).toFixed(2));
      setA(sc.arcadeGlow, "opacity", (Math.random() < (crush + crumbleAct) * .4 ? .6 : .12 + onBeat * groove * .3).toFixed(2));
      setA(sc.bedGlow, "opacity", (groove * .8 * onBeat + motion.strike * .25).toFixed(2));
      setA(sc.neonStrip, "opacity", (.1 + res * .5 * (.5 + .5 * Math.sin(now / 70)) + groove * .15).toFixed(2));
      if (Math.random() < .3) sc.wins.forEach(function (w) { if (Math.random() < .02 + motion.energy * .05) w.style.opacity = Math.random() < .3 + motion.energy ? "1" : "0"; });
      sc.stars.forEach(function (st, i) { st.style.opacity = (.4 + .6 * Math.abs(Math.sin(now / 700 + i))).toFixed(2); });
      sc.sways.forEach(function (l, i) { l.style.transform = "rotate(" + (Math.sin(now / 1500 + i) * 1.6 + motion.energy * Math.sin(beatClock * Math.PI * 2) * 2.5 + lfo * 6).toFixed(2) + "deg)"; });
      $("#fx-pixels").style.opacity = clamp((crush * .9 + degrade * .45) * amp, 0, 1).toFixed(2);
      if (crumbleAct > .05 || degrade > .3) crumbleGlitch(Math.max(crumbleAct, degrade * .5) * amp);
      stepDust(Math.max(dst, degrade * .3) * amp, dt);
    }
    const bars = $$("#grooveBars i");
    bars.forEach(function (b, i) { const lvl = clamp(motion.energy * 1.25 - i / bars.length + .08 + onBeat * groove * .15, 0, 1); b.style.height = (12 + lvl * 88) + "%"; b.style.opacity = lvl > .05 ? 1 : .35; });
    cards.forEach(function (c) {
      const a = clamp((T.act[c.mi] || 0) * 12, 0, 1) * (v(MOD[c.mi].k + "On") > .5 ? 1 : 0);
      c.meter.style.width = (a * 100) + "%";
      if (animMode === 0 || !c.art) return;
      const t = now / 1000, art = c.art;
      if (c.mi === 0) { const tr = art.querySelector(".ic-trail"); tr.style.transform = "translateX(" + (-a * 9 - Math.sin(t * 6) * a * 2).toFixed(1) + "px)"; tr.style.opacity = a; }
      if (c.mi === 1) art.querySelector(".crumbs").style.transform = "translate(" + (a * 4) + "px," + (Math.sin(t * 20) * a * 3).toFixed(1) + "px)";
      if (c.mi === 2) art.querySelector(".petals").style.transform = "scale(" + (.82 + a * .3).toFixed(2) + ") rotate(" + (t * 30 * a).toFixed(1) + "deg)";
      if (c.mi === 3) art.querySelector(".wave1").style.transform = "translateX(" + (Math.sin(t * 5) * 4 * (.3 + a)).toFixed(1) + "px)";
      if (c.mi === 4) { art.querySelector(".spin").style.transform = "rotate(" + (t * 160 * (.15 + a)).toFixed(1) + "deg)"; art.querySelector(".specks").style.opacity = .3 + a * .7; }
      if (c.mi === 5) art.querySelector(".px").style.transform = "translate(" + (Math.round(Math.sin(t * 13) * a * 3)) + "px,0)";
    });
    for (const n in keyEls) keyEls[n].classList.toggle("on", noteHeld(Number(n)));
    $("#tPlay").classList.toggle("on", !!T.preview);
    if (waveDirty || T.playing || T.preview) { drawWave(); waveDirty = false; }
    requestAnimationFrame(frame);
  }

  /* ================= boot ================= */
  function boot(init) {
    (init.params || []).forEach(function (p) { P[p.id] = p; });
    game.load(); knobDefs();
    $("#grooveBars").innerHTML = new Array(19).join("<i></i>");
    $("#leaves").innerHTML = ART.cornerLeaves();
    setupCustom(); buildStage();
    setupHeader(); setupSample(); setupEnv(); setupSynth(); setupPad();
    readPalette(); setTab("sound"); fit(); buildKeys(); game.hud(); onSample(init.sample); updatePill();
    $("#undoJam").disabled = !init.canUndoJam;
    if (R.simulated) toast("Prévia no navegador com motor simulado. No plugin, tudo usa o áudio real.", 4200);
    requestAnimationFrame(frame);
  }
  R.on("tick", function (t) {
    T = Object.assign(T, t);
    if (t.order) { const changed = t.order.join() !== order.join(); order = t.order.slice(); if (changed) renderRack(); }
    $("#undoJam").disabled = !t.canUndoJam;
    labTick(t.search); capTick(t.capture);
  });
  R.on("params", function (changes) {
    let rack = false;
    for (const id in changes) { apply(id, changes[id]); if (/Present$|Timing$/.test(id)) rack = true; }
    if (rack) renderRack();
  });
  R.on("sample", onSample);
  R.call("ready").then(boot);
})();
