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
    const cx = w / 2, cy = h * .47, R0 = Math.min(w, h) * .25;
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
      V.lastNote = s.noteName || ""; V.noteAlpha = 1;
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
      g.strokeStyle = rgba(L[1] === 1 ? [255, 255, 255] : col, (L[1] === 1 ? .85 : L[1]) * (.35 + .65 * Math.min(1, (s.meter || 0) * 6 + .15)));
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
    g.globalCompositeOperation = "source-over";
    // last note name, fading (what you just played)
    V.noteAlpha = Math.max(0, V.noteAlpha - dt * .7);
    if (V.lastNote && V.noteAlpha > .02) { g.fillStyle = "rgba(255,255,255," + (V.noteAlpha * .8).toFixed(2) + ")"; g.font = "700 " + Math.round(R0 * .32) + "px Poppins, sans-serif"; g.textAlign = "center"; g.textBaseline = "middle"; g.fillText(V.lastNote, cx, cy); }
    // effect legend (only the ones that are really active)
    let lx = 14; g.font = "700 11px Poppins, sans-serif"; g.textAlign = "left"; g.textBaseline = "middle";
    ["GHOST", "CRUMBLE", "BLOOM", "WOBBLE", "DUST", "CRUSH"].forEach(function (nm, i) {
      if (fx[i] < .02) return; const c = rgb(FXC[i]); g.fillStyle = rgba(c, .25 + .75 * Math.min(1, fx[i] * 2)); g.beginPath(); g.arc(lx + 5, h - 18, 5, 0, 6.283); g.fill(); g.fillStyle = "rgba(255,255,255,.8)"; g.fillText(nm, lx + 14, h - 18); lx += g.measureText(nm).width + 30;
    });
  }
  window.RastroViz = { create: create, noteName: function (n) { return NOTE[((n % 12) + 12) % 12] + (Math.floor(n / 12) - 1); } };
})();
