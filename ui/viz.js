// RASTRO PLAY — sound visualiser (replaces the character on the main screens).
// Everything drawn here comes from real signals: the output scope, envelope, level, notes and the
// real activity of each effect module. No random animation that is not tied to the sound.
//   ring   = live output waveform wrapped around a circle (the "rastro" of the sound)
//   trails = one ring per note, expanding and fading; GHOST adds its echoes (violet)
//   BLOOM pink glow · WOBBLE cyan ring sway · CRUMBLE amber gaps · DUST gold particles · CRUSH lavender pixels
(function () {
  "use strict";
  const FXC = ["#a96bff", "#ffa02e", "#ff4fd8", "#2fe0ff", "#ffd23f", "#c9a6ff"];
  const NOTE = ["C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"];
  const clamp = function (v, a, b) { return v < a ? a : v > b ? b : v; };
  function rgb(hex) { const n = parseInt(hex.slice(1), 16); return [n >> 16 & 255, n >> 8 & 255, n & 255]; }
  function rgba(c, a) { return "rgba(" + c[0] + "," + c[1] + "," + c[2] + "," + a.toFixed(3) + ")"; }
  function hash(n) { n = (n ^ 61) ^ (n >>> 16); n = n + (n << 3); n = n ^ (n >>> 4); n = Math.imul(n, 0x27d4eb2d); return ((n ^ (n >>> 15)) >>> 0) / 4294967296; }

  function create(host) {
    const cv = document.createElement("canvas"); cv.className = "viz-canvas";
    host.appendChild(cv);
    const V = { cv: cv, t: 0, rings: [], dust: [], peak: .05, rot: 0, lastNote: "", noteAlpha: 0 };
    for (let i = 0; i < 140; i++) V.dust.push({ a: hash(i * 7 + 1) * 6.283, r: .3 + hash(i * 13 + 5) * .9, s: .2 + hash(i * 3 + 9) * .8, ph: hash(i * 17) * 6.283 });
    V.draw = function (s, dt) { draw(V, s, dt); };
    V.destroy = function () { cv.remove(); };
    return V;
  }
  function draw(V, s, dt) {
    const cv = V.cv, host = cv.parentNode; if (!host) return;
    const w = host.clientWidth, h = host.clientHeight; if (!w || !h) return;
    const dpr = Math.min(2, window.devicePixelRatio || 1);
    if (cv.width !== Math.round(w * dpr) || cv.height !== Math.round(h * dpr)) { cv.width = Math.round(w * dpr); cv.height = Math.round(h * dpr); }
    const g = cv.getContext("2d"); g.setTransform(dpr, 0, 0, dpr, 0, 0);
    V.t += dt;
    const fx = s.fx || [0, 0, 0, 0, 0, 0], react = s.react === undefined ? 1 : s.react;
    const F = s.fig || null; // the light producer (Estúdio); null = rings only
    const hb = h - (s.bottomPad || 0); // usable height (JAM keeps the strip and keyboard below)
    const wide = w / h > 2.2, FK = wide ? h / 430 : hb / 720, deskY = wide ? h * 1.02 : hb * .76, headY = deskY - 270 * FK;
    const cx = w / 2, cy = F ? headY + 40 * FK : h * .47, R0 = Math.min(w, h) * (F ? .28 : .25);
    // colour: mix of the active effect colours (base cyan-violet when none), warmed by a closed filter
    let tot = 0, mix = [0, 0, 0];
    fx.forEach(function (a, i) { if (a > .01) { const c = rgb(FXC[i]); tot += a; mix[0] += c[0] * a; mix[1] += c[1] * a; mix[2] += c[2] * a; } });
    let col = tot > .01 ? mix.map(function (v) { return Math.round(v / tot); }) : [140, 200, 255];
    const warm = clamp(1 - (s.cut === undefined ? 1 : s.cut), 0, 1) * .5;
    col = [Math.round(col[0] + (255 - col[0]) * warm), Math.round(col[1] * (1 - warm * .4)), Math.round(col[2] * (1 - warm * .7))];
    // ---- background ----
    const bright = .55 + .45 * (s.cut === undefined ? 1 : s.cut);
    const bg = g.createRadialGradient(cx, cy, 10, cx, cy, Math.max(w, h) * .75);
    bg.addColorStop(0, rgba([Math.round(38 * bright + col[0] * .06 * tot), Math.round(22 * bright + col[1] * .04 * tot), Math.round(62 * bright + col[2] * .06 * tot)], 1));
    bg.addColorStop(1, "#08040f");
    g.globalCompositeOperation = "source-over"; g.fillStyle = bg; g.fillRect(0, 0, w, h);
    // faint polar grid
    g.strokeStyle = "rgba(255,255,255,.045)"; g.lineWidth = 1;
    for (let k = 1; k <= 4; k++) { g.beginPath(); g.arc(cx, cy, R0 * k * .55, 0, 6.283); g.stroke(); }
    for (let k = 0; k < 12; k++) { const a = k / 12 * 6.283; g.beginPath(); g.moveTo(cx + Math.cos(a) * R0 * .3, cy + Math.sin(a) * R0 * .3); g.lineTo(cx + Math.cos(a) * R0 * 2.3, cy + Math.sin(a) * R0 * 2.3); g.stroke(); }
    g.globalCompositeOperation = "lighter";
    // ---- BLOOM: soft pink glow, size and strength from the real BLOOM activity ----
    if (fx[2] > .01) { const rr = R0 * (1.3 + fx[2] * 1.2); const gg = g.createRadialGradient(cx, cy, R0 * .3, cx, cy, rr); gg.addColorStop(0, rgba(rgb(FXC[2]), .38 * fx[2])); gg.addColorStop(1, "rgba(0,0,0,0)"); g.fillStyle = gg; g.beginPath(); g.arc(cx, cy, rr, 0, 6.283); g.fill(); }
    // envelope: inner glow follows the real amp envelope
    const env = clamp(s.env || 0, 0, 1);
    if (env > .01) { const gg = g.createRadialGradient(cx, cy, 0, cx, cy, R0 * .95); gg.addColorStop(0, rgba(col, .32 * env * react)); gg.addColorStop(1, "rgba(0,0,0,0)"); g.fillStyle = gg; g.beginPath(); g.arc(cx, cy, R0 * .95, 0, 6.283); g.fill(); }
    // ---- note trails (and GHOST echoes) ----
    if (s.hit) {
      const v = clamp(s.vel || .7, .2, 1), hue = s.noteCol || col;
      V.rings.push({ r: R0, a: v, c: hue, w: 2 + v * 4, delay: 0 });
      if (fx[0] > .02) for (let k = 1; k <= 4; k++) V.rings.push({ r: R0, a: v * Math.pow(.45 + fx[0] * .4, k), c: rgb(FXC[0]), w: 2.5, delay: k * (s.echo || .3) });
      V.noteAlpha = 1;
      if (V.rings.length > 60) V.rings.splice(0, V.rings.length - 60);
    }
    for (let i = V.rings.length - 1; i >= 0; i--) {
      const q = V.rings[i];
      if (q.delay > 0) { q.delay -= dt; continue; }
      q.r += dt * R0 * (1.1 + q.a * .6); q.a *= Math.pow(.25, dt);
      if (q.a < .02 || q.r > Math.max(w, h)) { V.rings.splice(i, 1); continue; }
      g.strokeStyle = rgba(q.c, q.a * .8 * react); g.lineWidth = q.w; g.beginPath(); g.arc(cx, cy, q.r, 0, 6.283); g.stroke();
    }
    // ---- the live ring: output waveform around the circle ----
    const sc = s.scope || [];
    let pk = 0; for (let i = 0; i < sc.length; i++) pk = Math.max(pk, Math.abs(sc[i]));
    V.peak = Math.max(.05, Math.max(pk, V.peak * Math.exp(-dt / 3)));
    const gain = .55 / V.peak;
    V.rot += dt * (.08 + (s.lfo || 0) * .9 * (s.lfoDepth || 0));
    const wob = fx[3], crumble = fx[1], crush = fx[5], N = 256;
    const pts = [];
    for (let i = 0; i < N; i++) {
      const a = i / N * 6.283 + V.rot;
      let x = sc.length ? sc[Math.floor(i / N * sc.length)] : 0;
      let r = R0 * (1 + clamp(x * gain, -1, 1) * .42 * (.4 + .6 * react));
      if (wob > .01) r += Math.sin(a * 3 + V.t * (2 + (s.wobRate || 1) * 3)) * R0 * .09 * wob;
      let px = cx + Math.cos(a) * r, py = cy + Math.sin(a) * r;
      if (crush > .02) { const q = 2 + crush * 14; px = Math.round(px / q) * q; py = Math.round(py / q) * q; }
      pts.push([px, py]);
    }
    const bucket = Math.floor(V.t * 10);
    const gap = function (i) { return crumble > .02 && hash(Math.floor(i / 8) * 31 + bucket * 977) < crumble * .55; };
    [[10, .16], [4, .5], [1.6, 1]].forEach(function (L) {
      g.strokeStyle = rgba(L[1] === 1 ? [255, 255, 255] : col, (L[1] === 1 ? .85 : L[1]) * (.35 + .65 * Math.min(1, (s.meter || 0) * 6 + .15)) * (F ? .45 : 1));
      g.lineWidth = L[0]; g.lineJoin = "round"; g.beginPath();
      let pen = false;
      for (let i = 0; i <= N; i++) { const p = pts[i % N]; if (gap(i % N)) { pen = false; continue; } if (!pen) { g.moveTo(p[0], p[1]); pen = true; } else g.lineTo(p[0], p[1]); }
      g.stroke();
    });
    // CRUMBLE: amber fragments flying off the ring where it breaks
    if (crumble > .02) for (let i = 0; i < N; i += 8) if (gap(i)) { const p = pts[i], a = i / N * 6.283 + V.rot, d = ((V.t * 10) % 1) * R0 * .25; g.fillStyle = rgba(rgb(FXC[1]), .8 * crumble); g.fillRect(p[0] + Math.cos(a) * d - 3, p[1] + Math.sin(a) * d - 3, 6, 6); }
    // ---- DUST: gold particles, amount from the real DUST activity ----
    if (fx[4] > .02) {
      const n = Math.round(V.dust.length * fx[4]), gc = rgb(FXC[4]);
      for (let i = 0; i < n; i++) { const d = V.dust[i]; d.a += dt * d.s * .15; const rr = R0 * (.4 + d.r * 1.6), x = cx + Math.cos(d.a) * rr, y = cy + Math.sin(d.a) * rr * .8 - ((V.t * 12 * d.s + d.ph * 20) % 40); g.fillStyle = rgba(gc, (.3 + .5 * Math.abs(Math.sin(V.t * 2 + d.ph))) * fx[4]); g.fillRect(x, y, 2, 2); }
    }
    if (F) drawFigure(g, V, F, s, { x: cx, y: deskY, k: FK, col: col, fx: fx, react: react, gain: gain, scope: sc, dt: dt });
    g.globalCompositeOperation = "source-over";
    // what you are playing: chord name while 2+ notes are held (polyphonic), else the last note, fading
    if (s.chord) { V.lastNote = s.chord; V.noteAlpha = 1; } else if (s.hit) V.lastNote = s.noteName || "";
    if (s.chord) V.noteAlpha = 1; else V.noteAlpha = Math.max(0, V.noteAlpha - dt * .7);
    if (V.lastNote && V.noteAlpha > .02) { g.fillStyle = "rgba(255,255,255," + (V.noteAlpha * .8).toFixed(2) + ")"; g.font = "700 " + Math.round(R0 * (F ? .16 : V.lastNote.length > 5 ? .22 : .32)) + "px Poppins, sans-serif"; g.textAlign = "center"; g.textBaseline = "middle"; g.fillText(V.lastNote, cx, F ? h * .08 : cy); }
    // effect legend (only the ones that are really active)
    let lx = 14; g.font = "700 11px Poppins, sans-serif"; g.textAlign = "left"; g.textBaseline = "middle";
    ["GHOST", "CRUMBLE", "BLOOM", "WOBBLE", "DUST", "CRUSH"].forEach(function (nm, i) {
      if (fx[i] < .02) return; const c = rgb(FXC[i]); g.fillStyle = rgba(c, .25 + .75 * Math.min(1, fx[i] * 2)); g.beginPath(); g.arc(lx + 5, hb - 18, 5, 0, 6.283); g.fill(); g.fillStyle = "rgba(255,255,255,.8)"; g.fillText(nm, lx + 14, hb - 18); lx += g.measureText(nm).width + 30;
    });
  }
  // ======================= THE LIGHT PRODUCER =======================
  // A producer behind a desk, drawn only with lines of light. The outline is the real output waveform
  // running along the body; the same effect reactions as the rings. Local units: desk top at y = 0,
  // head centre at (0, -270). Options (Estúdio): hair, hat, phones, glasses, desk, style, colour.
  const FIG_DEFAULT = { hair: "puff", hat: "none", phones: "studio", glasses: "none", desk: "pads", style: "neon", colour: "fx" };
  const FIG_ITEMS = {
    hair: [["puff", "Black power", 1], ["crest", "Crista", 1], ["buns", "Coquinhos", 2], ["locks", "Dreads", 3], ["short", "Raspado", 1], ["long", "Longo", 2]],
    hat: [["none", "Nenhum", 1], ["cap", "Boné", 1], ["beanie", "Gorro", 2], ["fedora", "Fedora", 3], ["crown", "Coroa de luz", 5]],
    phones: [["none", "Nenhum", 1], ["studio", "Studio", 1], ["retro", "Retrô", 2], ["neck", "No pescoço", 3]],
    glasses: [["none", "Nenhum", 1], ["round", "Redondo", 1], ["visor", "Visor", 2], ["star", "Estrela", 4]],
    desk: [["pads", "Pad controller", 1], ["keys", "Teclado", 1], ["decks", "Toca-discos", 2], ["laptop", "Laptop", 1], ["synth", "Synth modular", 4]],
    style: [["neon", "Neon", 1], ["ribbon", "Fita dupla", 2], ["dots", "Pontilhado", 2], ["particles", "Partículas", 3], ["vector", "Vetor fino", 1]],
    colour: [["fx", "Segue os efeitos", 1], ["palette", "Paleta da interface", 1], ["gold", "Dourado", 2], ["ice", "Gelo", 2], ["rainbow", "Arco-íris por nota", 4]]
  };
  function ell(cx, cy, rx, ry, a0, a1, n) { const p = []; for (let i = 0; i <= n; i++) { const a = a0 + (a1 - a0) * i / n; p.push([cx + Math.cos(a) * rx, cy + Math.sin(a) * ry]); } return p; }
  function rot(p, o, a) { const c = Math.cos(a), s = Math.sin(a); return p.map(function (q) { const x = q[0] - o[0], y = q[1] - o[1]; return [o[0] + x * c - y * s, o[1] + x * s + y * c]; }); }
  function resample(pts, step) {
    const out = [pts[0]];
    for (let i = 1; i < pts.length; i++) {
      const a = pts[i - 1], b = pts[i], d = Math.hypot(b[0] - a[0], b[1] - a[1]), n = Math.max(1, Math.round(d / step));
      for (let k = 1; k <= n; k++) out.push([a[0] + (b[0] - a[0]) * k / n, a[1] + (b[1] - a[1]) * k / n]);
    }
    return out;
  }
  function arm(sh, t, l1, l2, out) {
    const dx = t[0] - sh[0], dy = t[1] - sh[1], d = Math.min(l1 + l2 - 1, Math.hypot(dx, dy)), a = Math.atan2(dy, dx);
    const c = clamp((l1 * l1 + d * d - l2 * l2) / (2 * l1 * d), -1, 1), b = Math.acos(c) * out;
    const el = [sh[0] + Math.cos(a + b) * l1, sh[1] + Math.sin(a + b) * l1];
    return [sh, el, [sh[0] + Math.cos(a) * d, sh[1] + Math.sin(a) * d]];
  }
  // build every line of the figure for this frame (local units)
  function figPaths(F, P) {
    const L = [], add = function (pts, kind, closed) { L.push({ p: pts, kind: kind || "body", closed: !!closed }); };
    const hx = P.sway, hy = -270 + P.bob, tilt = P.tilt, head = [hx, hy];
    const H = function (pts) { return rot(pts, [hx, hy + 60], tilt); };
    // --- desk / instrument (drawn first, behind the hands) ---
    const D = F.desk;
    if (D === "keys") {
      add([[-230, 0], [230, 0], [250, 40], [-250, 40], [-230, 0]], "desk", true);
      for (let i = -9; i <= 9; i++) add([[i * 24, 4], [i * 26, 38]], "detail");
    } else if (D === "decks") {
      add([[-250, 0], [250, 0], [262, 46], [-262, 46], [-250, 0]], "desk", true);
      [-130, 130].forEach(function (x) { add(ell(x, 20, 80, 18, 0, 6.283, 40), "desk", true); add(ell(x, 20, 8, 3, 0, 6.283, 10), "detail", true); const a = P.spin * (x < 0 ? 1 : -1); add([[x, 20], [x + Math.cos(a) * 70, 20 + Math.sin(a) * 16]], "detail"); });
    } else if (D === "laptop") {
      add([[-170, 0], [170, 0], [190, 26], [-190, 26], [-170, 0]], "desk", true);
      add([[-150, 0], [-130, -150], [130, -150], [150, 0]], "desk");
    } else if (D === "synth") {
      add([[-250, -40], [250, -40], [262, 46], [-262, 46], [-250, -40]], "desk", true);
      for (let i = 0; i < 10; i++) add(ell(-200 + i * 44, -16, 10, 10, 0, 6.283, 14), "detail", true);
      for (let i = 0; i < 6; i++) add([[-210 + i * 80, 10], [-170 + i * 80 + Math.sin(P.t * 2 + i) * 20, 30]], "detail");
    } else {
      add([[-220, 0], [220, 0], [240, 52], [-240, 52], [-220, 0]], "desk", true);
      for (let r2 = 0; r2 < 2; r2++) for (let c = 0; c < 8; c++) { const x0 = -150 + c * 38, y0 = 8 + r2 * 20; add([[x0, y0], [x0 + 30, y0], [x0 + 32, y0 + 15], [x0 - 2, y0 + 15], [x0, y0]], "pad" + (r2 * 8 + c), true); }
    }
    // --- torso (hoodie) ---
    const sh = [[-92 + hx * .6, -178 + P.bob * .5], [92 + hx * .6, -178 + P.bob * .5]];
    add([[-62, 0], [-80, -110], [sh[0][0], sh[0][1] + 20], [sh[0][0] + 10, sh[0][1]], [hx * .6 - 34, -196 + P.bob * .6], [hx * .6 + 34, -196 + P.bob * .6], [sh[1][0] - 10, sh[1][1]], [sh[1][0], sh[1][1] + 20], [80, -110], [62, 0]], "body");
    add([[hx * .6 - 20, -192 + P.bob * .6], [hx * .5 - 10, -150], [hx * .5 - 10, -120]], "detail");
    add([[hx * .6 + 20, -192 + P.bob * .6], [hx * .5 + 10, -150], [hx * .5 + 10, -120]], "detail");
    // --- head ---
    add(H(ell(hx, hy, 58, 66, 0, 6.283, 48)), "body", true);
    add(H([[hx - 22, hy + 62], [hx - 20, hy + 84]]), "body"); add(H([[hx + 22, hy + 62], [hx + 20, hy + 84]]), "body");
    // face: eyes close on beats (blink) and the smile opens on accents
    const ey = hy - 4, ex = 22, blink = P.blink;
    if (F.glasses === "none") { add(H([[hx - ex - 10, ey + blink * 2], [hx - ex, ey - 4 * (1 - blink)], [hx - ex + 10, ey + blink * 2]]), "face"); add(H([[hx + ex - 10, ey + blink * 2], [hx + ex, ey - 4 * (1 - blink)], [hx + ex + 10, ey + blink * 2]]), "face"); }
    add(H(ell(hx, hy + 24, 18, 6 + P.open * 10, .2, Math.PI - .2, 16)), "face");
    // hair
    const hair = F.hair;
    if (hair === "puff") add(H(ell(hx, hy - 30, 86, 70, Math.PI * .92, Math.PI * 2.08, 60).map(function (q, i) { const b = Math.sin(i * 1.6) * 7; const a = Math.atan2(q[1] - (hy - 30), q[0] - hx); return [q[0] + Math.cos(a) * b, q[1] + Math.sin(a) * b]; })), "hair");
    else if (hair === "crest") { const sp = []; for (let i = 0; i <= 8; i++) { const a = Math.PI * (1.1 + i * .1), r = i % 2 ? 70 : 104 + P.hairFly * 10; sp.push([hx + Math.cos(a) * r, hy - 10 + Math.sin(a) * r]); } add(H(sp), "hair"); }
    else if (hair === "buns") { add(H(ell(hx - 52, hy - 62, 26, 26, 0, 6.283, 24)), "hair", true); add(H(ell(hx + 52, hy - 62, 26, 26, 0, 6.283, 24)), "hair", true); add(H(ell(hx, hy - 6, 62, 68, Math.PI * 1.05, Math.PI * 1.95, 30)), "hair"); }
    else if (hair === "locks") { add(H(ell(hx, hy - 6, 64, 70, Math.PI * .95, Math.PI * 2.05, 36)), "hair"); for (let i = 0; i < 7; i++) { const x = hx - 66 + i * 22; add(H([[x, hy - 20 - Math.abs(i - 3) * 6], [x - 2 + P.hairFly * 6, hy + 40 + (i % 2) * 14]]), "hair"); } }
    else if (hair === "long") { add(H([[hx - 62, hy + 70 + P.hairFly * 4], [hx - 66, hy - 20], [hx - 40, hy - 64], [hx, hy - 74], [hx + 40, hy - 64], [hx + 66, hy - 20], [hx + 62, hy + 70 + P.hairFly * 4]]), "hair"); }
    else add(H(ell(hx, hy - 4, 60, 68, Math.PI * 1.08, Math.PI * 1.92, 26)), "hair");
    // hats (one at a time)
    if (F.hat === "cap") { add(H([[hx - 60, hy - 30], [hx - 54, hy - 66], [hx, hy - 80], [hx + 54, hy - 66], [hx + 60, hy - 30], [hx + 110, hy - 26]]), "hat"); }
    else if (F.hat === "beanie") { add(H([[hx - 62, hy - 26], [hx - 58, hy - 70], [hx, hy - 92], [hx + 58, hy - 70], [hx + 62, hy - 26], [hx - 62, hy - 26]]), "hat", true); add(H([[hx - 62, hy - 40], [hx + 62, hy - 40]]), "hat"); }
    else if (F.hat === "fedora") { add(H([[hx - 100, hy - 44], [hx + 100, hy - 44]]), "hat"); add(H([[hx - 52, hy - 46], [hx - 46, hy - 104], [hx, hy - 92], [hx + 46, hy - 104], [hx + 52, hy - 46]]), "hat"); }
    else if (F.hat === "crown") { add(H([[hx - 50, hy - 56], [hx - 50, hy - 100], [hx - 26, hy - 78], [hx, hy - 112], [hx + 26, hy - 78], [hx + 50, hy - 100], [hx + 50, hy - 56], [hx - 50, hy - 56]]), "hat", true); }
    // headphones
    if (F.phones === "studio" || F.phones === "retro") {
      add(H(ell(hx, hy - 6, 72, 78, Math.PI * 1.02, Math.PI * 1.98, 30)), "phones");
      const cw = F.phones === "retro" ? 16 : 22;
      [-1, 1].forEach(function (sd) { add(H(ell(hx + sd * 70, hy + 6, cw, 30, 0, 6.283, 24)), "phones", true); if (F.phones === "retro") add(H(ell(hx + sd * 70, hy + 6, 6, 6, 0, 6.283, 10)), "phonesL", true); });
    } else if (F.phones === "neck") { add(ell(hx * .6, -196, 54, 18, 0, Math.PI, 20), "phones"); [-1, 1].forEach(function (sd) { add(ell(hx * .6 + sd * 52, -196, 14, 20, 0, 6.283, 16), "phones", true); }); }
    // glasses
    if (F.glasses === "round") { add(H(ell(hx - ex, ey, 16, 16, 0, 6.283, 20)), "glasses", true); add(H(ell(hx + ex, ey, 16, 16, 0, 6.283, 20)), "glasses", true); add(H([[hx - 6, ey], [hx + 6, ey]]), "glasses"); }
    else if (F.glasses === "visor") add(H([[hx - 52, ey - 12], [hx + 52, ey - 12], [hx + 46, ey + 12], [hx - 46, ey + 12], [hx - 52, ey - 12]]), "glasses", true);
    else if (F.glasses === "star") [-1, 1].forEach(function (sd) { const st = []; for (let i = 0; i <= 10; i++) { const a = -Math.PI / 2 + i * Math.PI / 5, r = i % 2 ? 8 : 19; st.push([hx + sd * ex + Math.cos(a) * r, ey + Math.sin(a) * r]); } add(H(st), "glasses", true); });
    // --- arms to the instrument (two hands, always on the desk) ---
    const tL = [P.handL[0], P.handL[1]], tR = [P.handR[0], P.handR[1]];
    const aL = arm(sh[0], tL, 100, 96, -1), aR = arm(sh[1], tR, 100, 96, 1);
    add(aL, "body"); add(aR, "body");
    [aL[2], aR[2]].forEach(function (hd, i) { add(ell(hd[0], hd[1] - 4, 16, 12, 0, 6.283, 16), "hand" + i, true); });
    return L;
  }
  function drawFigure(g, V, F, s, o) {
    const st = V.fig || (V.fig = { bob: 0, sway: 0, tilt: 0, open: 0, blink: 0, blinkAt: 2, spin: 0, press: [0, 0], lastPad: 0, hairFly: 0, hitL: 0 });
    const dt = o.dt, alive = clamp((s.meter || 0) * 8, 0, 1), beat = s.beat || 0;
    // motion: groove from the real beat, accents from the real notes
    const ph = beat - Math.floor(beat), bounce = Math.pow(.5 + .5 * Math.cos(ph * 6.283), 3);
    const ease = function (cur, tgt, r) { return cur + (tgt - cur) * (1 - Math.exp(-r * dt)); };
    st.bob = ease(st.bob, (bounce * 14 - 4) * alive + Math.sin(V.t * 1.6) * 2 * (1 - alive), 12);
    st.sway = ease(st.sway, Math.sin(beat * Math.PI) * 10 * alive, 6);
    st.tilt = ease(st.tilt, Math.sin(beat * Math.PI) * .07 * alive, 6);
    st.hairFly = ease(st.hairFly, Math.sin(beat * Math.PI) * alive, 5);
    st.spin += dt * (1.8 + alive * 2);
    if (s.hit) { st.open = 1; const pad = s.pad || 0; st.lastPad = pad; st.hitL = (pad % 8) < 4 ? 1 : 0; st.press[st.hitL ? 0 : 1] = 1; }
    st.open = Math.max(0, st.open - dt * 3); st.press[0] = Math.max(0, st.press[0] - dt * 6); st.press[1] = Math.max(0, st.press[1] - dt * 6);
    if (V.t > st.blinkAt) { st.blink = 1; st.blinkAt = V.t + 2.5 + hash(Math.floor(V.t * 10)) * 3; }
    st.blink = Math.max(0, st.blink - dt * 6);
    // hands: on the pad / key that matches the note (left hand low notes, right hand high notes)
    const pitch = clamp(s.pitch === undefined ? .5 : s.pitch, 0, 1);
    const reach = F.desk === "decks" ? [[-130, 14], [130, 14]] : F.desk === "laptop" ? [[-60 + pitch * 40, 6], [40 + pitch * 40, 6]] : [[-150 + pitch * 120, 14], [30 + pitch * 120, 14]];
    const P = { bob: st.bob, sway: st.sway, tilt: st.tilt, blink: st.blink, open: st.open, spin: st.spin, hairFly: st.hairFly, t: V.t,
      handL: [reach[0][0], reach[0][1] + st.press[0] * 8 - 6], handR: [reach[1][0], reach[1][1] + st.press[1] * 8 - 6] };
    const paths = figPaths(F, P);
    // colour of the light
    let col = o.col;
    if (F.colour === "palette") { const cs = getComputedStyle(document.body).getPropertyValue("--ui-accent").trim(); if (/^#[0-9a-f]{6}$/i.test(cs)) col = rgb(cs); }
    else if (F.colour === "gold") col = [255, 196, 90]; else if (F.colour === "ice") col = [150, 230, 255];
    else if (F.colour === "rainbow") col = s.noteCol || col;
    const k = o.k, X = o.x, Y = o.y, fx = o.fx, sc = o.scope || [], N = sc.length || 1;
    const wob = fx[3], crumble = fx[1], crush = fx[5];
    // project and displace by the live waveform along each line's normal
    let idx = 0;
    const lines = paths.map(function (L) {
      const pts = resample(L.p, 7), out = [];
      const amp = (L.kind === "body" || L.kind === "hair" ? 9 : 5) * (.4 + .6 * o.react);
      for (let i = 0; i < pts.length; i++) {
        const a = pts[Math.max(0, i - 1)], b = pts[Math.min(pts.length - 1, i + 1)], dx = b[0] - a[0], dy = b[1] - a[1], ln = Math.hypot(dx, dy) || 1;
        const nx = -dy / ln, ny = dx / ln, w = sc.length ? clamp(sc[(idx + i * 3) % N] * o.gain, -1, 1) : 0;
        let d = w * amp + (wob > .01 ? Math.sin(V.t * 5 + (idx + i) * .25) * 10 * wob : 0);
        let px = X + (pts[i][0] + nx * d) * k, py = Y + (pts[i][1] + ny * d) * k;
        if (crush > .02) { const q = 2 + crush * 10; px = Math.round(px / q) * q; py = Math.round(py / q) * q; }
        out.push([px, py]);
      }
      idx += pts.length * 3;
      return { pts: out, kind: L.kind, closed: L.closed };
    });
    const bucket = Math.floor(V.t * 10);
    const style = F.style;
    const strokeAll = function (cl, alpha, widths, dxo) {
      lines.forEach(function (L, li) {
        let a = alpha * (L.kind === "desk" || L.kind === "detail" ? .7 : 1);
        let c = cl;
        if (L.kind.indexOf("pad") === 0) { const lit = +L.kind.slice(3) === st.lastPad ? st.open : 0; c = lit > .05 ? [255, 255, 255] : cl; a *= .5 + lit; }
        if (L.kind === "hand0") a *= 1 + st.press[0]; if (L.kind === "hand1") a *= 1 + st.press[1];
        widths.forEach(function (W) {
          g.strokeStyle = rgba(W[2] ? [255, 255, 255] : c, Math.min(1, a * W[1])); g.lineWidth = W[0] * k * 1.6; g.lineJoin = "round"; g.lineCap = "round";
          if (style === "dots" || style === "particles") {
            g.fillStyle = g.strokeStyle;
            L.pts.forEach(function (p, i) { if (i % (style === "dots" ? 2 : 1)) return; const j = style === "particles" ? (hash(i * 97 + li * 13 + bucket) - .5) * 6 * k : 0; g.beginPath(); g.arc(p[0] + j + (dxo || 0), p[1] - j, Math.max(.8, W[0] * k * .9), 0, 6.283); g.fill(); });
            return;
          }
          g.beginPath(); let pen = false;
          L.pts.forEach(function (p, i) {
            if (crumble > .02 && hash(Math.floor((li * 40 + i) / 6) * 31 + bucket * 977) < crumble * .5) { pen = false; return; }
            if (!pen) { g.moveTo(p[0] + (dxo || 0), p[1]); pen = true; } else g.lineTo(p[0] + (dxo || 0), p[1]);
          });
          if (L.closed && crumble < .02) g.closePath();
          g.stroke();
        });
      });
    };
    g.globalCompositeOperation = "lighter";
    // GHOST: violet copies trailing behind (strength = real GHOST activity)
    if (fx[0] > .02) for (let i = 3; i >= 1; i--) strokeAll(rgb(FXC[0]), fx[0] * (.35 - i * .07), [[2.2, 1]], -i * 18 * k * fx[0]);
    const widths = style === "vector" ? [[1.6, .9, true]] : style === "ribbon" ? [[7, .25], [2.4, .8], [1, .9, true]] : style === "dots" ? [[2.2, .9]] : style === "particles" ? [[1.4, .8]] : [[9, .14], [3.6, .45], [1.4, .95, true]];
    const lum = .45 + .55 * Math.min(1, (s.meter || 0) * 6 + .2) ;
    strokeAll(col, lum, widths);
    if (style === "ribbon") strokeAll(col, lum * .6, [[1.2, .9]], 5 * k);
  }
  // Chord name from the notes really held (pitch classes); root = the lowest note when it fits a shape.
  const SHAPES = [
    [[0, 4, 7], ""], [[0, 3, 7], "m"], [[0, 3, 6], "dim"], [[0, 4, 8], "aug"], [[0, 2, 7], "sus2"], [[0, 5, 7], "sus4"],
    [[0, 4, 7, 10], "7"], [[0, 4, 7, 11], "maj7"], [[0, 3, 7, 10], "m7"], [[0, 3, 7, 11], "m(maj7)"], [[0, 3, 6, 10], "m7b5"], [[0, 3, 6, 9], "dim7"],
    [[0, 4, 7, 9], "6"], [[0, 3, 7, 9], "m6"], [[0, 4, 7, 2], "add9"], [[0, 3, 7, 2], "m(add9)"], [[0, 5, 7, 10], "7sus4"],
    [[0, 4, 7, 10, 2], "9"], [[0, 4, 7, 11, 2], "maj9"], [[0, 3, 7, 10, 2], "m9"], [[0, 4, 10], "7"], [[0, 3, 10], "m7"], [[0, 4, 11], "maj7"],
    [[0, 7], "5"]
  ];
  function chordName(notes) {
    if (!notes || notes.length < 2) return "";
    const sorted = notes.slice().sort(function (a, b) { return a - b; }), bass = sorted[0] % 12;
    const pcs = []; sorted.forEach(function (n) { const p = n % 12; if (pcs.indexOf(p) < 0) pcs.push(p); });
    if (pcs.length < 2) return NOTE[bass]; // octaves
    let best = null;
    for (let ri = 0; ri < pcs.length; ri++) {
      const root = pcs[ri], rel = pcs.map(function (p) { return (p - root + 12) % 12; }).sort(function (a, b) { return a - b; });
      SHAPES.forEach(function (sh) {
        const set = sh[0].map(function (x) { return x % 12; }).slice().sort(function (a, b) { return a - b; });
        if (set.length === rel.length && set.every(function (x, i) { return x === rel[i]; })) {
          const score = (root === bass ? 2 : 0) - sh[1].length * .01;
          if (!best || score > best.score) best = { score: score, name: NOTE[root] + sh[1] + (root !== bass ? "/" + NOTE[bass] : "") };
        }
      });
    }
    return best ? best.name : sorted.map(function (n) { return NOTE[n % 12]; }).filter(function (x, i, a) { return a.indexOf(x) === i; }).join(" ");
  }
  // thumbnail of the light producer (Estúdio cards): static, with a gentle sine as the "sound"
  function figureThumb(F, size, crop) {
    const c = document.createElement("canvas"); c.width = c.height = size; const g = c.getContext("2d");
    g.fillStyle = "#120a1e"; g.fillRect(0, 0, size, size);
    const V = { t: 1, fig: null }, sc = []; for (let i = 0; i < 128; i++) sc.push(Math.sin(i * .35) * .5);
    const cr = crop || [0, -150, 520]; // centre x, centre y, width in local units
    const k = size / cr[2];
    drawFigure(g, V, Object.assign({}, FIG_DEFAULT, F), { meter: .2, beat: 0, pitch: .5 }, { x: size / 2 - cr[0] * k, y: size / 2 - cr[1] * k, k: k, col: [140, 200, 255], fx: [0, 0, 0, 0, 0, 0], react: 1, gain: 1, scope: sc, dt: 0 });
    return c;
  }
  window.RastroViz = { create: create, chordName: chordName, FIG_DEFAULT: FIG_DEFAULT, FIG_ITEMS: FIG_ITEMS, figureThumb: figureThumb, noteName: function (n) { return NOTE[((n % 12) + 12) % 12] + (Math.floor(n / 12) - 1); } };
})();
