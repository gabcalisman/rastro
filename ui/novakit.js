// RASTRO PLAY — NOVA modular (Gab's modular kit).
// One articulated base in "model space" (the kit's pivot sheet, 1024 × 1460 px): every piece (nk_*.png) was cut from
// the kit sheets and baked once into that space at the rest pose, with its bone written in novakit_data.js.
// At run time each piece only follows its bone: torso, head, upper arm, forearm, hand, leg (hip→ankle), foot,
// instrument. Changing an item changes only that slot. Skin (neck, torso, arms, legs) is drawn under the clothes.
// Same API as novaart.js: create(parent, slots) → {update, step, renderTo, setSlots, pose, destroy, L, canvas}.
(function () {
  "use strict";
  const D = window.NOVAKIT_DATA, P = D.pivots, IT = D.items;
  const S = .5, OX = 513, OY = 1373;                         // model px → character units (feet at 0)
  const SKIN = "#45cbe6", SKIN_D = "#2aa6c8", INK = "#160c1e";
  // ---------- catalogue (slot → items). lvl = Locker level needed (cosmetic only) ----------
  const NAMES = {
    hair: { crest: "Crest (padrão)", wavy: "Wavy", spiky: "Spiky", bob: "Bob", buns: "Space Buns", sweep: "Side Sweep" },
    headwear: { none: "Nenhum", beanie: "Gorro", cap: "Boné visor", fedora: "Fedora", horns: "Chifres" },
    glasses: { none: "Nenhum", cat: "Gatinho", visor: "Visor neon", round: "Redondo" },
    headphones: { none: "Nenhum", dark: "Studio", cream: "Retrô creme" },
    earring: { none: "Nenhum", star: "Estrela", planet: "Planeta", bolt: "Raio" },
    top: { jacket: "Jaqueta capuz", tank: "Regata", shirt: "Camisa 70s", crop: "Jaqueta cropped", cape: "Capa cósmica" },
    bottom: { track: "Calça listra", jogger: "Jogger", cargo: "Cargo", skirt: "Saia", flare: "Flare" },
    shoes: { sneaker: "Tênis", chunky: "Chunky", runner: "Runner", boot: "Bota" },
    instrument: { keytar: "Keytar", guitar: "Guitarra", bass: "Baixo", pads: "Pad controller" }
  };
  const LVL = {
    hair: { crest: 1, wavy: 1, spiky: 2, bob: 2, buns: 3, sweep: 4 }, headwear: { none: 1, beanie: 1, cap: 2, fedora: 3, horns: 5 },
    glasses: { none: 1, cat: 2, visor: 3, round: 1 }, headphones: { none: 1, dark: 1, cream: 3 }, earring: { none: 1, star: 1, planet: 2, bolt: 3 },
    top: { jacket: 1, tank: 1, shirt: 2, crop: 3, cape: 5 }, bottom: { track: 1, jogger: 1, cargo: 2, skirt: 3, flare: 4 },
    shoes: { sneaker: 1, chunky: 2, runner: 2, boot: 3 }, instrument: { keytar: 1, guitar: 2, bass: 3, pads: 2 }
  };
  const SLOTS = ["hair", "headwear", "glasses", "headphones", "earring", "top", "bottom", "shoes", "instrument"];
  const DEFAULT = { hair: "crest", headwear: "none", glasses: "none", headphones: "none", earring: "star", top: "jacket", bottom: "track", shoes: "sneaker", instrument: "keytar" };
  function items(slot) { return Object.keys(NAMES[slot]).map(function (id) { return { id: id, name: NAMES[slot][id], level: LVL[slot][id] || 1 }; }); }
  function valid(slot, id) { return !!NAMES[slot] && Object.prototype.hasOwnProperty.call(NAMES[slot], id); }
  function clean(o) { const s = Object.assign({}, DEFAULT); if (o) SLOTS.forEach(function (k) { if (valid(k, o[k])) s[k] = o[k]; }); return s; }

  // ---------- images ----------
  const IMG = {};
  function img(name) { if (!IT[name]) return null; if (!IMG[name]) { const i = new Image(); i.src = "nk_" + name + ".png"; IMG[name] = i; } return IMG[name]; }
  function ready(name) { const i = img(name); return i && i.complete && i.naturalWidth > 0; }
  function preload(slots) {
    const n = ["head", "handL", "handR"];
    n.push("hair_" + slots.hair);
    if (slots.headwear !== "none") n.push("hw_" + slots.headwear);
    if (slots.glasses !== "none") n.push("gl_" + slots.glasses);
    if (slots.headphones !== "none") n.push("hp_" + slots.headphones);
    if (slots.earring !== "none") n.push("er_" + slots.earring);
    Object.keys(IT).forEach(function (k) { if (k === "top_" + slots.top || k.indexOf("top_" + slots.top + "_") === 0) n.push(k); });
    if (slots.bottom === "skirt") n.push("bot_skirt"); else n.push("bot_" + slots.bottom + "_L", "bot_" + slots.bottom + "_R");
    n.push("shoe_" + slots.shoes + "_L", "shoe_" + slots.shoes + "_R", "inst_" + slots.instrument);
    n.forEach(img); return n;
  }

  // ---------- math ----------
  const V = function (x, y) { return { x: x, y: y }; };
  const pv = function (k) { return V(P[k][0], P[k][1]); };
  const add = function (a, b) { return V(a.x + b.x, a.y + b.y); }, sub = function (a, b) { return V(a.x - b.x, a.y - b.y); };
  const mul = function (a, k) { return V(a.x * k, a.y * k); }, len = function (a) { return Math.hypot(a.x, a.y); };
  const rot = function (a, r) { const c = Math.cos(r), s = Math.sin(r); return V(a.x * c - a.y * s, a.x * s + a.y * c); };
  const ang = function (a, b) { return Math.atan2(b.y - a.y, b.x - a.x); };
  const clamp = function (v, a, b) { return v < a ? a : v > b ? b : v; };
  const lerp = function (a, b, t) { return a + (b - a) * t; };
  const sstep = function (x) { return x * x * (3 - 2 * x); };
  const rad = function (d) { return d * Math.PI / 180; };
  function spring(st, key, target, w, dt) {
    let s = st[key]; if (!s) s = st[key] = { x: target, v: 0 };
    if (dt <= 0) { s.x = target; s.v = 0; return s.x; }
    const n = Math.max(1, Math.ceil(dt / (1 / 120))), h = dt / n;
    for (let i = 0; i < n; i++) { s.v += (w * w * (target - s.x) - 1.6 * w * s.v) * h; s.x += s.v * h; }
    return s.x;
  }
  function ik(a, t, l1, l2, bendSign) {
    const d = sub(t, a); let dl = len(d); const maxL = l1 + l2 - .5, minL = Math.abs(l1 - l2) + 1;
    const dd = clamp(dl, minL, maxL), dir = mul(d, 1 / (dl || 1));
    const cosA = clamp((l1 * l1 + dd * dd - l2 * l2) / (2 * l1 * dd), -1, 1), A = Math.acos(cosA) * bendSign;
    const mid = add(a, mul(rot(dir, A), l1)); const end = add(a, mul(dir, dd));
    return { mid: mid, end: end };
  }
  // rest skeleton (model space)
  const R0 = {
    neck: pv("neck"), shL: pv("shL"), shR: pv("shR"), elL: pv("elL"), elR: pv("elR"), wrL: pv("wrL"), wrR: pv("wrR"),
    hipL: pv("hipL"), hipR: pv("hipR"), anL: pv("anL"), anR: pv("anR"), pelvis: pv("pelvis"), eyes: pv("eyes")
  };
  const UP = len(sub(R0.elL, R0.shL)), FO = len(sub(R0.wrL, R0.elL));
  const HEAD_PIV = V(R0.neck.x, R0.neck.y - 10);

  // ---------- dance: the kit's six poses as keys (front view): anticipation, step L, keytar accent, low groove,
  // recovery, step R … interpolated with smoothstep; feet stay planted unless the pose moves them (then they lift).
  const K = {
    ant: { hx: 0, dp: 40, tor: 0, head: 3, nod: 6, kt: 0, fL: 0, fR: 0 },
    stL: { hx: -42, dp: 14, tor: -4, head: -5, nod: -2, kt: 2, fL: -60, fR: 0 },
    acc: { hx: -6, dp: 12, tor: 3, head: -8, nod: -6, kt: -9, fL: -10, fR: 6 },
    low: { hx: 0, dp: 110, tor: 0, head: 6, nod: 10, kt: 4, fL: -60, fR: 60 },
    rec: { hx: 0, dp: 4, tor: 0, head: 0, nod: 0, kt: 0, fL: 0, fR: 0 },
    stR: { hx: 42, dp: 14, tor: 4, head: 5, nod: -2, kt: 2, fL: 0, fR: 60 }
  };
  const PHRASE = [K.ant, K.stL, K.acc, K.low, K.rec, K.stR, K.acc, K.low];
  function phrase(b) {
    const N = PHRASE.length, n = Math.floor(b), f = sstep(b - n), A = PHRASE[((n % N) + N) % N], B = PHRASE[(((n + 1) % N) + N) % N], o = {};
    for (const k in A) o[k] = A[k] + (B[k] - A[k]) * f;
    o.liftL = Math.abs(B.fL - A.fL) > 4 ? Math.sin(Math.PI * f) * 22 : 0;
    o.liftR = Math.abs(B.fR - A.fR) > 4 ? Math.sin(Math.PI * f) * 22 : 0;
    return o;
  }

  function create(parent, slots) {
    const R = { root: parent, look: "nova-kit", L: { w: 520, hip: [260, 440], feet: 668, name: "Nova", inst: "keytar" }, st: {}, t: 0, P: null, slots: clean(slots) };
    const svg = parent.ownerSVGElement || parent, host = svg.parentNode;
    const canvas = document.createElement("canvas"); canvas.className = "puppet-canvas";
    canvas.style.cssText = "position:absolute;left:0;top:0;width:100%;height:100%;pointer-events:none;";
    host.appendChild(canvas); R.canvas = canvas;
    R.fig = document.createElement("canvas"); R.tint = document.createElement("canvas");
    R.blinkAt = 1.4; R.blink = 0; R.hitK = 0; R.press = 0; R.accent = 0; R.ear = { a: 0, v: 0 }; R.hair = { a: 0, v: 0 }; R.prevHx = 0;
    R.update = function (s, dt) { pose(R, s, dt); drawToCanvas(R, s); };
    R.step = function (s, dt) { pose(R, s, dt); };
    R.renderTo = function (g, s) { if (!R.P) pose(R, s || { alive: 0 }, 0); drawCharacter(g, R, s || {}, 1); };
    R.pose = function () { R.st = {}; R.ear = { a: 0, v: 0 }; R.hair = { a: 0, v: 0 }; pose(R, { alive: 0 }, 0); };
    R.setSlots = function (o) { R.slots = clean(Object.assign({}, R.slots, o || {})); R.L.inst = R.slots.instrument; preload(R.slots); };
    R.destroy = function () { canvas.remove(); };
    R.body = { getAttribute: function () { return ""; } };
    R.setSlots(slots);
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
    const G = phrase(b), breathe = Math.sin(R.t * 1.7);
    const p = {};
    p.hx = spring(st, "hx", G.hx * I + Math.sin(R.t * .45) * 3 * idle, 8, dt);
    p.dp = spring(st, "dp", G.dp * I + 10 * R.hitK * Math.max(.3, amp) + breathe * 2 * idle, 12, dt);
    p.tor = spring(st, "tor", G.tor * I + breathe * .4 * idle, 8, dt);
    p.head = spring(st, "hd", G.head * I + Math.sin(R.t * .7) * 2 * idle - R.accent * 6 * amp, 6, dt);
    p.nod = spring(st, "nd", G.nod * I + 6 * R.hitK * amp, 10, dt);
    p.kt = spring(st, "kt", G.kt * I - R.accent * 8 * amp, 9, dt);
    p.fL = spring(st, "fL", G.fL * I, 12, dt); p.fR = spring(st, "fR", G.fR * I, 12, dt);
    p.liftL = G.liftL * I; p.liftR = G.liftR * I;
    p.press = R.press * (s.playing ? 1 : .5) * Math.max(.3, amp);
    p.pitch = clamp(s.pitch === undefined ? .5 : s.pitch, 0, 1);
    p.breathe = breathe; p.blink = R.blink; p.alive = alive;
    // follow-through: hair and earring lag behind the head; cape behind the torso
    const vx = (p.hx - R.prevHx) / Math.max(dt, 1 / 240); R.prevHx = p.hx;
    if (dt > 0) {
      const h = Math.min(dt, 1 / 30);
      R.hair.v += (-90 * R.hair.a - vx * .0009 - rad(p.head) * 4) * h; R.hair.v *= Math.pow(.88, h * 60); R.hair.a = clamp(R.hair.a + R.hair.v * h, -.08, .08);
      R.ear.v += (-60 * R.ear.a - vx * .004 - rad(p.head + p.tor) * 2) * h; R.ear.v *= Math.pow(.9, h * 60); R.ear.a = clamp(R.ear.a + R.ear.v * h, -.6, .6);
    }
    p.hairA = R.hair.a; p.earA = R.ear.a;
    // ---- skeleton ----
    const pel = V(R0.pelvis.x + p.hx, R0.pelvis.y + p.dp), torR = rad(p.tor);
    const fromPel = function (q) { return add(pel, rot(sub(q, R0.pelvis), torR)); };
    p.pel = pel; p.torR = torR;
    p.hipL = add(pel, sub(R0.hipL, R0.pelvis)); p.hipR = add(pel, sub(R0.hipR, R0.pelvis));
    p.anL = V(R0.anL.x + p.fL, R0.anL.y - p.liftL); p.anR = V(R0.anR.x + p.fR, R0.anR.y - p.liftR);
    p.neck = fromPel(R0.neck); p.shL = fromPel(R0.shL); p.shR = fromPel(R0.shR);
    p.headR = torR + rad(p.head) * .8; p.headPiv = fromPel(HEAD_PIV); p.nodY = p.nod * .6;
    // instrument on the strap: follows the torso, tilts on accents
    const inst = IT["inst_" + R.slots.instrument];
    p.instR = torR + rad(p.kt);
    const ip = V(inst.pivot[0], inst.pivot[1]);
    p.instPiv0 = ip; p.instPiv = fromPel(ip);
    const tI = function (q) { return add(p.instPiv, rot(sub(V(q[0], q[1]), ip), p.instR)); };
    let gA = tI(inst.gA), gB = tI(inst.gB);
    // playing: the hand on the keys/pads follows the note pitch and presses on each note
    const along = R.slots.instrument === "pads" ? V(28, 0) : rot(V(70, 0), p.instR - .62);
    if (R.slots.instrument === "keytar" || R.slots.instrument === "pads") gA = add(gA, mul(along, (p.pitch - .5) * 1.2));
    gA = add(gA, V(0, 8 * p.press));
    if (R.slots.instrument === "guitar" || R.slots.instrument === "bass") gA = add(gA, V(0, Math.sin(R.t * 22) * 10 * p.press));
    p.ends = inst.ends.map(tI);
    // arms: two-bone IK to the grips (exactly two hands, both on the instrument)
    const wristFor = function (sh, g) { const d = sub(g, sh), l = len(d) || 1; return sub(g, mul(d, 30 / l)); };
    // elbows always point outwards (away from the body), whichever way the target lies
    const arm = function (sh, t, side) { const a1 = ik(sh, t, UP, FO, 1), a2 = ik(sh, t, UP, FO, -1); return (side < 0 ? a1.mid.x < a2.mid.x : a1.mid.x > a2.mid.x) ? a1 : a2; };
    const aL = arm(p.shL, wristFor(p.shL, gA), -1), aR = arm(p.shR, wristFor(p.shR, gB), 1);
    p.elL = aL.mid; p.wrL = aL.end; p.elR = aR.mid; p.wrR = aR.end;
    p.gA = gA; p.gB = gB;
    R.P = p;
  }

  // ---------- drawing ----------
  function piece(g, name) { const b = IT[name]; if (!b || !ready(name)) return; g.drawImage(IMG[name], b.box[0], b.box[1], b.box[2], b.box[3]); }
  // bone transform: rest joint/angle → current joint/angle
  function bone(g, j0, a0, j1, a1) { g.translate(j1.x, j1.y); g.rotate(a1 - a0); g.translate(-j0.x, -j0.y); }
  // leg transform: rest hip→ankle onto current hip→ankle (affine: stretch along the leg, width kept)
  function leg(g, h0, a0, h1, a1) {
    const u0 = sub(a0, h0), u1 = sub(a1, h1), l0 = len(u0), l1 = len(u1);
    const e0 = mul(u0, 1 / l0), n0 = V(-e0.y, e0.x), e1 = mul(u1, 1 / l1), n1 = V(-e1.y, e1.x), k = l1 / l0;
    // M = [e1*k n1] * [e0 n0]^T
    const a = e1.x * k * e0.x + n1.x * n0.x, c = e1.x * k * e0.y + n1.x * n0.y, b = e1.y * k * e0.x + n1.y * n0.x, d = e1.y * k * e0.y + n1.y * n0.y;
    g.translate(h1.x, h1.y); g.transform(a, b, c, d, 0, 0); g.translate(-h0.x, -h0.y);
  }
  // Smooth, tapered, shaded skin limbs. The old two-pass straight strokes looked like tubes beside
  // the richly painted kit. Sample the quadratic so shoulders, elbows and knees read as soft forms.
  function limbStroke(g, pts, w0, w1, bend) {
    const a = pts[0], b = pts[1], dx = b.x - a.x, dy = b.y - a.y, n = Math.hypot(dx, dy) || 1;
    const side = bend || (a.x < 513 ? 1 : -1);
    const c = { x: (a.x + b.x) * .5 - dy / n * Math.min(34, n * .12) * side,
                y: (a.y + b.y) * .5 + dx / n * Math.min(34, n * .12) * side };
    const at = function (t) { const q = 1 - t; return { x: q*q*a.x + 2*q*t*c.x + t*t*b.x, y: q*q*a.y + 2*q*t*c.y + t*t*b.y }; };
    const skin = g.createLinearGradient(a.x - 30, a.y, b.x + 30, b.y);
    skin.addColorStop(0, "#228fad"); skin.addColorStop(.42, SKIN); skin.addColorStop(.72, "#72e5ed"); skin.addColorStop(1, "#238eae");
    g.lineCap = "round"; g.lineJoin = "round";
    for (let i = 0; i < 12; i++) {
      const p0 = at(i / 12), p1 = at((i + 1) / 12), w = lerp(w0, w1, (i + .5) / 12);
      g.strokeStyle = INK; g.lineWidth = w + 8; g.beginPath(); g.moveTo(p0.x, p0.y); g.lineTo(p1.x, p1.y); g.stroke();
      g.strokeStyle = skin; g.lineWidth = w; g.beginPath(); g.moveTo(p0.x, p0.y); g.lineTo(p1.x, p1.y); g.stroke();
    }
    // A narrow reflected edge gives the cyan skin volume without adding a heavy glow.
    g.save(); g.globalAlpha = .24; g.strokeStyle = "#e7ffff"; g.lineWidth = 3; g.beginPath(); g.moveTo(a.x - side * w0 * .17, a.y); g.quadraticCurveTo(c.x - side * ((w0+w1)*.085), c.y, b.x - side * w1 * .17, b.y); g.stroke(); g.restore();
  }
  function skinFill(g, path, x1, y1, x2, y2) {
    const fill = g.createLinearGradient(x1, y1, x2, y2);
    fill.addColorStop(0, "#70e2ee"); fill.addColorStop(.42, SKIN); fill.addColorStop(1, "#218eaf");
    g.fillStyle = fill; g.fill(path); g.strokeStyle = INK; g.lineWidth = 7; g.lineJoin = "round"; g.stroke(path);
    g.save(); g.globalAlpha = .2; g.strokeStyle = "#edffff"; g.lineWidth = 3; g.stroke(path); g.restore();
  }
  function figure(g, R, P) {
    const S0 = R.slots;
    // --- legs: skin, shoes, bottoms ---
    limbStroke(g, [P.hipL, P.anL], 84, 64, 1); limbStroke(g, [P.hipR, P.anR], 84, 64, -1);
    ["L", "R"].forEach(function (sd) { const an = P["an" + sd], a0 = R0["an" + sd]; g.save(); g.translate(an.x - a0.x, an.y - a0.y); piece(g, "shoe_" + S0.shoes + "_" + sd); g.restore(); });
    if (S0.bottom === "skirt") { g.save(); g.translate(P.pel.x - R0.pelvis.x, P.pel.y - R0.pelvis.y); piece(g, "bot_skirt"); g.restore(); }
    else ["L", "R"].forEach(function (sd) { g.save(); leg(g, R0["hip" + sd], R0["an" + sd], P["hip" + sd], P["an" + sd]); piece(g, "bot_" + S0.bottom + "_" + sd); g.restore(); });
    // --- torso skin + neck, then the top ---
    g.save(); bone(g, R0.pelvis, 0, P.pel, P.torR);
    const neck = new Path2D();
    neck.moveTo(490, 470); neck.quadraticCurveTo(513, 478, 536, 470); neck.lineTo(544, 548);
    neck.quadraticCurveTo(513, 566, 482, 548); neck.closePath();
    skinFill(g, neck, 482, 470, 544, 558);
    const torso = new Path2D();
    torso.moveTo(458, 560); torso.quadraticCurveTo(485, 548, 513, 554); torso.quadraticCurveTo(541, 548, 568, 560);
    torso.bezierCurveTo(557, 616, 559, 675, 559, 706); torso.quadraticCurveTo(557, 770, 566, 805);
    torso.quadraticCurveTo(513, 821, 460, 805); torso.quadraticCurveTo(469, 770, 467, 706);
    torso.bezierCurveTo(467, 675, 469, 616, 458, 560); torso.closePath();
    skinFill(g, torso, 458, 562, 568, 810);
    // Soft clavicle lines and a central highlight keep exposed skin in the same illustrated style as the kit.
    g.save(); g.strokeStyle = "rgba(13,74,101,.34)"; g.lineWidth = 5; g.lineCap = "round";
    g.beginPath(); g.moveTo(474, 574); g.quadraticCurveTo(495, 589, 513, 578); g.quadraticCurveTo(532, 589, 552, 574); g.stroke();
    g.strokeStyle = "rgba(235,255,255,.34)"; g.lineWidth = 4; g.beginPath(); g.moveTo(493, 492); g.quadraticCurveTo(499, 524, 496, 545); g.stroke(); g.restore();
    g.restore();
    // strap behind the top (over the right shoulder to both ends of the instrument)
    const strap = function () {
      g.save(); g.strokeStyle = "#241a33"; g.lineCap = "round"; g.lineJoin = "round";
      const sh = add(P.shL, V(30, -8)), e0 = P.ends[0], e1 = P.ends[1];
      [[INK, 30], ["#2b2140", 22]].forEach(function (s) { g.strokeStyle = s[0]; g.lineWidth = s[1]; g.beginPath(); g.moveTo(e0.x, e0.y); g.quadraticCurveTo(sh.x - 40, (sh.y + e0.y) / 2, sh.x, sh.y); g.quadraticCurveTo(sh.x + 120, sh.y + 40, e1.x, e1.y); g.stroke(); });
      g.restore();
    };
    g.save(); bone(g, R0.pelvis, 0, P.pel, P.torR); piece(g, "top_" + S0.top); g.restore();
    // --- head: bald head, hair (with follow-through), headwear, glasses, headphones, earring ---
    g.save(); g.translate(0, P.nodY); bone(g, HEAD_PIV, 0, P.headPiv, P.headR);
    piece(g, "head"); eyes(g, P);
    g.save(); bone(g, V(541, 300), 0, V(541, 300), P.hairA); piece(g, "hair_" + S0.hair); g.restore();
    if (S0.earring !== "none") { const h = IT["er_" + S0.earring].hook; g.save(); bone(g, V(h[0], h[1]), 0, V(h[0], h[1]), P.earA); piece(g, "er_" + S0.earring); g.restore(); }
    if (S0.glasses !== "none") piece(g, "gl_" + S0.glasses);
    if (S0.headphones !== "none") piece(g, "hp_" + S0.headphones);
    if (S0.headwear !== "none") { g.save(); bone(g, V(541, 300), 0, V(541, 300), P.hairA * .4); piece(g, "hw_" + S0.headwear); g.restore(); }
    g.restore();
    // --- instrument on its strap ---
    strap();
    g.save(); bone(g, P.instPiv0, 0, P.instPiv, P.instR); piece(g, "inst_" + S0.instrument); g.restore();
    // --- arms (skin, sleeves, hands) in front of the instrument ---
    ["L", "R"].forEach(function (sd) {
      const sh = P["sh" + sd], el = P["el" + sd], wr = P["wr" + sd];
      const r0s = R0["sh" + sd], r0e = R0["el" + sd], r0w = R0["wr" + sd];
      const hasU = !!IT["top_" + S0.top + "_u" + sd], hasL = !!IT["top_" + S0.top + "_l" + sd];
      if (!hasU) limbStroke(g, [sh, el], 60, 54);
      if (!hasL) limbStroke(g, [el, wr], 54, 46);
      g.save(); bone(g, r0e, ang(r0e, r0w), el, ang(el, wr)); piece(g, "top_" + S0.top + "_l" + sd); g.restore();
      g.save(); bone(g, r0s, ang(r0s, r0e), sh, ang(sh, el)); piece(g, "top_" + S0.top + "_u" + sd); g.restore();
      g.save(); bone(g, r0w, ang(r0e, r0w), wr, ang(el, wr)); piece(g, "hand" + sd); g.restore();
    });
  }
  function eyes(g, P) {
    if (P.blink < .02) return;
    const c = 1 - Math.abs(1 - 2 * Math.min(1, P.blink * 1.15));
    [[473, 403], [609, 404]].forEach(function (e) {
      g.save(); g.beginPath(); g.ellipse(e[0], e[1], 46, 34, 0, 0, Math.PI * 2); g.clip();
      const y = e[1] - 36 + 72 * c; g.fillStyle = SKIN; g.fillRect(e[0] - 50, e[1] - 40, 100, y - (e[1] - 40));
      g.strokeStyle = INK; g.lineWidth = 6; g.beginPath(); g.moveTo(e[0] - 50, y); g.lineTo(e[0] + 50, y); g.stroke(); g.restore();
    });
  }
  // render the figure once per frame into an offscreen canvas, reused for effect tints
  function renderFig(R, scale) {
    const W = Math.ceil(1024 * S * scale), H = Math.ceil(1460 * S * scale), c = R.fig;
    if (c.width !== W || c.height !== H) { c.width = W; c.height = H; }
    const g = c.getContext("2d"); g.setTransform(1, 0, 0, 1, 0, 0); g.clearRect(0, 0, W, H);
    g.setTransform(S * scale, 0, 0, S * scale, 0, 0); figure(g, R, R.P);
    return c;
  }
  function tinted(R, col) {
    const src = R.fig, c = R.tint; if (c.width !== src.width || c.height !== src.height) { c.width = src.width; c.height = src.height; }
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
  // FX lights (Gab's colour map), strength = the real activity of each module (0 when bypassed or at mix 0)
  const FXC = ["#a96bff", "#ffa02e", "#ff4fd8", "#2fe0ff", "#ffd23f", "#c9a6ff"];
  function drawCharacter(g, R, s, pxScale) {
    const P = R.P; if (!P) return;
    const sc = clamp(pxScale || 1, .3, 2), fig = renderFig(R, sc), fw = fig.width / sc, fh = fig.height / sc;
    const ox = -OX * S, oy = -OY * S;
    const put = function (c, dx, alpha) { g.save(); g.globalAlpha = alpha; g.drawImage(c, ox + (dx || 0), oy, fw, fh); g.restore(); };
    if (s.reflect) { g.save(); g.beginPath(); g.rect(-400, 2, 800, 240); g.clip(); g.translate(0, 4); g.scale(1, -1); put(fig, 0, .14); g.restore(); }
    g.save(); g.globalAlpha = .45; g.fillStyle = "#000"; g.beginPath(); g.ellipse(P.hx * S * .5, 4, 175, 15, 0, 0, Math.PI * 2); g.fill(); g.restore();
    const fx = s.fx || [s.ghost || 0, 0, s.bloom || 0, s.wob || 0, 0, 0];
    // soft light of each active effect behind her; several effects share the light moderately (face stays readable)
    const total = fx.reduce(function (a, b) { return a + b; }, 0), norm = total > 1 ? 1 / total : 1;
    fx.forEach(function (a, i) {
      if (a < .02) return; const rr = 240 + a * 120;
      g.save(); g.translate(0, -330); g.scale(1, 1.3); const gr = g.createRadialGradient(0, 0, 20, 0, 0, rr); gr.addColorStop(0, FXC[i]); gr.addColorStop(1, "rgba(0,0,0,0)");
      g.globalAlpha = clamp(a * norm, 0, 1) * .5; g.globalCompositeOperation = "lighter"; g.fillStyle = gr; g.beginPath(); g.arc(0, 0, rr, 0, Math.PI * 2); g.fill(); g.restore();
    });
    // GHOST: violet afterimages trailing the movement
    if (fx[0] > .02) { const t = tinted(R, FXC[0]); for (let i = 3; i >= 0; i--) put(t, -(i + 1) * 16 * fx[0], clamp(fx[0] * (.42 - i * .09), 0, .5)); }
    // WOBBLE: cyan contour that sways with the WOBBLE rate
    if (fx[3] > .02) put(tinted(R, FXC[3]), Math.sin(R.t * (3 + (s.wobRate || 1) * 2)) * (10 + fx[3] * 16), fx[3] * .45);
    put(fig, 0, 1);
    // rim light on the figure in the mixed effect colour (kept under 35% so the face and clothes read)
    if (total > .05) {
      let rr = 0, gg = 0, bb = 0; fx.forEach(function (a, i) { const n = parseInt(FXC[i].slice(1), 16); rr += (n >> 16 & 255) * a; gg += (n >> 8 & 255) * a; bb += (n & 255) * a; });
      const col = "rgb(" + Math.round(rr / total) + "," + Math.round(gg / total) + "," + Math.round(bb / total) + ")";
      g.save(); g.globalCompositeOperation = "lighter"; put(tinted(R, col), 0, clamp(total, 0, 1) * .22); g.restore();
    }
  }
  window.RastroNovaKit = { create: create, SLOTS: SLOTS, DEFAULT: DEFAULT, NAMES: NAMES, items: items, valid: valid, clean: clean };
})();

