/* NOVA — RASTRO's cartoon hero, drawn in code from Gab's reference sheet (blue alien, spiky black hair with
   the blue crescent stripe, star earring, hoodie, track pants, hi-top sneakers, keytar).
   It is a real game character:
   - skeleton: hips → chest → neck → head; two-bone IK for both legs (feet planted) and both arms (hands on
     the instrument), so there are always exactly two arms and two hands;
   - physics: hair spikes, the star earring and the hoodie strings are spring/verlet points that lag,
     swing and settle with gravity and inertia;
   - action/reaction: notes press the keys under the right finger (by pitch), velocity drives the hit,
     accents throw the head back and open the grin, the body follows the groove loop on the beat,
     idle life (breathing, blinking, looking around, weight shifts) when nothing plays;
   - every visible piece is a slot (hair, headwear, headphones, glasses, top, pants, shoes, instrument)
     drawn on the same bones, so any combination fits.
   Rendering: one 2D canvas, flat cartoon fills with a dark ink outline. No SVG masks or filters. */
(function () {
  "use strict";
  const clamp = function (v, a, b) { return Math.min(b, Math.max(a, v)); };
  const INK = "#1b1029", OL = 6;
  const SKIN = "#3fc4ea", SKIN_D = "#2a93c4", SKIN_L = "#9ae9ff";
  let TINT = null; // silhouette mode (GHOST trails, WOBBLE echo)
  const F = function (c) { return TINT || c; };

  // ---------------- slot catalogue ----------------
  const ITEMS = {
    hair: [
      { id: "spiky", name: "Spiky Nova", level: 1 },
      { id: "quiff", name: "Flame Quiff", level: 1, col: ["#1d1633", "#ff8a1e"] },
      { id: "pony", name: "Star Pony", level: 5 }
    ],
    headwear: [
      { id: "none", name: "None", level: 1 },
      { id: "beanie", name: "Sunset Beanie", level: 1, col: ["#ff7a1e", "#d9560e"] },
      { id: "cap", name: "Street Cap", level: 2, col: ["#f1dcb8", "#ff7a1e"] },
      { id: "goggles", name: "Retro Goggles", level: 4 },
      { id: "fedora", name: "Funk Fedora", level: 6, col: ["#efd9b0", "#3a2140"] },
      { id: "catbeanie", name: "Cat Beanie", level: 8, col: ["#3b2470", "#2a1850"] },
      { id: "bluebeanie", name: "Cosmic Beanie", level: 10, col: ["#3c8ed8", "#ff9a1e"] }
    ],
    headphones: [
      { id: "none", name: "None", level: 1 },
      { id: "studio", name: "Studio Cans", level: 1, col: ["#ff8a1e", "#2a2240"] },
      { id: "neon", name: "Neon Cans", level: 3, col: ["#b06bff", "#2fe0ff"] }
    ],
    glasses: [
      { id: "none", name: "None", level: 1 },
      { id: "round", name: "Amber Rounds", level: 2, col: ["#ffb21e"] },
      { id: "visor", name: "Neon Visor", level: 4, col: ["#2fe0ff"] }
    ],
    top: [
      { id: "hoodie", name: "Night Hoodie", level: 1, col: ["#2a2142", "#ff8a1e", "#e9f6f2"] },
      { id: "tee", name: "Orange Tee", level: 1, col: ["#ff8a1e", "#ffb45e", "#1b1029"] },
      { id: "bomber", name: "Cosmic Bomber", level: 3, col: ["#6a3fb8", "#ff8ad6", "#ffd23f"] }
    ],
    pants: [
      { id: "track", name: "Night Track", level: 1, col: ["#2b2452", "#ff8a1e"] },
      { id: "cargo", name: "Olive Cargo", level: 2, col: ["#5e6b3a", "#2f3a1c"] },
      { id: "purple", name: "Purple Track", level: 3, col: ["#7a2fd0", "#ffffff"] }
    ],
    shoes: [
      { id: "hitop", name: "Sunset Hi-Tops", level: 1, col: ["#f5ead6", "#ff7a1e", "#ffffff"] },
      { id: "violet", name: "Violet Hi-Tops", level: 2, col: ["#7a3ae0", "#ffffff", "#f1e8ff"] },
      { id: "red", name: "Red Runners", level: 4, col: ["#e8402a", "#1b1029", "#ffffff"] }
    ],
    instrument: [
      { id: "keytar", name: "Keytar", level: 1 },
      { id: "guitar", name: "Guitar", level: 1 },
      { id: "pads", name: "Pad Controller", level: 2 }
    ]
  };
  const DEFAULT = { hair: "spiky", headwear: "none", headphones: "none", glasses: "none", top: "hoodie", pants: "track", shoes: "hitop", instrument: "keytar" };
  function item(slot, id) { const l = ITEMS[slot]; for (let i = 0; i < l.length; i++) if (l[i].id === id) return l[i]; return l[0]; }

  // ---------------- small vector helpers ----------------
  const V = function (x, y) { return { x: x, y: y }; };
  const add = function (a, b) { return V(a.x + b.x, a.y + b.y); };
  const sub = function (a, b) { return V(a.x - b.x, a.y - b.y); };
  const mul = function (a, k) { return V(a.x * k, a.y * k); };
  const len = function (a) { return Math.hypot(a.x, a.y); };
  const rot = function (p, a) { const c = Math.cos(a), s = Math.sin(a); return V(p.x * c - p.y * s, p.x * s + p.y * c); };
  const lerp = function (a, b, t) { return V(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t); };
  // two-bone IK in 2D; picks the elbow/knee that satisfies `prefer` (a function of the candidate)
  function ik(a, t, l1, l2, prefer) {
    let d = sub(t, a), dist = clamp(len(d), Math.abs(l1 - l2) + .01, l1 + l2 - .01);
    const base = Math.atan2(d.y, d.x), cosA = clamp((l1 * l1 + dist * dist - l2 * l2) / (2 * l1 * dist), -1, 1), A = Math.acos(cosA);
    const e1 = add(a, V(Math.cos(base + A) * l1, Math.sin(base + A) * l1)), e2 = add(a, V(Math.cos(base - A) * l1, Math.sin(base - A) * l1));
    const elbow = prefer(e1) >= prefer(e2) ? e1 : e2;
    const end = add(a, mul(V(Math.cos(base), Math.sin(base)), dist));
    return { mid: elbow, end: end };
  }
  function spring(st, k, target, w, dt, kick) {
    let s = st[k]; if (!s) s = st[k] = { x: target, v: 0 };
    if (kick) s.v += kick;
    if (dt <= 0) { s.x = target; s.v = 0; return s.x; }
    const n = Math.ceil(dt / (1 / 120)), h = dt / n;
    for (let i = 0; i < n; i++) { s.v += (w * w * (target - s.x) - 2 * w * s.v) * h; s.x += s.v * h; }
    return s.x;
  }

  // ---------------- drawing primitives (flat fill + ink outline) ----------------
  function ink(g, fill, w) {
    if (fill) { g.fillStyle = F(fill); g.fill(); }
    if (!TINT) { g.lineWidth = w || OL; g.strokeStyle = INK; g.lineJoin = "round"; g.lineCap = "round"; g.stroke(); }
  }
  function detail(g, color, w, path) { if (TINT) return; g.beginPath(); path(); g.lineWidth = w; g.strokeStyle = color; g.lineCap = "round"; g.lineJoin = "round"; g.stroke(); }
  // tapered limb between two points (radius r0 at a, r1 at b)
  function capsulePath(g, a, b, r0, r1) {
    const d = sub(b, a), L = len(d) || 1e-6, ang = Math.atan2(d.y, d.x);
    g.beginPath(); g.arc(a.x, a.y, r0, ang + Math.PI / 2, ang - Math.PI / 2, false);
    g.lineTo(b.x + Math.cos(ang - Math.PI / 2) * r1, b.y + Math.sin(ang - Math.PI / 2) * r1);
    g.arc(b.x, b.y, r1, ang - Math.PI / 2, ang + Math.PI / 2, false); g.closePath();
    return L;
  }
  // run fn in a frame placed at a, with +y pointing towards b (length returned)
  function frame(g, a, b, fn) {
    const d = sub(b, a), L = len(d), ang = Math.atan2(d.y, d.x) - Math.PI / 2;
    g.save(); g.translate(a.x, a.y); g.rotate(ang); fn(L); g.restore();
  }
  function star(g, x, y, r, rot0) {
    g.beginPath();
    for (let i = 0; i < 10; i++) { const a = (rot0 || 0) - Math.PI / 2 + i * Math.PI / 5, rr = i % 2 ? r * .45 : r; g.lineTo(x + Math.cos(a) * rr, y + Math.sin(a) * rr); }
    g.closePath();
  }

  // ---------------- the character ----------------
  // groove loop: 6 poses from Gab's sheet, one per beat, blended (units: character pixels)
  const GROOVE = [
    { hx: 0, dp: 6, lean: -.05, tilt: -.05, kt: .04, lf: [0, 0], rf: [0, 0], nod: 0 },          // 1 stand, lean back a touch
    { hx: -14, dp: 16, lean: .06, tilt: .06, kt: 0, lf: [-24, 18], rf: [0, 0], nod: .06 },     // 2 step out left, heel lifted
    { hx: -6, dp: 12, lean: .1, tilt: .09, kt: -.03, lf: [-14, 6], rf: [10, 0], nod: .08 },     // 3 walk through
    { hx: 12, dp: 10, lean: .14, tilt: .12, kt: -.06, lf: [0, 0], rf: [14, 0], nod: .1 },       // 4 lean in over the keys
    { hx: 18, dp: 22, lean: .02, tilt: -.04, kt: .02, lf: [0, 0], rf: [22, 14], nod: .02 },     // 5 shift right, toe tap
    { hx: 0, dp: 58, lean: .16, tilt: .1, kt: .05, lf: [-10, 0], rf: [10, 0], nod: .12 }       // 6 crouch accent
  ];
  const sstep = function (x) { return x * x * (3 - 2 * x); };
  function groovePose(b) {
    const n = Math.floor(b), f = sstep(b - n), A = GROOVE[((n % 6) + 6) % 6], B = GROOVE[(((n + 1) % 6) + 6) % 6], o = {};
    for (const k in A) o[k] = Array.isArray(A[k]) ? A[k].map(function (v, i) { return v + (B[k][i] - v) * f; }) : A[k] + (B[k] - A[k]) * f;
    return o;
  }
  // skeleton (feet at y=0, up is negative)
  const SK = { hipY: -232, hipW: 34, thigh: 98, shin: 96, ankleH: 46, footL: V(-70, 0), footR: V(70, 0), chest: 158, shoulderW: 64, upper: 74, fore: 70, neck: 34, headR: 104 };

  function create(parent, slots) {
    const R = { root: parent, look: "nova", L: { w: 520, hip: [260, 440], feet: 668, name: "Nova", inst: "keytar" }, st: {}, t: 0, slots: Object.assign({}, DEFAULT, slots || {}), hist: [], P: null };
    const svg = parent.ownerSVGElement || parent, host = svg.parentNode;
    const canvas = document.createElement("canvas"); canvas.className = "puppet-canvas";
    canvas.style.cssText = "position:absolute;left:0;top:0;width:100%;height:100%;pointer-events:none;";
    host.appendChild(canvas); R.canvas = canvas;
    R.phys = makePhysics();
    R.blinkAt = 1.5; R.blink = 0; R.accent = 0; R.hitK = 0; R.press = 0; R.lookT = 0; R.lookAt = 3;
    R.update = function (s, dt) { pose(R, s, dt); drawToCanvas(R, s); };
    R.pose = function () { R.st = {}; R.phys = makePhysics(); pose(R, { alive: 0 }, 0); };
    R.destroy = function () { canvas.remove(); };
    R.setSlots = function (o) { R.slots = Object.assign({}, DEFAULT, o || {}); R.L.inst = R.slots.instrument; };
    R.body = { getAttribute: function () { return ""; } };
    R.renderTo = function (g, s) { if (!R.P) pose(R, s || { alive: 0 }, 0); drawCharacter(g, R, s || {}); };
    R.step = function (s, dt) { pose(R, s, dt); };
    R.L.inst = R.slots.instrument;
    pose(R, { alive: 0 }, 0);
    return R;
  }

  // physics points: hair tips, earring, hoodie strings (positions in character space)
  function makePhysics() { return { pts: {}, ready: false }; }
  function physPoint(R, key, target, k, damp, grav, dt) {
    let p = R.phys.pts[key];
    if (!p || dt <= 0) { p = R.phys.pts[key] = { x: target.x, y: target.y, vx: 0, vy: 0 }; return p; }
    const h = Math.min(dt, 1 / 30);
    p.vx += ((target.x - p.x) * k) * h; p.vy += ((target.y - p.y) * k + grav) * h;
    p.vx *= Math.pow(damp, h * 60); p.vy *= Math.pow(damp, h * 60);
    p.x += p.vx * h; p.y += p.vy * h;
    return p;
  }

  function pose(R, s, dt) {
    const st = R.st; R.t += dt;
    const amp = s.amp === undefined ? 1 : s.amp, alive = clamp((s.alive || 0) * amp, 0, 1);
    const I = clamp(s.intensity === undefined ? .8 : s.intensity, 0, 1.3) * alive;
    const style = s.style || "groove", speed = style === "half" ? .5 : 1, b = (s.beats || 0) * speed;
    const idle = 1 - clamp(alive * 2, 0, 1);
    // reactions
    if (s.hit) { const v = clamp(s.vel || .7, 0, 1); R.hitK = 1; R.press = 1; spring(st, "nodS", 0, 1, 0); st.nodS.v += 3.5 * v * Math.max(.3, amp); if (v > .8) R.accent = 1; }
    R.hitK = Math.max(0, R.hitK - dt * 3.5); R.press = Math.max(0, R.press - dt * 8); R.accent = Math.max(0, R.accent - dt * 1.6);
    // blink & idle look-around
    if (R.t > R.blinkAt) { R.blink = 1; R.blinkAt = R.t + 2.2 + Math.random() * 3.2; }
    R.blink = Math.max(0, R.blink - dt * 7);
    if (R.t > R.lookAt) { R.lookT = (Math.random() - .5) * 1.4 * idle; R.lookAt = R.t + 1.5 + Math.random() * 2.5; }
    const G = groovePose(b);
    const bounce = Math.pow(.5 + .5 * Math.cos(2 * Math.PI * b), 2);
    const breathe = Math.sin(R.t * 1.7);
    const hipX = spring(st, "hx", G.hx * I + Math.sin(R.t * .4) * 5 * idle, 9, dt);
    const dip = spring(st, "dp", (G.dp + 8 * bounce) * I + 6 * R.hitK * amp, 13, dt);
    const lean = spring(st, "ln", G.lean * I + R.accent * -.1 * amp, 8, dt);
    const look = spring(st, "lk", clamp((s.look || 0) + R.lookT, -1, 1), 5, dt);
    const nod = spring(st, "nodS", G.nod * I + .05 * Math.sin(2 * Math.PI * b - .6) * I, 11, dt);
    const tilt = spring(st, "tl", G.tilt * I + Math.sin(b * Math.PI) * .05 * I + look * .06, 8, dt);
    const kt = spring(st, "kt", G.kt * I - R.accent * .12 * amp, 9, dt);
    const P = R.P = {};
    P.hip = V(hipX, SK.hipY + dip + breathe * .8);
    // feet (IK targets)
    const fl = V(SK.footL.x + G.lf[0] * I, -Math.max(0, G.lf[1] * I)), fr = V(SK.footR.x + G.rf[0] * I, -Math.max(0, G.rf[1] * I));
    P.footL = V(spring(st, "flx", fl.x, 12, dt), spring(st, "fly", fl.y, 15, dt));
    P.footR = V(spring(st, "frx", fr.x, 12, dt), spring(st, "fry", fr.y, 15, dt));
    P.footLiftL = -P.footL.y; P.footLiftR = -P.footR.y;
    const hipL = add(P.hip, V(-SK.hipW, 0)), hipR = add(P.hip, V(SK.hipW, 0));
    const anL = add(P.footL, V(0, -SK.ankleH)), anR = add(P.footR, V(0, -SK.ankleH));
    const legL = ik(hipL, anL, SK.thigh, SK.shin, function (k) { return -k.x + k.y * .1; });
    const legR = ik(hipR, anR, SK.thigh, SK.shin, function (k) { return k.x + k.y * .1; });
    P.legL = [hipL, legL.mid, legL.end]; P.legR = [hipR, legR.mid, legR.end];
    // chest (rotates over the hips), breathing
    P.lean = lean; P.chestTop = add(P.hip, rot(V(0, -SK.chest - breathe * 1.5), lean));
    P.shL = add(P.hip, rot(V(-SK.shoulderW, -SK.chest + 18), lean)); P.shR = add(P.hip, rot(V(SK.shoulderW, -SK.chest + 18), lean));
    // head
    P.headAng = lean * .6 + tilt - R.accent * .22 * amp;
    P.neck = P.chestTop;
    P.head = add(P.neck, rot(V(4, -SK.headR + 6 - nod * 30), P.headAng));
    P.look = look; P.nod = nod;
    // instrument frame (strapped to the chest)
    const inst = R.slots.instrument;
    if (inst === "guitar") { P.instC = add(P.hip, rot(V(-8, -70), lean)); P.instA = lean - .55 + kt; }
    else if (inst === "pads") { P.instC = add(P.hip, rot(V(4, -46), lean)); P.instA = lean * .4; }
    else { P.instC = add(P.hip, rot(V(0, -78), lean)); P.instA = lean - .52 + kt; }
    // hands: left (screen-left arm) plays by pitch, right holds the neck (keytar/guitar) or plays too (pads)
    const pitch = clamp(s.pitch === undefined ? .5 : s.pitch, 0, 1), playing = !!s.playing;
    const keyX = spring(st, "kx", -70 + 120 * pitch, 10, dt);
    const press = R.press * (playing ? 1 : .4) * Math.max(.3, amp);
    let tl, tr;
    const inF = function (p) { return add(P.instC, rot(p, P.instA)); };
    if (inst === "pads") {
      tl = inF(V(-40 + 30 * pitch, -18 + 10 * press)); tr = inF(V(30 + 20 * Math.sin(R.t * 3) * alive, -16 + 8 * press * (R.hitK > .5 ? 1 : 0)));
    } else if (inst === "guitar") {
      tl = inF(V(-60, 6 + 14 * Math.sin(R.t * 18) * press)); tr = inF(V(110 + 30 * pitch, -6));
    } else {
      tl = inF(V(keyX, -26 + 10 * press)); tr = inF(V(150, 4));
    }
    tl.y += Math.sin(R.t * 10) * 2 * (playing ? alive : 0);
    const armL = ik(P.shL, tl, SK.upper, SK.fore, function (k) { return k.y - k.x * .4; });
    const armR = ik(P.shR, tr, SK.upper, SK.fore, function (k) { return k.y + k.x * .4; });
    P.armL = [P.shL, armL.mid, armL.end]; P.armR = [P.shR, armR.mid, armR.end];
    P.press = press;
    // expressions
    P.blink = R.blink; P.smile = .6 + .4 * alive; P.open = clamp(R.accent * .9 + R.hitK * .35 * amp, 0, 1); P.squint = clamp(R.accent * .6 + I * .2, 0, .7);
    // physics: hair tips, earring, strings
    const hTr = function (p) { return add(P.head, rot(p, P.headAng)); };
    P.hairTips = HAIR_TIPS.map(function (t, i) {
      const target = hTr(t.tip), p = physPoint(R, "h" + i, target, t.k, .86, 180, dt);
      return { base: hTr(t.base), tip: V(p.x, p.y), w: t.w, back: t.back };
    });
    const ear = hTr(V(-92, 34)), e1 = physPoint(R, "ear", add(ear, V(0, 26)), 70, .9, 900, dt);
    const ev = sub(V(e1.x, e1.y), ear); const el = len(ev) || 1; P.ear = ear; P.earStar = add(ear, mul(ev, 26 / el));
    const sA = add(P.hip, rot(V(-16, -SK.chest + 26), lean)), sB = add(P.hip, rot(V(14, -SK.chest + 26), lean));
    P.strings = [sA, sB].map(function (a, i) { const p = physPoint(R, "s" + i, add(a, V(0, 46)), 60, .88, 700, dt); const v = sub(V(p.x, p.y), a), l = len(v) || 1; return [a, add(a, mul(v, 46 / l))]; });
    P.alive = alive; P.I = I; P.idle = idle;
    if (dt > 0) { R.hist.unshift(JSON.parse(JSON.stringify(P))); if (R.hist.length > 36) R.hist.pop(); }
  }
  // hair: spike tips in head space (head centre = 0,0; radius ≈ 104). k = stiffness (lower = floppier)
  const HAIR_TIPS = [
    { base: V(-80, -50), tip: V(-150, -20), w: 40, k: 220, back: true },   // left side flick
    { base: V(-86, -10), tip: V(-118, 70), w: 30, k: 180, back: true },    // sideburn lock by the ear
    { base: V(-60, -95), tip: V(-140, -120), w: 38, k: 260, back: true },
    { base: V(0, -110), tip: V(70, -235), w: 56, k: 300, back: true },     // big swoop up
    { base: V(40, -105), tip: V(150, -175), w: 52, k: 270, back: true },
    { base: V(80, -70), tip: V(175, -70), w: 44, k: 240, back: true },
    { base: V(90, -20), tip: V(160, 50), w: 36, k: 200, back: true },
    { base: V(84, 20), tip: V(118, 110), w: 26, k: 170, back: true },      // long lock on the right
    { base: V(20, -90), tip: V(120, -30), w: 40, k: 230, back: false },    // bangs over the forehead
    { base: V(56, -70), tip: V(126, 40), w: 30, k: 200, back: false }
  ];

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
    drawCharacter(g, R, s);
  }
  function drawCharacter(g, R, s) {
    const P = R.P;
    // reflection on the platform
    if (s.reflect) { g.save(); g.beginPath(); g.rect(-400, 2, 800, 240); g.clip(); g.translate(0, 4); g.scale(1, -1); g.globalAlpha = .14; body(g, R, P); g.restore(); }
    // contact shadow
    g.save(); g.globalAlpha = .45; g.fillStyle = "#000"; g.beginPath(); g.ellipse(P.hip.x * .5, 4, 150 - Math.abs(P.hip.x) * .3, 14, 0, 0, Math.PI * 2); g.fill(); g.restore();
    // aura (sound colour, BLOOM)
    const ga = clamp(s.glowAmt || 0, 0, 1), col = s.glow || "#5fe3ff", bloom = clamp(s.bloom || 0, 0, 1);
    if (ga + bloom > .02) { g.save(); g.translate(0, -320); g.scale(1, 1.25); const gr = g.createRadialGradient(0, 0, 20, 0, 0, 300 * (1 + bloom * .4)); gr.addColorStop(0, col); gr.addColorStop(1, "rgba(0,0,0,0)"); g.globalAlpha = (.12 + ga * .25 + bloom * .45); g.globalCompositeOperation = "lighter"; g.fillStyle = gr; g.beginPath(); g.arc(0, 0, 300 * (1 + bloom * .4), 0, Math.PI * 2); g.fill(); g.restore(); }
    // GHOST: violet afterimages of earlier poses
    if (s.ghost > .02 && R.hist.length) {
      const sp = Math.max(1, s.ghostSpacing || 4);
      for (let i = 3; i >= 0; i--) { const h = R.hist[Math.min(R.hist.length - 1, (i + 1) * sp)]; if (!h) continue; g.save(); g.globalAlpha = clamp(s.ghost * (.5 - i * .1), 0, .6); g.translate(-(i + 1) * 14 * s.ghost, 0); TINT = "#b06bff"; body(g, R, h); TINT = null; g.restore(); }
    }
    // WOBBLE: cyan echo that sways
    const wob = clamp(s.wob || 0, 0, 1);
    if (wob > .02) { g.save(); g.globalAlpha = wob * .5; g.translate(Math.sin(R.t * (3 + (s.wobRate || 1) * 2)) * (10 + wob * 16), 0); TINT = "#30e0ff"; body(g, R, P); TINT = null; g.restore(); }
    // sound-coloured rim glow
    if (ga > .05) { g.save(); g.globalAlpha = ga * .55; g.shadowColor = col; g.shadowBlur = 30; TINT = col; body(g, R, P); TINT = null; g.restore(); }
    body(g, R, P);
  }

  // ---------------- the body, back to front ----------------
  function body(g, R, P) {
    const S = R.slots;
    hairBack(g, P, S);
    legs(g, P, S);
    torso(g, P, S);
    if (S.instrument === "pads") { instrument(g, P, S); arm(g, P.armL, S, true); arm(g, P.armR, S, false); head(g, P, S); }
    else { upperArm(g, P.armR, S); head(g, P, S); instrument(g, P, S); arm(g, P.armL, S, true); foreArm(g, P.armR, S); }
  }

  function legs(g, P, S) {
    const pc = item("pants", S.pants).col, sc = item("shoes", S.shoes).col;
    [[P.legR, P.footR, 1], [P.legL, P.footL, -1]].forEach(function (q) {
      const L = q[0], ft = q[1], side = q[2];
      // baggy pants: thigh + shin, bunched at the ankle
      capsulePath(g, L[0], L[1], 40, 36); ink(g, pc[0]);
      capsulePath(g, L[1], L[2], 36, 40); ink(g, pc[0]);
      frame(g, L[1], L[2], function (l) { g.beginPath(); g.ellipse(0, l - 8, 44, 18, 0, 0, Math.PI * 2); ink(g, pc[0], 5); detail(g, "rgba(0,0,0,.25)", 3, function () { g.moveTo(-30, l - 14); g.quadraticCurveTo(0, l - 2, 30, l - 14); }); });
      // side stripe
      if (S.pants !== "cargo") { const so = side * 30; detail(g, pc[1], 9, function () { const a = lerp(L[0], L[1], .1), m = L[1], b = L[2]; g.moveTo(a.x + so, a.y); g.lineTo(m.x + so * .95, m.y); g.lineTo(b.x + so * .9, b.y - 6); }); }
      else frame(g, L[0], L[1], function (l) { g.beginPath(); g.roundRect ? g.roundRect(side * 6 - 16, l * .35, 34, 34, 6) : g.rect(side * 6 - 16, l * .35, 34, 34); ink(g, pc[1], 4); });
      shoe(g, ft, side, sc, P);
    });
  }
  function shoe(g, ft, side, c, P) {
    g.save(); g.translate(ft.x, ft.y);
    const lift = side < 0 ? P.footLiftL : P.footLiftR; g.rotate(side * -.002 * lift * 6);
    g.scale(side, 1);
    // sole
    g.beginPath(); g.moveTo(-44, -2); g.quadraticCurveTo(-48, -22, -36, -26); g.lineTo(52, -24); g.quadraticCurveTo(70, -20, 66, -2); g.closePath(); ink(g, c[2]);
    // upper (hi-top) pointing outwards
    g.beginPath(); g.moveTo(-38, -24); g.lineTo(-36, -70); g.quadraticCurveTo(-30, -82, -12, -82); g.lineTo(6, -80); g.quadraticCurveTo(14, -54, 30, -46); g.quadraticCurveTo(64, -38, 62, -24); g.closePath(); ink(g, c[0]);
    // toe cap + spots / star
    g.beginPath(); g.moveTo(28, -26); g.quadraticCurveTo(32, -44, 52, -40); g.quadraticCurveTo(64, -34, 62, -26); g.closePath(); ink(g, c[1], 4);
    if (!TINT) { g.fillStyle = c[1]; [[-24, -52, 9], [-6, -64, 6], [10, -40, 7]].forEach(function (o) { g.beginPath(); g.arc(o[0], o[1], o[2], 0, Math.PI * 2); g.fill(); }); }
    detail(g, "#fff8ec", 4, function () { for (let i = 0; i < 3; i++) { g.moveTo(-2 + i * 8, -72 + i * 9); g.lineTo(14 + i * 8, -66 + i * 9); } });
    detail(g, INK, 3, function () { g.moveTo(-40, -30); g.lineTo(60, -28); });
    g.restore();
  }
  function torso(g, P, S) {
    const tc = item("top", S.top).col, top = S.top;
    g.save(); g.translate(P.hip.x, P.hip.y); g.rotate(P.lean);
    const H = SK.chest;
    // hoodie / top body
    g.beginPath(); g.moveTo(-54, 6); g.quadraticCurveTo(-74, -H * .5, -66, -H + 14); g.quadraticCurveTo(0, -H - 14, 66, -H + 14); g.quadraticCurveTo(74, -H * .5, 54, 6); g.quadraticCurveTo(0, 18, -54, 6); g.closePath();
    ink(g, tc[0]);
    if (top === "hoodie") {
      // open front: tee with stripe + zip
      g.beginPath(); g.moveTo(-20, 4); g.lineTo(-24, -H + 10); g.quadraticCurveTo(0, -H + 26, 24, -H + 10); g.lineTo(20, 4); g.closePath(); ink(g, tc[2], 4);
      detail(g, tc[1], 7, function () { g.moveTo(-8, -H + 26); g.lineTo(-8, 2); });
      detail(g, "#3fc4ea", 4, function () { g.moveTo(6, -H + 24); g.lineTo(6, 2); });
      // orange side panels + hem
      detail(g, tc[1], 10, function () { g.moveTo(-60, -10); g.quadraticCurveTo(-68, -H * .5, -60, -H + 24); });
      detail(g, tc[1], 10, function () { g.moveTo(60, -10); g.quadraticCurveTo(68, -H * .5, 60, -H + 24); });
      g.beginPath(); g.moveTo(-56, 0); g.quadraticCurveTo(0, 14, 56, 0); g.lineTo(54, 10); g.quadraticCurveTo(0, 26, -54, 10); g.closePath(); ink(g, tc[0], 4);
      // hood around the neck
      g.beginPath(); g.moveTo(-58, -H + 16); g.quadraticCurveTo(-40, -H - 22, 0, -H - 20); g.quadraticCurveTo(40, -H - 22, 58, -H + 16); g.quadraticCurveTo(30, -H, 0, -H + 2); g.quadraticCurveTo(-30, -H, -58, -H + 16); g.closePath(); ink(g, tc[0], 5);
    } else if (top === "tee") {
      detail(g, tc[1], 8, function () { g.moveTo(-30, -H * .55); g.lineTo(30, -H * .55); });
      if (!TINT) { star(g, 0, -H * .78, 16, 0); g.fillStyle = tc[2]; g.fill(); }
    } else {
      // bomber: pattern + ribbed hem and collar
      if (!TINT) { g.save(); g.clip(); for (let i = 0; i < 6; i++) { g.beginPath(); g.ellipse(-50 + i * 22, -H * .3 - (i % 2) * 40, 16, 26, .6, 0, Math.PI * 2); g.fillStyle = i % 2 ? tc[1] : tc[2]; g.globalAlpha = .55; g.fill(); } g.restore(); }
      detail(g, INK, 4, function () { g.moveTo(0, -H + 8); g.lineTo(0, 6); });
      g.beginPath(); g.rect(-56, -4, 112, 14); ink(g, "#2a1850", 4);
    }
    g.restore();
    // hoodie strings (physics)
    if (top === "hoodie") P.strings.forEach(function (s) { detail(g, INK, 8, function () { g.moveTo(s[0].x, s[0].y); g.lineTo(s[1].x, s[1].y); }); detail(g, "#f5ead6", 4, function () { g.moveTo(s[0].x, s[0].y); g.lineTo(s[1].x, s[1].y); }); });
  }
  function sleeveCol(S) { const t = item("top", S.top); return S.top === "tee" ? null : t.col[0]; }
  function upperArm(g, A, S) { const c = sleeveCol(S); capsulePath(g, A[0], A[1], 28, 25); ink(g, c || SKIN); if (S.top === "tee") { capsulePath(g, A[0], lerp(A[0], A[1], .55), 30, 27); ink(g, item("top", S.top).col[0]); } }
  function foreArm(g, A, S) {
    const c = sleeveCol(S), tc = item("top", S.top).col;
    capsulePath(g, A[1], A[2], 25, 20); ink(g, c || SKIN);
    if (c) { frame(g, A[1], A[2], function (l) { g.beginPath(); g.ellipse(0, l - 8, 25, 13, 0, 0, Math.PI * 2); ink(g, S.top === "hoodie" ? tc[1] : tc[2], 4); }); }
    hand(g, A);
  }
  function arm(g, A, S, front) { upperArm(g, A, S); foreArm(g, A, S); }
  function hand(g, A) {
    frame(g, A[1], A[2], function (l) {
      g.translate(0, l + 4);
      // palm + four cartoon fingers + thumb
      g.beginPath(); g.ellipse(0, 6, 22, 20, 0, 0, Math.PI * 2); ink(g, SKIN);
      for (let i = 0; i < 4; i++) { const x = -15 + i * 10; g.beginPath(); g.ellipse(x, 26, 6, 14, (i - 1.5) * .12, 0, Math.PI * 2); ink(g, SKIN, 4); }
      g.beginPath(); g.ellipse(22, 8, 8, 12, -.6, 0, Math.PI * 2); ink(g, SKIN, 4);
    });
  }

  function hairShape(g, base, tip, w) {
    const d = sub(tip, base), L = len(d) || 1, n = V(-d.y / L, d.x / L);
    const a = add(base, mul(n, w * .5)), b = add(base, mul(n, -w * .5)), c1 = add(lerp(base, tip, .55), mul(n, w * .35)), c2 = add(lerp(base, tip, .55), mul(n, -w * .1));
    g.moveTo(a.x, a.y); g.quadraticCurveTo(c1.x, c1.y, tip.x, tip.y); g.quadraticCurveTo(c2.x, c2.y, b.x, b.y);
  }
  const HAIR = "#1d1633", HAIR_L = "#4b2e78";
  function hairCol(S) { return S.hair === "quiff" ? item("hair", "quiff").col : [HAIR, HAIR_L]; }
  function hairBack(g, P, S) {
    if (S.hair === "bob") { g.save(); g.translate(P.head.x, P.head.y); g.rotate(P.headAng); g.beginPath(); g.ellipse(-4, 4, 132, 124, 0, 0, Math.PI * 2); ink(g, HAIR); g.restore(); return; }
    const hc = hairCol(S);
    g.save(); g.translate(P.head.x, P.head.y); g.rotate(P.headAng); g.beginPath(); g.ellipse(0, -18, 118, 112, 0, 0, Math.PI * 2); ink(g, hc[0]); g.restore();
    P.hairTips.forEach(function (t) { if (!t.back) return; g.beginPath(); hairShape(g, t.base, t.tip, t.w * 1.6); g.closePath(); ink(g, hc[0], 5); detail(g, hc[1], 5, function () { const m = lerp(t.base, t.tip, .25), e = lerp(t.base, t.tip, .75); g.moveTo(m.x, m.y); g.lineTo(e.x, e.y); }); });
    if (S.hair === "pony") { const a = add(P.head, rot(V(-90, -40), P.headAng)), tip = P.hairTips[1].tip; g.beginPath(); hairShape(g, a, add(tip, V(-20, 60)), 60); g.closePath(); ink(g, hc[0]); }
  }
  function head(g, P, S) {
    const hw = S.headwear, hp = item("headphones", S.headphones), gl = item("glasses", S.glasses), hc = hairCol(S);
    g.save(); g.translate(P.head.x, P.head.y); g.rotate(P.headAng);
    const lk = P.look * 18; // pseudo turn: face features slide toward the look direction
    const hatOn = hw === "beanie" || hw === "bluebeanie" || hw === "catbeanie" || hw === "cap" || hw === "fedora";
    if (!hatOn) {
    // wide crescent crest (behind the face edge, hidden under hats) rising from the forehead and curling back (Nova's signature)
    g.beginPath(); g.moveTo(10 + lk * .5, -40); g.bezierCurveTo(30, -150, -20, -230, -120, -230); g.bezierCurveTo(-60, -200, -50, -120, -48 + lk * .4, -36); g.quadraticCurveTo(-20, -30, 10 + lk * .5, -40); g.closePath(); ink(g, SKIN, 5);
    if (!TINT) { g.save(); g.clip(); g.fillStyle = SKIN_L; g.globalAlpha = .55; g.beginPath(); g.ellipse(-10, -150, 18, 60, .4, 0, Math.PI * 2); g.fill(); g.globalAlpha = 1; g.restore();
      g.fillStyle = "#ff7a2e"; [[-34, -170, 11], [-16, -128, 10], [-12, -86, 8], [-62, -200, 7], [-36, -112, 6]].forEach(function (o) { g.beginPath(); g.arc(o[0], o[1], o[2], 0, Math.PI * 2); g.fill(); g.lineWidth = 3; g.strokeStyle = INK; g.stroke(); }); }
    }
    // face (round top, full cheeks, small pointed chin)
    g.beginPath(); g.moveTo(-96, -10); g.bezierCurveTo(-104, -100, 96, -112, 100, -16); g.bezierCurveTo(104, 40, 60, 92, 14 + lk * .3, 104); g.bezierCurveTo(-30, 100, -94, 56, -96, -10); g.closePath(); ink(g, SKIN);
    if (!TINT) { g.save(); g.clip(); g.fillStyle = SKIN_D; g.beginPath(); g.ellipse(70 + lk * .4, 40, 60, 90, -.3, 0, Math.PI * 2); g.globalAlpha = .35; g.fill(); g.restore(); }
    // crescent stripe with orange spots (from the forehead over the top)
    // ear + star earring (physics)
    g.beginPath(); g.ellipse(-96, 20, 14, 22, -.3, 0, Math.PI * 2); ink(g, SKIN, 5);
    g.restore();
    detail(g, INK, 3, function () { g.moveTo(P.ear.x, P.ear.y); g.lineTo(P.earStar.x, P.earStar.y); });
    star(g, P.earStar.x, P.earStar.y, 16, 0); ink(g, "#ffd23f", 4);
    g.save(); g.translate(P.head.x, P.head.y); g.rotate(P.headAng);
    // eyes
    eye(g, -34 + lk, 0, 1, P, gl.id !== "visor"); eye(g, 44 + lk, -4, -1, P, gl.id !== "visor");
    // grin with teeth; opens on accents
    const op = P.open;
    g.beginPath(); g.moveTo(-14 + lk, 58); g.quadraticCurveTo(18 + lk, 74 + op * 22, 50 + lk, 52); g.quadraticCurveTo(18 + lk, 62 + op * 6, -14 + lk, 58); g.closePath(); ink(g, "#4a1030", 4);
    if (!TINT) { g.fillStyle = "#ffffff"; g.beginPath(); g.moveTo(0 + lk, 60); g.lineTo(10 + lk, 60 + 8 + op * 6); g.lineTo(16 + lk, 61); g.closePath(); g.fill(); g.beginPath(); g.moveTo(26 + lk, 61); g.lineTo(32 + lk, 61 + 8 + op * 6); g.lineTo(40 + lk, 58); g.closePath(); g.fill(); }
    // front hair over the forehead
    g.restore();
    P.hairTips.forEach(function (t) { if (t.back) return; g.beginPath(); hairShape(g, t.base, t.tip, t.w * 1.5); g.closePath(); ink(g, hc[0], 5); detail(g, hc[1], 5, function () { const m = lerp(t.base, t.tip, .3), e = lerp(t.base, t.tip, .7); g.moveTo(m.x, m.y); g.lineTo(e.x, e.y); }); });
    g.save(); g.translate(P.head.x, P.head.y); g.rotate(P.headAng);
    if (S.hair === "quiff") { g.beginPath(); g.moveTo(-60, -80); g.bezierCurveTo(-40, -200, 120, -200, 90, -110); g.bezierCurveTo(60, -140, 0, -130, -60, -80); g.closePath(); ink(g, hc[0]); detail(g, hc[1], 8, function () { g.moveTo(-30, -110); g.bezierCurveTo(0, -170, 70, -170, 80, -130); }); }
    if (S.hair === "bob") { g.beginPath(); g.moveTo(-110, 40); g.bezierCurveTo(-130, -140, 130, -150, 112, 30); g.bezierCurveTo(90, -60, 10, -90, -40, -60); g.bezierCurveTo(-80, -40, -96, 0, -110, 40); g.closePath(); ink(g, HAIR); }
    // glasses
    if (gl.id === "round") { [-36 + lk, 42 + lk].forEach(function (x) { g.beginPath(); g.arc(x, 6, 30, 0, Math.PI * 2); g.globalAlpha = .45; g.fillStyle = F(gl.col[0]); g.fill(); g.globalAlpha = 1; g.lineWidth = 6; g.strokeStyle = F(INK); g.stroke(); }); detail(g, INK, 6, function () { g.moveTo(-6 + lk, 4); g.lineTo(12 + lk, 4); }); }
    if (gl.id === "visor") { g.beginPath(); g.moveTo(-90, -20); g.quadraticCurveTo(10 + lk, -36, 100, -24); g.lineTo(96, 30); g.quadraticCurveTo(10 + lk, 40, -86, 26); g.closePath(); ink(g, "#120a24"); detail(g, gl.col[0], 6, function () { g.moveTo(-70, 4); g.quadraticCurveTo(10 + lk, -6, 84, 2); }); }
    // headphones
    if (hp.id !== "none") {
      detail(g, INK, 22, function () { g.moveTo(-108, 6); g.bezierCurveTo(-120, -170, 120, -170, 108, 0); });
      detail(g, hp.col[0], 13, function () { g.moveTo(-108, 6); g.bezierCurveTo(-120, -170, 120, -170, 108, 0); });
      [[-110, 14], [110, 6]].forEach(function (c) { g.beginPath(); g.ellipse(c[0], c[1], 26, 40, 0, 0, Math.PI * 2); ink(g, hp.col[0]); g.beginPath(); g.ellipse(c[0], c[1], 14, 24, 0, 0, Math.PI * 2); ink(g, hp.col[1], 4); });
    }
    // headwear (one slot: never two hats)
    const hi = item("headwear", hw);
    if (hw === "beanie" || hw === "bluebeanie" || hw === "catbeanie") {
      g.beginPath(); g.moveTo(-104, -56); g.bezierCurveTo(-110, -190, 110, -200, 108, -60); g.closePath(); ink(g, hi.col[0]);
      if (!TINT) { g.save(); g.clip(); g.strokeStyle = hi.col[1]; g.lineWidth = 4; for (let x = -100; x < 110; x += 16) { g.beginPath(); g.moveTo(x, -50); g.lineTo(x + 6, -190); g.stroke(); } g.restore(); }
      g.beginPath(); g.moveTo(-112, -40); g.quadraticCurveTo(0, -84, 114, -42); g.lineTo(112, -76); g.quadraticCurveTo(0, -118, -110, -74); g.closePath(); ink(g, hi.col[1]);
      if (hw === "bluebeanie") { star(g, 50, -130, 26, .2); ink(g, hi.col[1], 4); }
      if (hw === "catbeanie") { [[-70, -150, -1], [70, -152, 1]].forEach(function (e) { g.beginPath(); g.moveTo(e[0] - 26, e[1] + 24); g.lineTo(e[0] + e[2] * 10, e[1] - 50); g.lineTo(e[0] + 30, e[1] + 22); g.closePath(); ink(g, hi.col[0]); }); }
    } else if (hw === "cap") {
      g.beginPath(); g.moveTo(-100, -60); g.bezierCurveTo(-100, -180, 100, -180, 102, -60); g.closePath(); ink(g, hi.col[0]);
      g.beginPath(); g.moveTo(30, -66); g.quadraticCurveTo(140, -76, 170, -40); g.quadraticCurveTo(110, -40, 40, -50); g.closePath(); ink(g, hi.col[1]);
      g.beginPath(); g.moveTo(-100, -60); g.bezierCurveTo(-90, -140, -20, -170, 10, -150); g.lineTo(0, -64); g.closePath(); ink(g, hi.col[1], 4);
    } else if (hw === "goggles") {
      detail(g, INK, 18, function () { g.moveTo(-106, -70); g.quadraticCurveTo(0, -120, 106, -70); });
      detail(g, "#ff7a2e", 10, function () { g.moveTo(-106, -70); g.quadraticCurveTo(0, -120, 106, -70); });
      [[-36, -96, "#ffd23f"], [40, -98, "#2fe0ff"]].forEach(function (o) { g.beginPath(); g.arc(o[0], o[1], 28, 0, Math.PI * 2); ink(g, "#3a2a5a"); g.beginPath(); g.arc(o[0], o[1], 18, 0, Math.PI * 2); ink(g, o[2], 4); });
    } else if (hw === "fedora") {
      g.beginPath(); g.ellipse(4, -78, 160, 30, -.05, 0, Math.PI * 2); ink(g, hi.col[0]);
      g.beginPath(); g.moveTo(-84, -84); g.bezierCurveTo(-90, -200, 96, -206, 92, -86); g.quadraticCurveTo(0, -60, -84, -84); g.closePath(); ink(g, hi.col[0]);
      g.beginPath(); g.moveTo(-86, -100); g.quadraticCurveTo(0, -78, 92, -102); g.lineTo(92, -122); g.quadraticCurveTo(0, -98, -86, -120); g.closePath(); ink(g, hi.col[1], 4);
    }
    g.restore();
  }
  function eye(g, x, y, side, P, show) {
    if (!show) return;
    g.save(); g.translate(x, y); g.rotate(side * .16); g.scale(1.25, 1.25);
    const open = 1 - Math.sin(Math.min(1, P.blink) * Math.PI) * .95, sq = P.squint;
    // eyeshadow
    if (!TINT) { g.fillStyle = "rgba(122,60,200,.5)"; g.beginPath(); g.ellipse(-2, 22, 24, 6, 0, 0, Math.PI * 2); g.fill(); }
    g.save(); g.scale(1, Math.max(.06, open * (1 - sq * .45)));
    g.beginPath(); g.moveTo(-30, 4); g.quadraticCurveTo(-4, -32, 32, -6); g.quadraticCurveTo(10, 26, -30, 4); g.closePath();
    if (!TINT) { const gr = g.createLinearGradient(0, -24, 0, 18); gr.addColorStop(0, "#ffd84a"); gr.addColorStop(1, "#ff8a1e"); g.fillStyle = gr; } else g.fillStyle = TINT;
    g.fill();
    if (!TINT) { g.save(); g.clip(); g.fillStyle = INK; g.beginPath(); g.ellipse(6 + P.look * 8, -2, 10, 17, 0, 0, Math.PI * 2); g.fill(); g.fillStyle = "#fff"; g.beginPath(); g.arc(1 + P.look * 8, -9, 4, 0, Math.PI * 2); g.fill(); g.restore(); }
    g.lineWidth = 4; g.strokeStyle = F(INK); g.stroke();
    g.restore();
    // heavy upper lash line
    detail(g, INK, 8, function () { const yy = -14 * Math.max(.1, open) * (1 - sq * .4); g.moveTo(-34, 6); g.quadraticCurveTo(-6, yy - 18, 36, -6 + (1 - open) * 8); });
    g.restore();
  }

  function instrument(g, P, S) {
    const kind = S.instrument;
    g.save(); g.translate(P.instC.x, P.instC.y); g.rotate(P.instA);
    if (kind === "keytar") {
      // body: rounded left end, long neck to the right
      g.beginPath(); g.moveTo(-150, -34); g.lineTo(60, -34); g.lineTo(120, -18); g.lineTo(200, -16); g.lineTo(206, 10); g.lineTo(120, 12); g.lineTo(60, 34); g.lineTo(-150, 34); g.quadraticCurveTo(-176, 0, -150, -34); g.closePath(); ink(g, "#ff7a2e");
      g.beginPath(); g.rect(-136, -26, 196, 48); ink(g, "#f5ead6", 4);
      // keys (white + black), the key under the playing finger lights up
      if (!TINT) {
        const n = 14, w = 196 / n;
        for (let i = 0; i < n; i++) { g.fillStyle = "#fffaf0"; g.fillRect(-136 + i * w + 1, -24, w - 2, 44); }
        g.fillStyle = INK; for (let i = 0; i < n - 1; i++) { const k = i % 7; if (k === 2 || k === 6) continue; g.fillRect(-136 + (i + 1) * w - w * .3, -24, w * .6, 26); }
      }
      detail(g, INK, 3, function () { g.moveTo(-136, 26); g.lineTo(60, 26); });
      if (!TINT) { g.fillStyle = "#2fe0ff"; [[150, -2], [172, -2]].forEach(function (o) { g.beginPath(); g.arc(o[0], o[1], 6, 0, Math.PI * 2); g.fill(); }); g.fillStyle = "#1b1029"; g.fillRect(-150, 26, 12, 6); }
      // pressed key glow
      if (!TINT && P.press > .05) { const kx = clamp(P.armL[2].x, -1e9, 1e9); g.globalAlpha = P.press * .8; g.fillStyle = "#ffd23f"; g.beginPath(); const local = rot(sub(P.armL[2], P.instC), -P.instA); g.ellipse(local.x, -4, 12, 20, 0, 0, Math.PI * 2); g.fill(); g.globalAlpha = 1; void kx; }
    } else if (kind === "guitar") {
      g.beginPath(); g.moveTo(-40, -40); g.bezierCurveTo(-130, -70, -170, 0, -120, 40); g.bezierCurveTo(-90, 70, -30, 60, -10, 34); g.bezierCurveTo(10, 10, 0, -30, -40, -40); g.closePath(); ink(g, "#e8402a");
      g.beginPath(); g.rect(-10, -9, 210, 18); ink(g, "#7a4a2a", 5);
      g.beginPath(); g.moveTo(200, -14); g.lineTo(236, -18); g.lineTo(236, 18); g.lineTo(200, 14); g.closePath(); ink(g, "#e8402a", 5);
      g.beginPath(); g.arc(-70, 4, 16, 0, Math.PI * 2); ink(g, INK, 3);
      detail(g, "#f5ead6", 2, function () { for (let i = -1; i <= 1; i++) { g.moveTo(-100, i * 4); g.lineTo(230, i * 4); } });
    } else {
      g.beginPath(); if (g.roundRect) g.roundRect(-110, -36, 220, 72, 12); else g.rect(-110, -36, 220, 72); ink(g, "#2a2142");
      const cols = ["#ff7a2e", "#ffd23f", "#2fe0ff", "#b06bff", "#ff4fd8", "#7bff8a", "#ff7a2e", "#2fe0ff"];
      for (let i = 0; i < 8; i++) { const x = -96 + (i % 4) * 34, y = -26 + Math.floor(i / 4) * 30; g.beginPath(); if (g.roundRect) g.roundRect(x, y, 28, 24, 5); else g.rect(x, y, 28, 24); ink(g, cols[i], 3); }
      g.beginPath(); g.arc(70, -6, 20, 0, Math.PI * 2); ink(g, "#4b3a7a", 4);
    }
    g.restore();
  }

  window.RastroNova = { create: create, ITEMS: ITEMS, DEFAULT: DEFAULT, item: item };
})();
