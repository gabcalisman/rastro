/* RASTRO painted alien — the official 2.5D art (one painted pose per instrument look) turned into
   a cut-out puppet: legs, upper body and head are separate soft-masked layers of the same picture.
   Legs shear from the planted feet (weight shift, knee dip), the torso rotates over the hips and
   the head follows with its own lag. This keeps the art exactly as painted while it dances.
   When the art arrives as separate parts, ui/alien.js (the full IK rig) takes over the same states. */
(function () {
  "use strict";
  const NS = "http://www.w3.org/2000/svg";
  const clamp = function (v, a, b) { return Math.min(b, Math.max(a, v)); };
  const f1 = function (n) { return (Math.round(n * 10) / 10).toString(); };
  // Measured on each picture (pixels of the 700-px-high PNG).
  const LOOKS = {
    guitar: { img: "al_guitar", w: 490, inst: "guitar", name: "Techwear · Guitarra", head: [180, 112, 96, 98], neck: [175, 205], hip: [230, 400], feet: 668, strum: [178, 330], fret: [[250, 290], [372, 172]], level: 1 },
    bass: { img: "al_bass", w: 469, inst: "bass", name: "Disco Suit · Baixo", head: [195, 95, 96, 98], neck: [190, 190], hip: [200, 410], feet: 668, strum: [128, 322], fret: [[240, 240], [380, 120]], level: 2 },
    pads: { img: "al_pads", w: 463, inst: "pads", name: "Hoodie Holo · Pads", head: [210, 102, 92, 98], neck: [205, 195], hip: [210, 420], feet: 668, pads: [[222, 280], [334, 300]], level: 3 },
    disco: { img: "al_disco", w: 381, inst: "keytar", name: "Flare 70s · Keytar", head: [165, 112, 80, 98], neck: [160, 200], hip: [170, 400], feet: 668, keys: [[92, 330], [250, 272]], level: 1 }
  };
  // Outfit sheet from Gab (9 complete looks, no instrument in hand: the free hand points, the other rests on the hip).
  [["velvet", "Velvet Sunset", 446, 232, 1], ["neonwave", "Neon Wave", 453, 234, 1], ["verdeamarela", "Verde-Amarela", 444, 226, 1], ["college", "College Violeta", 438, 226, 2],
   ["chrome", "Chrome Cargo", 450, 235, 2], ["flower", "Flower Power", 427, 217, 3], ["knit", "Retro Knit", 428, 220, 3], ["circuit", "Circuit Tailor", 436, 231, 4], ["starpilot", "Star Pilot", 433, 214, 5]].forEach(function (o) {
    LOOKS["o_" + o[0]] = { img: "al_o_" + o[0], w: o[2], inst: "none", name: o[1], head: [o[3], 118, 98, 100], neck: [o[3] - 8, 208], hip: [o[3] - 6, 330], feet: 668, level: o[4], outfit: true };
  });
  // The "Varsity · Keytar" picture (al_keytar.png) was generated with THREE hands; it is withdrawn until
  // it is redrawn with two. Saved looks that used it open with the Flare 70s keytar instead.
  const ALIAS = { keytar: "disco" };
  let uid = 0;
  // ---------- canvas rendering (performance) ----------
  // The painted look is split once into soft-edged layers (legs / body / head) on offscreen canvases, and the
  // character is drawn each frame with plain 2D transforms on one canvas. No SVG masks, no filters, no blend
  // modes: those forced the browser to re-rasterise the whole picture on every frame and overloaded the Mac.
  const cache = {};
  function canvasOf(w, h) { const c = document.createElement("canvas"); c.width = w; c.height = h; return c; }
  function layersFor(L, done) {
    const key = L.img;
    if (cache[key]) { if (cache[key].ready) done(cache[key]); else cache[key].wait.push(done); return; }
    const C = cache[key] = { ready: false, wait: [done], tints: {} };
    const img = new Image(), glow = new Image(); let n = 0;
    const fin = function () {
      if (++n < 2) return;
      const W = L.w, H = 700, hip = L.hip[1], hx = L.head[0], hy = L.head[1], hrx = L.head[2], hry = L.head[3];
      const make = function (paint) { const c = canvasOf(W, H), g = c.getContext("2d"); g.drawImage(img, 0, 0, W, H); g.globalCompositeOperation = "destination-in"; paint(g); return c; };
      const headMask = function (g) {
        g.save(); g.translate(hx, hy); g.scale(hrx, hry);
        const r = g.createRadialGradient(0, 0, 0, 0, 0, 1);
        r.addColorStop(0, "rgba(0,0,0,1)"); r.addColorStop(.82, "rgba(0,0,0,1)"); r.addColorStop(1, "rgba(0,0,0,0)");
        g.fillStyle = r; g.beginPath(); g.arc(0, 0, 1, 0, Math.PI * 2); g.fill(); g.restore();
      };
      C.legs = make(function (g) { const gr = g.createLinearGradient(0, hip - 34, 0, hip + 6); gr.addColorStop(0, "rgba(0,0,0,0)"); gr.addColorStop(1, "rgba(0,0,0,1)"); g.fillStyle = gr; g.fillRect(0, 0, W, H); });
      C.body = make(function (g) {
        const gr = g.createLinearGradient(0, hip - 6, 0, hip + 34); gr.addColorStop(0, "rgba(0,0,0,1)"); gr.addColorStop(1, "rgba(0,0,0,0)"); g.fillStyle = gr; g.fillRect(0, 0, W, H);
        g.globalCompositeOperation = "destination-out"; headMask(g);
      });
      C.head = make(function (g) { headMask(g); });
      C.glowImg = glow;
      C.ready = true; C.wait.forEach(function (f) { f(C); }); C.wait = [];
    };
    img.onload = glow.onload = fin; img.onerror = glow.onerror = fin;
    img.src = L.img + ".png"; glow.src = L.img + "_glow.png";
  }
  function tint(C, col) {
    const q = col.toLowerCase(); if (C.tints[q]) return C.tints[q];
    const keys = Object.keys(C.tints); if (keys.length > 24) delete C.tints[keys[0]];
    const c = canvasOf(C.glowImg.width || 1, C.glowImg.height || 1), g = c.getContext("2d");
    g.drawImage(C.glowImg, 0, 0); g.globalCompositeOperation = "source-in"; g.fillStyle = q; g.fillRect(0, 0, c.width, c.height);
    return (C.tints[q] = c);
  }
  // colours drift continuously with the sound; quantise so the tint cache stays small
  function quant(hex) {
    if (!/^#[0-9a-f]{6}$/i.test(hex)) return "#5fe3ff";
    const n = parseInt(hex.slice(1), 16), f = function (v) { return Math.min(255, Math.round(v / 24) * 24); };
    return "#" + [n >> 16 & 255, n >> 8 & 255, n & 255].map(function (v) { const h = f(v).toString(16); return h.length < 2 ? "0" + h : h; }).join("");
  }
  function create(parent, lookId) {
    lookId = LOOKS[lookId] ? lookId : ALIAS[lookId] || "disco";
    const id = "pp" + (++uid), L = LOOKS[lookId];
    const svg = parent.ownerSVGElement || parent, host = svg.parentNode;
    const canvas = document.createElement("canvas"); canvas.className = "puppet-canvas";
    canvas.style.cssText = "position:absolute;left:0;top:0;width:100%;height:100%;pointer-events:none;";
    host.appendChild(canvas);
    const R = { root: parent, id: id, look: lookId, L: L, springs: {}, t: 0, canvas: canvas, ghostHist: [], P: {}, st: { eye: 0, mouth: 0, hit: 0, hitV: 0, nod: 0, nodV: 0, strum: 0, strumV: 0, press: 0, hop: 0, hopV: 0, sq: 0, sqV: 0, lastHop: -1, air: false } };
    R.C = null; layersFor(L, function (C) { R.C = C; R.dirty = true; });
    R.update = function (s, dt) { update(R, s, dt); draw(R, s); };
    R.pose = function () { R.springs = {}; update(R, { alive: 0 }, 0); };
    R.destroy = function () { canvas.remove(); };
    R.body = { getAttribute: function () { return ""; } }; // compatibility: ghosts are drawn here now
    update(R, { alive: 0 }, 0);
    return R;
  }
  function draw(R, s) {
    const cv = R.canvas, host = cv.parentNode; if (!host || !R.C) return;
    const cw = host.clientWidth, ch = host.clientHeight; if (!cw || !ch) return;
    const dpr = Math.min(2, window.devicePixelRatio || 1);
    if (cv.width !== Math.round(cw * dpr) || cv.height !== Math.round(ch * dpr)) { cv.width = Math.round(cw * dpr); cv.height = Math.round(ch * dpr); }
    const g = cv.getContext("2d");
    const m = R.root.getScreenCTM(); if (!m) return;
    const r = host.getBoundingClientRect(), k = r.width / cw;
    const base = [m.a / k * dpr, m.b / k * dpr, m.c / k * dpr, m.d / k * dpr, (m.e - r.left) / k * dpr, (m.f - r.top) / k * dpr];
    const P = R.P, C = R.C, L = R.L, W = L.w, H = 700, F = L.feet;
    // clear only where the character was / is (plus margin): far less work than the whole window
    g.setTransform(1, 0, 0, 1, 0, 0);
    const bx = base[4] - 140 * base[0], by = base[5] - 80 * base[3], bw = (W + 280) * base[0], bh = (H + 380) * base[3];
    if (R.lastBox) g.clearRect(R.lastBox[0], R.lastBox[1], R.lastBox[2], R.lastBox[3]);
    g.clearRect(bx, by, bw, bh); R.lastBox = [bx, by, bw, bh];
    const setBase = function () { g.setTransform(base[0], base[1], base[2], base[3], base[4], base[5]); };
    const whole = function () { g.translate(P.fx0 + P.idleX, P.fy0 + P.hop); g.rotate(P.lean * Math.PI / 180); g.transform(1, Math.tan(P.yaw * 3.2 * Math.PI / 180), 0, 1, 0, 0); g.scale(P.sxW, P.syW); g.translate(-P.fx0, -P.fy0); };
    const bodyT = function () { g.translate(P.hipX, P.dipB); g.translate(P.px, P.py); g.rotate(P.torR * Math.PI / 180); g.translate(-P.px, -P.py); };
    const headT = function () { g.translate(L.neck[0], L.neck[1]); g.rotate(P.headR * Math.PI / 180); g.translate(-L.neck[0], -L.neck[1] + P.headY); };
    const figure = function () {
      g.save(); g.transform(1, 0, -P.shx, P.shy, P.shx * F, F * (1 - P.shy)); g.drawImage(C.legs, 0, 0, W, H); g.restore();
      g.save(); bodyT(); g.drawImage(C.body, 0, 0, W, H); g.save(); headT(); g.drawImage(C.head, 0, 0, W, H); g.restore(); g.restore();
    };
    // reflection on the glossy platform (flipped, faint, only below the feet)
    if (s.reflect) {
      setBase(); g.save(); g.beginPath(); g.rect(-60, F + 2, W + 120, 240); g.clip();
      g.translate(0, 2 * F + 4); g.scale(1, -1); g.globalAlpha = .16; whole(); figure(); g.restore();
    }
    setBase();
    // contact shadow
    g.save(); g.globalAlpha = .45 * P.shadowOp; g.fillStyle = "#000"; g.beginPath(); g.ellipse(L.hip[0], F + 4, Math.max(1, P.shadowRx), 12, 0, 0, Math.PI * 2); g.fill(); g.restore();
    // aura (BLOOM, envelope)
    if (P.auraOp > .01) {
      g.save(); g.translate(W / 2, 315); g.scale(P.auraS * W * .62, P.auraS * H * .52);
      const ag = g.createRadialGradient(0, 0, 0, 0, 0, 1); ag.addColorStop(0, P.col); ag.addColorStop(1, "rgba(0,0,0,0)");
      g.globalAlpha = P.auraOp * .45; g.globalCompositeOperation = "lighter"; g.fillStyle = ag; g.beginPath(); g.arc(0, 0, 1, 0, Math.PI * 2); g.fill(); g.restore();
    }
    // GHOST afterimages: violet silhouettes of earlier poses (strength = real GHOST activity)
    if (s.ghost > .02 && R.ghostHist.length) {
      const sil = tint(C, "#b06bff"), sp = Math.max(1, s.ghostSpacing || 4);
      for (let i = 0; i < 4; i++) {
        const hst = R.ghostHist[Math.min(R.ghostHist.length - 1, (i + 1) * sp)]; if (!hst) break;
        g.save(); g.globalAlpha = Math.max(0, s.ghost * (.55 - i * .12)); g.translate(-(i + 1) * 12 * s.ghost, 0);
        g.translate(hst.hipX, hst.dipB); g.translate(hst.px, hst.py); g.rotate(hst.torR * Math.PI / 180); g.translate(-hst.px, -hst.py);
        g.drawImage(sil, 0, 0, W, H); g.restore();
      }
    }
    g.save(); whole();
    // sound-coloured glow around the silhouette and the WOBBLE cyan echo
    g.save(); bodyT(); g.globalAlpha = P.glowOp; g.drawImage(tint(C, P.colQ), 0, 0, W, H); g.restore();
    if (P.wobOp > .01) { g.save(); bodyT(); g.translate(P.wobX, 0); g.globalAlpha = P.wobOp; g.drawImage(tint(C, "#30e0ff"), 0, 0, W, H); g.restore(); }
    figure();
    // instrument lights and face glows (additive)
    g.save(); bodyT(); g.globalCompositeOperation = "lighter";
    if (P.keyOp > .01) { g.globalAlpha = P.keyOp; g.fillStyle = P.col; g.beginPath(); g.ellipse(P.keyX, P.keyY, 16, 9, 0, 0, Math.PI * 2); g.fill(); }
    if (P.strOp > .01 && L.strum) { g.globalAlpha = P.strOp; g.strokeStyle = P.col; g.lineWidth = 2.4; g.beginPath(); g.moveTo(L.fret[0][0], L.fret[0][1] + P.strY); g.lineTo(L.strum[0], L.strum[1] + P.strY); g.stroke(); }
    if (P.pads) P.pads.forEach(function (c) { if (c.v > .02) { g.globalAlpha = c.v * .85; g.fillStyle = P.col; g.fillRect(c.x - 6, c.y - 4, 12, 8); } });
    g.save(); headT();
    const face = function (x, y, rx, ry, c0, c1, a) { if (a < .01) return; g.save(); g.translate(x, y); g.scale(rx, ry); const fg = g.createRadialGradient(0, 0, 0, 0, 0, 1); fg.addColorStop(0, c0); fg.addColorStop(.45, c1); fg.addColorStop(1, "rgba(0,0,0,0)"); g.globalAlpha = a; g.fillStyle = fg; g.beginPath(); g.arc(0, 0, 1, 0, Math.PI * 2); g.fill(); g.restore(); };
    face(P.ex[0], P.ex[1], L.head[2] * .62, 26, "#bff8ff", "rgba(47,224,255,.5)", P.eyeOp);
    face(P.mo[0], P.mo[1], 30, P.mouthRy, "#ffd0f4", "rgba(255,79,216,.5)", P.mouthOp);
    g.restore(); g.restore();
    g.restore();
  }
  function spring(R, key, target, w, dt, kick) {
    let sp = R.springs[key]; if (!sp) sp = R.springs[key] = { x: target, v: 0 };
    if (kick) sp.v += kick;
    if (dt <= 0) { sp.x = target; sp.v = 0; return sp.x; }
    const n = Math.ceil(dt / (1 / 120)), h = dt / n;
    for (let i = 0; i < n; i++) { sp.v += (w * w * (target - sp.x) - 2 * w * sp.v) * h; sp.x += sp.v * h; }
    return sp.x;
  }
  function update(R, s, dt) {
    const L = R.L, st = R.st, W = L.w;
    R.t += dt;
    const amp = s.amp === undefined ? 1 : s.amp, alive = (s.alive || 0) * amp, I = (s.intensity === undefined ? .8 : s.intensity) * alive;
    const D = window.RastroAlien && window.RastroAlien.dance ? window.RastroAlien.dance(s.style || "groove", s.beats || 0) : { hipX: 0, hipY: 0, hipR: 0, torR: 0, chestX: 0, headR: 0, headY: 0, inst: 0 };
    if (s.hit) { st.eye = Math.max(st.eye, .45 + (s.vel || .7) * .55); if ((s.vel || .7) > .78) st.mouth = 1; }
    st.eye = Math.max(0, st.eye - dt * 2.6); st.mouth = Math.max(0, st.mouth - dt * 2.2);
    const P = R.P, amp0 = s.amp === undefined ? 1 : s.amp;
    P.eyeOp = Math.min(1, st.eye * .8 + (s.glowAmt || 0) * .15) * Math.min(1, amp0 + .3);
    P.mouthOp = st.mouth * .85 * Math.min(1, amp0 + .3); P.mouthRy = 10 + st.mouth * 10;
    P.ex = L.eyes || [L.head[0] - 4, L.head[1] + 22]; P.mo = L.mouth || [L.head[0] + 10, L.head[1] + 62];
    if (s.hit) { const v = (s.vel || .7) * amp; st.hitV += 70 * v; st.nodV += 150 * v; st.strumV += (st.strum > 0 ? -1 : 1) * 260 * v; st.press = 1; }
    st.hitV += (-st.hit * 380 - st.hitV * 28) * dt; st.hit += st.hitV * dt;
    st.nodV += (-st.nod * 240 - st.nodV * 22) * dt; st.nod += st.nodV * dt;
    st.strumV += (-st.strum * 320 - st.strumV * 20) * dt; st.strum = clamp(st.strum + st.strumV * dt, -10, 10);
    st.press = Math.max(0, st.press - dt * 6);
    const k = 1.25; // dance units -> picture pixels
    const hipX = spring(R, "hx", D.hipX * I * k * 1.3, 15, dt), dip = spring(R, "dp", (D.hipY * I + clamp(st.hit, -2, 6)) * k * 1.4, 17, dt);
    const torR = spring(R, "tr", (D.torR * .8 + D.hipR * .4) * I + (s.look || 0) * 1.5 + (L.inst === "guitar" || L.inst === "bass" ? -st.strum * .12 : 0), 11, dt);
    const headR = spring(R, "hr", D.headR * I * 1.1 + (s.look || 0) * 5 - clamp(st.nod, -3, 8) * .5, 7.5, dt);
    const headY = spring(R, "hy", (D.headY * I + clamp(st.nod, -2, 6) * .4) * k, 9, dt);
    const breathe = Math.sin(R.t * 1.5) * 1.2;
    // --- game feel ---
    // hops: on the downbeat of every 2nd beat when the groove is strong (Disco / Funk / Expressivo); the landing squashes
    const beats = s.beats || 0, beatN = Math.floor(beats);
    if (I > .55 && beatN !== st.lastHop && beatN % 2 === 0 && (s.style === "disco" || s.style === "funk" || I > .95) && !st.air) { st.hopV = -(120 + 160 * Math.min(1, I - .5)) * amp; st.air = true; st.sqV -= 2.2; }
    st.lastHop = beatN;
    if (st.air) { st.hopV += 1500 * dt; st.hop += st.hopV * dt; if (st.hop >= 0) { st.hop = 0; st.air = false; st.sqV += 3.6 * Math.min(1, Math.abs(st.hopV) / 260); st.hopV = 0; } }
    else if (dt <= 0) { st.hop = 0; st.hopV = 0; }
    st.sqV += (-st.sq * 260 - st.sqV * 16) * dt; st.sq += st.sqV * dt;
    if (s.hit) st.sqV += .9 * (s.vel || .7) * amp;
    const sq = clamp(st.sq, -.5, .6);
    // yaw: the body turns a little toward the notes / mouse and with the 4-bar phrase of the dance
    const yaw = spring(R, "yw", clamp(Math.sin(beats * Math.PI / 8) * .55 * I + (s.look || 0) * .5, -1, 1), 3.2, dt);
    const lean = spring(R, "ln", (D.hipR || 0) * .35 * I + (s.look || 0) * 1.2, 6, dt);
    const idle = (1 - Math.min(1, alive * 2)) * Math.min(1, amp + .3);
    const idleX = Math.sin(R.t * .55) * 3 * idle;
    const sxW = (1 - Math.abs(yaw) * .1) * (1 + sq * .05), syW = 1 - sq * .07 + (st.air ? .025 : 0);
    const fx0 = L.hip[0], fy0 = L.feet;
    P.fx0 = fx0; P.fy0 = fy0; P.idleX = idleX; P.hop = st.hop * .8; P.lean = lean; P.yaw = yaw; P.sxW = sxW; P.syW = syW;
    // legs: shear from the planted feet, compress for the knee dip
    const F = L.feet, Lh = F - L.hip[1], sx = hipX / Lh, sy = 1 - dip / Lh;
    P.shx = sx; P.shy = sy; P.px = L.hip[0]; P.py = L.hip[1]; P.hipX = hipX; P.dipB = dip + breathe * .4; P.torR = torR; P.headR = headR; P.headY = headY;
    if (dt > 0) { R.ghostHist.unshift({ hipX: P.hipX, dipB: P.dipB, px: P.px, py: P.py, torR: P.torR }); if (R.ghostHist.length > 40) R.ghostHist.pop(); }
    // WOBBLE: a cyan echo of the outline that sways sideways with the real WOBBLE activity
    const wob = clamp(s.wob || 0, 0, 1);
    P.wobOp = wob * .55; P.wobX = Math.sin(R.t * (3 + (s.wobRate || 1) * 2)) * (6 + wob * 10);
    // glow colour/intensity from the sound (envelope, effects, filter)
    const ga = clamp(s.glowAmt || 0, 0, 1), col = s.glow || "#5fe3ff";
    P.col = col; P.colQ = quant(col); P.glowOp = .28 + ga * .6;
    const bloom = clamp(s.bloom || 0, 0, 1);
    P.auraOp = .18 + ga * .35 + bloom * .55; P.auraS = 1 + bloom * .45 + ga * .05;
    const air = clamp(-st.hop / 90, 0, 1);
    P.shadowRx = (W * .36 + Math.abs(hipX) * .5) * (1 - air * .3) * (1 + sq * .06); P.shadowOp = 1 - air * .45;
    // instrument lights from real notes
    const pitch = clamp(s.pitch === undefined ? .5 : s.pitch, 0, 1), playing = !!s.playing;
    P.keyOp = 0; P.strOp = 0;
    if (L.keys) { const a = L.keys[0], b = L.keys[1]; P.keyX = a[0] + (b[0] - a[0]) * pitch; P.keyY = a[1] + (b[1] - a[1]) * pitch; P.keyOp = playing ? .45 + st.press * .45 : 0; }
    if (L.strum) { P.strOp = playing ? .35 + Math.abs(st.strum) / 10 * .5 : 0; P.strY = Math.sin(R.t * 90) * Math.abs(st.strum) * .25; }
    if (L.pads) {
      if (!P.pads) { P.pads = []; for (let r = 0; r < 2; r++) for (let c = 0; c < 8; c++) P.pads.push({ x: L.pads[0][0] + (L.pads[1][0] - L.pads[0][0]) * (c + .5) / 8, y: L.pads[0][1] + (L.pads[1][1] - L.pads[0][1]) * (r + .5) / 2, v: 0 }); }
      if (s.hit && s.pad !== undefined && P.pads[s.pad % 16]) P.pads[s.pad % 16].v = 1;
      P.pads.forEach(function (c) { c.v = Math.max(0, c.v - dt * 3); });
    }
    R.points = { head: [L.head[0], L.head[1] - L.head[3] * .7], phones: [L.head[0] - L.head[2] * .8, L.head[1] + 10], eyes: [L.head[0] + 20, L.head[1] + 20], top: [L.hip[0] - 40, L.hip[1] - 120], bottom: [L.hip[0] - 30, L.hip[1] + 100], shoes: [L.hip[0] + 80, L.feet - 30], inst: L.keys ? L.keys[1] : L.fret ? L.fret[1] : L.pads ? L.pads[1] : [W / 2, 300] };
  }
  window.RastroPuppet = { LOOKS: LOOKS, ALIAS: ALIAS, create: create };
})();
