/* RASTRO Play — interface logic. Every control maps to a real plugin parameter or action. */
(function () {
  "use strict";
  const R = window.Rastro, ART = window.RastroArt;
  const $ = function (s, el) { return (el || document).querySelector(s); };
  const $$ = function (s, el) { return Array.prototype.slice.call((el || document).querySelectorAll(s)); };
  const clamp = function (v, a, b) { return Math.min(b, Math.max(a, v)); };
  const lerp = function (a, b, t) { return a + (b - a) * t; };
  const NOTE = ["C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"];
  const noteName = function (n) { n = Math.round(n); return NOTE[((n % 12) + 12) % 12] + (Math.floor(n / 12) - 1); };

  /* ================= state ================= */
  const P = {};               // id -> {v,min,max,step,def}
  const views = {};           // id -> [fn(value)]
  const busy = {};            // ids being dragged by the user
  let T = { meter: 0, act: [0, 0, 0, 0, 0, 0], env: 0, wob: 0, dust: 0, crush: 0, bloom: 0, pos: 0, playing: false, vel: 0, serial: 0, bpm: 0, beat: -1, notes: [0, 0, 0, 0] };
  let order = [0, 1, 2, 3, 4, 5];
  let sample = { loaded: false, name: "", seconds: 0 };
  let peaks = null;
  let S = 1; // UI scale

  function v(id) { return P[id] ? P[id].v : 0; }
  function watch(id, fn) { (views[id] = views[id] || []).push(fn); if (P[id]) fn(P[id].v); }
  function apply(id, value) { if (!P[id]) return; P[id].v = value; if (!busy[id]) (views[id] || []).forEach(function (f) { f(value); }); }
  function setParam(id, value, phase) {
    if (!P[id]) return;
    value = clamp(value, P[id].min, P[id].max);
    if (P[id].step) value = Math.round(value / P[id].step) * P[id].step;
    P[id].v = value;
    (views[id] || []).forEach(function (f) { f(value); });
    R.call("setParam", id, value, phase || 0);
  }
  function gesture(id, value) { R.call("setParam", id, value === undefined ? null : value, 1); setParam(id, value === undefined ? v(id) : value, 2); }

  /* ================= scaling ================= */
  function fit() {
    const w = window.innerWidth, h = window.innerHeight;
    S = Math.min(w / 1200, h / 780);
    const app = $("#app");
    app.style.transform = "translate(" + Math.max(0, (w - 1200 * S) / 2) + "px," + Math.max(0, (h - 780 * S) / 2) + "px) scale(" + S + ")";
    sizeCanvas();
  }
  window.addEventListener("resize", fit);

  /* ================= toast ================= */
  let toastTimer = 0;
  function toast(msg, ms) {
    const t = $("#toast"); t.textContent = msg; t.classList.add("show");
    clearTimeout(toastTimer); toastTimer = setTimeout(function () { t.classList.remove("show"); }, ms || 2600);
  }

  /* ================= knobs ================= */
  const fmt = {
    pct: function (x) { return Math.round(x * 100) + "%"; },
    ms: function (x) { return x < 1 ? Math.round(x * 1000) + " ms" : x.toFixed(2) + " s"; },
    hz: function (x) { return x >= 1000 ? (x / 1000).toFixed(1) + " kHz" : Math.round(x) + " Hz"; },
    lfo: function (x) { return x.toFixed(2) + " Hz"; },
    note: function (x) { return noteName(x); },
    db: function (x) { return x <= 0.0001 ? "−∞" : (20 * Math.log10(x)).toFixed(1) + " dB"; },
    bip: function (x) { return (x > 0 ? "+" : "") + Math.round(x * 100) + "%"; },
    xms: function (x) { return Math.round(x) + " ms"; },
    raw: function (x) { return x.toFixed(2); }
  };
  function knob(parent, id, label, opts) {
    opts = opts || {};
    const size = opts.size || 40, r = size / 2 - 4, el = document.createElement("div");
    el.className = "knob"; el.title = opts.title || label;
    el.innerHTML = '<svg width="' + size + '" height="' + size + '" viewBox="0 0 ' + size + ' ' + size + '">' +
      '<circle cx="' + size / 2 + '" cy="' + size / 2 + '" r="' + r + '" fill="none" stroke="var(--knob-track)" stroke-width="5" stroke-dasharray="' + (r * Math.PI * 1.5) + ' 999" transform="rotate(135 ' + size / 2 + ' ' + size / 2 + ')" stroke-linecap="round"/>' +
      '<circle class="arc" cx="' + size / 2 + '" cy="' + size / 2 + '" r="' + r + '" fill="none" stroke="' + (opts.color || "var(--accent)") + '" stroke-width="5" stroke-linecap="round" transform="rotate(135 ' + size / 2 + ' ' + size / 2 + ')"/>' +
      '<circle cx="' + size / 2 + '" cy="' + size / 2 + '" r="' + (r - 6) + '" fill="var(--panel2)" stroke="var(--line)" stroke-width="2.5"/>' +
      '<line class="ptr" x1="' + size / 2 + '" y1="' + size / 2 + '" x2="' + size / 2 + '" y2="' + (size / 2 - r + 8) + '" stroke="var(--knob)" stroke-width="3" stroke-linecap="round"/>' +
      '</svg><div class="k-label">' + label + '</div><div class="k-value"></div>';
    parent.appendChild(el);
    const arc = $(".arc", el), ptr = $(".ptr", el), val = $(".k-value", el), circ = r * Math.PI * 1.5;
    const p = function () { return P[id]; };
    const log = opts.curve === "log";
    function toT(x) { const q = p(); if (!q) return 0; if (log) { const lo = Math.max(q.min, 1e-4); return Math.log(Math.max(x, lo) / lo) / Math.log(q.max / lo); } return (x - q.min) / (q.max - q.min); }
    function fromT(t) { const q = p(); t = clamp(t, 0, 1); if (log) { const lo = Math.max(q.min, 1e-4); return lo * Math.pow(q.max / lo, t); } return q.min + t * (q.max - q.min); }
    function draw(x) {
      const t = clamp(toT(x), 0, 1);
      arc.setAttribute("stroke-dasharray", (t * circ) + " 999");
      ptr.setAttribute("transform", "rotate(" + (-135 + t * 270) + " " + size / 2 + " " + size / 2 + ")");
      val.textContent = (opts.format || fmt.pct)(x);
    }
    if (opts.owned) draw.rackOwned = true;
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
      const k = e.shiftKey ? 0.0012 : 0.0055;
      const x = fromT(startT + (startY - e.clientY) / S * k);
      setParam(id, x); draw(P[id].v);
    });
    function end(e) { if (!busy[id]) return; busy[id] = false; el.classList.remove("dragging"); R.call("setParam", id, null, 2); if (opts.onEnd) opts.onEnd(); }
    el.addEventListener("pointerup", end); el.addEventListener("pointercancel", end);
    el.addEventListener("dblclick", function () { if (p()) gesture(id, p().def); });
    el.addEventListener("wheel", function (e) { if (!p()) return; e.preventDefault(); setParam(id, fromT(toT(v(id)) - e.deltaY * 0.0008)); }, { passive: false });
    return el;
  }

  /* ================= segmented controls ================= */
  function seg(selector, attr, read, write) {
    const buttons = $$(selector + " button");
    buttons.forEach(function (b) { b.addEventListener("click", function () { write(Number(b.dataset[attr])); }); });
    return function refresh() { const cur = read(); buttons.forEach(function (b) { b.classList.toggle("on", Number(b.dataset[attr]) === cur); }); };
  }
  function bindSeg(selector, attr, id) {
    const refresh = seg(selector, attr, function () { return Math.round(v(id)); }, function (x) { gesture(id, x); });
    watch(id, refresh);
  }
  function bindToggle(btn, id, after) {
    btn.addEventListener("click", function () { gesture(id, v(id) > .5 ? 0 : 1); if (after) after(); });
    watch(id, function (x) { btn.classList.toggle("on", x > .5); btn.setAttribute("aria-pressed", x > .5); });
  }

  /* ================= sample / waveform ================= */
  const canvas = $("#wave"), ctx = canvas.getContext("2d"), waveBox = $("#waveBox");
  let view = [0, 1], cols = null, waveDirty = true, dragMarker = null;
  function sizeCanvas() {
    const dpr = (window.devicePixelRatio || 1) * S;
    canvas.width = Math.round(waveBox.clientWidth * dpr); canvas.height = Math.round(waveBox.clientHeight * dpr);
    cols = null; waveDirty = true;
  }
  function mode() { const m = Math.round(v("playMode")); return m === 0 && v("loop") > .5 ? 1 : m; }
  function cut(i) { const n = Math.round(v("sliceCount")); if (i <= 0) return 0; if (i >= n) return 1; return v("slice" + i); }
  function cutAbs(i) { return v("start") + (v("end") - v("start")) * cut(i); }
  function buildCols() {
    if (!peaks || !peaks.l) { cols = null; return; }
    const W = canvas.width, chans = peaks.stereo && peaks.r ? [peaks.l, peaks.r] : [peaks.l];
    cols = chans.map(function (data) {
      const n = data.length / 2, out = new Float32Array(W * 2);
      for (let x = 0; x < W; x++) {
        const a = view[0] + (view[1] - view[0]) * x / W, b = view[0] + (view[1] - view[0]) * (x + 1) / W;
        let i0 = Math.floor(a * n), i1 = Math.max(i0 + 1, Math.ceil(b * n)), lo = 0, hi = 0;
        for (let i = i0; i < i1 && i < n; i++) { lo = Math.min(lo, data[i * 2]); hi = Math.max(hi, data[i * 2 + 1]); }
        out[x * 2] = lo; out[x * 2 + 1] = hi;
      }
      return out;
    });
  }
  function px(norm) { return (norm - view[0]) / (view[1] - view[0]) * canvas.width; }
  function normAt(clientX) { const r = canvas.getBoundingClientRect(); return view[0] + (view[1] - view[0]) * (clientX - r.left) / r.width; }
  function cssColor(name) { return getComputedStyle(document.body).getPropertyValue(name).trim(); }
  let palette = {};
  function readPalette() { ["--wave", "--wave2", "--sel", "--loop", "--wave-bg", "--accent", "--teal", "--violet"].forEach(function (k) { palette[k] = cssColor(k); }); waveDirty = true; }
  function drawWave() {
    const W = canvas.width, H = canvas.height, d = (window.devicePixelRatio || 1) * S;
    ctx.clearRect(0, 0, W, H);
    ctx.fillStyle = palette["--wave-bg"]; ctx.fillRect(0, 0, W, H);
    if (!sample.loaded) return;
    if (!cols) buildCols();
    const st = v("start"), en = v("end"), m = mode();
    ctx.fillStyle = palette["--sel"]; ctx.fillRect(px(st), 0, px(en) - px(st), H);
    if (v("regionLoop") > .5) { const a = Math.max(st, v("loopStart")), b = Math.min(en, v("loopEnd")); ctx.fillStyle = palette["--loop"]; ctx.fillRect(px(a), H * .08, px(b) - px(a), H * .84); }
    if (cols) {
      const lanes = cols.length, laneH = H / lanes;
      cols.forEach(function (c, li) {
        const mid = laneH * li + laneH / 2, amp = laneH * .44;
        for (let x = 0; x < W; x++) {
          const n = view[0] + (view[1] - view[0]) * x / W, inside = n >= st && n <= en;
          ctx.fillStyle = inside ? (li ? palette["--wave2"] : palette["--wave"]) : "rgba(255,255,255,.18)";
          const y0 = mid - c[x * 2 + 1] * amp, y1 = mid - c[x * 2] * amp;
          ctx.fillRect(x, y0, 1, Math.max(1, y1 - y0));
        }
        if (lanes > 1) { ctx.fillStyle = "rgba(255,255,255,.55)"; ctx.font = (9 * d) + "px Menlo, monospace"; ctx.fillText(li ? "R" : "L", 5 * d, laneH * li + 12 * d); }
      });
    }
    const handle = function (x, color, top, label) {
      ctx.fillStyle = color; ctx.fillRect(x - 1.5 * d, 0, 3 * d, H);
      ctx.beginPath(); const y = top ? 0 : H, s = top ? 1 : -1;
      ctx.moveTo(x - 7 * d, y); ctx.lineTo(x + 7 * d, y); ctx.lineTo(x, y + s * 10 * d); ctx.closePath(); ctx.fill();
      if (label) { ctx.font = "bold " + (9 * d) + "px Menlo, monospace"; ctx.fillText(label, x + 4 * d, top ? 20 * d : H - 6 * d); }
    };
    if (m === 4) for (let i = 1; i < Math.round(v("sliceCount")); i++) handle(px(cutAbs(i)), palette["--violet"], false, noteName(v("root") + i));
    if (v("regionLoop") > .5) { handle(px(v("loopStart")), palette["--teal"], false, "L"); handle(px(v("loopEnd")), palette["--teal"], false, "L"); }
    handle(px(st), palette["--accent"], true, "IN"); handle(px(en), palette["--accent"], true, "OUT");
    if (T.playing || T.preview) { ctx.fillStyle = "#fff"; ctx.fillRect(px(T.pos) - d, 0, 2 * d, H); }
  }
  function markerInfo() {
    const s = sample.seconds || 0, ms = function (x) { return Math.round(x * s * 1000) + " ms"; };
    let t = "IN " + ms(v("start")) + " · OUT " + ms(v("end"));
    if (v("regionLoop") > .5) t += " · LOOP " + ms(v("loopStart")) + "–" + ms(v("loopEnd"));
    $("#markerInfo").textContent = t;
  }
  function markers() {
    const list = [{ id: "start", x: v("start") }, { id: "end", x: v("end") }];
    if (v("regionLoop") > .5) list.push({ id: "loopStart", x: v("loopStart") }, { id: "loopEnd", x: v("loopEnd") });
    if (mode() === 4) for (let i = 1; i < Math.round(v("sliceCount")); i++) list.push({ id: "slice" + i, x: cutAbs(i), slice: i });
    return list;
  }
  waveBox.addEventListener("pointerdown", function (e) {
    if (!sample.loaded) { doImport(); return; }
    const r = canvas.getBoundingClientRect();
    let best = null, bd = 12;
    markers().forEach(function (mk) { const dx = Math.abs((mk.x - view[0]) / (view[1] - view[0]) * r.width - (e.clientX - r.left)); if (dx < bd) { bd = dx; best = mk; } });
    if (!best) return;
    dragMarker = best; busy[best.id] = true; waveBox.setPointerCapture(e.pointerId);
    R.call("setParam", best.id, null, 1);
    if (best.id.indexOf("loop") === 0 && v("loopRegionSet") < .5) gesture("loopRegionSet", 1);
  });
  waveBox.addEventListener("pointermove", function (e) {
    const r = canvas.getBoundingClientRect(), n = clamp(normAt(e.clientX), 0, 1);
    if (!dragMarker) {
      let near = false;
      if (sample.loaded) markers().forEach(function (mk) { if (Math.abs((mk.x - view[0]) / (view[1] - view[0]) * r.width - (e.clientX - r.left)) < 12) near = true; });
      waveBox.style.cursor = near ? "ew-resize" : sample.loaded ? "default" : "pointer";
      return;
    }
    const id = dragMarker.id;
    let x = n;
    if (id === "start") x = Math.min(x, v("end") - .001);
    else if (id === "end") x = Math.max(x, v("start") + .001);
    else if (id === "loopStart") x = Math.min(x, v("loopEnd") - .001);
    else if (id === "loopEnd") x = Math.max(x, v("loopStart") + .001);
    else if (dragMarker.slice) {
      const i = dragMarker.slice, span = v("end") - v("start");
      x = clamp((n - v("start")) / span, cut(i - 1) + .0001, cut(i + 1) - .0001);
    }
    setParam(id, x); waveDirty = true; markerInfo();
  });
  function endMarker() { if (!dragMarker) return; busy[dragMarker.id] = false; R.call("setParam", dragMarker.id, null, 2); dragMarker = null; }
  waveBox.addEventListener("pointerup", endMarker); waveBox.addEventListener("pointercancel", endMarker);
  function zoom(kind, at) {
    const span = view[1] - view[0], c = at === undefined ? (view[0] + view[1]) / 2 : at;
    if (kind === "in") { const s2 = Math.max(.002, span / 1.8); view = [c - (c - view[0]) / span * s2, 0]; view[1] = view[0] + s2; }
    if (kind === "out") { const s2 = Math.min(1, span * 1.8); view = [c - (c - view[0]) / span * s2, 0]; view[1] = view[0] + s2; }
    if (kind === "all") view = [0, 1];
    if (kind === "sel") { const pad = (v("end") - v("start")) * .06; view = [v("start") - pad, v("end") + pad]; }
    if (view[0] < 0) { view[1] -= view[0]; view[0] = 0; } if (view[1] > 1) { view[0] -= view[1] - 1; view[1] = 1; }
    view[0] = Math.max(0, view[0]); cols = null; waveDirty = true;
  }
  $$("[data-zoom]").forEach(function (b) { b.addEventListener("click", function () { zoom(b.dataset.zoom); }); });
  waveBox.addEventListener("wheel", function (e) {
    if (!sample.loaded) return; e.preventDefault();
    if (Math.abs(e.deltaX) > Math.abs(e.deltaY)) { const span = view[1] - view[0], dx = e.deltaX / 600 * span; view = [clamp(view[0] + dx, 0, 1 - span), 0]; view[1] = view[0] + span; cols = null; waveDirty = true; }
    else zoom(e.deltaY < 0 ? "in" : "out", normAt(e.clientX));
  }, { passive: false });
  waveBox.addEventListener("keydown", function (e) {
    if (e.key === "+" || e.key === "=") zoom("in"); else if (e.key === "-") zoom("out"); else if (e.key === "Home") zoom("all"); else if (e.key === "f" || e.key === "F") zoom("sel"); else return;
    e.preventDefault(); e.stopPropagation();
  });

  /* ---- import ---- */
  function importResult(r) {
    if (!r) return;
    if (r.ok) { toast("Sample carregado. Toque no teclado!"); game.award(20, "import"); }
    else toast(r.message || "Não foi possível importar.", 4200);
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
    reader.onload = function () { const b64 = String(reader.result).split(",")[1] || ""; R.call("importData", f.name, b64).then(importResult); };
    reader.readAsDataURL(f);
  });
  function onSample(info) {
    sample = info || sample;
    $("#sampleName").textContent = sample.loaded ? sample.name : "Arraste um áudio aqui";
    waveBox.classList.toggle("loaded", !!sample.loaded);
    peaks = null; cols = null; view = [0, 1]; waveDirty = true; markerInfo();
    if (sample.loaded) R.call("waveform", 4096).then(function (w) { peaks = w && w.l ? w : null; cols = null; waveDirty = true; });
  }

  /* ---- sample controls ---- */
  function setupSample() {
    const k = $("#sampleKnobs");
    knob(k, "root", "RAIZ", { format: fmt.note, title: "Nota em que o sample toca na altura original" });
    knob(k, "level", "SAÍDA", { format: fmt.pct });
    knob(k, "velocity", "VEL→FILTRO", { format: fmt.pct, title: "Quanto a força da nota abre o filtro" });
    const xf = knob(k, "loopCrossfade", "XFADE", { format: fmt.xms, color: "var(--teal)", title: "Crossfade na emenda do LOOP" });
    watch("regionLoop", function (x) { xf.classList.toggle("disabled", x < .5); });
    const refreshMode = seg("#modeSeg", "mode", mode, function (m) { R.call("playbackMode", m); P.playMode.v = m; P.loop.v = m === 1 ? 1 : 0; refreshMode(); slicesUi(); waveDirty = true; });
    watch("playMode", function () { refreshMode(); slicesUi(); waveDirty = true; }); watch("loop", refreshMode);
    bindToggle($("#loopBtn"), "regionLoop", function () { waveDirty = true; markerInfo(); });
    watch("regionLoop", function () { waveDirty = true; markerInfo(); });
    bindSeg("#pitchSeg", "pitch", "pitchMode");
    function slicesUi() { const n = Math.round(v("sliceCount")); $("#sliceCount").textContent = n + " cortes"; $("#sliceCtl").classList.toggle("off", mode() !== 4); keyboardSlices(); }
    watch("sliceCount", slicesUi);
    $$("[data-slices]").forEach(function (b) { b.addEventListener("click", function () { R.call("slices", Math.round(v("sliceCount")) + Number(b.dataset.slices)); }); });
    ["start", "end", "loopStart", "loopEnd"].forEach(function (id) { watch(id, function () { waveDirty = true; markerInfo(); }); });
    for (let i = 1; i < 16; i++) watch("slice" + i, function () { waveDirty = true; });
    let previewing = false;
    $("#previewBtn").addEventListener("click", function () { previewing = !previewing; R.call("preview", previewing); $("#previewBtn").textContent = previewing ? "■ Parar" : "▶ Ouvir"; });
    setInterval(function () { if (previewing && !T.preview && !T.playing) { previewing = false; $("#previewBtn").textContent = "▶ Ouvir"; } }, 400);
  }

  /* ================= envelope ================= */
  const envSvg = $("#envSvg");
  const EW = 270, EH = 70, seg1 = 82, seg2 = 74, hold = 36, seg4 = 70;
  function tPos(x, max, w) { const lo = .001; return 6 + (w - 6) * Math.log(Math.max(x, lo) / lo) / Math.log(max / lo); }
  function tFrom(px, max, w) { const lo = .001; return lo * Math.pow(max / lo, clamp((px - 6) / (w - 6), 0, 1)); }
  function drawEnv() {
    const a = tPos(v("ampAttack"), 4, seg1), d = tPos(v("ampDecay"), 4, seg2), s = v("ampSustain"), r = tPos(v("ampRelease"), 8, seg4);
    const y0 = EH - 6, top = 8, sy = y0 - (y0 - top) * s;
    const p1 = [4 + a, top], p2 = [p1[0] + d, sy], p3 = [p2[0] + hold, sy], p4 = [p3[0] + r, y0];
    const live = v("synthEnable") > .5 || Math.round(v("triggerMode")) === 2;
    envSvg.innerHTML =
      '<path d="M4 ' + y0 + ' L' + p1 + ' L' + p2 + ' L' + p3 + ' L' + p4 + ' Z" fill="' + (live ? "rgba(240,122,58,.28)" : "rgba(255,255,255,.08)") + '"/>' +
      '<path d="M4 ' + y0 + ' L' + p1 + ' L' + p2 + ' L' + p3 + ' L' + p4 + '" fill="none" stroke="' + (live ? "var(--wave)" : "rgba(255,255,255,.4)") + '" stroke-width="2.5" stroke-linejoin="round" vector-effect="non-scaling-stroke"/>' +
      [["A", p1], ["D", p2], ["R", p4]].map(function (q) { return '<circle data-h="' + q[0] + '" cx="' + q[1][0] + '" cy="' + q[1][1] + '" r="5" fill="#fff" stroke="#17101f" stroke-width="2" vector-effect="non-scaling-stroke"/>'; }).join("") +
      '<circle id="envDot" r="4" fill="var(--accent)" cx="-10" cy="-10"/>';
    $("#envNote").textContent = live ? "" : "ADSR inativo: ligue SYNTH ou use disparo Gate.";
    $$("#envKnobs .knob").forEach(function (k) { k.classList.toggle("disabled", !live); });
  }
  function setupEnv() {
    const k = $("#envKnobs");
    knob(k, "ampAttack", "ATTACK", { format: fmt.ms, curve: "log", size: 32 });
    knob(k, "ampDecay", "DECAY", { format: fmt.ms, curve: "log", size: 32 });
    knob(k, "ampSustain", "SUSTAIN", { format: fmt.pct, size: 32 });
    knob(k, "ampRelease", "RELEASE", { format: fmt.ms, curve: "log", size: 32 });
    ["ampAttack", "ampDecay", "ampSustain", "ampRelease", "synthEnable", "triggerMode"].forEach(function (id) { watch(id, drawEnv); });
    bindToggle($("#synthBtn"), "synthEnable");
    let h = null;
    envSvg.addEventListener("pointerdown", function (e) {
      const t = e.target.getAttribute && e.target.getAttribute("data-h"); if (!t) return;
      h = t; envSvg.setPointerCapture(e.pointerId);
      (h === "A" ? ["ampAttack"] : h === "D" ? ["ampDecay", "ampSustain"] : ["ampRelease"]).forEach(function (id) { busy[id] = false; R.call("setParam", id, null, 1); });
    });
    envSvg.addEventListener("pointermove", function (e) {
      if (!h) return;
      const r = envSvg.getBoundingClientRect(), x = (e.clientX - r.left) / r.width * EW, y = (e.clientY - r.top) / r.height * EH;
      const a = tPos(v("ampAttack"), 4, seg1);
      if (h === "A") setParam("ampAttack", tFrom(x - 4, 4, seg1));
      if (h === "D") { setParam("ampDecay", tFrom(x - 4 - a, 4, seg2)); setParam("ampSustain", clamp((EH - 6 - y) / (EH - 14), 0, 1)); }
      if (h === "R") { const d = tPos(v("ampDecay"), 4, seg2); setParam("ampRelease", tFrom(x - 4 - a - d - hold, 8, seg4)); }
      drawEnv();
    });
    envSvg.addEventListener("pointerup", function () { if (!h) return; (h === "A" ? ["ampAttack"] : h === "D" ? ["ampDecay", "ampSustain"] : ["ampRelease"]).forEach(function (id) { R.call("setParam", id, null, 2); }); h = null; });
  }

  /* ================= filter, voice, macros ================= */
  function setupSynth() {
    const f = $("#filterKnobs");
    knob(f, "filterCutoff", "CUTOFF", { format: fmt.hz, curve: "log", color: "var(--teal)", size: 34 });
    knob(f, "filterResonance", "RESSON.", { format: fmt.pct, color: "var(--teal)", size: 34 });
    knob(f, "envAmount", "ENV→", { format: fmt.bip, color: "var(--teal)", size: 34, title: "Quanto o envelope mexe no destino" });
    const l = $("#lfoKnobs");
    knob(l, "lfoRate", "LFO RATE", { format: fmt.lfo, curve: "log", color: "var(--violet)", size: 34 });
    knob(l, "lfoDepth", "LFO DEPTH", { format: fmt.pct, color: "var(--violet)", size: 34 });
    bindSeg("#lfoTargetSeg", "lfo", "lfoTarget");
    watch("synthEnable", function (x) { $$("#filterPanel .knob").forEach(function (k) { k.classList.toggle("disabled", x < .5); }); });
    bindSeg("#voiceSeg", "voice", "voiceMode");
    bindSeg("#trigSeg", "trig", "triggerMode");
    const vk = $("#voiceKnobs");
    const g = knob(vk, "glide", "GLIDE", { size: 30, format: function (x) { return Math.round(x * 1000) + " ms"; }, title: "Portamento em Mono/Legato" });
    watch("voiceMode", function (x) { g.classList.toggle("disabled", Math.round(x) === 0); });
    const m = $("#macroKnobs");
    knob(m, "presence", "PRESENÇA", { format: fmt.pct, size: 34 });
    knob(m, "trail", "RASTRO", { format: fmt.pct, size: 34, color: "var(--violet)" });
    knob(m, "mutation", "JAM %", { format: fmt.pct, size: 34, title: "Intensidade do JAM" });
    knob(m, "drywet", "ORIG/RACK", { format: fmt.pct, size: 34, title: "Mistura entre o som original e o rack" });
  }

  /* ================= PLAY pad ================= */
  function setupPad() {
    const pad = $("#pad"), dot = $("#padDot"), trail = $("#padTrail");
    let dragging = false;
    function place() { dot.style.left = (v("playX") * 100) + "%"; dot.style.top = ((1 - v("playY")) * 100) + "%"; $("#padInfo").textContent = "X " + Math.round(v("playX") * 100) + "% · Y " + Math.round(v("playY") * 100) + "%"; }
    function note() {
      const wob = v("wOn") > .5 && v("wPresent") > .5;
      const deg = ["c", "d", "k"].some(function (m) { return v(m + "On") > .5 && v(m + "Present") > .5; });
      const msg = [];
      if (!wob) msg.push("X sem efeito: WOBBLE desligado");
      if (!deg) msg.push("Y sem efeito: CRUMBLE, DUST e CRUSH desligados");
      $("#padNote").textContent = msg.length ? msg.join(" · ") : "X → WOBBLE · Y → CRUMBLE, DUST, CRUSH. Duplo clique centraliza.";
    }
    watch("playX", place); watch("playY", place);
    ["wOn", "wPresent", "cOn", "cPresent", "dOn", "dPresent", "kOn", "kPresent"].forEach(function (id) { watch(id, note); });
    function at(e) { const r = pad.getBoundingClientRect(); setParam("playX", clamp((e.clientX - r.left) / r.width, 0, 1)); setParam("playY", clamp(1 - (e.clientY - r.top) / r.height, 0, 1)); place(); }
    pad.addEventListener("pointerdown", function (e) { dragging = true; busy.playX = busy.playY = true; pad.setPointerCapture(e.pointerId); R.call("setParam", "playX", null, 1); R.call("setParam", "playY", null, 1); at(e); });
    pad.addEventListener("pointermove", function (e) {
      if (!dragging) return; at(e);
      const s = document.createElement("span"); s.style.left = dot.style.left; s.style.top = dot.style.top; trail.appendChild(s);
      s.animate ? s.animate([{ opacity: .9, transform: "scale(1)" }, { opacity: 0, transform: "scale(.2)" }], { duration: 600 }).onfinish = function () { s.remove(); } : setTimeout(function () { s.remove(); }, 600);
    });
    function end() { if (!dragging) return; dragging = false; busy.playX = busy.playY = false; R.call("setParam", "playX", null, 2); R.call("setParam", "playY", null, 2); }
    pad.addEventListener("pointerup", end); pad.addEventListener("pointercancel", end);
    pad.addEventListener("dblclick", function () { gesture("playX", .5); gesture("playY", .5); });
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
  const timeIds = [["gTime", "gFreeMs", "gDivision"], ["cRate", "cFreeHz", "cDivision"], ["wRate", "wFreeHz", "wDivision"]];
  const timingIds = ["gTiming", "cTiming", "wTiming"];
  const DIV = ["1/4", "1/8", "1/8.", "1/8T"];
  function timeParam(mi) { const slot = MOD[mi].slot; const t = Math.round(v(timingIds[slot])); return timeIds[slot][t]; }
  let openModule = -1, cards = [];
  function renderRack() {
    const rack = $("#rack"); rack.innerHTML = ""; cards = [];
    for (const k in views) views[k] = views[k].filter(function (f) { return !f.rackOwned; });
    order.forEach(function (mi, pos) {
      const M = MOD[mi];
      if (v(M.k + "Present") < .5) return;
      const card = document.createElement("div");
      card.className = "card" + (openModule === mi ? " open" : ""); card.style.setProperty("--mc", M.color); card.dataset.pos = pos; card.dataset.mod = mi;
      card.innerHTML = '<div class="card-main">' + ART.icon(mi) + '<div><div class="card-title">' + M.name + '</div><div class="card-desc">' + M.desc + '</div></div><div class="card-tools"><div class="tool-row" style="display:flex;gap:4px;align-items:center"><button class="power" title="Ativar/bypass" aria-label="Ativar ' + M.name + '"></button><button class="x" title="Remover do rack" aria-label="Remover ' + M.name + '">✕</button></div></div></div><div class="card-detail"><div class="knob-row"></div><div class="timing"></div></div><div class="card-meter"></div>';
      rack.appendChild(card);
      const tools = $(".card-tools", card), mainKnobs = document.createElement("div");
      mainKnobs.style.display = "flex"; tools.appendChild(mainKnobs);
      const firstId = M.ctl[0] === "T" ? timeParam(mi) : M.ctl[0];
      knob(mainKnobs, firstId, M.labels[0], { size: 28, color: "var(--line)", format: formatFor(firstId), owned: true });
      knob(mainKnobs, M.k + "Mix", "MIX", { size: 28, color: "var(--line)", owned: true });
      const detail = $(".card-detail .knob-row", card);
      M.ctl.slice(1).forEach(function (c, i) { const id = c === "T" ? timeParam(mi) : c; knob(detail, id, M.labels[i + 1], { size: 28, color: "var(--line)", format: formatFor(id), owned: true }); });
      if (M.slot !== undefined) {
        const tm = $(".timing", card), tid = timingIds[M.slot];
        [[1, "FREE"], [2, "SYNC"]].forEach(function (o) { const b = document.createElement("button"); b.textContent = o[1]; b.classList.toggle("on", Math.round(v(tid)) === o[0]); b.addEventListener("click", function () { gesture(tid, o[0]); renderRack(); }); tm.appendChild(b); });
        if (Math.round(v(tid)) === 0) { const b = document.createElement("button"); b.textContent = "LEGADO"; b.className = "on"; b.title = "Modo de tempo de sessões antigas"; tm.appendChild(b); }
      }
      const power = $(".power", card), onId = M.k + "On";
      const pw = function (x) { power.classList.toggle("on", x > .5); card.classList.toggle("off", x < .5); }; pw.rackOwned = true; watch(onId, pw);
      power.addEventListener("click", function (e) { e.stopPropagation(); gesture(onId, v(onId) > .5 ? 0 : 1); });
      $(".x", card).addEventListener("click", function (e) { e.stopPropagation(); gesture(M.k + "Present", 0); renderRack(); });
      dragCard(card, $(".card-main", card), pos, mi);
      cards.push({ el: card, mi: mi, art: $(".card-art", card), meter: $(".card-meter", card) });
    });
    $("#addModule").disabled = MOD.every(function (M) { return v(M.k + "Present") > .5; });
  }
  function formatFor(id) {
    if (/Division$/.test(id)) return function (x) { return DIV[Math.round(x)] || ""; };
    if (id === "gFreeMs") return fmt.xms;
    if (/FreeHz$/.test(id)) return fmt.lfo;
    return fmt.pct;
  }
  function dragCard(card, handle, pos, mi) {
    let startY = 0, moved = false, dragging = false;
    handle.addEventListener("pointerdown", function (e) {
      if (e.target.closest(".knob, button")) return;
      startY = e.clientY; moved = false; dragging = true; handle.setPointerCapture(e.pointerId);
    });
    handle.addEventListener("pointermove", function (e) {
      if (!dragging) return;
      const dy = (e.clientY - startY) / S;
      if (Math.abs(dy) > 4) { moved = true; card.classList.add("dragging"); }
      if (moved) card.style.transform = "translateY(" + dy + "px) scale(1.03) rotate(-1.2deg)";
    });
    handle.addEventListener("pointerup", function (e) {
      if (!dragging) return; dragging = false;
      if (!moved) { openModule = openModule === mi ? -1 : mi; renderRack(); return; }
      const visible = cards.map(function (c) { return c.el; });
      const mid = e.clientY;
      let target = visible.length - 1;
      for (let i = 0; i < visible.length; i++) { const r = visible[i].getBoundingClientRect(); if (mid < r.top + r.height / 2) { target = i; break; } }
      const toPos = Number(visible[target].dataset.pos);
      card.style.transform = ""; card.classList.remove("dragging");
      if (toPos !== pos) { R.call("moveModule", pos, toPos); const m = order.splice(pos, 1)[0]; order.splice(toPos, 0, m); renderRack(); game.award(3, "rack"); toast("Ordem do rack = ordem do som."); }
    });
  }
  $("#addModule").addEventListener("click", function (e) {
    const menu = $("#addMenu"); menu.innerHTML = "";
    MOD.forEach(function (M) { if (v(M.k + "Present") > .5) return; const b = document.createElement("button"); b.textContent = "+ " + M.name; b.addEventListener("click", function () { gesture(M.k + "Present", 1); menu.hidden = true; renderRack(); }); menu.appendChild(b); });
    menu.style.left = "1030px"; menu.style.top = "112px"; menu.hidden = false;
    e.stopPropagation();
  });
  document.addEventListener("click", function (e) { if (!e.target.closest("#addMenu")) $("#addMenu").hidden = true; });

  /* ================= keyboard ================= */
  let base = 48;
  const keyEls = {};
  const kb = $("#keyboard");
  function buildKeys() {
    kb.innerHTML = "";
    const whites = [], W = kb.clientWidth, n0 = base, n1 = base + 36;
    for (let n = n0; n < n1; n++) if ([1, 3, 6, 8, 10].indexOf(n % 12) < 0) whites.push(n);
    const ww = W / whites.length;
    whites.forEach(function (n, i) { const k = document.createElement("div"); k.className = "key w"; k.style.left = (i * ww) + "px"; k.style.width = ww + "px"; k.dataset.n = n; if (n % 12 === 0) k.innerHTML = "<span>" + noteName(n) + "</span>"; kb.appendChild(k); keyEls[n] = k; });
    for (let n = n0; n < n1; n++) if ([1, 3, 6, 8, 10].indexOf(n % 12) >= 0) {
      const i = whites.indexOf(n - 1), k = document.createElement("div"); k.className = "key b"; k.style.left = ((i + 1) * ww - ww * .3) + "px"; k.style.width = (ww * .6) + "px"; k.dataset.n = n; kb.appendChild(k); keyEls[n] = k;
    }
    $("#octLabel").textContent = noteName(base);
    keyboardSlices();
  }
  function keyboardSlices() {
    const sl = mode() === 4, r = Math.round(v("root")), n = Math.round(v("sliceCount"));
    for (const k in keyEls) keyEls[k].classList.toggle("slice", sl && k >= r && k < r + n);
  }
  const down = {};
  function play(n, vel, on) {
    if (on) { if (down[n]) return; down[n] = true; } else { if (!down[n]) return; delete down[n]; }
    R.call("note", n, vel, on);
  }
  let mouseNote = -1;
  function keyAt(e) { const el = document.elementFromPoint(e.clientX, e.clientY); return el && el.classList.contains("key") ? Number(el.dataset.n) : (el && el.parentNode && el.parentNode.classList && el.parentNode.classList.contains("key") ? Number(el.parentNode.dataset.n) : -1); }
  function velAt(e, n) { const el = keyEls[n]; if (!el) return .8; const r = el.getBoundingClientRect(); return clamp(.35 + (e.clientY - r.top) / r.height * .65, .2, 1); }
  kb.addEventListener("pointerdown", function (e) { kb.setPointerCapture(e.pointerId); const n = keyAt(e); if (n >= 0) { mouseNote = n; play(n, velAt(e, n), true); } });
  kb.addEventListener("pointermove", function (e) { if (mouseNote < 0) return; const n = keyAt(e); if (n >= 0 && n !== mouseNote) { play(mouseNote, 0, false); mouseNote = n; play(n, velAt(e, n), true); } });
  function kbUp() { if (mouseNote >= 0) play(mouseNote, 0, false); mouseNote = -1; }
  kb.addEventListener("pointerup", kbUp); kb.addEventListener("pointercancel", kbUp);
  $("#octDown").addEventListener("click", function () { base = Math.max(12, base - 12); buildKeys(); });
  $("#octUp").addEventListener("click", function () { base = Math.min(84, base + 12); buildKeys(); });
  const KEYMAP = { a: 0, w: 1, s: 2, e: 3, d: 4, f: 5, t: 6, g: 7, y: 8, h: 9, u: 10, j: 11, k: 12, o: 13, l: 14 };
  const keyNote = {};
  document.addEventListener("keydown", function (e) {
    if (e.metaKey || e.ctrlKey || e.altKey || e.repeat || (e.target && /INPUT|TEXTAREA/.test(e.target.tagName))) return;
    const k = e.key.toLowerCase();
    if (k === "z") { base = Math.max(12, base - 12); buildKeys(); return; }
    if (k === "x") { base = Math.min(84, base + 12); buildKeys(); return; }
    if (KEYMAP[k] === undefined) return;
    const n = base + 12 + KEYMAP[k]; keyNote[k] = n; play(n, .85, true);
  });
  document.addEventListener("keyup", function (e) { const k = e.key.toLowerCase(); if (keyNote[k] !== undefined) { play(keyNote[k], 0, false); delete keyNote[k]; } });
  window.addEventListener("blur", function () { Object.keys(down).forEach(function (n) { play(Number(n), 0, false); }); });

  /* ================= stage & character ================= */
  let charKind = "fuzz", rig = {}, anchors = null, charSvg = null;
  const motion = { bob: 0, bobV: 0, squash: 0, squashV: 0, lean: 0, hair: 0, hairV: 0, strike: 0, look: 0, blink: 0, nextBlink: 2, energy: 0, groove: 0, beatPhase: 0, shake: 0, hist: [] };
  function buildStage() {
    $("#stageArt").innerHTML = ART.scene();
    charSvg = $("#stageArt svg");
    buildCharacter();
  }
  function buildCharacter() {
    const C = ART.characters[charKind]();
    anchors = C.anchors;
    const layer = $("#charLayer", charSvg);
    layer.innerHTML = '<g id="charPos" transform="translate(240 412)"><g transform="scale(' + (charKind === "nova" ? .84 : 1) + ')">' + C.svg + '</g></g>';
    rig = {};
    ["c-root", "c-head", "c-hair", "c-eyes", "c-armL", "c-armR", "c-inst", "c-torso", "c-legL", "c-legR", "c-mouth", "c-mouthOpen", "c-hand"].forEach(function (id) { rig[id] = layer.querySelector("#" + id); });
    rig.pos = layer.querySelector("#charPos"); rig.pupils = $$(".c-pupil", layer);
    wearItems();
    // GHOST trail: real snapshots of past poses (cloned parts), shifted back in time.
    const gl = $("#ghosts", charSvg); gl.innerHTML = "";
    rig.ghosts = [0, 1, 2, 3].map(function () { const g = document.createElementNS("http://www.w3.org/2000/svg", "g"); g.setAttribute("class", "ghost"); g.setAttribute("opacity", "0"); gl.appendChild(g); return { el: g, pose: null }; });
    rig.ghostClock = 0;
  }
  function wearItems() {
    const worn = game.state.worn || [];
    $$(".acc", charSvg).forEach(function (n) { n.remove(); });
    worn.forEach(function (id) {
      const g = document.createElementNS("http://www.w3.org/2000/svg", "g");
      g.setAttribute("class", "acc"); g.innerHTML = ART.accessory(id, anchors);
      if (id === "halo") rig["c-head"].insertBefore(g, rig["c-head"].firstChild);
      else if (ART.behind[id]) rig["c-root"].insertBefore(g, rig["c-root"].firstChild);
      else if (id === "chain") rig["c-torso"].appendChild(g);
      else rig["c-head"].appendChild(g);
    });
  }
  let mouseX = 0, mouseY = 0;
  $("#stage").addEventListener("pointermove", function (e) { const r = e.currentTarget.getBoundingClientRect(); mouseX = (e.clientX - r.left) / r.width - .5; mouseY = (e.clientY - r.top) / r.height - .5; });
  $("#stage").addEventListener("pointerleave", function () { mouseX = mouseY = 0; });
  $("#stage").addEventListener("pointerdown", function () { motion.squashV -= .04; spawnNotes(240, 250, 2, .6); });

  function spawnNotes(x, y, count, vel) {
    if (animMode < 2) return;
    const g = $("#notesP", charSvg), glyphs = ["♪", "♫", "★", "✦"];
    for (let i = 0; i < count; i++) {
      const t = document.createElementNS("http://www.w3.org/2000/svg", "text");
      t.textContent = glyphs[(Math.random() * glyphs.length) | 0];
      t.setAttribute("x", x + (Math.random() - .5) * 40); t.setAttribute("y", y);
      t.setAttribute("font-size", 14 + vel * 16); t.setAttribute("fill", ["#ffd23f", "#ff7a3d", "#39e3d2", "#ff8ad1"][(Math.random() * 4) | 0]);
      t.setAttribute("stroke", "#17101f"); t.setAttribute("stroke-width", "1.5"); t.setAttribute("font-weight", "900");
      g.appendChild(t);
      const dx = (Math.random() - .5) * 140, dy = -80 - Math.random() * 90, rot = (Math.random() - .5) * 80;
      if (t.animate) t.animate([{ transform: "translate(0,0) rotate(0deg)", opacity: 1 }, { transform: "translate(" + dx + "px," + dy + "px) rotate(" + rot + "deg)", opacity: 0 }], { duration: 900 + Math.random() * 500, easing: "cubic-bezier(.2,.7,.3,1)" }).onfinish = function () { t.remove(); };
      else setTimeout(function () { t.remove(); }, 1200);
    }
  }
  const dust = [];
  function stepDust(amount, dt) {
    const g = $("#dustP", charSvg);
    if (animMode >= 1 && Math.random() < amount * 2.5 * dt * 60 && dust.length < 60) {
      const c = document.createElementNS("http://www.w3.org/2000/svg", "circle");
      c.setAttribute("r", (Math.random() * 1.8 + .6).toFixed(1)); c.setAttribute("fill", Math.random() < .5 ? "#fff6dc" : "#17101f"); g.appendChild(c);
      dust.push({ el: c, x: Math.random() * 480, y: Math.random() * 300 + 40, vx: (Math.random() - .5) * 12, vy: 8 + Math.random() * 14, life: 2 + Math.random() * 2 });
    }
    for (let i = dust.length - 1; i >= 0; i--) {
      const d = dust[i]; d.life -= dt; d.x += d.vx * dt; d.y += d.vy * dt;
      if (d.life <= 0) { d.el.remove(); dust.splice(i, 1); continue; }
      d.el.setAttribute("cx", d.x.toFixed(1)); d.el.setAttribute("cy", d.y.toFixed(1)); d.el.setAttribute("opacity", Math.min(1, d.life).toFixed(2));
    }
  }
  function crumbleGlitch(amount) {
    const layer = $("#fx-crumble");
    if (animMode < 2 || Math.random() > amount * .6) return;
    const s = document.createElement("span");
    s.style.left = (Math.random() * 90) + "%"; s.style.top = (Math.random() * 85) + "%"; s.style.width = (10 + Math.random() * 60) + "px"; s.style.height = (4 + Math.random() * 18) + "px";
    layer.appendChild(s); setTimeout(function () { s.remove(); }, 90 + Math.random() * 120);
  }

  /* ================= game layer ================= */
  const game = {
    state: { xp: 0, level: 1, worn: [], on: 1 },
    combo: 0, lastNote: 0, comboTimer: 0,
    load: function () { try { const s = JSON.parse(localStorage.getItem("rastro.game") || "null"); if (s) this.state = Object.assign(this.state, s); } catch (e) { /* storage unavailable: session only */ } },
    save: function () { try { localStorage.setItem("rastro.game", JSON.stringify(this.state)); } catch (e) { /* ignore */ } },
    need: function (lvl) { return 60 + lvl * 40; },
    award: function (xp, why) {
      if (!this.state.on) return;
      this.state.xp += xp;
      while (this.state.xp >= this.need(this.state.level)) {
        this.state.xp -= this.need(this.state.level); this.state.level++;
        const item = ART.items.filter(function (it) { return it.level === game.state.level; })[0];
        popup("NÍVEL " + this.state.level + "!", 240, 150, "#ffd23f");
        toast(item ? "Desbloqueou: " + item.name + " — abra a COLEÇÃO para vestir." : "Nível " + this.state.level + "!", 3600);
      }
      this.save(); this.hud();
      if (why === "import" || why === "rack") popup("+" + xp + " XP", 240, 300, "#39e3d2");
    },
    hud: function () {
      $("#lvlBadge").textContent = "NV " + this.state.level;
      $("#xpFill").style.width = Math.min(100, this.state.xp / this.need(this.state.level) * 100) + "%";
      $("#xpText").textContent = Math.floor(this.state.xp) + " / " + this.need(this.state.level) + " XP";
      const unlocked = ART.items.filter(function (it) { return it.level <= game.state.level; }).length;
      $("#collectionCount").textContent = unlocked + "/" + ART.items.length;
      document.body.classList.toggle("game-off", !this.state.on);
    },
    note: function (vel, now) {
      if (!this.state.on) return;
      const gap = now - this.lastNote; this.lastNote = now;
      this.combo = gap < 1400 ? this.combo + 1 : 1;
      let xp = 1, onBeat = false;
      if (T.hostPlaying && T.beat >= 0) { const ph = (T.beat * 2) % 1; onBeat = ph < .12 || ph > .88; }
      if (onBeat) { xp += 2; if (Math.random() < .5) popup("NO TEMPO!", 150 + Math.random() * 180, 210, "#39e3d2"); }
      if (this.combo > 0 && this.combo % 8 === 0) { const words = ["GROOVY!", "QUE RASTRO!", "ON FIRE!", "LENDÁRIO!"]; popup(words[Math.min(3, this.combo / 8 - 1 | 0)] + " ×" + this.combo, 240, 170, "#ff7a3d"); xp += 4; }
      this.award(xp);
      const c = $(".hud-combo"); $("#comboNum").textContent = this.combo; c.classList.add("bump"); setTimeout(function () { c.classList.remove("bump"); }, 90);
    },
    tick: function (now) { if (this.combo && now - this.lastNote > 2200) { this.combo = 0; $("#comboNum").textContent = "0"; } }
  };
  function popup(text, x, y, color) {
    if (!game.state.on) return;
    const p = document.createElement("div"); p.className = "pop"; p.textContent = text; p.style.left = (x - 60) + "px"; p.style.top = y + "px"; p.style.color = color || "#fff";
    $("#popups").appendChild(p); setTimeout(function () { p.remove(); }, 950);
  }
  function renderCollection() {
    const grid = $("#collectionGrid"); grid.innerHTML = "";
    const fake = { head: [0, 0], headW: 60, chest: [0, 0], back: [0, -10], top: [0, -6] };
    ART.items.forEach(function (it) {
      const ok = it.level <= game.state.level, worn = game.state.worn.indexOf(it.id) >= 0;
      const b = document.createElement("button"); b.className = "item" + (ok ? "" : " locked") + (worn ? " worn" : "");
      b.innerHTML = '<svg viewBox="-70 -60 140 110">' + ART.accessory(it.id, fake) + '</svg>' + (ok ? it.name : "Nível " + it.level);
      b.disabled = !ok;
      b.addEventListener("click", function () { const i = game.state.worn.indexOf(it.id); if (i >= 0) game.state.worn.splice(i, 1); else game.state.worn.push(it.id); game.save(); wearItems(); renderCollection(); });
      grid.appendChild(b);
    });
  }
  $("#collectionBtn").addEventListener("click", function () { renderCollection(); $("#collection").hidden = false; });
  $("#settingsBtn").addEventListener("click", function () { $("#settings").hidden = false; });
  $$(".sheet").forEach(function (s) { s.addEventListener("click", function (e) { if (e.target === s || e.target.hasAttribute("data-close")) s.hidden = true; }); });

  /* ================= header ================= */
  let animMode = 2;
  function setupHeader() {
    $("#jam").addEventListener("click", function () {
      R.call("jam").then(function (can) { $("#undoJam").disabled = !can; });
      const j = $("#jam"); j.classList.remove("flash"); void j.offsetWidth; j.classList.add("flash");
      motion.squashV -= .06; spawnNotes(240, 220, 6, .9); game.award(5); popup("JAM!", 240, 190, "#ff4fa3");
    });
    $("#undoJam").addEventListener("click", function () { R.call("undoJam").then(function (can) { $("#undoJam").disabled = !can; }); toast("Voltou à combinação anterior."); });
    $$("#themeSeg button").forEach(function (b) { b.addEventListener("click", function () { gesture("colourSet", b.dataset.theme === "night" ? 8 : 0); }); });
    watch("colourSet", function (x) {
      const night = [2, 5, 6, 8].indexOf(Math.round(x)) >= 0;
      document.body.className = document.body.className.replace(/theme-\w+/, "") + " theme-" + (night ? "night" : "sunset");
      $$("#themeSeg button").forEach(function (b) { b.classList.toggle("on", (b.dataset.theme === "night") === night); });
      readPalette();
    });
    $$("#charSeg button").forEach(function (b) { b.addEventListener("click", function () { gesture("avatarSpecies", b.dataset.char === "fuzz" ? 1 : 2); }); });
    watch("avatarSpecies", function (x) {
      const k = Math.round(x) === 1 ? "fuzz" : "nova";
      $$("#charSeg button").forEach(function (b) { b.classList.toggle("on", b.dataset.char === k); });
      if (k !== charKind || !rig.pos) { charKind = k; if (charSvg) buildCharacter(); }
    });
    bindSeg("#animSeg", "anim", "animation");
    watch("animation", function (x) { animMode = Math.round(x); document.body.classList.toggle("anim-off", animMode === 0); });
    const gameRefresh = seg("#gameSeg", "game", function () { return game.state.on ? 1 : 0; }, function (x) { game.state.on = x; game.save(); game.hud(); gameRefresh(); });
    gameRefresh();
  }

  /* ================= frame loop ================= */
  let lastSerial = -1, lastFrame = performance.now(), midiFlash = 0, beatClock = 0;
  function noteHeld(n) { return ((T.notes[n >> 5] >>> 0) & (1 << (n & 31))) !== 0; }
  function frame(now) {
    const dt = Math.min(.05, (now - lastFrame) / 1000); lastFrame = now;
    const amp = animMode === 2 ? 1 : animMode === 1 ? .5 : 0;
    // new notes
    if (T.serial !== lastSerial) {
      if (lastSerial >= 0) {
        const vel = T.vel || .7;
        motion.squashV -= .05 + vel * .09 * amp; motion.strike = 1; motion.shake = vel * 4 * amp; midiFlash = 1;
        spawnNotes(240 + (Math.random() - .5) * 60, 250, 1 + Math.round(vel * 3), vel);
        game.note(vel, now);
      }
      lastSerial = T.serial;
    }
    game.tick(now);
    $("#midiDot").classList.toggle("on", midiFlash > .1); midiFlash *= .85;
    $("#bpm").textContent = T.bpm > 0 ? Math.round(T.bpm) + " BPM · HOST" : Math.round(v("tempo")) + " BPM";
    // energy & beat
    motion.energy = lerp(motion.energy, clamp(T.meter * 1.6, 0, 1), .2);
    const bpm = T.bpm > 0 ? T.bpm : v("tempo");
    if (T.beat >= 0) beatClock = T.beat; else beatClock = (beatClock + dt * bpm / 60) % 4;
    const phase = beatClock % 1, onBeat = Math.pow(1 - phase, 4);
    const groove = motion.energy > .02 ? motion.energy : 0;
    // springs
    motion.squashV += (-motion.squash * 120 - motion.squashV * 9) * dt; motion.squash += motion.squashV;
    motion.squash = clamp(motion.squash, -.18, .18);
    motion.strike = Math.max(0, motion.strike - dt * 6);
    motion.shake *= .82;
    const dance = groove * amp, beatBob = Math.sin(beatClock * Math.PI * 2) * dance;
    const sway = Math.sin(beatClock * Math.PI) * dance;
    const breathe = Math.sin(now / 900) * .012;
    const wob = clamp(T.wob * 3, 0, 1), crush = clamp(T.crush * 3, 0, 1), bloom = clamp(T.bloom * 3, 0, 1), dst = clamp(T.dust * 3, 0, 1);
    const ghostAct = clamp((T.act[0] || 0) * 14, 0, 1), crumbleAct = clamp((T.act[1] || 0) * 14, 0, 1);
    // blink
    motion.nextBlink -= dt; if (motion.nextBlink <= 0) { motion.blink = 1; motion.nextBlink = 2 + Math.random() * 3; }
    motion.blink = Math.max(0, motion.blink - dt * 7);
    // look toward the played note
    let lookTarget = mouseX * .8;
    for (let n = 0; n < 128; n++) if (noteHeld(n)) { lookTarget = clamp((n - base - 18) / 18, -1, 1); break; }
    motion.look = lerp(motion.look, lookTarget, .12);
    if (rig.pos && animMode > 0) {
      let x = 240 + sway * 10 + Math.sin(now / 160) * wob * 6 * amp, y = 412 - Math.abs(beatBob) * 9;
      if (crush > .05) { const q = 2 + crush * 6; x = Math.round(x / q) * q; y = Math.round(y / q) * q; }
      const sq = motion.squash + breathe, sx = 1 - sq * .7, sy = 1 + sq;
      rig.pos.setAttribute("transform", "translate(" + x.toFixed(1) + " " + y.toFixed(1) + ") rotate(" + (sway * 3 + Math.sin(now / 210) * wob * 5 * amp).toFixed(2) + ") scale(" + sx.toFixed(3) + " " + sy.toFixed(3) + ")");
      const hy = anchors.head[1];
      rig["c-head"].setAttribute("transform", "rotate(" + (Math.sin(beatClock * Math.PI * 2 + .6) * 6 * dance + motion.look * 4).toFixed(2) + " 0 " + (hy + 40) + ")");
      motion.hairV += ((-sway * 14 - Math.abs(beatBob) * 6) - motion.hair) * 40 * dt; motion.hairV *= .86; motion.hair += motion.hairV * dt;
      rig["c-hair"].setAttribute("transform", "rotate(" + motion.hair.toFixed(2) + " 0 " + (hy - 20) + ")");
      const ey = hy + 4, bl = 1 - motion.blink * .9;
      rig["c-eyes"].setAttribute("transform", "translate(0 " + ey + ") scale(1 " + bl.toFixed(2) + ") translate(0 " + (-ey) + ")");
      rig.pupils.forEach(function (p) { p.setAttribute("transform", "translate(" + (motion.look * 4).toFixed(1) + " " + (mouseY * 3).toFixed(1) + ")"); });
      const open = clamp(T.env * 1.2, 0, 1) * (T.playing ? 1 : 0);
      rig["c-mouthOpen"].setAttribute("ry", (open * (charKind === "fuzz" ? 16 : 8)).toFixed(1));
      rig["c-mouth"].setAttribute("opacity", open > .15 ? 0 : 1);
      const handX = motion.look * 10, handY = motion.strike * 6;
      rig["c-armL"].setAttribute("transform", "translate(" + handX.toFixed(1) + " " + handY.toFixed(1) + ")");
      rig["c-armR"].setAttribute("transform", "rotate(" + (Math.sin(beatClock * Math.PI * 2) * 4 * dance) + " 40 " + (anchors.chest[1] - 20) + ")");
      rig["c-inst"].setAttribute("transform", rig["c-inst"].getAttribute("transform").replace(/rotate\([^)]*\)/, "rotate(" + ((charKind === "fuzz" ? -20 : -24) + motion.strike * 3 + sway * 2).toFixed(2) + ")"));
      const knee = Math.abs(beatBob) * 5;
      rig["c-legL"].setAttribute("transform", "rotate(" + (-knee).toFixed(2) + " -20 " + (charKind === "fuzz" ? -64 : -124) + ")");
      rig["c-legR"].setAttribute("transform", "rotate(" + knee.toFixed(2) + " 20 " + (charKind === "fuzz" ? -64 : -124) + ")");
      // GHOST: the character leaves a trail of echoes — the RASTRO
      motion.hist.unshift([x, y]); if (motion.hist.length > 40) motion.hist.pop();
      rig.ghostClock += dt;
      if (ghostAct > .04 && amp > 0 && rig.ghostClock > .09) {
        rig.ghostClock = 0;
        const last = rig.ghosts.pop(); rig.ghosts.unshift(last);
        const snap = rig.pos.cloneNode(true);
        snap.removeAttribute("id"); $$("[id]", snap).forEach(function (n) { n.removeAttribute("id"); });
        last.el.innerHTML = ""; last.el.appendChild(snap);
      }
      rig.ghosts.forEach(function (gh, i) {
        const op = ghostAct * amp * (.5 - i * .11);
        gh.el.setAttribute("opacity", Math.max(0, op).toFixed(2));
        gh.el.setAttribute("transform", "translate(" + (-(i + 1) * 16 * ghostAct).toFixed(1) + " " + (-(i + 1) * 3 * ghostAct).toFixed(1) + ")");
      });
    }
    // scene reactions
    if (charSvg && animMode > 0) {
      const art = $("#stageArt");
      art.style.transform = "translate(" + ((Math.random() - .5) * motion.shake).toFixed(1) + "px," + ((Math.random() - .5) * motion.shake).toFixed(1) + "px)";
      $("#pxSky", charSvg).setAttribute("transform", "translate(" + (-mouseX * 10).toFixed(1) + " " + (-mouseY * 6).toFixed(1) + ")");
      $("#pxCity", charSvg).setAttribute("transform", "translate(" + (-mouseX * 18).toFixed(1) + " " + (-mouseY * 4).toFixed(1) + ")");
      $("#fg", charSvg).setAttribute("transform", "translate(" + (mouseX * 16).toFixed(1) + " " + (mouseY * 6 + motion.energy * 3).toFixed(1) + ")");
      const cone = 1 + motion.energy * .35 * amp + onBeat * motion.energy * .2;
      ["coneL1", "coneL2", "coneR1", "coneR2"].forEach(function (id) { const c = charSvg.getElementById(id); if (c) c.setAttribute("transform", "scale(" + cone.toFixed(3) + ")"); });
      $("#spot", charSvg).setAttribute("opacity", (.25 + motion.energy * .9 + motion.strike * .3).toFixed(2));
      const bulbs = $$(".bulb", charSvg), lit = Math.floor(beatClock * 2) % 3;
      bulbs.forEach(function (b, i) { b.classList.toggle("lit", groove > .03 && i % 3 === lit); });
      const spin = (now / 1000) * (T.playing ? 220 : 0);
      ["reel1", "reel2"].forEach(function (id, i) { const r = charSvg.getElementById(id); r.setAttribute("transform", "translate(" + (i ? 80 : 44) + " 207) rotate(" + spin.toFixed(1) + ")"); });
      charSvg.getElementById("vu").setAttribute("width", (motion.energy * 64).toFixed(1));
      const lava = now / 1000 * (.4 + bloom * 2.4);
      ["blob1", "blob2", "blob3"].forEach(function (id, i) { const b = charSvg.getElementById(id); b.setAttribute("transform", "translate(" + (Math.sin(lava * (1 + i * .3) + i) * 5).toFixed(1) + " " + (Math.sin(lava * .7 + i * 2) * 14).toFixed(1) + ")"); });
      charSvg.getElementById("bloomVeil").setAttribute("style", "fill:" + (document.body.classList.contains("theme-night") ? "rgba(156,123,255," : "rgba(255,200,150,") + (bloom * .28 * amp).toFixed(3) + ")");
      const neon = charSvg.getElementById("neon"); neon.style.opacity = (.75 + bloom * .25 + onBeat * groove * .2).toFixed(2);
      $$(".win", charSvg).forEach(function (w, i) { if (Math.random() < .02 + motion.energy * .05) w.style.opacity = Math.random() < .3 + motion.energy ? "1" : "0"; });
      $$(".star", charSvg).forEach(function (s, i) { s.style.opacity = (.4 + .6 * Math.abs(Math.sin(now / 700 + i))).toFixed(2); });
      $$(".sway", charSvg).concat($$("#leaves .sway")).forEach(function (l, i) { l.style.transform = "rotate(" + (Math.sin(now / 1300 + i) * 2 + motion.energy * Math.sin(beatClock * Math.PI * 2) * 3).toFixed(2) + "deg)"; });
      $("#fx-pixels").style.opacity = (crush * .9 * amp).toFixed(2);
      if (crumbleAct > .05) crumbleGlitch(crumbleAct * amp);
      stepDust(dst * amp, dt);
    }
    // groove bars
    const bars = $$("#grooveBars i");
    bars.forEach(function (b, i) { const lvl = clamp(motion.energy * 1.25 - i / bars.length + .08 + onBeat * groove * .15, 0, 1); b.style.height = (12 + lvl * 88) + "%"; b.style.opacity = lvl > .05 ? 1 : .35; });
    // rack card art
    cards.forEach(function (c) {
      const a = clamp((T.act[c.mi] || 0) * 12, 0, 1) * (v(MOD[c.mi].k + "On") > .5 ? 1 : 0);
      c.meter.style.width = (a * 100) + "%";
      if (animMode === 0) return;
      const t = now / 1000, art = c.art;
      if (c.mi === 0) { const tr = art.querySelector(".ic-trail"); tr.style.transform = "translateX(" + (-a * 9 - Math.sin(t * 6) * a * 2).toFixed(1) + "px)"; tr.style.opacity = a; }
      if (c.mi === 1) art.querySelector(".crumbs").style.transform = "translate(" + (a * 4) + "px," + (Math.sin(t * 20) * a * 3).toFixed(1) + "px)";
      if (c.mi === 2) art.querySelector(".petals").style.transform = "scale(" + (.82 + a * .3).toFixed(2) + ") rotate(" + (t * 30 * a).toFixed(1) + "deg)";
      if (c.mi === 3) art.querySelector(".wave1").style.transform = "translateX(" + (Math.sin(t * 5) * 4 * (.3 + a)).toFixed(1) + "px)";
      if (c.mi === 4) { art.querySelector(".spin").style.transform = "rotate(" + (t * 160 * (.15 + a)).toFixed(1) + "deg)"; art.querySelector(".specks").style.opacity = .3 + a * .7; }
      if (c.mi === 5) art.querySelector(".px").style.transform = "translate(" + (Math.round(Math.sin(t * 13) * a * 3)) + "px,0)";
    });
    // keyboard highlights
    for (const n in keyEls) keyEls[n].classList.toggle("on", noteHeld(Number(n)));
    if (waveDirty || T.playing || T.preview) { drawWave(); waveDirty = false; }
    requestAnimationFrame(frame);
  }

  /* ================= boot ================= */
  function boot(init) {
    (init.params || []).forEach(function (p) { P[p.id] = p; });
    game.load();
    $("#grooveBars").innerHTML = new Array(18).join("<i></i>");
    $("#leaves").innerHTML = ART.cornerLeaves();
    buildStage();
    setupHeader(); setupSample(); setupEnv(); setupSynth(); setupPad();
    readPalette(); fit(); buildKeys(); renderRack(); game.hud(); onSample(init.sample);
    $("#undoJam").disabled = !init.canUndoJam;
    if (R.simulated) toast("Prévia no navegador com motor simulado — no plugin, tudo usa o áudio real.", 4200);
    requestAnimationFrame(frame);
  }
  R.on("tick", function (t) {
    const prevAct = T.act;
    T = Object.assign(T, t);
    if (t.order) { const changed = t.order.join() !== order.join(); order = t.order.slice(); if (changed) renderRack(); }
    $("#undoJam").disabled = !t.canUndoJam;
    void prevAct;
  });
  R.on("params", function (changes) {
    let rack = false;
    for (const id in changes) { apply(id, changes[id]); if (/Present$|Timing$/.test(id)) rack = true; }
    if (rack) renderRack();
  });
  R.on("sample", onSample);
  R.call("ready").then(boot);
})();
