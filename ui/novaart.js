// RASTRO PLAY — Nova, Gab's official art, animated as a cut-out rig.
// The drawing is exactly Gab's picture, split into parts (nv_*.png). The parts are put back together pixel for
// pixel at rest; what is hidden behind a front part was painted in a narrow band so small rotations never show holes.
// Skeleton: feet planted → legs bend/shear from the feet → hips → torso → neck/head; keytar on the strap with the
// right hand on its neck; left arm from the shoulder on the keys; star earring as a pendulum (physics).
// Same API as nova.js / puppet.js: create(parent) → {update, step, renderTo, setSlots, pose, destroy, L, canvas}.
(function () {
  "use strict";
  const PARTS = { legR: [592, 837, 335, 505], legL: [216, 837, 414, 502], strap: [550, 917, 69, 143], torso: [349, 463, 595, 451], head: [250, 25, 605, 507], earring: [330, 384, 87, 104], keytar: [391, 518, 610, 429], handR: [806, 600, 102, 114], armL: [298, 554, 320, 248] };
  // HAIR: alternative heads (same face, other hairstyle) aligned to the original head. Provisional art cut from Gab's
  // Locker mock-up (low resolution); "default" is the original drawing.
  const HAIRS = {"fluffy": [240, 45, 655, 487], "spikes": [255, 18, 645, 514], "longsweep": [245, 32, 643, 500], "spacebuns": [280, 9, 608, 523], "shaggy": [247, 35, 640, 497], "halfup": [261, 9, 620, 523], "crest": [230, 14, 627, 518], "curls": [248, 37, 639, 495], "sidecut": [323, 18, 537, 514]};
  const HAIR_NAMES = { "default": "Default", fluffy: "Fluffy", spikes: "Spikes", longsweep: "Long Sweep", spacebuns: "Space Buns", shaggy: "Shaggy", halfup: "Half Up", crest: "Crest", curls: "Curls", sidecut: "Side Cut", starlight: "Starlight", galaxy: "Galaxy" };
  const hairImg = {};
  function hairImage(id) {
    if (!HAIRS[id]) return null;
    if (!hairImg[id]) { const im = new Image(); im.src = "nv_hair_" + id + ".png"; hairImg[id] = im; }
    return hairImg[id];
  }
  // pivots in art pixels
  const K = { feet: [555, 1330], hip: [560, 900], neck: [556, 516], shoulder: [432, 598], earHook: [372, 398], keyPivot: [470, 805], footL: [330, 1330], footR: [770, 1330] };
  const S = .49;                       // art pixels → character units (feet at 0, ≈ 640 tall like nova.js)
  const EYES = [
    { pts: [[472, 322], [515, 335], [575, 370], [605, 422], [570, 422], [525, 407], [490, 375], [477, 345]] },
    { pts: [[640, 435], [675, 425], [710, 420], [720, 435], [710, 460], [675, 472], [650, 460]] }
  ];
  const SKIN = "#56d0e8", INK = "#160c1e";
  // 8-beat phrase (anticipation, side-step, accent, follow-through, recovery, mirrored). Units: art px / degrees.
  const PHRASE = [
    { hx: 0, dp: 10, tor: 0, head: 2, kt: 0, nod: 4 },        // anticipation: sink
    { hx: -26, dp: 2, tor: -3, head: -4, kt: 1, nod: -2 },    // step left, shoulders counter
    { hx: -18, dp: 16, tor: -1.5, head: 3, kt: -5, nod: 8 },  // keytar accent (neck lifts)
    { hx: -8, dp: 4, tor: 1, head: 5, kt: -1, nod: -3 },      // follow-through
    { hx: 0, dp: 10, tor: 0, head: -2, kt: 0, nod: 4 },       // recovery / sink
    { hx: 26, dp: 2, tor: 3, head: 4, kt: 1, nod: -2 },       // step right
    { hx: 18, dp: 16, tor: 1.5, head: -3, kt: -5, nod: 8 },   // accent
    { hx: 8, dp: 4, tor: -1, head: -5, kt: -1, nod: -3 }      // follow-through
  ];
  const clamp = function (v, a, b) { return v < a ? a : v > b ? b : v; };
  const sstep = function (x) { return x * x * (3 - 2 * x); };
  function phrase(b) {
    const N = PHRASE.length, n = Math.floor(b), f = sstep(b - n), A = PHRASE[((n % N) + N) % N], B = PHRASE[(((n + 1) % N) + N) % N], o = {};
    for (const k in A) o[k] = A[k] + (B[k] - A[k]) * f;
    return o;
  }
  function spring(st, key, target, w, dt) {
    let s = st[key]; if (!s) s = st[key] = { x: target, v: 0 };
    if (dt <= 0) { s.x = target; s.v = 0; return s.x; }
    const n = Math.max(1, Math.ceil(dt / (1 / 120))), h = dt / n;
    for (let i = 0; i < n; i++) { s.v += (w * w * (target - s.x) - 2 * w * s.v * .8) * h; s.x += s.v * h; }
    return s.x;
  }
  // images (shared by every instance)
  const IMG = {}; let loaded = 0, total = 0; const waiters = [];
  function load(done) {
    if (total && loaded === total) { done(); return; }
    waiters.push(done); if (total) return;
    Object.keys(PARTS).forEach(function (k) {
      total++; const im = new Image();
      im.onload = im.onerror = function () { if (++loaded === total) { waiters.forEach(function (f) { f(); }); waiters.length = 0; } };
      im.src = "nv_" + k + ".png"; IMG[k] = im;
    });
  }
  const tints = {};
  function canvasOf(w, h) { const c = document.createElement("canvas"); c.width = w; c.height = h; return c; }

  function create(parent, slots) {
    const R = { root: parent, look: "nova-art", L: { w: 520, hip: [260, 440], feet: 668, name: "Nova", inst: "keytar" }, st: {}, t: 0, hist: [], P: null, ready: false };
    const svg = parent.ownerSVGElement || parent, host = svg.parentNode;
    const canvas = document.createElement("canvas"); canvas.className = "puppet-canvas";
    canvas.style.cssText = "position:absolute;left:0;top:0;width:100%;height:100%;pointer-events:none;";
    host.appendChild(canvas); R.canvas = canvas;
    R.fig = canvasOf(8, 8); R.ear = { a: 0, v: 0 };
    R.blinkAt = 1.2; R.blink = 0; R.hitK = 0; R.press = 0; R.accent = 0;
    load(function () { R.ready = true; });
    R.update = function (s, dt) { pose(R, s, dt); drawToCanvas(R, s); };
    R.step = function (s, dt) { pose(R, s, dt); };
    R.renderTo = function (g, s) { if (!R.P) pose(R, s || { alive: 0 }, 0); drawCharacter(g, R, s || {}); };
    R.pose = function () { R.st = {}; R.ear = { a: 0, v: 0 }; pose(R, { alive: 0 }, 0); };
    R.setSlots = function (o) { R.hair = o && HAIRS[o.hair] ? o.hair : "default"; hairImage(R.hair); };
    R.setSlots(slots);
    R.destroy = function () { canvas.remove(); };
    R.body = { getAttribute: function () { return ""; } };
    pose(R, { alive: 0 }, 0);
    return R;
  }

  function pose(R, s, dt) {
    const st = R.st; R.t += dt;
    const amp = s.amp === undefined ? 1 : s.amp, alive = clamp((s.alive || 0) * amp, 0, 1);
    const I = clamp(s.intensity === undefined ? .8 : s.intensity, 0, 1.3) * alive;
    const style = s.style || "groove", speed = style === "half" ? .5 : 1, b = (s.beats || 0) * speed;
    const idle = 1 - clamp(alive * 2, 0, 1);
    if (s.hit) { const v = clamp(s.vel || .7, 0, 1); R.hitK = 1; R.press = 1; if (v > .8) R.accent = 1; }
    R.hitK = Math.max(0, R.hitK - dt * 3.5); R.press = Math.max(0, R.press - dt * 7); R.accent = Math.max(0, R.accent - dt * 1.8);
    if (R.t > R.blinkAt) { R.blink = 1; R.blinkAt = R.t + 2.4 + Math.random() * 3; }
    R.blink = Math.max(0, R.blink - dt * 6);
    const G = phrase(b), bounce = Math.pow(.5 + .5 * Math.cos(2 * Math.PI * b), 2), breathe = Math.sin(R.t * 1.7);
    const P = {};
    P.hx = spring(st, "hx", G.hx * I + Math.sin(R.t * .45) * 3 * idle, 8, dt);
    P.dp = spring(st, "dp", (G.dp + 14 * bounce) * I + 8 * R.hitK * Math.max(.3, amp) + breathe * 1.5 * idle, 13, dt);
    P.tor = spring(st, "tor", G.tor * I + breathe * .3 * idle, 8, dt);
    // head lags behind the torso (follow-through) and nods on the beat and on accents
    P.head = spring(st, "hd", G.head * I + Math.sin(R.t * .7) * 2 * idle - R.accent * 6 * amp, 6, dt);
    P.nod = spring(st, "nd", G.nod * I + 6 * R.hitK * amp, 10, dt);
    P.kt = spring(st, "kt", G.kt * I - R.accent * 5 * amp, 9, dt);
    P.press = R.press * (s.playing ? 1 : .5) * Math.max(.3, amp);
    P.breathe = breathe; P.blink = R.blink; P.alive = alive;
    // star earring: pendulum driven by the head's motion
    const headAcc = (P.hx - (R.prevHx === undefined ? P.hx : R.prevHx)) / Math.max(dt, 1 / 240);
    R.prevHx = P.hx;
    if (dt > 0) { const h = Math.min(dt, 1 / 30); R.ear.v += (-60 * R.ear.a - headAcc * .004 - (P.head + P.tor) * .02) * h; R.ear.v *= Math.pow(.9, h * 60); R.ear.a += R.ear.v * h; R.ear.a = clamp(R.ear.a, -.6, .6); }
    P.ear = R.ear.a; P.hair = R.hair;
    R.P = P;
    if (dt > 0) { R.hist.unshift(P); if (R.hist.length > 36) R.hist.pop(); }
  }

  const rad = function (d) { return d * Math.PI / 180; };
  function R_hair(P) { return P.hair && P.hair !== "default" ? hairImage(P.hair) : null; }
  function about(g, p, deg) { g.translate(p[0], p[1]); g.rotate(rad(deg)); g.translate(-p[0], -p[1]); }
  function part(g, k) { const b = PARTS[k], im = IMG[k]; if (im && im.complete && im.naturalWidth) g.drawImage(im, b[0], b[1], b[2], b[3]); }
  // the whole figure in art pixels
  function figure(g, P) {
    const hipH = K.feet[1] - K.hip[1];
    // legs: feet planted; the hip end follows the sway (shear) and the dip (squash from the feet)
    ["legR", "legL"].forEach(function (k) {
      const f = k === "legL" ? K.footL : K.footR;
      const sy = 1 - P.dp / hipH * .9, sh = -P.hx / hipH;
      g.save(); g.transform(1, 0, sh, sy, -sh * f[1], f[1] * (1 - sy)); part(g, k); g.restore();
    });
    g.save();
    g.translate(P.hx, P.dp * .9);
    about(g, K.hip, P.tor);
    // breathing: chest rises a little
    g.translate(K.hip[0], K.hip[1]); g.scale(1, 1 + P.breathe * .004); g.translate(-K.hip[0], -K.hip[1]);
    part(g, "torso"); part(g, "strap");
    // head (+ earring) on the neck
    g.save(); about(g, K.neck, P.head * .8); g.translate(0, P.nod * .6);
    const hi = R_hair(P);
    if (hi && hi.complete && hi.naturalWidth) { const b = HAIRS[P.hair]; g.drawImage(hi, b[0], b[1], b[2], b[3]); } else part(g, "head");
    eyes(g, P);
    g.save(); about(g, K.earHook, P.ear * 57); part(g, "earring"); g.restore();
    g.restore();
    // keytar pivots near the hip; the right hand rides on its neck; left arm presses the keys from the shoulder
    g.save(); about(g, K.keyPivot, P.kt); part(g, "keytar"); part(g, "handR"); g.restore();
    g.save(); about(g, K.shoulder, P.kt * .55 + P.press * 1.6); part(g, "armL"); g.restore();
    g.restore();
  }
  function eyes(g, P) {
    if (P.blink < .02) return;
    const c = 1 - Math.abs(1 - 2 * Math.min(1, P.blink * 1.15)); // 0 → 1 → 0
    EYES.forEach(function (e) {
      let y0 = 1e9, y1 = -1e9; e.pts.forEach(function (p) { y0 = Math.min(y0, p[1]); y1 = Math.max(y1, p[1]); });
      const lid = y0 - 4 + (y1 - y0 + 8) * c;
      g.save(); g.beginPath(); e.pts.forEach(function (p, i) { if (i) g.lineTo(p[0], p[1]); else g.moveTo(p[0], p[1]); }); g.closePath();
      g.clip(); g.fillStyle = SKIN; g.fillRect(e.pts[0][0] - 60, y0 - 10, 260, lid - y0 + 10);
      g.strokeStyle = INK; g.lineWidth = 7; g.beginPath(); g.moveTo(e.pts[0][0] - 60, lid); g.lineTo(e.pts[0][0] + 200, lid); g.stroke();
      g.restore();
    });
  }
  // render the figure once per frame into an offscreen canvas (character units ×2), reused for effects
  function renderFig(R, P, scale) {
    const W = Math.round(1144 * S * scale), H = Math.round(1375 * S * scale);
    const c = R.fig; if (c.width !== W || c.height !== H) { c.width = W; c.height = H; }
    const g = c.getContext("2d"); g.setTransform(1, 0, 0, 1, 0, 0); g.clearRect(0, 0, W, H);
    g.setTransform(S * scale, 0, 0, S * scale, 0, 0); figure(g, P);
    return c;
  }
  function tinted(R, col, alphaKey) {
    const src = R.fig, key = alphaKey; let c = tints[key];
    if (!c || c.width !== src.width || c.height !== src.height) { c = tints[key] = canvasOf(src.width, src.height); }
    const g = c.getContext("2d"); g.globalCompositeOperation = "copy"; g.drawImage(src, 0, 0); g.globalCompositeOperation = "source-in"; g.fillStyle = col; g.fillRect(0, 0, c.width, c.height); g.globalCompositeOperation = "source-over";
    return c;
  }
  function drawToCanvas(R, s) {
    const cv = R.canvas, host = cv.parentNode; if (!host) return;
    const cw = host.clientWidth, ch = host.clientHeight; if (!cw || !ch) return;
    const dpr = Math.min(2, window.devicePixelRatio || 1);
    if (cv.width !== Math.round(cw * dpr) || cv.height !== Math.round(ch * dpr)) { cv.width = Math.round(cw * dpr); cv.height = Math.round(ch * dpr); }
    const g = cv.getContext("2d"), m = R.root.getScreenCTM(); if (!m) return;
    const r = host.getBoundingClientRect(), k = r.width / cw;
    g.setTransform(1, 0, 0, 1, 0, 0); g.clearRect(0, 0, cv.width, cv.height);
    g.setTransform(m.a / k * dpr, m.b / k * dpr, m.c / k * dpr, m.d / k * dpr, (m.e - r.left) / k * dpr, (m.f - r.top) / k * dpr);
    g.translate(R.L.hip[0], R.L.feet);
    drawCharacter(g, R, s, Math.min(2, Math.abs(m.a / k * dpr)));
  }
  function drawCharacter(g, R, s, pxScale) {
    const P = R.P; if (!R.ready || !P) return;
    const sc = clamp(pxScale || 1, .3, 2);
    const fig = renderFig(R, P, sc), fw = fig.width / sc, fh = fig.height / sc;
    const ox = -K.feet[0] * S, oy = -K.feet[1] * S;           // figure canvas origin in character space
    const put = function (c, dx, alpha) { g.save(); g.globalAlpha = alpha; g.drawImage(c, ox + (dx || 0), oy, fw, fh); g.restore(); };
    if (s.reflect) { g.save(); g.beginPath(); g.rect(-400, 2, 800, 240); g.clip(); g.translate(0, 4); g.scale(1, -1); put(fig, 0, .14); g.restore(); }
    g.save(); g.globalAlpha = .45; g.fillStyle = "#000"; g.beginPath(); g.ellipse(P.hx * S * .5, 4, 170 - Math.abs(P.hx) * .1, 14, 0, 0, Math.PI * 2); g.fill(); g.restore();
    const ga = clamp(s.glowAmt || 0, 0, 1), col = s.glow || "#5fe3ff", bloom = clamp(s.bloom || 0, 0, 1);
    const aura = function (c, alpha, rr) { g.save(); g.translate(0, -320); g.scale(1, 1.25); const gr = g.createRadialGradient(0, 0, 20, 0, 0, rr); gr.addColorStop(0, c); gr.addColorStop(1, "rgba(0,0,0,0)"); g.globalAlpha = alpha; g.globalCompositeOperation = "lighter"; g.fillStyle = gr; g.beginPath(); g.arc(0, 0, rr, 0, Math.PI * 2); g.fill(); g.restore(); };
    if (ga > .02) aura(col, .1 + ga * .22, 280);
    if (bloom > .02) aura("#ff4fd8", bloom * .55, 300 * (1 + bloom * .45));
    // GHOST: violet afterimages trailing the movement (strength = real GHOST activity)
    if (s.ghost > .02) {
      const t = tinted(R, "#b06bff", "g");
      for (let i = 3; i >= 0; i--) put(t, -(i + 1) * 16 * s.ghost, clamp(s.ghost * (.45 - i * .09), 0, .55));
    }
    const wob = clamp(s.wob || 0, 0, 1);
    if (wob > .02) put(tinted(R, "#30e0ff", "w"), Math.sin(R.t * (3 + (s.wobRate || 1) * 2)) * (10 + wob * 16), wob * .5);
    if (ga > .05) { g.save(); g.shadowColor = col; g.shadowBlur = 30; put(tinted(R, col, "r"), 0, ga * .5); g.restore(); }
    put(fig, 0, 1);
  }

  window.RastroNovaArt = { create: create, PARTS: PARTS, HAIRS: Object.keys(HAIRS), HAIR_NAMES: HAIR_NAMES };
})();
