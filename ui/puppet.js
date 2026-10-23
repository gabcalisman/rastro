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
  function el(parent, tag, attrs) { const e = document.createElementNS(NS, tag); for (const k in attrs) e.setAttribute(k, attrs[k]); parent.appendChild(e); return e; }
  function create(parent, lookId) {
    lookId = LOOKS[lookId] ? lookId : ALIAS[lookId] || "disco";
    const id = "pp" + (++uid), L = LOOKS[lookId], W = L.w, H = 700;
    const root = el(parent, "g", { class: "puppet" });
    const href = L.img + ".png", ghref = L.img + "_glow.png";
    const hx = L.head[0], hy = L.head[1], hrx = L.head[2], hry = L.head[3], hip = L.hip[1];
    const defs = el(root, "defs", {});
    defs.innerHTML =
      '<linearGradient id="' + id + 'lg" gradientUnits="userSpaceOnUse" x1="0" y1="' + (hip - 34) + '" x2="0" y2="' + (hip + 6) + '"><stop offset="0" stop-color="#000"/><stop offset="1" stop-color="#fff"/></linearGradient>' +
      '<linearGradient id="' + id + 'bg" gradientUnits="userSpaceOnUse" x1="0" y1="' + (hip - 6) + '" x2="0" y2="' + (hip + 34) + '"><stop offset="0" stop-color="#fff"/><stop offset="1" stop-color="#000"/></linearGradient>' +
      '<radialGradient id="' + id + 'hh" gradientUnits="userSpaceOnUse" cx="' + hx + '" cy="' + hy + '" r="1" gradientTransform="translate(' + hx + ' ' + hy + ') scale(' + hrx + ' ' + hry + ') translate(' + (-hx) + ' ' + (-hy) + ')"><stop offset=".82" stop-color="#000" stop-opacity="1"/><stop offset="1" stop-color="#000" stop-opacity="0"/></radialGradient>' +
      '<radialGradient id="' + id + 'hw" gradientUnits="userSpaceOnUse" cx="' + hx + '" cy="' + hy + '" r="1" gradientTransform="translate(' + hx + ' ' + hy + ') scale(' + hrx + ' ' + hry + ') translate(' + (-hx) + ' ' + (-hy) + ')"><stop offset=".82" stop-color="#fff" stop-opacity="1"/><stop offset="1" stop-color="#fff" stop-opacity="0"/></radialGradient>' +
      '<mask id="' + id + 'mL" maskUnits="userSpaceOnUse" x="-40" y="-40" width="' + (W + 80) + '" height="' + (H + 80) + '"><rect x="-40" y="-40" width="' + (W + 80) + '" height="' + (H + 80) + '" fill="url(#' + id + 'lg)"/></mask>' +
      '<mask id="' + id + 'mB" maskUnits="userSpaceOnUse" x="-40" y="-40" width="' + (W + 80) + '" height="' + (H + 80) + '"><rect x="-40" y="-40" width="' + (W + 80) + '" height="' + (H + 80) + '" fill="url(#' + id + 'bg)"/><ellipse cx="' + hx + '" cy="' + hy + '" rx="' + hrx + '" ry="' + hry + '" fill="url(#' + id + 'hh)"/></mask>' +
      '<mask id="' + id + 'mH" maskUnits="userSpaceOnUse" x="-40" y="-40" width="' + (W + 80) + '" height="' + (H + 80) + '"><ellipse cx="' + hx + '" cy="' + hy + '" rx="' + hrx + '" ry="' + hry + '" fill="url(#' + id + 'hw)"/></mask>' +
      '<mask id="' + id + 'mG" maskUnits="userSpaceOnUse" x="0" y="0" width="' + W + '" height="' + H + '"><image href="' + ghref + '" width="' + W + '" height="' + H + '"/></mask>' +
      '<mask id="' + id + 'mR" maskUnits="userSpaceOnUse" x="-40" y="-40" width="' + (W + 80) + '" height="' + (H + 80) + '"><image href="' + ghref + '" width="' + W + '" height="' + H + '"/><image class="rimcut" href="' + ghref + '" width="' + W + '" height="' + H + '" style="filter:brightness(0)" transform="translate(10 0)"/></mask>' +
      '<radialGradient id="' + id + 'au"><stop offset="0" stop-color="#fff" stop-opacity=".6" class="a0"/><stop offset="1" stop-color="#fff" stop-opacity="0" class="a1"/></radialGradient>';
    const R = { root: root, id: id, look: lookId, L: L, springs: {}, t: 0, st: { eye: 0, mouth: 0, hit: 0, hitV: 0, nod: 0, nodV: 0, strum: 0, strumV: 0, press: 0, hop: 0, hopV: 0, sq: 0, sqV: 0, lastHop: -1, air: false } };
    R.aura = el(root, "ellipse", { cx: W / 2, cy: H * .45, rx: W * .62, ry: H * .52, fill: "url(#" + id + "au)", opacity: ".4" });
    R.shadow = el(root, "ellipse", { cx: L.hip[0], cy: L.feet + 4, rx: W * .38, ry: 12, fill: "rgba(0,0,0,.45)" });
    // "whole" carries the game-feel body motion: hops with squash on landing, fake turning (yaw), lean.
    const whole = R.whole = el(root, "g", {});
    // glow layers (silhouette blur tinted by the sound): base glow + a cyan WOBBLE echo
    R.glowG = el(whole, "g", {});
    R.glow = el(R.glowG, "rect", { x: 0, y: 0, width: W, height: H, fill: "#5fe3ff", mask: "url(#" + id + "mG)", opacity: ".5" });
    R.wobG = el(whole, "g", { opacity: "0" });
    el(R.wobG, "rect", { x: 0, y: 0, width: W, height: H, fill: "#2fe0ff", mask: "url(#" + id + "mG)" });
    R.legs = el(whole, "g", {}); el(R.legs, "image", { href: href, width: W, height: H, mask: "url(#" + id + "mL)" });
    R.body = el(whole, "g", {}); el(R.body, "image", { href: href, width: W, height: H, mask: "url(#" + id + "mB)" });
    // instrument lights sit on the body layer so they move with the hands
    R.fx = el(R.body, "g", {});
    if (L.keys) { R.keyLight = el(R.fx, "ellipse", { rx: 16, ry: 9, fill: "#fff", opacity: 0, class: "pp-light" }); }
    if (L.strum) { R.string = el(R.fx, "path", { d: "M" + L.fret[0] + " L" + L.strum, stroke: "#fff", "stroke-width": 2.4, fill: "none", opacity: 0, class: "pp-light" }); }
    if (L.pads) { R.padCells = []; for (let r = 0; r < 2; r++) for (let c = 0; c < 8; c++) { const x0 = L.pads[0][0] + (L.pads[1][0] - L.pads[0][0]) * (c + .5) / 8, y0 = L.pads[0][1] + (L.pads[1][1] - L.pads[0][1]) * (r + .5) / 2; R.padCells.push({ el: el(R.fx, "rect", { x: x0 - 6, y: y0 - 4, width: 12, height: 8, rx: 2, fill: "#fff", opacity: 0, class: "pp-light" }), v: 0 }); } }
    R.head = el(whole, "g", {});
    // rim light: the painted silhouette lit from the side the body turns away from (sells the volume)
    R.rimG = el(R.head.parentNode, "g", { opacity: "0" }); el(R.rimG, "rect", { x: 0, y: 0, width: W, height: H, fill: "#ffffff", mask: "url(#" + id + "mR)" }); el(R.head, "image", { href: href, width: W, height: H, mask: "url(#" + id + "mH)" });
    R.stops = defs.querySelectorAll("#" + id + "au stop"); R.rimCut = defs.querySelector(".rimcut");
    // expression: soft glows over the painted eyes and mouth (eyes flash on notes, mouth opens on accents)
    defs.insertAdjacentHTML("beforeend", '<radialGradient id="' + id + 'eg"><stop offset="0" stop-color="#bff8ff" stop-opacity=".95"/><stop offset=".45" stop-color="#2fe0ff" stop-opacity=".5"/><stop offset="1" stop-color="#2fe0ff" stop-opacity="0"/></radialGradient>' +
      '<radialGradient id="' + id + 'mg"><stop offset="0" stop-color="#ffd0f4" stop-opacity=".95"/><stop offset=".5" stop-color="#ff4fd8" stop-opacity=".5"/><stop offset="1" stop-color="#ff4fd8" stop-opacity="0"/></radialGradient>');
    const ex = L.eyes || [hx - 4, hy + 22], mo = L.mouth || [hx + 10, hy + 62];
    R.eyeG = el(R.head, "ellipse", { cx: ex[0], cy: ex[1], rx: hrx * .62, ry: 26, fill: "url(#" + id + "eg)", opacity: 0, style: "mix-blend-mode:screen" });
    R.mouthG = el(R.head, "ellipse", { cx: mo[0], cy: mo[1], rx: 30, ry: 16, fill: "url(#" + id + "mg)", opacity: 0, style: "mix-blend-mode:screen" });
    R.mo = mo;
    R.update = function (s, dt) { update(R, s, dt); };
    R.pose = function () { R.springs = {}; update(R, { alive: 0 }, 0); };
    update(R, { alive: 0 }, 0);
    return R;
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
    if (R.eyeG) {
      const amp0 = s.amp === undefined ? 1 : s.amp;
      R.eyeG.setAttribute("opacity", (Math.min(1, st.eye * .8 + (s.glowAmt || 0) * .15) * Math.min(1, amp0 + .3)).toFixed(2));
      R.mouthG.setAttribute("opacity", (st.mouth * .85 * Math.min(1, amp0 + .3)).toFixed(2));
      R.mouthG.setAttribute("ry", (10 + st.mouth * 10).toFixed(1));
    }
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
    R.whole.setAttribute("transform", "translate(" + f1(fx0 + idleX) + " " + f1(fy0 + st.hop * .8) + ") rotate(" + lean.toFixed(2) + ") skewY(" + (yaw * 3.2).toFixed(2) + ") scale(" + sxW.toFixed(4) + " " + syW.toFixed(4) + ") translate(" + (-fx0) + " " + (-fy0) + ")");
    R.rimG.setAttribute("opacity", (clamp(Math.abs(yaw) * .5 + (s.glowAmt || 0) * .25, 0, .55) * Math.min(1, amp + .2)).toFixed(2));
    R.rimCut.setAttribute("transform", "translate(" + (yaw >= 0 ? 9 : -9) + " 3)");
    R.rimG.firstChild.setAttribute("fill", s.glow || "#5fe3ff");
    // legs: shear from the planted feet, compress for the knee dip
    const F = L.feet, Lh = F - L.hip[1], sx = hipX / Lh, sy = 1 - dip / Lh;
    R.legs.setAttribute("transform", "matrix(1 0 " + (-sx).toFixed(5) + " " + sy.toFixed(5) + " " + (sx * F).toFixed(2) + " " + (F * (1 - sy)).toFixed(2) + ")");
    const px = L.hip[0], py = L.hip[1];
    const bodyT = "translate(" + f1(hipX) + " " + f1(dip + breathe * .4) + ") rotate(" + torR.toFixed(2) + " " + px + " " + py + ")";
    R.body.setAttribute("transform", bodyT);
    R.head.setAttribute("transform", bodyT + " rotate(" + headR.toFixed(2) + " " + L.neck[0] + " " + L.neck[1] + ") translate(0 " + f1(headY) + ")");
    R.glowG.setAttribute("transform", bodyT);
    R.rimG.setAttribute("transform", bodyT);
    // WOBBLE: a cyan echo of the outline that sways sideways with the real WOBBLE activity
    const wob = clamp(s.wob || 0, 0, 1);
    R.wobG.setAttribute("opacity", (wob * .55).toFixed(2));
    R.wobG.setAttribute("transform", bodyT + " translate(" + f1(Math.sin(R.t * (3 + (s.wobRate || 1) * 2)) * (6 + wob * 10)) + " 0)");
    // glow colour/intensity from the sound (envelope, effects, filter)
    const ga = clamp(s.glowAmt || 0, 0, 1), col = s.glow || "#5fe3ff";
    R.glow.setAttribute("fill", col); R.glow.setAttribute("opacity", (.28 + ga * .6).toFixed(2));
    R.stops[0].setAttribute("stop-color", s.aura || col); R.stops[1].setAttribute("stop-color", s.aura || col);
    const bloom = clamp(s.bloom || 0, 0, 1);
    R.aura.setAttribute("opacity", (.18 + ga * .35 + bloom * .55).toFixed(2));
    R.aura.setAttribute("transform", "translate(" + W / 2 + " " + 315 + ") scale(" + (1 + bloom * .45 + ga * .05).toFixed(3) + ") translate(" + (-W / 2) + " -315)");
    const air = clamp(-st.hop / 90, 0, 1);
    R.shadow.setAttribute("rx", f1((W * .36 + Math.abs(hipX) * .5) * (1 - air * .3) * (1 + sq * .06)));
    R.shadow.setAttribute("opacity", (1 - air * .45).toFixed(2));
    // instrument lights from real notes
    const pitch = clamp(s.pitch === undefined ? .5 : s.pitch, 0, 1), playing = !!s.playing;
    if (R.keyLight) { const a = L.keys[0], b = L.keys[1]; R.keyLight.setAttribute("cx", f1(a[0] + (b[0] - a[0]) * pitch)); R.keyLight.setAttribute("cy", f1(a[1] + (b[1] - a[1]) * pitch)); R.keyLight.setAttribute("fill", col); R.keyLight.setAttribute("opacity", (playing ? .45 + st.press * .45 : 0).toFixed(2)); }
    if (R.string) { R.string.setAttribute("stroke", col); R.string.setAttribute("opacity", (playing ? .35 + Math.abs(st.strum) / 10 * .5 : 0).toFixed(2)); R.string.setAttribute("transform", "translate(0 " + f1(Math.sin(R.t * 90) * Math.abs(st.strum) * .25) + ")"); }
    if (R.padCells) {
      if (s.hit && s.pad !== undefined && R.padCells[s.pad % 16]) R.padCells[s.pad % 16].v = 1;
      R.padCells.forEach(function (c) { c.v = Math.max(0, c.v - dt * 3); c.el.setAttribute("fill", col); c.el.setAttribute("opacity", (c.v * .85).toFixed(2)); });
    }
    R.points = { head: [L.head[0], L.head[1] - L.head[3] * .7], phones: [L.head[0] - L.head[2] * .8, L.head[1] + 10], eyes: [L.head[0] + 20, L.head[1] + 20], top: [L.hip[0] - 40, L.hip[1] - 120], bottom: [L.hip[0] - 30, L.hip[1] + 100], shoes: [L.hip[0] + 80, L.feet - 30], inst: L.keys ? L.keys[1] : L.fret ? L.fret[1] : L.pads ? L.pads[1] : [W / 2, 300] };
  }
  window.RastroPuppet = { LOOKS: LOOKS, ALIAS: ALIAS, create: create };
})();
