/* RASTRO alien — the main character, drawn in code as a real 2D rig (not a flat picture).
   Bones: feet -> legs (2-bone IK) -> pelvis -> torso -> shoulders -> arms (2-bone IK) -> hands, and
   neck -> head. Clothes and items are drawn along the same bones every frame, so they follow the
   joints instead of floating. Everything here is visual: nothing reads or writes the audio. */
(function () {
  "use strict";
  const NS = "http://www.w3.org/2000/svg";
  const clamp = function (v, a, b) { return Math.min(b, Math.max(a, v)); };
  const lerp = function (a, b, t) { return a + (b - a) * t; };
  const smooth = function (a, b, x) { const t = clamp((x - a) / (b - a), 0, 1); return t * t * (3 - 2 * t); };
  const f1 = function (n) { return (Math.round(n * 10) / 10).toString(); };
  let uid = 0;

  /* ---------------- catalogue ---------------- */
  // level = character level needed (1 = free from the start). Nothing here changes the sound.
  const SLOTS = [
    { id: "head", pt: "Cabeça", en: "Head", note: "chapéu, gorro, boné", noteEn: "hat, beanie, cap" },
    { id: "phones", pt: "Fones", en: "Phones" },
    { id: "eyes", pt: "Olhos", en: "Eyes", note: "óculos e visores", noteEn: "glasses, visors" },
    { id: "hair", pt: "Topete", en: "Crest", note: "cabelo, chifres", noteEn: "hair, horns" },
    { id: "top", pt: "Roupa", en: "Top" },
    { id: "bottom", pt: "Calça", en: "Pants" },
    { id: "shoes", pt: "Calçado", en: "Shoes" },
    { id: "inst", pt: "Instrumento", en: "Instrument" },
    { id: "acc", pt: "Acessório", en: "Extra" },
    { id: "face", pt: "Expressão", en: "Face" },
    { id: "glow", pt: "Brilho", en: "Glow" }
  ];
  const ITEMS = {
    head: [["none", "Nada", "None", 1], ["beanie", "Gorro de tricô", "Knit beanie", 1], ["bucket", "Bucket hat", "Bucket hat", 2], ["cap", "Boné futurista", "Future cap", 4], ["fedora", "Fedora swirl", "Swirl fedora", 3], ["aviator", "Gorro aviador", "Aviator hat", 5], ["halo", "Auréola neon", "Neon halo", 7]],
    phones: [["none", "Nada", "None", 1], ["retro", "Fone retrô", "Retro phones", 1], ["chrome", "Fone cromado", "Chrome phones", 3], ["neon", "Fone neon", "Neon band", 6]],
    eyes: [["none", "Nada", "None", 1], ["round", "Óculos redondos", "Round glasses", 2], ["visor", "Visor envolvente", "Wrap visor", 3], ["star", "Óculos estrela", "Star shades", 5], ["diva", "Óculos diva", "Diva frames", 2], ["shield", "Visor escudo", "Shield shades", 4]],
    hair: [["flame", "Chama", "Flame", 1], ["tall", "Chama alta", "Tall flame", 2], ["curls", "Cachos", "Curls", 3], ["horns", "Chifres", "Horns", 4], ["antenna", "Antenas", "Antennae", 5], ["spikes", "Cristais", "Crystal spikes", 6], ["bald", "Liso", "Smooth", 1]],
    top: [["none", "Só o brilho", "Bare glow", 1], ["varsity", "Jaqueta varsity", "Varsity jacket", 1], ["hoodie", "Hoodie techwear", "Techwear hoodie", 2], ["suit", "Terno flare 70s", "70s flare suit", 3], ["jumpsuit", "Macacão mecânico", "Mech jumpsuit", 5], ["jersey", "Camisa Seleção 3000", "Jersey 3000", 1], ["shirt", "Camisa swirl", "Swirl shirt", 2], ["bomber", "Bomber prata", "Silver bomber", 4]],
    bottom: [["none", "Nada", "None", 1], ["swirl", "Calça swirl", "Swirl pants", 1], ["cargo", "Cargo joelheira", "Cargo pants", 2], ["flare", "Boca de sino", "Flares", 3], ["track", "Calça de treino", "Track pants", 4]],
    shoes: [["none", "Descalço", "Barefoot", 1], ["chunky", "Tênis volumoso", "Chunky sneakers", 1], ["platform", "Plataforma 70s", "Platforms", 3], ["glow", "Tênis de luz", "Light runners", 5]],
    inst: [["none", "Mãos livres", "Free hands", 1], ["keytar", "Keytar", "Keytar", 1], ["guitar", "Guitarra estrela", "Star guitar", 2], ["bass", "Baixo", "Bass", 3], ["pads", "Controlador de pads", "Pad controller", 4]],
    acc: [["none", "Nada", "None", 1], ["chain", "Corrente estrela", "Star chain", 2], ["ribbons", "Fitas", "Ribbons", 3], ["scarf", "Lenço swirl", "Swirl scarf", 2], ["gloves", "Luvas neon", "Neon gloves", 3], ["cape", "Capa iridescente", "Iridescent cape", 6]],
    face: [["happy", "Feliz", "Happy", 1], ["open", "Olhos abertos", "Open eyes", 1], ["cool", "Tranquilo", "Chill", 2], ["wink", "Piscadinha", "Wink", 3]],
    glow: [["auto", "Cor dos efeitos", "Effect colours", 1], ["cyan", "Ciano", "Cyan", 1], ["orange", "Âmbar", "Amber", 1], ["violet", "Violeta", "Violet", 2], ["pink", "Rosa", "Pink", 3], ["lime", "Lima", "Lime", 4]]
  };
  const GLOW = { auto: "#5fe3ff", cyan: "#5fe3ff", orange: "#ffae3d", violet: "#b07bff", pink: "#ff6fc4", lime: "#b6ff5c" };
  const DEFAULT_LOOK = { head: "none", phones: "retro", eyes: "none", hair: "flame", top: "varsity", bottom: "swirl", shoes: "chunky", inst: "keytar", acc: "none", face: "happy", glow: "auto" };
  const DANCES = [
    { id: "groove", pt: "Groove 70s", en: "Groove 70s", level: 1, tip: "Passo e toque, peso de um pé pro outro", tipEn: "Step-touch, weight from foot to foot" },
    { id: "disco", pt: "Disco", en: "Disco", level: 1, tip: "Quadril de lado e dedo pro alto", tipEn: "Hip bumps and pointing" },
    { id: "half", pt: "Half-Time", en: "Half-Time", level: 1, tip: "Pesado e lento, metade do pulso", tipEn: "Heavy swagger at half the pulse" },
    { id: "funk", pt: "Funk Bounce", en: "Funk Bounce", level: 2, tip: "Quique em colcheias, ombros soltos", tipEn: "Eighth-note bounce, loose shoulders" },
    { id: "cosmic", pt: "Cosmic Flow", en: "Cosmic Flow", level: 4, tip: "Flutua em oito, braços líquidos", tipEn: "Floating figure-eight, liquid arms" }
  ];
  function itemInfo(slot, id) { const l = ITEMS[slot] || []; for (let i = 0; i < l.length; i++) if (l[i][0] === id) return { id: l[i][0], pt: l[i][1], en: l[i][2], level: l[i][3] }; return null; }
  function normalise(look) {
    const out = {};
    for (const k in DEFAULT_LOOK) out[k] = look && itemInfo(k, look[k]) ? look[k] : DEFAULT_LOOK[k];
    if (out.top === "jumpsuit") out.bottom = "jumpsuit"; // the jumpsuit is one piece
    return out;
  }

  /* ---------------- tiny 2D maths ---------------- */
  function rot(p, a) { const c = Math.cos(a), s = Math.sin(a); return [p[0] * c - p[1] * s, p[0] * s + p[1] * c]; }
  function xf(T, p) { const r = rot(p, T.r); return [r[0] + T.x, r[1] + T.y]; }
  function chain(A, B) { const p = xf(A, [B.x, B.y]); return { x: p[0], y: p[1], r: A.r + B.r }; }
  // Two-bone IK: picks the elbow/knee that points away from the body (side = -1 left, +1 right).
  function ik(root, target, a, b, side, preferDown) {
    let dx = target[0] - root[0], dy = target[1] - root[1], d = Math.hypot(dx, dy);
    const dd = clamp(d, Math.abs(a - b) + .5, a + b - .3);
    if (d < 1e-3) { dx = 0; dy = 1; d = 1; }
    const th = Math.atan2(dy, dx), ca = clamp((a * a + dd * dd - b * b) / (2 * a * dd), -1, 1), al = Math.acos(ca);
    const c1 = [root[0] + a * Math.cos(th + al), root[1] + a * Math.sin(th + al)], c2 = [root[0] + a * Math.cos(th - al), root[1] + a * Math.sin(th - al)];
    const score = function (j) { return (j[0] - root[0]) * side + (preferDown ? (j[1] - root[1]) * 1.2 : 0); };
    const j = score(c1) >= score(c2) ? c1 : c2;
    const end = [root[0] + dx / d * dd, root[1] + dy / d * dd];
    return { j: j, e: end };
  }
  function P(pts) { let s = "M" + f1(pts[0][0]) + " " + f1(pts[0][1]); for (let i = 1; i < pts.length; i++) s += " L" + f1(pts[i][0]) + " " + f1(pts[i][1]); return s; }
  function offsetLine(pts, k) {
    const out = [];
    for (let i = 0; i < pts.length; i++) {
      const a = pts[Math.max(0, i - 1)], b = pts[Math.min(pts.length - 1, i + 1)];
      let nx = -(b[1] - a[1]), ny = b[0] - a[0]; const l = Math.hypot(nx, ny) || 1; nx /= l; ny /= l;
      out.push([pts[i][0] + nx * k, pts[i][1] + ny * k]);
    }
    return out;
  }
  // Smooth tube outline around a polyline with a width per point (used for flares and bell sleeves).
  function tube(pts, w) {
    const L = offsetLine(pts, 1), R = [];
    const left = pts.map(function (p, i) { return [p[0] + (L[i][0] - p[0]) * w[i] / 2, p[1] + (L[i][1] - p[1]) * w[i] / 2]; });
    for (let i = pts.length - 1; i >= 0; i--) R.push([pts[i][0] - (L[i][0] - pts[i][0]) * w[i] / 2, pts[i][1] - (L[i][1] - pts[i][1]) * w[i] / 2]);
    return P(left.concat(R)) + " Z";
  }
  function el(parent, tag, attrs) { const e = document.createElementNS(NS, tag); for (const k in attrs) e.setAttribute(k, attrs[k]); parent.appendChild(e); return e; }
  function g(parent, cls) { return el(parent, "g", cls ? { class: cls } : {}); }

  /* ---------------- palettes (fixed art colours; the glow colour comes from the sound) ---------------- */
  const C = {
    skin: "#150d24", skin2: "#2a1b45", ol: "#0b0614", hi: "rgba(255,255,255,.28)",
    orange: "#ff9a2e", amber: "#ffc23d", cream: "#f6e7c8", red: "#e8552c", purple: "#8a3cf0", violet: "#b57bff", teal: "#25c4bc", teal2: "#168f8b",
    pink: "#ff5fae", white: "#fff6e8", black: "#1c1426", gold: "#ffcf4a", cyan: "#58e8ff", chrome1: "#e9f0ff", chrome2: "#8a95b5"
  };
  // Clothing colour sets: [main, secondary, accent, outline-detail]
  const TOP = { varsity: [C.red, C.cream, C.orange], hoodie: [C.teal, C.teal2, C.orange], suit: [C.purple, C.orange, C.amber], jumpsuit: [C.teal, C.teal2, C.amber], jersey: ["#ffd21f", "#17a34a", "#2563eb"], shirt: [C.purple, C.orange, C.amber], bomber: ["#c9cfe0", "#8a95b5", C.violet] };
  const SHORT = { jersey: 1, shirt: 1 };
  const BOT = { swirl: [C.cream, C.purple, C.orange], cargo: [C.teal, C.violet, C.amber], flare: [C.purple, C.orange, C.pink], track: [C.black, C.orange, C.violet], jumpsuit: [C.teal, C.teal2, C.amber] };

  /* ---------------- the rig ---------------- */
  function create(parent, lookIn) {
    const id = "al" + (++uid);
    const root = g(parent, "alien");
    root.setAttribute("style", "--ag:#5fe3ff;--ag2:#ff9a2e;--gw:.3");
    const defs = el(root, "defs", {});
    defs.innerHTML =
      '<radialGradient id="' + id + 'aura"><stop offset="0" stop-color="#5fe3ff" stop-opacity=".55" class="a0"/><stop offset="1" stop-color="#5fe3ff" stop-opacity="0" class="a1"/></radialGradient>' +
      '<linearGradient id="' + id + 'gloss" x1="0" y1="0" x2="0" y2="1"><stop offset="0" stop-color="#3b2763"/><stop offset=".55" stop-color="#150d24"/><stop offset="1" stop-color="#0b0614"/></linearGradient>' +
      '<linearGradient id="' + id + 'irid" x1="0" y1="0" x2="1" y2="1"><stop offset="0" stop-color="#9fe8ff" stop-opacity=".5"/><stop offset=".5" stop-color="#d59bff" stop-opacity=".45"/><stop offset="1" stop-color="#ffc58a" stop-opacity=".5"/></linearGradient>' +
      '<linearGradient id="' + id + 'visor" x1="0" y1="0" x2="0" y2="1"><stop offset="0" stop-color="#8ef4ff" stop-opacity=".85"/><stop offset="1" stop-color="#4b6dff" stop-opacity=".7"/></linearGradient>' +
      '<linearGradient id="' + id + 'chrome" x1="0" y1="0" x2="0" y2="1"><stop offset="0" stop-color="#ffffff"/><stop offset=".5" stop-color="#9aa6c8"/><stop offset="1" stop-color="#e7ecff"/></linearGradient>' +
      '<pattern id="' + id + 'paisley" width="16" height="16" patternUnits="userSpaceOnUse"><rect width="16" height="16" fill="' + C.purple + '"/><path d="M3 12 C1 6 7 2 10 6 C12 9 8 11 6 9" fill="none" stroke="' + C.orange + '" stroke-width="1.6"/><circle cx="12" cy="13" r="1.4" fill="' + C.amber + '"/></pattern>';
    const R = { root: root, id: id, look: normalise(lookIn), parts: {}, dyn: [], t: 0 };
    const aura = el(root, "ellipse", { cx: 0, cy: -150, rx: 120, ry: 150, fill: "url(#" + id + "aura)", opacity: ".6" });
    const shadow = el(root, "ellipse", { cx: 0, cy: -2, rx: 52, ry: 8, fill: "rgba(0,0,0,.35)" });
    const body = g(root, "al-body");
    R.aura = aura; R.shadow = shadow; R.body = body;
    R.stops = defs.querySelectorAll("stop.a0, stop.a1");
    build(R);
    R.state = restState();
    R.springs = {};
    R.update = function (s, dt) { update(R, s, dt); };
    R.setLook = function (l) { R.look = normalise(l); build(R); update(R, R.lastS || {}, 0); };
    R.pose = pose;
    return R;
  }
  function restState() { return { strum: 0, strumV: 0, press: 0, hitHip: 0, hitHipV: 0, nod: 0, nodV: 0, blink: 0, nextBlink: 2.5, cape: [] }; }

  // A limb is drawn as layered strokes along its bones: wide soft glow, glow rim, then dark skin.
  function limb(parent, w) {
    return {
      glowW: el(parent, "path", { class: "al-gw", fill: "none", "stroke-width": w + 12, "stroke-linecap": "round", "stroke-linejoin": "round" }),
      rim: el(parent, "path", { class: "al-rim", fill: "none", "stroke-width": w + 4, "stroke-linecap": "round", "stroke-linejoin": "round" }),
      skin: el(parent, "path", { fill: "none", stroke: C.skin, "stroke-width": w, "stroke-linecap": "round", "stroke-linejoin": "round" }),
      set: function (d) { this.glowW.setAttribute("d", d); this.rim.setAttribute("d", d); this.skin.setAttribute("d", d); }
    };
  }
  function cloth(parent, w, fill, extra) {
    const o = {
      ol: el(parent, "path", { fill: "none", stroke: C.ol, "stroke-width": w + 5, "stroke-linecap": "round", "stroke-linejoin": "round" }),
      fl: el(parent, "path", { fill: "none", stroke: fill, "stroke-width": w, "stroke-linecap": "round", "stroke-linejoin": "round" }),
      set: function (d) { this.ol.setAttribute("d", d); this.fl.setAttribute("d", d); }
    };
    if (extra) for (const k in extra) o.fl.setAttribute(k, extra[k]);
    return o;
  }
  function shape(parent, fill, d, sw) { return el(parent, "path", { d: d || "", fill: fill, stroke: C.ol, "stroke-width": sw === undefined ? 2.6 : sw, "stroke-linejoin": "round" }); }

  function build(R) {
    const L = R.look, b = R.body, id = R.id;
    b.innerHTML = ""; R.parts = {};
    const p = R.parts;
    p.capeBack = g(b);
    // legs (both behind the torso)
    p.legs = [0, 1].map(function () { const lg = g(b); return { g: lg, limb: limb(lg, 13), pants: null, stripes: [], bell: null, foot: g(lg) }; });
    p.legs.forEach(function (lg, i) {
      const bot = L.bottom !== "none" ? BOT[L.bottom] : null;
      if (bot) {
        if (L.bottom === "flare") { lg.bell = shape(lg.g, "url(#" + id + "paisley)", "", 2.6); }
        lg.pants = cloth(lg.g, L.bottom === "track" ? 24 : L.bottom === "flare" ? 24 : 30, bot[0]);
        if (L.bottom === "swirl") { lg.stripes = [el(lg.g, "path", { fill: "none", stroke: bot[1], "stroke-width": 6, "stroke-linecap": "round", opacity: .95 }), el(lg.g, "path", { fill: "none", stroke: bot[2], "stroke-width": 4, "stroke-linecap": "round" })]; }
        if (L.bottom === "track") lg.stripes = [el(lg.g, "path", { fill: "none", stroke: bot[1], "stroke-width": 4, "stroke-linecap": "round" })];
        if (L.bottom === "flare") lg.stripes = [el(lg.g, "path", { fill: "none", stroke: bot[1], "stroke-width": 4, "stroke-linecap": "round", opacity: .9 })];
        if (L.bottom === "cargo") { lg.pad = el(lg.g, "circle", { r: 9, fill: C.black, stroke: C.ol, "stroke-width": 2.4 }); lg.padMark = el(lg.g, "path", { fill: "none", stroke: bot[2], "stroke-width": 3, "stroke-linecap": "round" }); lg.pocket = el(lg.g, "rect", { width: 12, height: 13, rx: 2, fill: C.black, stroke: C.ol, "stroke-width": 2 }); }
        if (L.bottom === "jumpsuit") lg.stripes = [el(lg.g, "path", { fill: "none", stroke: bot[2], "stroke-width": 3.4, "stroke-linecap": "round" })];
      }
      lg.g.appendChild(lg.foot); // shoes are drawn over the pants hem
      buildFoot(lg.foot, L.shoes, i === 0 ? -1 : 1);
    });
    // pelvis + torso
    p.torso = g(b);
    buildTorso(R, p.torso);
    // head
    p.head = g(b);
    buildHead(R, p.head);
    // strap + instrument
    p.strap = el(b, "path", { fill: "none", stroke: C.ol, "stroke-width": 6, "stroke-linecap": "round" });
    p.strapIn = el(b, "path", { fill: "none", stroke: L.inst === "keytar" ? C.orange : C.violet, "stroke-width": 3, "stroke-linecap": "round" });
    p.inst = g(b);
    buildInstrument(R, p.inst, L.inst);
    if (L.inst === "none") { p.strap.setAttribute("display", "none"); p.strapIn.setAttribute("display", "none"); }
    // arms over everything but the hands
    const top = L.top !== "none" ? TOP[L.top] : null;
    p.arms = [0, 1].map(function (i) {
      const ag = g(b);
      const a = { g: ag, limb: limb(ag, 11), sleeve: null, cuff: null, bell: null, hand: g(ag) };
      if (top) {
        if (L.top === "suit") a.bell = shape(ag, "url(#" + id + "paisley)", "", 2.6);
        const sleeveFill = L.top === "varsity" ? top[1] : L.top === "jersey" ? top[1] : L.top === "shirt" ? "url(#" + id + "paisley)" : L.top === "bomber" ? "url(#" + id + "chrome)" : top[0];
        a.sleeve = cloth(ag, L.top === "hoodie" || L.top === "bomber" ? 20 : 18, sleeveFill);
        a.cuff = cloth(ag, 13, L.top === "jersey" ? top[2] : L.top === "bomber" ? C.black : top[2]);
        if (L.top === "hoodie") a.strap = el(ag, "path", { fill: "none", stroke: C.black, "stroke-width": 4, "stroke-linecap": "round" });
      }
      ag.appendChild(a.hand); // the hand sits over the sleeve cuff
      buildHand(a.hand, i, L.acc === "gloves");
      return a;
    });
    // ribbons / cape front edge
    p.ribbons = [];
    if (L.acc === "ribbons") for (let i = 0; i < 2; i++) p.ribbons.push(cloth(b, 5, i ? C.amber : C.orange));
    if (L.acc === "scarf") { p.scarf = cloth(b, 8, "url(#" + id + "paisley)"); }
    if (L.acc === "cape") {
      p.cape = el(p.capeBack, "path", { fill: "url(#" + id + "irid)", stroke: "rgba(255,255,255,.6)", "stroke-width": 2, "stroke-linejoin": "round" });
      p.capeLine = el(p.capeBack, "path", { fill: "none", stroke: "rgba(255,255,255,.35)", "stroke-width": 1.5 });
    }
  }

  function buildFoot(f, kind, side) {
    // drawn for the right foot (toe to +x) and mirrored for the left one
    const m = g(f); m.setAttribute("transform", "scale(" + side + " 1)");
    if (kind === "none") {
      el(m, "path", { class: "al-rim", d: "M-9 -6 Q-10 -16 2 -15 Q16 -12 20 -2 Q22 6 14 7 L-8 7 Q-12 4 -9 -6 Z", fill: C.skin, "stroke-width": 2.6 });
      return;
    }
    if (kind === "chunky") {
      shape(m, C.white, "M-14 4 Q-16 14 -6 15 L26 15 Q34 14 32 6 L28 2 L-12 2 Z");
      el(m, "path", { d: "M-12 10 L30 10", stroke: C.violet, "stroke-width": 3, fill: "none" });
      shape(m, C.black, "M-11 -8 Q-12 -18 0 -18 Q10 -17 18 -8 Q27 -2 29 4 L-12 5 Z");
      el(m, "path", { d: "M-2 -16 Q8 -10 6 0 M8 -12 L16 -6", stroke: C.orange, "stroke-width": 3, fill: "none", "stroke-linecap": "round" });
      el(m, "circle", { cx: -6, cy: -2, r: 3.4, fill: C.violet, stroke: C.ol, "stroke-width": 1.6 });
      el(m, "path", { d: "M14 -2 Q22 -1 27 3", stroke: C.cyan, "stroke-width": 2, fill: "none", class: "al-glowline" });
    } else if (kind === "platform") {
      shape(m, C.gold, "M-12 2 L28 2 L28 16 Q28 18 24 18 L-10 18 Q-13 18 -13 15 Z");
      el(m, "path", { d: "M-12 9 L28 9", stroke: C.orange, "stroke-width": 2.4, fill: "none" });
      shape(m, C.purple, "M-10 -10 Q-10 -18 0 -18 Q12 -16 20 -6 Q28 -1 28 3 L-11 3 Z");
      el(m, "path", { d: "M2 -16 Q10 -10 8 0", stroke: C.amber, "stroke-width": 2.6, fill: "none" });
    } else if (kind === "glow") {
      shape(m, "#1d1630", "M-14 4 Q-16 14 -6 14 L26 14 Q33 13 31 6 L27 2 L-12 2 Z");
      el(m, "path", { d: "M-12 11 L29 11", class: "al-sole", stroke: C.cyan, "stroke-width": 3.4, fill: "none", "stroke-linecap": "round" });
      shape(m, C.white, "M-11 -8 Q-12 -18 0 -18 Q10 -17 18 -8 Q27 -2 29 4 L-12 5 Z");
      el(m, "path", { d: "M-4 -14 L16 -4 M0 -6 L20 2", stroke: C.cyan, "stroke-width": 2.4, fill: "none", "stroke-linecap": "round" });
    }
  }
  function buildHand(h, i, gloves) {
    // drawn pointing along +x (the forearm direction); mirrored for the other hand
    const m = g(h); m.setAttribute("transform", "scale(1 " + (i === 0 ? 1 : -1) + ")");
    const glove = function (d) { return el(m, "path", { d: d, fill: C.skin, class: "al-rim", "stroke-width": 2.4, "stroke-linejoin": "round" }); };
    h._open = g(m); h._point = g(m); h._fist = g(m);
    let save = m;
    m.appendChild(h._open); m.appendChild(h._point); m.appendChild(h._fist);
    const into = function (grp, d) { const e = el(grp, "path", { d: d, fill: C.skin, class: "al-rim", "stroke-width": 2.4, "stroke-linejoin": "round" }); return e; };
    into(h._open, "M-2 -6 Q6 -10 15 -7 L20 -5 Q23 -2 20 0 L14 0 L20 2 Q23 5 19 6 L12 6 Q6 9 0 7 Q-4 2 -2 -6 Z");
    into(h._open, "M4 -6 Q6 -13 11 -12 Q12 -9 9 -6 Z");
    into(h._point, "M-2 -6 Q4 -9 10 -7 L24 -6 Q28 -4 24 -2 L12 -1 Q14 4 10 7 Q4 9 0 7 Q-4 2 -2 -6 Z");
    into(h._point, "M3 -6 Q4 -14 9 -14 Q11 -11 8 -6 Z");
    el(h._point, "circle", { cx: 25, cy: -4, r: 2.4, fill: "var(--ag2)", class: "al-tip" });
    into(h._fist, "M-2 -7 Q6 -11 13 -7 Q17 -2 13 4 Q8 9 0 7 Q-4 1 -2 -7 Z");
    el(h._fist, "path", { d: "M7 -7 Q11 -3 8 3 M11 -5 Q14 -1 11 4", fill: "none", stroke: "var(--ag)", "stroke-width": 1.4, opacity: .6 });
    if (gloves) [h._open, h._point, h._fist].forEach(function (grp) {
      el(grp, "path", { d: "M-3 -8 L-3 8", stroke: C.orange, "stroke-width": 5, "stroke-linecap": "round" });
      el(grp, "circle", { cx: 5, cy: 0, r: 3.4, fill: "none", stroke: "var(--ag2)", "stroke-width": 1.8 });
    });
    h._pose = "";
    void save; void glove;
  }

  function buildTorso(R, t) {
    const L = R.look, id = R.id, top = L.top !== "none" ? TOP[L.top] : null;
    // hood / collar behind the neck
    if (L.top === "hoodie") shape(t, C.teal2, "M-24 -60 Q-30 -82 0 -84 Q30 -82 24 -60 Q0 -52 -24 -60 Z");
    // body
    const bodyD = "M-17 4 C-21 -16 -25 -38 -24 -58 Q0 -70 24 -58 C25 -38 21 -16 17 4 Q0 10 -17 4 Z";
    el(t, "path", { d: bodyD, class: "al-gw", fill: "none", "stroke-width": 12 });
    el(t, "path", { d: bodyD, class: "al-rim", fill: "url(#" + id + "gloss)", "stroke-width": 3 });
    // glowing chest lines (part of the creature, under any jacket)
    el(t, "path", { d: "M0 -50 L0 -26 M0 -42 L-9 -50 M0 -42 L9 -50 M0 -32 L-8 -24 M0 -32 L8 -24", class: "al-vein", fill: "none", "stroke-width": 2, "stroke-linecap": "round" });
    // neck
    el(t, "path", { d: "M-7 -60 L-7 -70 Q0 -74 7 -70 L7 -60 Z", fill: C.skin, class: "al-rim", "stroke-width": 2.4 });
    if (top) {
      const m = top[0], s2 = top[1], ac = top[2];
      if (L.top === "varsity") {
        shape(t, m, "M-26 -58 Q-30 -30 -24 2 L-6 4 L-8 -56 Q-16 -62 -26 -58 Z");
        shape(t, m, "M26 -58 Q30 -30 24 2 L6 4 L8 -56 Q16 -62 26 -58 Z");
        shape(t, ac, "M-24 -2 L-6 0 L-6 7 L-24 6 Z"); shape(t, ac, "M24 -2 L6 0 L6 7 L24 6 Z");
        el(t, "path", { d: "M-24 2 L-6 3.5 M24 2 L6 3.5", stroke: s2, "stroke-width": 1.6 });
        shape(t, s2, "M-9 -60 L-15 -44 L-6 -52 Z"); shape(t, s2, "M9 -60 L15 -44 L6 -52 Z");
        shape(t, C.amber, "M-18 -40 l2.2 4.6 5 .6 -3.7 3.4 1 5 -4.5 -2.5 -4.5 2.5 1 -5 -3.7 -3.4 5 -.6 Z", 1.8);
        el(t, "path", { d: "M15 -40 q4 -3 6 1 q2 4 -3 5", fill: "none", stroke: C.pink, "stroke-width": 3, "stroke-linecap": "round" });
      } else if (L.top === "hoodie") {
        shape(t, m, "M-27 -58 Q-31 -28 -24 4 L24 4 Q31 -28 27 -58 Q14 -64 6 -60 L0 -48 L-6 -60 Q-14 -64 -27 -58 Z");
        el(t, "path", { d: "M-20 -56 L14 4 M20 -56 L-14 4", stroke: C.black, "stroke-width": 4.5, fill: "none" });
        shape(t, ac, "M-6 -26 h12 v8 h-12 Z", 1.8);
        shape(t, C.black, "M-24 -14 h10 v14 h-10 Z", 1.8); shape(t, ac, "M-22 -12 h6 v4 h-6 Z", 1.2);
        el(t, "path", { d: "M-24 2 L24 2", stroke: s2, "stroke-width": 4 });
      } else if (L.top === "suit") {
        shape(t, "url(#" + id + "paisley)", "M-28 -58 Q-32 -26 -28 8 L-4 8 L-6 -56 Q-16 -62 -28 -58 Z");
        shape(t, "url(#" + id + "paisley)", "M28 -58 Q32 -26 28 8 L4 8 L6 -56 Q16 -62 28 -58 Z");
        shape(t, s2, "M-8 -62 L-22 -60 L-14 -30 L-4 -46 Z"); shape(t, s2, "M8 -62 L22 -60 L14 -30 L4 -46 Z");
        shape(t, ac, "M-4 -58 L0 -44 L4 -58 Z", 1.8);
      } else if (L.top === "jersey") {
        // futuristic football-style jersey in yellow/green/blue; no crest, no brand
        shape(t, m, "M-27 -58 Q-30 -28 -24 6 L24 6 Q30 -28 27 -58 Q14 -64 8 -60 L0 -48 L-8 -60 Q-14 -64 -27 -58 Z");
        shape(t, ac, "M-27 -58 Q-31 -30 -26 6 L-18 6 Q-22 -30 -18 -56 Z", 1.8); shape(t, ac, "M27 -58 Q31 -30 26 6 L18 6 Q22 -30 18 -56 Z", 1.8);
        el(t, "path", { d: "M-8 -60 L0 -46 L8 -60", stroke: s2, "stroke-width": 4, fill: "none", "stroke-linejoin": "round" });
        el(t, "path", { d: "M-14 -36 L-4 -20 M14 -36 L4 -20", stroke: s2, "stroke-width": 3, "stroke-linecap": "round" });
        el(t, "path", { d: "M-24 4 L24 4", stroke: s2, "stroke-width": 3 });
        el(t, "path", { d: "M-6 -30 l6 -6 6 6 -6 6 Z", fill: s2, stroke: C.ol, "stroke-width": 1.4 });
      } else if (L.top === "shirt") {
        shape(t, "url(#" + id + "paisley)", "M-27 -58 Q-30 -28 -24 6 L24 6 Q30 -28 27 -58 Q14 -64 8 -60 L0 -40 L-8 -60 Q-14 -64 -27 -58 Z");
        shape(t, s2, "M-9 -62 L-20 -58 L-12 -42 L0 -40 Z"); shape(t, s2, "M9 -62 L20 -58 L12 -42 L0 -40 Z");
        [-30, -18, -6].forEach(function (y) { el(t, "circle", { cx: 0, cy: y, r: 1.8, fill: C.amber }); });
      } else if (L.top === "bomber") {
        shape(t, "url(#" + id + "chrome)", "M-28 -58 Q-32 -28 -25 2 L25 2 Q32 -28 28 -58 Q14 -64 0 -60 Q-14 -64 -28 -58 Z");
        el(t, "path", { d: "M-20 -50 Q-24 -30 -20 -10 M18 -50 Q22 -30 18 -10", stroke: "rgba(80,90,120,.55)", "stroke-width": 1.6, fill: "none" });
        el(t, "path", { d: "M0 -60 L0 2", stroke: ac, "stroke-width": 3 });
        shape(t, C.black, "M-12 -62 Q0 -56 12 -62 L10 -54 Q0 -50 -10 -54 Z", 1.8);
        shape(t, C.black, "M-25 -2 L25 -2 L25 6 L-25 6 Z", 1.8);
        el(t, "circle", { cx: -15, cy: -40, r: 6, fill: "none", class: "al-cupS", stroke: "var(--ag)", "stroke-width": 2.4 });
      } else if (L.top === "jumpsuit") {
        shape(t, m, "M-26 -58 Q-30 -28 -22 6 L22 6 Q30 -28 26 -58 Q12 -64 0 -60 Q-12 -64 -26 -58 Z");
        el(t, "path", { d: "M0 -60 L0 6", stroke: s2, "stroke-width": 2.6 });
        shape(t, ac, "M-20 -46 h12 v9 h-12 Z", 1.8); shape(t, C.black, "M-22 -4 h44 v7 h-44 Z", 1.8); shape(t, ac, "M-4 -3 h8 v5 h-8 Z", 1.4);
        shape(t, ac, "M12 -48 l5 0 0 5 -5 0 Z", 1.4);
      }
    } else {
      el(t, "path", { d: "M-17 -2 Q0 4 17 -2", stroke: "var(--ag)", "stroke-width": 2, fill: "none", opacity: .7 });
    }
    if (L.acc === "chain") {
      el(t, "path", { d: "M-10 -60 Q-6 -36 0 -32 Q6 -36 10 -60", fill: "none", stroke: C.gold, "stroke-width": 2.2, "stroke-dasharray": "3 2" });
      shape(t, C.gold, "M0 -36 l2.6 5 5.4 .7 -4 3.7 1 5.4 -5 -2.7 -5 2.7 1 -5.4 -4 -3.7 5.4 -.7 Z", 1.6);
    }
    if (L.bottom !== "none" && L.top !== "jumpsuit") {
      const bot = BOT[L.bottom];
      shape(t, bot[0], "M-19 -2 Q0 2 19 -2 L20 10 Q0 14 -20 10 Z");
      if (L.acc !== "ribbons") el(t, "path", { d: "M-18 3 Q0 7 18 3", stroke: C.black, "stroke-width": 3, fill: "none" });
    }
    if (L.acc === "scarf") shape(t, "url(#" + id + "paisley)", "M-14 -64 Q0 -56 14 -64 L15 -56 Q0 -48 -15 -56 Z", 2);
    if (L.acc === "ribbons") { shape(t, C.black, "M-20 0 Q0 5 20 0 L20 6 Q0 11 -20 6 Z", 1.8); shape(t, C.orange, "M-3 1 h7 v7 h-7 Z", 1.6); }
  }

  function crestPath(kind, tucked) {
    if (tucked) return "M-40 -58 Q-52 -66 -50 -78 Q-42 -70 -34 -70 Z M40 -58 Q52 -66 50 -78 Q42 -70 34 -70 Z";
    if (kind === "flame") return "M-40 -60 C-52 -90 -34 -122 -2 -130 C14 -134 30 -136 44 -146 C40 -128 30 -118 16 -112 C8 -108 4 -100 10 -94 C18 -88 30 -84 36 -70 Q0 -96 -40 -60 Z" +
      " M-44 -56 C-58 -66 -62 -82 -56 -94 C-52 -82 -46 -74 -36 -68 Z M42 -60 C54 -64 62 -74 62 -86 C56 -78 48 -74 38 -70 Z";
    if (kind === "tall") return "M-36 -68 C-46 -110 -20 -150 12 -158 C40 -164 52 -140 42 -122 C36 -112 26 -114 22 -122 C30 -132 24 -144 10 -140 C-6 -134 -8 -110 4 -94 C14 -82 30 -78 36 -66 Q0 -96 -36 -68 Z" +
      " M-42 -62 C-58 -80 -60 -100 -48 -114 C-48 -98 -42 -84 -32 -74 Z";
    if (kind === "curls") {
      let s = ""; const pts = [[-40, -70], [-26, -86], [-8, -94], [10, -94], [28, -86], [42, -72], [-46, -52], [46, -52]];
      pts.forEach(function (q) { s += "M" + (q[0] - 12) + " " + q[1] + " a12 12 0 1 0 24 0 a12 12 0 1 0 -24 0 Z "; });
      return s;
    }
    if (kind === "horns") return "M-30 -74 C-42 -84 -48 -98 -40 -108 C-34 -110 -32 -100 -28 -94 C-24 -88 -18 -86 -14 -84 Z M30 -74 C42 -84 48 -98 40 -108 C34 -110 32 -100 28 -94 C24 -88 18 -86 14 -84 Z";
    if (kind === "antenna") return "";
    if (kind === "spikes") return "M-40 -62 L-60 -104 L-30 -80 L-34 -126 L-12 -88 L0 -138 L12 -90 L30 -128 L28 -84 L58 -110 L40 -64 Q0 -92 -40 -62 Z";
    return "";
  }
  function buildHead(R, h) {
    const L = R.look, id = R.id, hat = L.head !== "none" && L.head !== "halo";
    const head = "M-45 -42 C-46 -70 -24 -86 0 -86 C26 -86 46 -70 46 -42 C46 -16 26 -2 0 -2 C-24 -2 -45 -16 -45 -42 Z";
    const crest = crestPath(L.hair, hat);
    R.parts.headBack = g(h);
    if (L.hair === "antenna" && !hat) {
      el(h, "path", { d: "M-16 -80 Q-24 -110 -34 -122 M16 -80 Q24 -110 34 -122", stroke: C.skin, "stroke-width": 5, fill: "none", "stroke-linecap": "round", class: "al-rimS" });
      el(h, "circle", { cx: -35, cy: -124, r: 7, class: "al-orb" }); el(h, "circle", { cx: 35, cy: -124, r: 7, class: "al-orb" });
    }
    if (L.phones !== "none") {
      // the headband goes over the top of the skull, behind the crest
      el(h, "path", { d: "M-46 -44 Q-48 -96 0 -98 Q48 -96 46 -44", fill: "none", stroke: C.ol, "stroke-width": 9, "stroke-linecap": "round" });
      el(h, "path", { d: "M-46 -44 Q-48 -96 0 -98 Q48 -96 46 -44", fill: "none", stroke: L.phones === "chrome" ? "url(#" + id + "chrome)" : L.phones === "neon" ? "var(--ag)" : C.orange, "stroke-width": 5, "stroke-linecap": "round" });
    }
    if (crest) {
      el(h, "path", { d: crest, class: "al-gw", fill: "none", "stroke-width": 10 });
      el(h, "path", { d: crest, class: "al-rim", fill: "url(#" + id + "gloss)", "stroke-width": 3 });
      if (L.hair === "flame" && !hat) el(h, "path", { d: "M-32 -68 C-42 -94 -26 -118 0 -122 C14 -125 26 -128 36 -136 M-46 -62 C-54 -72 -56 -82 -54 -90 M8 -98 C14 -92 26 -86 30 -76", class: "al-crestline", fill: "none", "stroke-width": 3, "stroke-linecap": "round" });
      if (L.hair === "spikes" && !hat) el(h, "path", { d: "M-30 -80 L-34 -120 M0 -92 L0 -130 M30 -84 L30 -122", class: "al-crestline", fill: "none", "stroke-width": 2.4, "stroke-linecap": "round" });
      if (L.hair === "tall" && !hat) el(h, "path", { d: "M-28 -72 C-36 -110 -14 -142 12 -148 C32 -152 44 -140 40 -126", class: "al-crestline", fill: "none", "stroke-width": 3, "stroke-linecap": "round" });
    }
    el(h, "path", { d: head, class: "al-gw", fill: "none", "stroke-width": 12 });
    el(h, "path", { d: head, class: "al-rim", fill: "url(#" + id + "gloss)", "stroke-width": 3.2 });
    el(h, "path", { d: "M-30 -66 Q-18 -80 4 -80", stroke: C.hi, "stroke-width": 5, fill: "none", "stroke-linecap": "round" });
    // face (moves a little sideways to suggest the head turning)
    const face = g(h); R.parts.face = face;
    const eyes = g(face); R.parts.eyes = eyes;
    R.parts.eyeL = g(eyes); R.parts.eyeR = g(eyes);
    [R.parts.eyeL, R.parts.eyeR].forEach(function (eg, i) {
      const x = i ? 17 : -17, kind = L.face === "wink" && i === 1 ? "happy" : L.face === "wink" ? "open" : L.face;
      eg.setAttribute("data-x", x);
      if (kind === "open") {
        el(eg, "ellipse", { cx: x, cy: -46, rx: 8.5, ry: 11, class: "al-eye" });
        el(eg, "circle", { cx: x + 2.5, cy: -50, r: 2.6, fill: "#fff" });
      } else if (kind === "cool") {
        el(eg, "path", { d: "M" + (x - 10) + " -45 Q" + x + " -51 " + (x + 10) + " -45 Q" + x + " -41 " + (x - 10) + " -45 Z", class: "al-eye" });
      } else {
        el(eg, "path", { d: "M" + (x - 10) + " -42 Q" + x + " -58 " + (x + 10) + " -42", class: "al-eyeS", fill: "none", "stroke-width": 5.5, "stroke-linecap": "round" });
      }
    });
    R.parts.mouth = el(face, "path", { fill: "#ff4f9a", stroke: C.ol, "stroke-width": 2.2, "stroke-linejoin": "round" });
    R.parts.tongue = el(face, "path", { fill: "#ff9bd0" });
    if (L.eyes === "round") {
      [-17, 17].forEach(function (x) { el(face, "circle", { cx: x, cy: -46, r: 13, fill: "rgba(88,232,255,.35)", stroke: C.gold, "stroke-width": 3.4 }); });
      el(face, "path", { d: "M-4 -48 Q0 -52 4 -48", stroke: C.gold, "stroke-width": 3, fill: "none" });
    } else if (L.eyes === "visor") {
      el(face, "path", { d: "M-44 -58 Q0 -66 44 -58 L42 -36 Q0 -28 -42 -36 Z", fill: "url(#" + id + "visor)", stroke: C.ol, "stroke-width": 2.6 });
      el(face, "path", { d: "M-34 -56 Q0 -62 34 -56", stroke: "rgba(255,255,255,.7)", "stroke-width": 2.4, fill: "none" });
    } else if (L.eyes === "diva") {
      // big expressive frames with tinted translucent lenses
      [-1, 1].forEach(function (sd) {
        el(face, "path", { d: "M" + (sd * 4) + " -50 Q" + (sd * 4) + " -62 " + (sd * 18) + " -62 Q" + (sd * 36) + " -64 " + (sd * 40) + " -70 Q" + (sd * 38) + " -50 " + (sd * 28) + " -36 Q" + (sd * 14) + " -30 " + (sd * 6) + " -38 Z", fill: "rgba(255,95,174,.42)", stroke: C.violet, "stroke-width": 4, "stroke-linejoin": "round" });
      });
      el(face, "path", { d: "M-5 -48 Q0 -52 5 -48", stroke: C.violet, "stroke-width": 3.4, fill: "none" });
      el(face, "path", { d: "M-30 -56 L-22 -52 M22 -56 L30 -52", stroke: "rgba(255,255,255,.7)", "stroke-width": 2, "stroke-linecap": "round" });
    } else if (L.eyes === "shield") {
      el(face, "path", { d: "M-46 -60 L46 -60 L38 -38 Q20 -32 6 -40 L0 -44 L-6 -40 Q-20 -32 -38 -38 Z", fill: "rgba(255,154,46,.55)", stroke: C.ol, "stroke-width": 2.6, "stroke-linejoin": "round" });
      el(face, "path", { d: "M-40 -56 L40 -56", stroke: "rgba(255,255,255,.6)", "stroke-width": 2 });
    } else if (L.eyes === "star") {
      [-17, 17].forEach(function (x) { el(face, "path", { d: "M" + x + " -60 l4 9 10 1 -7.5 7 2 10 -8.5 -5 -8.5 5 2 -10 -7.5 -7 10 -1 Z", fill: "rgba(255,95,174,.55)", stroke: C.pink, "stroke-width": 2.8, "stroke-linejoin": "round" }); });
    }
    if (L.phones !== "none") {
      [-1, 1].forEach(function (s) {
        const x = s * 47, cup = L.phones === "chrome" ? "url(#" + id + "chrome)" : L.phones === "neon" ? "#1a1230" : C.orange;
        el(h, "ellipse", { cx: x, cy: -44, rx: 13, ry: 17, fill: cup, stroke: C.ol, "stroke-width": 2.8 });
        el(h, "ellipse", { cx: x + s * 3, cy: -44, rx: 8, ry: 11, fill: L.phones === "retro" ? C.violet : "#2a2140", stroke: C.ol, "stroke-width": 2 });
        el(h, "ellipse", { cx: x + s * 3, cy: -44, rx: 4, ry: 6, class: "al-cup" });
      });
    }
    if (L.head === "beanie") {
      shape(h, C.orange, "M-46 -58 Q-48 -104 0 -106 Q48 -104 46 -58 Z");
      el(h, "path", { d: "M-30 -64 L-28 -98 M-15 -62 L-14 -104 M0 -62 L0 -106 M15 -62 L14 -104 M30 -64 L28 -98", stroke: "#d97a1c", "stroke-width": 2.4 });
      shape(h, C.purple, "M-48 -70 Q0 -80 48 -70 L48 -56 Q0 -66 -48 -56 Z");
      el(h, "circle", { cx: 0, cy: -112, r: 9, fill: C.amber, stroke: C.ol, "stroke-width": 2.6 });
    } else if (L.head === "bucket") {
      shape(h, C.cream, "M-38 -70 Q-36 -104 0 -106 Q36 -104 38 -70 Z");
      el(h, "path", { d: "M-36 -80 Q0 -88 36 -80", stroke: C.purple, "stroke-width": 5, fill: "none" });
      shape(h, C.cream, "M-62 -64 Q0 -84 62 -64 Q66 -56 56 -56 Q0 -72 -56 -56 Q-66 -56 -62 -64 Z");
    } else if (L.head === "cap") {
      shape(h, C.black, "M-42 -64 Q-42 -104 0 -104 Q42 -104 42 -64 Z");
      el(h, "path", { d: "M-40 -68 Q0 -76 40 -68", class: "al-band", fill: "none", "stroke-width": 3.4 });
      shape(h, C.violet, "M-44 -64 Q0 -74 44 -64 Q52 -58 40 -56 Q0 -64 -40 -56 Q-52 -58 -44 -64 Z");
      el(h, "circle", { cx: 0, cy: -90, r: 6, class: "al-orb" });
    } else if (L.head === "fedora") {
      shape(h, "url(#" + id + "paisley)", "M-34 -70 Q-36 -104 -8 -108 Q0 -100 8 -108 Q36 -104 34 -70 Z");
      el(h, "path", { d: "M-34 -76 Q0 -84 34 -76", stroke: C.orange, "stroke-width": 6, fill: "none" });
      shape(h, C.purple, "M-64 -66 Q-30 -84 0 -80 Q36 -84 66 -72 Q70 -64 58 -64 Q0 -74 -56 -60 Q-70 -58 -64 -66 Z");
    } else if (L.head === "aviator") {
      shape(h, C.cream, "M-46 -50 Q-48 -104 0 -106 Q48 -104 46 -50 L48 -22 Q40 -16 34 -24 L34 -50 Q0 -62 -34 -50 L-34 -24 Q-40 -16 -48 -22 Z");
      el(h, "path", { d: "M-44 -54 Q0 -66 44 -54", stroke: C.orange, "stroke-width": 4, fill: "none" });
      el(h, "path", { d: "M-40 -86 Q0 -96 40 -86", stroke: C.black, "stroke-width": 6, fill: "none" });
      [-14, 14].forEach(function (x) { el(h, "circle", { cx: x, cy: -90, r: 11, fill: "rgba(88,232,255,.55)", stroke: C.gold, "stroke-width": 4 }); });
      el(h, "circle", { cx: -42, cy: -40, r: 5, fill: C.amber, stroke: C.ol, "stroke-width": 1.6 });
    } else if (L.head === "halo") {
      el(h, "ellipse", { cx: 0, cy: -132, rx: 30, ry: 8, fill: "none", class: "al-halo", "stroke-width": 5 });
    }
  }

  function buildInstrument(R, ig, kind) {
    const id = R.id;
    R.parts.keys = null; R.parts.padCells = [];
    if (kind === "keytar") {
      // local frame: x along the body, y down. Keys from x -34 to 22.
      shape(ig, C.amber, "M-54 -4 Q-60 14 -44 18 L28 14 L70 -2 L84 -14 Q88 -20 80 -20 L64 -12 L30 -10 L-46 -14 Q-56 -14 -54 -4 Z", 3);
      el(ig, "circle", { cx: -44, cy: 4, r: 10, fill: "#1b1530", stroke: C.ol, "stroke-width": 2.4 });
      el(ig, "circle", { cx: -44, cy: 4, r: 6, class: "al-cup" });
      shape(ig, C.white, "M-32 -10 L24 -9 L24 6 L-32 6 Z", 2.2);
      let k = "";
      for (let i = 1; i < 10; i++) k += "M" + (-32 + i * 5.6) + " -9.6 V6 ";
      el(ig, "path", { d: k, stroke: C.ol, "stroke-width": 1, fill: "none" });
      let bk = "";
      [0, 1, 3, 4, 5, 7, 8].forEach(function (i) { bk += "M" + (-32 + i * 5.6 + 3.6) + " -9.5 h3.4 v8.5 h-3.4 Z "; });
      el(ig, "path", { d: bk, fill: C.ol });
      R.parts.keyLight = el(ig, "rect", { x: -32, y: -10, width: 6, height: 16, rx: 1, class: "al-keylit", opacity: 0 });
      el(ig, "path", { d: "M34 -6 h22", stroke: "var(--ag)", "stroke-width": 3, "stroke-linecap": "round", class: "al-glowline" });
      el(ig, "circle", { cx: 40, cy: 4, r: 2.4, fill: C.violet }); el(ig, "circle", { cx: 48, cy: 2, r: 2.4, fill: C.cyan });
      R.parts.instPts = { a: [-6, -11], aRest: [-14, -22], b: [72, -10], strapA: [-50, -8], strapB: [80, -16], keyFrom: -30, keyTo: 20 };
    } else if (kind === "guitar") {
      // star guitar: body at the origin, neck to +x
      shape(ig, C.white, "M-40 -20 L-14 -18 L-4 -34 L4 -14 L22 -10 L6 2 L14 22 L-6 10 L-30 26 L-24 4 L-48 -2 Z", 3);
      shape(ig, "#22183a", "M-30 -12 L-12 -12 L-6 -22 L0 -10 L12 -6 L2 2 L6 14 L-6 6 L-22 16 L-18 2 Z", 0);
      el(ig, "path", { d: "M-30 -12 L-12 -12 L-6 -22 L0 -10 L12 -6 L2 2 L6 14 L-6 6 L-22 16 L-18 2 Z", fill: "none", class: "al-glowline", stroke: "var(--ag)", "stroke-width": 1.6 });
      shape(ig, "#3a2a5c", "M8 -4 L96 -4 L96 4 L8 4 Z", 2.4);
      el(ig, "path", { d: "M14 -2 H96 M14 0 H96 M14 2 H96", stroke: "rgba(255,255,255,.45)", "stroke-width": .6 });
      shape(ig, C.violet, "M94 -6 L112 -12 L118 -2 L110 2 L94 6 Z", 2.4);
      shape(ig, C.violet, "M-14 -6 h6 v12 h-6 Z M-2 -6 h6 v12 h-6 Z", 1.6);
      R.parts.instPts = { a: [-10, -4], aRest: [-16, -14], b: [70, 0], bFrom: 40, bTo: 88, strapA: [-40, -18], strapB: [96, -4] };
    } else if (kind === "bass") {
      shape(ig, C.black, "M-44 -6 Q-50 -26 -28 -26 Q-16 -26 -10 -16 Q-2 -24 8 -18 Q14 -10 8 -2 Q14 8 8 16 Q-2 22 -10 14 Q-18 26 -32 24 Q-50 20 -44 -6 Z", 3);
      el(ig, "path", { d: "M-44 -6 Q-50 -26 -28 -26 Q-16 -26 -10 -16 Q-2 -24 8 -18 Q14 -10 8 -2 Q14 8 8 16 Q-2 22 -10 14 Q-18 26 -32 24 Q-50 20 -44 -6 Z", fill: "none", stroke: C.gold, "stroke-width": 1.6 });
      shape(ig, C.gold, "M-30 -6 h12 v12 h-12 Z", 1.8);
      shape(ig, "#2a2033", "M6 -3.5 L112 -3.5 L112 3.5 L6 3.5 Z", 2.2);
      el(ig, "path", { d: "M10 -1.6 H112 M10 1.6 H112", stroke: C.gold, "stroke-width": .7 });
      shape(ig, C.black, "M110 -6 L130 -8 L132 4 L110 5 Z", 2.4);
      el(ig, "path", { d: "M116 -9 v-3 M122 -10 v-3 M118 6 v3 M126 5 v3", stroke: C.gold, "stroke-width": 3, "stroke-linecap": "round" });
      R.parts.instPts = { a: [-22, -2], aRest: [-28, -14], b: [80, 0], bFrom: 48, bTo: 104, strapA: [-40, -18], strapB: [110, -4] };
    } else if (kind === "pads") {
      shape(ig, "#241b3a", "M-46 -12 L46 -12 L50 10 L-50 10 Z", 3);
      shape(ig, C.white, "M-42 -18 L-6 -18 L-6 -11 L-42 -11 Z", 1.6);
      let k = ""; for (let i = 1; i < 6; i++) k += "M" + (-42 + i * 6) + " -18 V-11 ";
      el(ig, "path", { d: k, stroke: C.ol, "stroke-width": .8 });
      for (let r = 0; r < 2; r++) for (let c = 0; c < 8; c++) {
        const x = -40 + c * 10, y = -8 + r * 8;
        el(ig, "rect", { x: x, y: y, width: 8, height: 6, rx: 1.4, fill: ["#ff7a3d", "#b57bff", "#58e8ff", "#ff5fae"][(c + r) % 4], opacity: .55 });
        R.parts.padCells.push({ el: el(ig, "rect", { x: x - 1, y: y - 1, width: 10, height: 8, rx: 2, class: "al-keylit", opacity: 0 }), v: 0, x: x + 4, y: y + 3 });
      }
      el(ig, "circle", { cx: 26, cy: -15, r: 4, class: "al-cup" }); el(ig, "circle", { cx: 38, cy: -15, r: 4, fill: C.orange, stroke: C.ol, "stroke-width": 1.4 });
      R.parts.instPts = { a: [-20, -6], aRest: [-22, -20], b: [20, -6], bRest: [22, -20], strapA: [-46, -10], strapB: [46, -10] };
    } else {
      R.parts.instPts = null;
    }
  }

  /* ---------------- dances ----------------
     Each style maps the musical position (beats) to body targets. Intensity scales them; springs
     with different speeds per body part add weight, overlap and follow-through. */
  function bump(ph) { return Math.exp(-6.5 * ph) - .22 * smooth(.62, 1, ph); } // impact, recovery, anticipation
  function dance(style, b) {
    const ph = ((b % 1) + 1) % 1, s = Math.sin(Math.PI * b), c2 = Math.sin(Math.PI * 2 * b);
    const o = { hipX: 0, hipY: 0, hipR: 0, torR: 0, chestX: 0, shL: 0, shR: 0, headR: 0, headY: 0, fL: [0, 0, 0], fR: [0, 0, 0], arms: null, inst: 0 };
    if (style === "groove") {
      const bm = bump(ph);
      o.hipX = 10 * s; o.hipY = 6 * bm; o.hipR = -4 * s; o.torR = 3.4 * s; o.chestX = -3 * s;
      o.shL = 3 * Math.max(0, Math.sin(Math.PI * 2 * b + .6)); o.shR = 3 * Math.max(0, -Math.sin(Math.PI * 2 * b + .6));
      o.headR = 4 * Math.sin(Math.PI * b - .7); o.headY = 3 * bm;
      const fl = Math.max(0, s), fr = Math.max(0, -s);
      o.fL = [5 * fl, 9 * fl * fl, -12 * fl]; o.fR = [-5 * fr, 9 * fr * fr, 12 * fr];
      o.arms = [[-10 + 8 * s, 38 - 6 * bm, bm > .55 ? "point" : "open"], [10 + 8 * s, 38 - 6 * bm, bm > .55 ? "point" : "open"]];
      o.inst = 3 * s;
    } else if (style === "disco") {
      const bm = bump(ph), bar = Math.floor(b / 2) % 2 === 0;
      o.hipX = 9 * s; o.hipY = 7 * bm; o.hipR = -6 * s; o.torR = -4 * s; o.chestX = 2 * s;
      o.headR = (bar ? -7 : 5) + 3 * bm; o.headY = 2 * bm;
      o.fL = [0, 5 * Math.max(0, s), -8 * Math.max(0, s)]; o.fR = [0, 5 * Math.max(0, -s), 8 * Math.max(0, -s)];
      o.arms = bar ? [[-34, -46, "point"], [14, 22, "fist"]] : [[22, 30, "fist"], [-6, 20, "open"]];
      o.inst = 4 * s;
    } else if (style === "funk") {
      const e8 = bump(((2 * b) % 1 + 1) % 1), bm = bump(ph), off = bump(((b + .5) % 1 + 1) % 1);
      o.hipX = 4 * s; o.hipY = 6 * e8 * (.55 + .45 * bm) + 3 * bm; o.hipR = -2 * s; o.torR = 2.4 * c2; o.chestX = 3 * c2;
      o.shL = 5 * off; o.shR = 5 * off; o.headR = 2 * s; o.headY = 6 * bm;
      const tap = Math.max(0, s), tap2 = Math.max(0, -s);
      o.fL = [2 * tap, 7 * tap * bm, -14 * tap]; o.fR = [-2 * tap2, 7 * tap2 * bm, 14 * tap2];
      o.arms = [[-18, 20 - 8 * bm, "fist"], [18, 20 - 8 * off, "fist"]];
      o.inst = 2 * c2;
    } else if (style === "half") {
      const hb = b / 2, bm = bump(((hb % 1) + 1) % 1), sh = Math.sin(Math.PI * hb);
      o.hipX = 7 * sh; o.hipY = 11 * bm; o.hipR = -4 * sh; o.torR = 5 * sh; o.chestX = -4 * sh;
      o.headR = 3 * sh; o.headY = 8 * bump((((hb - .08) % 1) + 1) % 1);
      o.fL = [0, 4 * Math.max(0, sh), -6 * Math.max(0, sh)]; o.fR = [0, 4 * Math.max(0, -sh), 6 * Math.max(0, -sh)];
      o.arms = [[-16 + 6 * sh, 42 - 4 * bm, "open"], [16 + 6 * sh, 42 - 4 * bm, "open"]];
      o.inst = 5 * sh;
    } else if (style === "cosmic") {
      const q = Math.PI * b / 2;
      o.hipX = 9 * Math.sin(q); o.hipY = -6 - 4 * Math.sin(2 * q); o.hipR = -3 * Math.sin(q); o.torR = 5 * Math.sin(q + 1); o.chestX = 4 * Math.sin(q + .5);
      o.headR = 6 * Math.sin(q - .8); o.headY = 0;
      o.fL = [0, 5 + 4 * Math.sin(2 * q), -6 * Math.sin(q)]; o.fR = [0, 5 + 4 * Math.sin(2 * q + 1), 6 * Math.sin(q)];
      o.arms = [[-34, 6 + 16 * Math.sin(q + 1.6), "open"], [34, 6 + 16 * Math.sin(q - 1.6), "open"]];
      o.inst = 6 * Math.sin(q);
    }
    return o;
  }

  /* ---------------- per-frame pose ----------------
     s: { alive 0..1, intensity 0..1.2, beats, style, playing (notes held), pitch 0..1, hit 0..1 (new note),
          vel, env 0..1, look -1..1, glow, glow2, glowAmt 0..1, amp 0..1 (animation mode), freeze } */
  function spring(R, key, target, w, dt, impulse) {
    let sp = R.springs[key];
    if (!sp) sp = R.springs[key] = { x: target, v: 0 };
    if (impulse) sp.v += impulse;
    if (dt <= 0) { sp.x = target; sp.v = 0; return sp.x; }
    const steps = Math.ceil(dt / (1 / 120));
    const h = dt / steps;
    for (let i = 0; i < steps; i++) { sp.v += (w * w * (target - sp.x) - 2 * w * sp.v) * h; sp.x += sp.v * h; }
    return sp.x;
  }
  const D2R = Math.PI / 180;
  function update(R, s, dt) {
    R.lastS = s;
    const L = R.look, p = R.parts, st = R.state;
    R.t += dt;
    const amp = s.amp === undefined ? 1 : s.amp, alive = (s.alive || 0) * amp, I = (s.intensity === undefined ? .8 : s.intensity) * alive;
    const d = dance(s.style || "groove", s.beats || 0);
    const k = function (x) { return x * I; };
    // note accents, rate-limited so bursts of notes never make the body twitch
    if (s.hit) {
      const v = (s.vel || .7) * amp;
      st.hitHipV += 40 * v; st.nodV += 120 * v;
      st.strumV += (st.strum > 0 ? -1 : 1) * 220 * v; st.press = 1;
    }
    st.hitHipV += (-st.hitHip * 380 - st.hitHipV * 30) * dt; st.hitHip += st.hitHipV * dt;
    st.nodV += (-st.nod * 260 - st.nodV * 24) * dt; st.nod += st.nodV * dt;
    st.strumV += (-st.strum * 300 - st.strumV * 20) * dt; st.strum = clamp(st.strum + st.strumV * dt, -12, 12);
    st.press = Math.max(0, st.press - dt * 7);
    const breathe = Math.sin(R.t * 1.6) * 1.2;
    // body targets -> springs (pelvis fastest, head slowest = follow-through)
    const hipX = spring(R, "hipX", k(d.hipX), 16, dt), hipY = spring(R, "hipY", k(d.hipY) + clamp(st.hitHip, -2, 6), 18, dt);
    const hipR = spring(R, "hipR", k(d.hipR), 14, dt), torR = spring(R, "torR", k(d.torR) + (s.look || 0) * 2, 11, dt);
    const chestX = spring(R, "chX", k(d.chestX), 11, dt);
    const shL = spring(R, "shL", k(d.shL), 14, dt), shR = spring(R, "shR", k(d.shR), 14, dt);
    const headR = spring(R, "hdR", k(d.headR) + (s.look || 0) * 7 - clamp(st.nod, -3, 9) * .6, 8, dt);
    const headY = spring(R, "hdY", k(d.headY) + clamp(st.nod, -2, 6) * .5, 10, dt);
    const T_p = { x: hipX, y: -88 + hipY, r: hipR * D2R };
    const T_t = chain(T_p, { x: 0, y: 0, r: torR * D2R });
    // feet: planted on the floor, lifting/turning as the dance asks
    const feet = [0, 1].map(function (i) {
      const f = i ? d.fR : d.fL, sd = i ? 1 : -1;
      const fx = spring(R, "fx" + i, sd * 24 + k(f[0]), 20, dt), lift = spring(R, "fl" + i, Math.max(0, k(f[1])), 20, dt), fr = spring(R, "fr" + i, k(f[2]), 18, dt);
      return { x: fx, y: -10 - Math.max(0, lift), r: fr };
    });
    const footScale = L.shoes === "platform" ? 1.06 : 1;
    // legs (IK hip -> ankle)
    [0, 1].forEach(function (i) {
      const sd = i ? 1 : -1, lg = p.legs[i];
      const hip = xf(T_p, [sd * 11, 2]), ank = [feet[i].x, feet[i].y - (L.shoes === "platform" ? 6 : 0)];
      const r = ik(hip, ank, 42, 40, sd, false), knee = r.j, a2 = r.e;
      const pts = [hip, knee, a2];
      lg.limb.set(P(pts));
      if (lg.pants) {
        const top = [hip[0] + (hip[0] - T_p.x) * .25, hip[1] - 4];
        const cuffEnd = [a2[0] + (a2[0] - knee[0]) * .06, a2[1] + (a2[1] - knee[1]) * .06];
        const pp = [top, knee, cuffEnd];
        lg.pants.set(P(pp));
        if (lg.bell) lg.bell.setAttribute("d", tube([knee, [lerp(knee[0], cuffEnd[0], .5), lerp(knee[1], cuffEnd[1], .5)], [cuffEnd[0], cuffEnd[1] + 6]], [22, 30, 48]));
        if (lg.stripes.length) {
          if (L.bottom === "swirl") { lg.stripes[0].setAttribute("d", P(offsetLine(pp, sd * 7))); lg.stripes[1].setAttribute("d", P(offsetLine(pp, -sd * 5))); }
          else if (L.bottom === "track") lg.stripes[0].setAttribute("d", P(offsetLine(pp, sd * 9)));
          else if (L.bottom === "flare") lg.stripes[0].setAttribute("d", P(offsetLine([knee, [cuffEnd[0], cuffEnd[1] + 4]], sd * 6)));
          else lg.stripes[0].setAttribute("d", P(offsetLine(pp, sd * 10)));
        }
        if (lg.pad) { lg.pad.setAttribute("cx", f1(knee[0])); lg.pad.setAttribute("cy", f1(knee[1])); lg.padMark.setAttribute("d", "M" + f1(knee[0] - 4) + " " + f1(knee[1] - 3) + " l8 6 M" + f1(knee[0] + 4) + " " + f1(knee[1] - 3) + " l-8 6"); lg.pocket.setAttribute("x", f1(lerp(top[0], knee[0], .55) + sd * 6 - 6)); lg.pocket.setAttribute("y", f1(lerp(top[1], knee[1], .55) - 6)); }
      }
      lg.foot.setAttribute("transform", "translate(" + f1(a2[0]) + " " + f1(a2[1] + 2) + ") rotate(" + f1(feet[i].r * (i ? 1 : 1)) + ") scale(" + footScale + ")");
    });
    // torso
    p.torso.setAttribute("transform", "translate(" + f1(T_t.x + chestX * .3) + " " + f1(T_t.y) + ") rotate(" + f1(T_t.r / D2R) + ") scale(1 " + (1 + breathe * .006).toFixed(4) + ")");
    const neck = xf(T_t, [chestX * .3, -68]);
    const headAng = T_t.r / D2R + headR;
    p.head.setAttribute("transform", "translate(" + f1(neck[0]) + " " + f1(neck[1] + headY) + ") rotate(" + f1(headAng) + ")");
    p.face.setAttribute("transform", "translate(" + f1((s.look || 0) * 5) + " 0)");
    // blink + mouth
    st.nextBlink -= dt; if (st.nextBlink <= 0) { st.blink = 1; st.nextBlink = 2 + Math.random() * 3.5; }
    st.blink = Math.max(0, st.blink - dt * 8);
    const bl = 1 - Math.min(1, st.blink * 1.6) * .9;
    p.eyes.setAttribute("transform", "translate(0 -46) scale(1 " + bl.toFixed(2) + ") translate(0 46)");
    const open = clamp((s.env || 0) * 1.3, 0, 1) * (s.playing ? 1 : 0) * Math.max(.4, amp);
    const mo = spring(R, "mouth", open, 18, dt);
    const smile = L.face === "cool" ? .25 : .55, mo2 = smile + mo * .9, mw = 14 + mo * 2, my = -26;
    p.mouth.setAttribute("d", "M" + (-mw) + " " + my + " Q0 " + f1(my + 6 + mo2 * 14) + " " + mw + " " + my + " Q0 " + f1(my + 3) + " " + (-mw) + " " + my + " Z");
    const ty0 = my + 3 + mo2 * 6;
    p.tongue.setAttribute("d", "M-6 " + f1(ty0 + 2) + " Q0 " + f1(ty0 - 2) + " 6 " + f1(ty0 + 2) + " Q0 " + f1(my + 5 + mo2 * 12) + " -6 " + f1(ty0 + 2) + " Z");
    // instrument (strapped to the torso, swaying with the dance)
    const ip = p.instPts, playing = !!s.playing;
    const playW = spring(R, "playW", playing ? 1 : 0, 9, dt);
    let T_i = null;
    if (ip) {
      const base = L.inst === "keytar" ? { x: 2, y: -4, r: -12 } : L.inst === "guitar" ? { x: -6, y: 0, r: -24 } : L.inst === "bass" ? { x: -6, y: 4, r: -22 } : { x: 0, y: -2, r: 0 };
      const sway = spring(R, "instR", k(d.inst) + (L.inst === "guitar" || L.inst === "bass" ? -st.strum * .15 : 0), 10, dt);
      T_i = chain(T_t, { x: base.x + chestX * .3, y: base.y, r: (base.r + sway) * D2R });
      p.inst.setAttribute("transform", "translate(" + f1(T_i.x) + " " + f1(T_i.y) + ") rotate(" + f1(T_i.r / D2R) + ")");
      const sa = xf(T_i, ip.strapA), sb = xf(T_i, ip.strapB), shoulderB = xf(T_t, [20, -56]);
      p.strap.setAttribute("d", "M" + f1(sa[0]) + " " + f1(sa[1]) + " Q" + f1(shoulderB[0] - 40) + " " + f1(shoulderB[1] - 6) + " " + f1(shoulderB[0]) + " " + f1(shoulderB[1]) + " M" + f1(sb[0]) + " " + f1(sb[1]) + " L" + f1(shoulderB[0] + 4) + " " + f1(shoulderB[1] + 2));
      p.strapIn.setAttribute("d", p.strap.getAttribute("d"));
    }
    // arms: instrument targets while playing, dance targets otherwise
    const pitch = clamp(s.pitch === undefined ? .5 : s.pitch, 0, 1);
    [0, 1].forEach(function (i) {
      const sd = i ? 1 : -1, a = p.arms[i];
      const sh = xf(T_t, [sd * 23 + chestX * .3, -55 - (i ? shR : shL) + breathe * .3]);
      let tgt, poseName = "open", handAng = null;
      // dance arms (relative to the shoulder, in torso space)
      const da = d.arms ? d.arms[i] : [sd * 10, 40, "open"];
      const restArm = [sd * 6, 44];
      const danceT = xf(T_t, [sd * 24 + lerp(restArm[0], da[0], I) , -56 + lerp(restArm[1], da[1], I)]);
      if (ip) {
        let local;
        if (i === 0) {
          if (L.inst === "keytar") local = [lerp(ip.keyFrom, ip.keyTo, pitch), -10 + st.press * 3];
          else if (L.inst === "pads") { const c = s.pad === undefined ? 0 : s.pad; local = c % 8 < 4 ? [p.padCells[c % 16] ? p.padCells[c % 16].x : -20, -4 + st.press * 3] : [-24, -16]; }
          else local = [ip.a[0], ip.a[1] + st.strum];
          const rest = ip.aRest;
          local = [lerp(rest[0], local[0], playW), lerp(rest[1], local[1], playW)];
          poseName = L.inst === "keytar" || L.inst === "pads" ? "point" : "fist";
        } else {
          if (L.inst === "guitar" || L.inst === "bass") local = [lerp(ip.bTo, ip.bFrom, pitch * playW + .3 * (1 - playW)), ip.b[1] + 1];
          else if (L.inst === "pads") { const c = s.pad === undefined ? 0 : s.pad; local = c % 8 >= 4 ? [p.padCells[c % 16] ? p.padCells[c % 16].x : 20, -4 + st.press * 3] : [24, -16]; local = [lerp(ip.bRest[0], local[0], playW), lerp(ip.bRest[1], local[1], playW)]; }
          else local = ip.b;
          poseName = L.inst === "pads" ? "point" : "fist";
        }
        tgt = xf(T_i, local);
        // between notes, a free hand may join the dance a little (point on the beat)
        if (i === 1 && L.inst === "keytar" && !playing && alive > .3 && d.arms && d.arms[1][2] === "point") { tgt = [lerp(tgt[0], danceT[0], .55), lerp(tgt[1], danceT[1], .55)]; poseName = "point"; }
        handAng = L.inst === "keytar" ? (i === 0 ? 70 : T_i.r / D2R + 10) : L.inst === "pads" ? 80 : null;
      } else {
        tgt = danceT; poseName = alive > .2 ? da[2] : "open";
      }
      const tx = spring(R, "ax" + i, tgt[0], ip ? 26 : 13, dt), ty = spring(R, "ay" + i, tgt[1], ip ? 26 : 13, dt);
      const r = ik(sh, [tx, ty], 31, 29, sd, true), elb = r.j, wr = r.e;
      a.limb.set(P([sh, elb, wr]));
      if (a.sleeve) {
        const short = SHORT[L.top], cut = short ? .12 : .82;
        const cuffP = short ? [lerp(sh[0], elb[0], 1.12), lerp(sh[1], elb[1], 1.12)] : [lerp(elb[0], wr[0], cut), lerp(elb[1], wr[1], cut)], startP = [sh[0] - sd * 2, sh[1] - 2];
        a.sleeve.set(short ? P([startP, cuffP]) : P([startP, elb, cuffP]));
        a.cuff.set(short ? P([[lerp(sh[0], elb[0], 1.0), lerp(sh[1], elb[1], 1.0)], cuffP]) : P([[lerp(elb[0], wr[0], .74), lerp(elb[1], wr[1], .74)], cuffP]));
        if (a.bell) a.bell.setAttribute("d", tube([elb, [lerp(elb[0], wr[0], .5), lerp(elb[1], wr[1], .5)], [lerp(elb[0], wr[0], .96), lerp(elb[1], wr[1], .96)]], [18, 24, 30]));
        if (a.strap) a.strap.setAttribute("d", P([[lerp(sh[0], elb[0], .4), lerp(sh[1], elb[1], .4) - 6], [lerp(sh[0], elb[0], .4), lerp(sh[1], elb[1], .4) + 6]]));
      }
      const fore = Math.atan2(wr[1] - elb[1], wr[0] - elb[0]) / D2R;
      let ang = handAng === null ? fore : handAng;
      if (poseName === "point" && !ip) ang = fore;
      if (i === 1 && handAng === null) ang = fore;
      a.hand.setAttribute("transform", "translate(" + f1(wr[0]) + " " + f1(wr[1]) + ") rotate(" + f1(ang) + ") scale(1.35)");
      if (a.hand._pose !== poseName) {
        a.hand._pose = poseName;
        a.hand._open.setAttribute("display", poseName === "open" ? "inline" : "none");
        a.hand._point.setAttribute("display", poseName === "point" ? "inline" : "none");
        a.hand._fist.setAttribute("display", poseName === "fist" ? "inline" : "none");
      }
    });
    // instrument lights follow real notes
    if (p.keyLight) { p.keyLight.setAttribute("x", f1(lerp(ip.keyFrom, ip.keyTo, pitch) - 3)); p.keyLight.setAttribute("opacity", (playing ? .55 + st.press * .4 : 0).toFixed(2)); }
    if (p.padCells.length) {
      if (s.hit && s.pad !== undefined && p.padCells[s.pad % 16]) p.padCells[s.pad % 16].v = 1;
      p.padCells.forEach(function (c) { c.v = Math.max(0, c.v - dt * 3); c.el.setAttribute("opacity", (c.v * .9).toFixed(2)); });
    }
    // ribbons and cape trail behind the motion (simple delayed follow)
    if (p.ribbons.length) p.ribbons.forEach(function (rb, i) {
      const sd = i ? 1 : -1, a0 = xf(T_t, [sd * 16, 4]);
      const lagX = spring(R, "rbx" + i, a0[0], 6, dt), lagY = spring(R, "rby" + i, a0[1], 6, dt);
      rb.set("M" + f1(a0[0]) + " " + f1(a0[1]) + " Q" + f1(lagX + sd * 8) + " " + f1(lagY + 22) + " " + f1(lagX - (a0[0] - lagX) * 1.5 + sd * 4) + " " + f1(lagY + 44));
    });
    if (p.scarf) {
      const a0 = xf(T_t, [8, -60]);
      const lx = spring(R, "scx", a0[0], 5, dt), ly = spring(R, "scy", a0[1], 5, dt);
      p.scarf.set("M" + f1(a0[0]) + " " + f1(a0[1]) + " Q" + f1(lx + 16) + " " + f1(ly + 10) + " " + f1(lx + 18 - (a0[0] - lx) * 1.8) + " " + f1(ly + 30));
    }
    if (p.cape) {
      const l = xf(T_t, [-26, -58]), r2 = xf(T_t, [26, -58]);
      const lx = spring(R, "cpx", T_t.x, 5, dt), sw = (lx - T_t.x) * 1.6;
      p.cape.setAttribute("d", "M" + f1(l[0]) + " " + f1(l[1]) + " Q" + f1(l[0] - 22 + sw) + " " + f1(l[1] + 60) + " " + f1(l[0] - 30 + sw * 1.6) + " " + f1(l[1] + 120) + " L" + f1(r2[0] + 30 + sw * 1.6) + " " + f1(r2[1] + 120) + " Q" + f1(r2[0] + 22 + sw) + " " + f1(r2[1] + 60) + " " + f1(r2[0]) + " " + f1(r2[1]) + " Z");
      p.capeLine.setAttribute("d", "M" + f1(l[0] - 10) + " " + f1(l[1] + 20) + " Q" + f1(l[0] - 18 + sw) + " " + f1(l[1] + 70) + " " + f1(l[0] - 22 + sw * 1.4) + " " + f1(l[1] + 110));
    }
    // glow: always softly lit; colour and intensity come from the sound
    const ga = clamp(s.glowAmt === undefined ? 0 : s.glowAmt, 0, 1);
    const glow = s.glow || GLOW[L.glow] || GLOW.cyan;
    R.root.setAttribute("style", "--ag:" + glow + ";--ag2:" + (s.glow2 || "#ff9a2e") + ";--gw:" + (.22 + ga * .45).toFixed(2));
    R.stops[0].setAttribute("stop-color", glow); R.stops[1].setAttribute("stop-color", glow);
    R.aura.setAttribute("opacity", (.28 + ga * .6 + (s.bloom || 0) * .5).toFixed(2));
    const auraS = 1 + (s.bloom || 0) * .35 + ga * .08;
    R.aura.setAttribute("transform", "translate(0 -150) scale(" + auraS.toFixed(3) + ") translate(0 150)");
    R.shadow.setAttribute("rx", f1(48 + Math.abs(hipX) * .6 - Math.max(0, -hipY) * .8));
    R.points = { head: xf({ x: neck[0], y: neck[1], r: headAng * D2R }, [0, -96]), phones: xf({ x: neck[0], y: neck[1], r: headAng * D2R }, [-47, -44]), eyes: xf({ x: neck[0], y: neck[1], r: headAng * D2R }, [17, -46]),
      hair: xf({ x: neck[0], y: neck[1], r: headAng * D2R }, [-30, -100]), face: xf({ x: neck[0], y: neck[1], r: headAng * D2R }, [0, -22]), top: xf(T_t, [-18, -40]), bottom: [(p.legs[0] && feet[0].x) * .6, -50],
      shoes: [feet[1].x, feet[1].y], inst: T_i ? xf(T_i, [0, 0]) : xf(T_t, [20, -10]), acc: xf(T_t, [14, -6]), glow: xf(T_t, [0, -40]) };
  }
  // Static pose for thumbnails and cards: resting, holding the instrument.
  function pose(R) { R.springs = {}; update(R, { alive: 0, amp: 1 }, 0); }

  window.RastroAlien = { SLOTS: SLOTS, ITEMS: ITEMS, DANCES: DANCES, DEFAULT_LOOK: DEFAULT_LOOK, GLOW: GLOW, itemInfo: itemInfo, normalise: normalise, create: create };
})();
