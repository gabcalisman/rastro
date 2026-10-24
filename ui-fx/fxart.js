/* RASTRO FX — identity of each device: colour, material, lettering, labels and its own animated
   display. Displays draw what the module does with its *current* settings; their energy comes from
   the real input level and the module's mix. Purely visual: nothing here touches the audio. */
(function() {
"use strict";
// Safari < 16 has no CanvasRenderingContext2D.roundRect: small polyfill (rectangle with radius).
if (window.CanvasRenderingContext2D && !CanvasRenderingContext2D.prototype.roundRect)
{
    CanvasRenderingContext2D.prototype.roundRect = function(x, y, w, h, r) {
        r = Math.min(typeof r === "number" ? r : 0, Math.abs(w) / 2, Math.abs(h) / 2);
        this.moveTo(x + r, y);
        this.arcTo(x + w, y, x + w, y + h, r);
        this.arcTo(x + w, y + h, x, y + h, r);
        this.arcTo(x, y + h, x, y, r);
        this.arcTo(x, y, x + w, y, r);
        this.closePath();
    };
}
const TAU = Math.PI * 2;
const clamp = function(v, a, b) { return v < a ? a : v > b ? b : v; };
function hex(c, a)
{
    const n = parseInt(c.slice(1), 16);
    return "rgba(" + (n >> 16 & 255) + "," + (n >> 8 & 255) + "," + (n & 255) + "," + a + ")";
}
function hash(n)
{
    n = (n ^ 61) ^ (n >>> 16);
    n = n + (n << 3);
    n = n ^ (n >>> 4);
    n = Math.imul(n, 0x27d4eb2d);
    return ((n ^ (n >>> 15)) >>> 0) / 4294967296;
}

// id: colour, material (CSS class), lettering style, knob skin, Portuguese copy and control labels.
const DEV = [
    {
        c : "#b06bff",
        mat : "glass",
        font : "serif-i",
        knob : "purple",
        sub : "Eco fantasma",
        tag : "ecos filtrados do PLAY",
        labels : [ "TEMPO", "FEEDBACK", "TOM", "RASTRO" ]
    },
    {
        c : "#ffa02e",
        mat : "tile",
        font : "slab",
        knob : "amber",
        sub : "Fragmentos",
        tag : "repete pedaços do som",
        labels : [ "RITMO", "VARIAÇÃO", "MUTAÇÃO" ]
    },
    {
        c : "#ff4fd8",
        mat : "lacquer",
        font : "script",
        knob : "pearl",
        sub : "Espaço difuso",
        tag : "reverb aveludado",
        labels : [ "TAMANHO", "TOM", "CAUDA", "RASTRO" ]
    },
    {
        c : "#2fe0ff",
        mat : "tape",
        font : "wide",
        knob : "teal",
        sub : "Fita errante",
        tag : "wow & flutter de fita",
        labels : [ "PROFUND.", "VELOC." ]
    },
    {
        c : "#ffd23f",
        mat : "vinyl",
        font : "serif",
        knob : "cream",
        sub : "Poeira",
        tag : "textura que segue o som",
        labels : [ "QUANTIDADE", "CARÁTER" ]
    },
    {
        c : "#c9a6ff",
        mat : "lcd",
        font : "pixel",
        knob : "rubber",
        sub : "Bitcrusher",
        tag : "bits e amostras a menos",
        labels : [ "ESMAGAR" ]
    },
    {
        c : "#ff6b6b",
        mat : "velvet",
        font : "round",
        knob : "red",
        sub : "Boca de vogais",
        tag : "ressonâncias A E I O U",
        labels : [ "VOGAL", "FORMANTE", "RESSON." ]
    },
    {
        c : "#6f8bff",
        mat : "cosmos",
        font : "wide",
        knob : "chrome",
        sub : "Flanger orbital",
        tag : "jato que passa pelo espaço",
        labels : [ "VELOC.", "PROFUND.", "FEEDBACK", "CENTRO" ]
    },
    {
        c : "#ff8a3d",
        mat : "comet",
        font : "slab",
        knob : "amp",
        sub : "Eco limpo",
        tag : "repetições com cor opcional",
        labels : [ "TEMPO", "FEEDBACK", "TOM", "COR" ]
    },
    {
        c : "#5ef2b0",
        mat : "mint",
        font : "round",
        knob : "ice",
        sub : "Coro de 3 vozes",
        tag : "chorus largo e doce",
        labels : [ "VELOC.", "PROFUND.", "ATRASO", "ABERTURA" ]
    },
    {
        c : "#9ff3ff",
        mat : "prism",
        font : "serif-i",
        knob : "pearl",
        sub : "Eco de cristal",
        tag : "ecos que mudam de altura",
        labels : [ "ALTURA", "TEMPO", "FEEDBACK", "TOM" ]
    },
    {
        c : "#ff5a2a",
        mat : "copper",
        font : "script",
        knob : "bakelite",
        sub : "Válvula & fita",
        tag : "saturação 2x oversampled",
        labels : [ "DRIVE", "TOM", "BIAS", "MODO" ],
        modes : [ "DRIVE", "VÁLVULA", "FITA" ]
    },
    {
        c : "#b6ff3d",
        mat : "rubber",
        font : "round",
        knob : "rubber",
        sub : "Compressor elástico",
        tag : "aperta e satura",
        labels : [ "DRIVE", "LIMIAR", "RAZÃO", "SOLTURA" ]
    },
    {
        c : "#4fb6ff",
        mat : "steel",
        font : "wide",
        knob : "chrome",
        sub : "Filtro ressonante",
        tag : "passa-baixa · banda · alta",
        labels : [ "CORTE", "RESSON.", "TIPO" ],
        modes : [ "LP", "BP", "HP" ]
    },
    {
        c : "#ff7ad9",
        mat : "neon",
        font : "pixel",
        knob : "neon",
        sub : "Panorama em 8 passos",
        tag : "desenhe o movimento",
        labels : [ "VELOC.", "PROFUND.", "SUAVIZAR", "FASE" ]
    }
];

/* ---------------- display painters ----------------
   g: 2D context in CSS pixels, w/h size, t seconds, s = { p:[physical values], n:[normalized],
   mix, e (input energy 0..1), on (0..1), pan:[8] }                                              */
function glowLine(g, c, wdt, blur)
{
    g.strokeStyle = c;
    g.lineWidth = wdt;
    g.shadowColor = c;
    g.shadowBlur = blur;
}
function wave(x, t, k) { return Math.sin(x * k + t) * .6 + Math.sin(x * k * 2.3 + t * 1.7) * .25; }
const PAINT = [
    // GHOST: a phrase followed by its echoes, each later, darker and blurrier.
    function(g, w, h, t, s, c) {
        const repeats = 2 + Math.round(s.n[1] * 5), gap = 14 + s.n[0] * 30,
              dark = .55 + s.n[2] * .45;
        for (let r = repeats; r >= 0; r--)
        {
            const a = (r === 0 ? 1 : Math.pow(.35 + s.n[1] * .55, r) * dark) * (.35 + s.on * .65);
            glowLine(g, hex(c, a), r === 0 ? 2.2 : 1.6, 10 - r);
            g.beginPath();
            for (let x = 0; x <= 60; x++)
            {
                const px = 10 + x + r * gap, env = Math.sin(x / 60 * Math.PI);
                const y = h / 2 + wave(x / 9, t * 3 - r * .6, 1) * env * h * .3 * (.4 + s.e * .9) *
                                      (r ? 1 - r * .06 : 1);
                if (px < w - 6)
                    x === 0 ? g.moveTo(px, y) : g.lineTo(px, y);
            }
            g.stroke();
        }
    },
    // CRUMBLE: blocks of audio breaking into windowed fragments.
    function(g, w, h, t, s, c) {
        const n = 6 + Math.round(s.n[0] * 10), bw = (w - 20) / n;
        for (let i = 0; i < n; i++)
        {
            const seed = hash(i * 31 + Math.floor(t * (2 + s.n[0] * 6)));
            const lift = (seed - .5) * s.n[2] * 22, len = .35 + (1 - s.n[1]) * .55 + seed * .1;
            const hh = h * (.15 + .55 * (.4 + s.e * .6) * (.5 + .5 * Math.sin(i * 1.7 + t * 4)));
            g.fillStyle = hex(c, (.35 + .65 * s.on) * (.55 + seed * .45));
            g.shadowColor = c;
            g.shadowBlur = 8;
            g.beginPath();
            g.roundRect(10 + i * bw + 1, h / 2 - hh / 2 + lift, bw * len, hh, 3);
            g.fill();
        }
    },
    // BLOOM: petals opening with Size, glowing with the tail.
    function(g, w, h, t, s, c) {
        const cx = w / 2, cy = h / 2 + 6, size = (.45 + s.n[0] * .6) * h * .42 * (1 + s.e * .15);
        const petals = 7;
        for (let ring = 2; ring >= 0; ring--)
        {
            for (let i = 0; i < petals; i++)
            {
                const a = i / petals * TAU + t * .15 * (ring % 2 ? -1 : 1) + ring * .3;
                const r = size * (1 - ring * .22) * (1 + .06 * Math.sin(t * 2 + i));
                g.save();
                g.translate(cx, cy);
                g.rotate(a);
                g.fillStyle = hex(ring === 0 ? "#ffd6f4" : c,
                                  (.12 + .25 * s.n[2]) * (.4 + .6 * s.on) * (1 - ring * .2));
                g.shadowColor = c;
                g.shadowBlur = 14;
                g.beginPath();
                g.ellipse(0, -r * .5, r * .22, r * .55, 0, 0, TAU);
                g.fill();
                g.restore();
            }
        }
        g.fillStyle = hex("#fff3fb", .7 * (.4 + s.on * .6));
        g.beginPath();
        g.arc(cx, cy, 4 + s.e * 3, 0, TAU);
        g.fill();
    },
    // WOBBLE: tape reels turning, the tape line bending with depth/rate.
    function(g, w, h, t, s, c) {
        const rate = s.p[1], depth = s.n[0], y = h * .7;
        [w * .28, w * .72].forEach(function(x, k) {
            g.save();
            g.translate(x, h * .38);
            g.rotate(t * (1.5 + rate) * (k ? 1 : 1.04));
            g.strokeStyle = hex(c, .9);
            g.lineWidth = 2;
            g.shadowColor = c;
            g.shadowBlur = 8;
            g.beginPath();
            g.arc(0, 0, h * .22, 0, TAU);
            g.stroke();
            for (let i = 0; i < 3; i++)
            {
                g.rotate(TAU / 3);
                g.beginPath();
                g.moveTo(0, 0);
                g.lineTo(h * .2, 0);
                g.stroke();
            }
            g.fillStyle = hex(c, .4);
            g.beginPath();
            g.arc(0, 0, h * .06, 0, TAU);
            g.fill();
            g.restore();
        });
        glowLine(g, hex(c, .5 + .5 * s.on), 2, 10);
        g.beginPath();
        for (let x = 10; x <= w - 10; x += 2)
        {
            const yy = y + Math.sin(x * .05 + t * rate * 4) * depth * 10 * (.5 + s.e) +
                       Math.sin(x * .17 + t * 17) * depth * 1.5;
            x === 10 ? g.moveTo(x, yy) : g.lineTo(x, yy);
        }
        g.stroke();
    },
    // DUST: a spinning record with specks and crackles that follow the source.
    function(g, w, h, t, s, c) {
        const cx = w / 2, cy = h / 2, r = h * .42;
        g.fillStyle = "#120a08";
        g.beginPath();
        g.arc(cx, cy, r, 0, TAU);
        g.fill();
        g.strokeStyle = "rgba(255,255,255,.06)";
        g.lineWidth = 1;
        for (let k = 6; k < r; k += 3)
        {
            g.beginPath();
            g.arc(cx, cy, k, 0, TAU);
            g.stroke();
        }
        g.fillStyle = hex(c, .9);
        g.beginPath();
        g.arc(cx, cy, r * .3, 0, TAU);
        g.fill();
        g.strokeStyle = "rgba(255,255,255,.25)";
        g.lineWidth = 2;
        g.beginPath();
        g.arc(cx, cy, r * .8, t * 3, t * 3 + .4);
        g.stroke();
        const n = Math.round(10 + s.n[0] * 70);
        for (let i = 0; i < n; i++)
        {
            const life = (t * (.3 + hash(i) * .5) + hash(i * 7)) % 1,
                  a = Math.sin(life * Math.PI) * (.3 + s.e * .9) * (.3 + .7 * s.on);
            const x = 8 + hash(i * 13) * (w - 16), y = h - life * h;
            g.fillStyle = hex(i % 4 ? c : "#fff7d6", clamp(a, 0, 1));
            const sz = 1 + hash(i * 3) * (1 + s.n[1] * 2);
            g.fillRect(x, y, sz, sz);
        }
    },
    // CRUSH: the wave in big steps; fewer bits and samples as Amount rises.
    function(g, w, h, t, s, c) {
        const amt = s.n[0], steps = Math.max(2, Math.round(Math.pow(2, 6 - amt * 4.5))),
              hold = 2 + Math.round(amt * amt * 14);
        g.fillStyle = hex(c, .07);
        for (let x = 6; x < w - 6; x += 6)
            for (let y = 6; y < h - 6; y += 6)
                g.fillRect(x, y, 4, 4);
        g.fillStyle = hex(c, .45 + .55 * s.on);
        g.shadowColor = c;
        g.shadowBlur = 6;
        for (let x = 6; x < w - 6; x += 6)
        {
            const xi = Math.floor((x - 6) / (6 * hold)) * 6 * hold;
            const v = wave(xi / 26, t * 3, 1) * (.4 + s.e * .7);
            const q = Math.round(v * steps) / steps, y = h / 2 - q * h * .38;
            g.fillRect(x, Math.round(y / 6) * 6, 4, 4);
        }
    },
    // MOUTH: lips shaping the current vowel.
    function(g, w, h, t, s, c) {
        const vowels = "AEIOU", v = s.p[0], i = Math.min(4, Math.round(v));
        const shapes = [ [ 1, .9 ], [ 1.1, .45 ], [ 1.15, .25 ], [ .7, .8 ], [ .55, .6 ] ];
        const a = Math.min(3, Math.floor(v)), f = v - a, sh = [
            shapes[a][0] + (shapes[a + 1][0] - shapes[a][0]) * f,
            shapes[a][1] + (shapes[a + 1][1] - shapes[a][1]) * f
        ];
        const cx = w * .4, cy = h / 2, mw = h * .55 * sh[0],
              open = h * .32 * sh[1] * (.45 + s.e * .7);
        g.fillStyle = hex(c, .9);
        g.shadowColor = c;
        g.shadowBlur = 12 * (.3 + s.on);
        g.beginPath();
        g.moveTo(cx - mw, cy);
        g.bezierCurveTo(cx - mw * .5, cy - open - 8, cx - 6, cy - open - 2, cx, cy - open * .8);
        g.bezierCurveTo(cx + 6, cy - open - 2, cx + mw * .5, cy - open - 8, cx + mw, cy);
        g.bezierCurveTo(cx + mw * .5, cy + open + 10, cx - mw * .5, cy + open + 10, cx - mw, cy);
        g.fill();
        g.shadowBlur = 0;
        g.fillStyle = "#2a0710";
        g.beginPath();
        g.ellipse(cx, cy, mw * .72, Math.max(1.5, open * .55), 0, 0, TAU);
        g.fill();
        g.fillStyle = "#fff4f0";
        g.font = "700 30px RastroSans, sans-serif";
        g.textAlign = "center";
        g.textBaseline = "middle";
        g.fillText(vowels[i], w * .82, h / 2 + 1);
    },
    // ORBIT: a moon orbiting a planet; orbit speed = rate, radius = depth, ring tilt = feedback.
    function(g, w, h, t, s, c) {
        const cx = w / 2, cy = h / 2, R = h * (.18 + s.n[1] * .2), ph = t * (s.p[0] * 2.2 + .2);
        g.strokeStyle = hex(c, .35);
        g.lineWidth = 1.2;
        g.beginPath();
        g.ellipse(cx, cy, R * 2, R * (.45 + s.n[2] * .25), -.25, 0, TAU);
        g.stroke();
        const pg = g.createRadialGradient(cx - 6, cy - 6, 2, cx, cy, R);
        pg.addColorStop(0, "#c9d4ff");
        pg.addColorStop(.6, c);
        pg.addColorStop(1, "#151a4a");
        g.fillStyle = pg;
        g.shadowColor = c;
        g.shadowBlur = 18 * (.4 + s.e);
        g.beginPath();
        g.arc(cx, cy, R * .75, 0, TAU);
        g.fill();
        for (let k = 0; k < 12; k++)
        {
            const a = ph - k * .12,
                  x = cx + Math.cos(a) * R * 2 * Math.cos(.25) -
                      Math.sin(a) * R * .5 * Math.sin(.25),
                  y = cy + Math.sin(a) * R * (.45 + s.n[2] * .25);
            g.fillStyle = hex(k ? c : "#ffffff", (k ? .5 - k * .04 : 1) * (.35 + s.on * .65));
            g.shadowBlur = k ? 0 : 10;
            g.beginPath();
            g.arc(x, y, k ? 3 - k * .2 : 4.5, 0, TAU);
            g.fill();
        }
    },
    // TRAIL: a comet and the dots of its repeats, darker as the tone closes.
    function(g, w, h, t, s, c) {
        const fb = s.p[1], n = 1 + Math.round(fb * 10), spacing = 10 + s.n[0] * 26, tone = s.n[2],
              colour = s.n[3];
        for (let k = n; k >= 0; k--)
        {
            const x = 18 + k * spacing, y = h / 2 + Math.sin(t * 2 + k * .7) * 8 * colour;
            if (x > w - 10)
                continue;
            const a = Math.pow(Math.max(.05, fb), k) * (.35 + .65 * s.on),
                  r = (k ? 4 : 7) * (1 + s.e * .4);
            const cc = k ? (tone > .5 ? c : "#c46d3a") : "#fff1d6";
            g.fillStyle = hex(cc, clamp(a, 0, 1));
            g.shadowColor = c;
            g.shadowBlur = k ? 6 : 18;
            g.beginPath();
            g.arc(x, y, r, 0, TAU);
            g.fill();
        }
        const tail = g.createLinearGradient(0, 0, 60, 0);
        tail.addColorStop(0, hex(c, 0));
        tail.addColorStop(1, hex(c, .5 * (.4 + s.on)));
        g.fillStyle = tail;
        g.beginPath();
        g.moveTo(0, h / 2 - 4);
        g.lineTo(18, h / 2 - 7);
        g.lineTo(18, h / 2 + 7);
        g.lineTo(0, h / 2 + 4);
        g.fill();
    },
    // CHOIR: three voices singing slightly out of time.
    function(g, w, h, t, s, c) {
        for (let v = 0; v < 3; v++)
        {
            const off = v / 3 * TAU * (.5 + .5 * s.n[3]), x = w * (.22 + v * .28),
                  y = h * .56 + Math.sin(t * s.p[0] * 2 + off) * s.n[1] * 6;
            g.fillStyle = hex(c, .8 * (.4 + s.on * .6));
            g.shadowColor = c;
            g.shadowBlur = 10;
            g.beginPath();
            g.arc(x, y, h * .17, 0, TAU);
            g.fill();
            g.shadowBlur = 0;
            g.fillStyle = "#06261b";
            const o = h * .05 * (.6 + s.e * 1.4 + .3 * Math.sin(t * 6 + v));
            g.beginPath();
            g.ellipse(x, y + h * .04, h * .05, Math.max(1, o), 0, 0, TAU);
            g.fill();
            g.fillRect(x - h * .07, y - h * .06, 3, 3);
            g.fillRect(x + h * .05, y - h * .06, 3, 3);
            const ny = (t * .6 + v * .33) % 1;
            g.fillStyle = hex("#eafff6", (1 - ny) * .8 * s.e);
            g.font = "700 12px RastroSans, sans-serif";
            g.fillText("♪", x + 8 + ny * 10, y - h * .25 - ny * 14);
        }
    },
    // PRISM: a beam entering a crystal and leaving as a staircase of pitched rays.
    function(g, w, h, t, s, c) {
        const cx = w * .42, cy = h / 2, st = Math.round(s.p[0]);
        g.strokeStyle = "rgba(255,255,255,.85)";
        g.lineWidth = 2;
        g.shadowColor = "#fff";
        g.shadowBlur = 8;
        g.beginPath();
        g.moveTo(6, cy + 6);
        g.lineTo(cx - 8, cy);
        g.stroke();
        const rays = 2 + Math.round(s.n[2] * 5),
              cols =
                  [ "#ff5f7a", "#ffb340", "#fff36b", "#5ef2b0", "#2fe0ff", "#8a7bff", "#d36bff" ];
        for (let k = 0; k < rays; k++)
        {
            const a = (k - rays / 2) * (.08 + Math.abs(st) / 24 * .12) + (st < 0 ? .25 : -.1),
                  len = (w - cx) * (1 - k * .06);
            g.strokeStyle =
                hex(cols[k % cols.length], (.35 + .65 * s.on) * Math.pow(.82, k) * (.5 + s.e * .6));
            g.shadowColor = cols[k % cols.length];
            g.shadowBlur = 10;
            g.beginPath();
            g.moveTo(cx + 8, cy);
            g.lineTo(cx + 8 + Math.cos(a) * len, cy + Math.sin(a) * len);
            g.stroke();
        }
        g.fillStyle = "rgba(220,250,255,.18)";
        g.strokeStyle = hex(c, .95);
        g.lineWidth = 1.6;
        g.shadowBlur = 12;
        g.shadowColor = c;
        g.beginPath();
        g.moveTo(cx, cy - h * .32);
        g.lineTo(cx + h * .28, cy + h * .26);
        g.lineTo(cx - h * .28, cy + h * .26);
        g.closePath();
        g.fill();
        g.stroke();
        g.shadowBlur = 0;
        g.fillStyle = "#eaffff";
        g.font = "700 13px RastroSans, sans-serif";
        g.textAlign = "right";
        g.fillText((st > 0 ? "+" : "") + st + " st", w - 8, 16);
    },
    // EMBER: a vacuum tube whose filament heats with Drive.
    function(g, w, h, t, s, c) {
        const cx = w / 2, cy = h * .55,
              heat = clamp(s.n[0] * .8 + s.e * .5 * s.on, 0, 1.2) * (.3 + .7 * s.on);
        const gg = g.createRadialGradient(cx, cy, 2, cx, cy, h * .55);
        gg.addColorStop(0, hex("#ffd27a", .65 * heat));
        gg.addColorStop(.5, hex(c, .35 * heat));
        gg.addColorStop(1, hex(c, 0));
        g.fillStyle = gg;
        g.fillRect(0, 0, w, h);
        g.strokeStyle = "rgba(255,230,210,.5)";
        g.lineWidth = 1.5;
        g.beginPath();
        g.moveTo(cx - h * .2, h - 6);
        g.lineTo(cx - h * .2, h * .32);
        g.arc(cx, h * .32, h * .2, Math.PI, 0);
        g.lineTo(cx + h * .2, h - 6);
        g.stroke();
        g.strokeStyle = hex("#fff1c4", clamp(.3 + heat, 0, 1));
        g.shadowColor = "#ffb340";
        g.shadowBlur = 16 * heat;
        g.lineWidth = 2;
        g.beginPath();
        for (let y = 0; y <= 40; y++)
        {
            const yy = h * .3 + y * h * .012,
                  xx = cx + Math.sin(y * .9 + t * 6 * heat) * (3 + heat * 3);
            y ? g.lineTo(xx, yy) : g.moveTo(xx, yy);
        }
        g.stroke();
        const mode = s.p[3], label = mode < .5 ? "DRIVE" : mode < 1.5 ? "VÁLVULA" : "FITA";
        g.shadowBlur = 0;
        g.fillStyle = "#ffe3c4";
        g.font = "700 10px RastroSans, sans-serif";
        g.textAlign = "left";
        g.fillText(label, 8, 15);
    },
    // SQUASH: a rubber ball flattened by the gain reduction.
    function(g, w, h, t, s, c) {
        const thr = s.n[1], ratio = s.n[2],
              squeeze = clamp(s.e * (1 - thr) * (.4 + ratio) * 1.6 * s.on, 0, .7);
        const cx = w / 2, floor = h - 10, r = h * .32, sx = 1 + squeeze * .8,
              sy = 1 - squeeze * .55;
        g.strokeStyle = "rgba(255,255,255,.18)";
        g.lineWidth = 1;
        g.setLineDash([ 3, 4 ]);
        const ty = floor - h * (.25 + thr * .55);
        g.beginPath();
        g.moveTo(8, ty);
        g.lineTo(w - 8, ty);
        g.stroke();
        g.setLineDash([]);
        g.fillStyle = hex(c, .9);
        g.shadowColor = c;
        g.shadowBlur = 14;
        g.beginPath();
        g.ellipse(cx, floor - r * sy, r * sx, r * sy, 0, 0, TAU);
        g.fill();
        g.shadowBlur = 0;
        g.fillStyle = "rgba(255,255,255,.35)";
        g.beginPath();
        g.ellipse(cx - r * .35 * sx, floor - r * sy * 1.45, r * .25, r * .12, -.4, 0, TAU);
        g.fill();
        g.fillStyle = "#16210a";
        g.font = "700 11px RastroSans, sans-serif";
        g.textAlign = "center";
        g.fillText("−" + (squeeze * 12).toFixed(1) + " dB", cx, floor - r * sy + 4);
    },
    // SIEVE: the real filter curve (LP→BP→HP morph) with its resonance peak.
    function(g, w, h, t, s, c) {
        const cut = s.n[0], q = s.p[1], type = s.p[2];
        g.strokeStyle = "rgba(255,255,255,.07)";
        g.lineWidth = 1;
        for (let x = 0; x < w; x += w / 6)
        {
            g.beginPath();
            g.moveTo(x, 6);
            g.lineTo(x, h - 6);
            g.stroke();
        }
        const resp = function(x) {
            const r = Math.pow(1000, x - cut) * 2, k = 1 / q;
            const den = Math.sqrt((1 - r * r) * (1 - r * r) + (k * r) * (k * r));
            const lp = 1 / den, bp = k * r / den, hp = r * r / den;
            return type <= 1 ? lp + (bp - lp) * type : bp + (hp - bp) * (type - 1);
        };
        g.beginPath();
        for (let x = 0; x <= w; x += 2)
        {
            const m = 20 * Math.log10(resp(x / w) + 1e-4), y = h * .45 - m * h / 60;
            x ? g.lineTo(x, clamp(y, 4, h - 2)) : g.moveTo(x, clamp(y, 4, h - 2));
        }
        g.lineTo(w, h);
        g.lineTo(0, h);
        g.closePath();
        const fill = g.createLinearGradient(0, 0, 0, h);
        fill.addColorStop(0, hex(c, .5 * (.4 + s.on * .6)));
        fill.addColorStop(1, hex(c, 0));
        g.fillStyle = fill;
        g.fill();
        g.beginPath();
        for (let x = 0; x <= w; x += 2)
        {
            const m = 20 * Math.log10(resp(x / w) + 1e-4), y = h * .45 - m * h / 60;
            x ? g.lineTo(x, clamp(y, 4, h - 2)) : g.moveTo(x, clamp(y, 4, h - 2));
        }
        glowLine(g, hex(c, .95), 2, 10);
        g.stroke();
        g.shadowBlur = 0;
        g.fillStyle = "#fff";
        g.beginPath();
        g.arc(cut * w, h * .45 - 20 * Math.log10(resp(cut) + 1e-4) * h / 60 + (s.e * 2), 3, 0, TAU);
        g.fill();
    },
    // SWING: the eight-point pan pattern (editable on the device) with a moving playhead.
    function(g, w, h, t, s, c) {
        const pan = s.pan || [ 0, .2, .5, .8, 1, .8, .5, .2 ], bw = (w - 16) / 8;
        const pos = ((t * s.p[0] * .5 + s.n[3]) % 1) * 8;
        g.strokeStyle = "rgba(255,255,255,.12)";
        g.beginPath();
        g.moveTo(8, h / 2);
        g.lineTo(w - 8, h / 2);
        g.stroke();
        for (let i = 0; i < 8; i++)
        {
            const v = (pan[i] * 2 - 1) * s.n[1], x = 8 + i * bw, active = Math.floor(pos) === i;
            g.fillStyle = hex(c, active ? .95 : .4 + .2 * s.on);
            g.shadowColor = c;
            g.shadowBlur = active ? 12 : 0;
            const y0 = h / 2, y1 = h / 2 - v * (h / 2 - 10);
            g.beginPath();
            g.roundRect(x + 3, Math.min(y0, y1), bw - 6, Math.max(2, Math.abs(y1 - y0)), 3);
            g.fill();
        }
        g.shadowBlur = 0;
        g.fillStyle = "rgba(255,255,255,.5)";
        g.font = "600 9px RastroSans, sans-serif";
        g.textAlign = "left";
        g.fillText("L", 4, 12);
        g.fillText("R", 4, h - 4);
    }
];

// Small emblem per device for the shelf (stroke icons, coloured by the device).
const EMBLEM = [
    '<circle cx="12" cy="11" r="7"/><path d="M5 11v9l2.3-2 2.3 2 2.4-2 2.4 2 2.3-2 2.3 2v-9"/><circle cx="9.5" cy="10" r="1"/><circle cx="14.5" cy="10" r="1"/>',
    '<path d="M4 6h6v5H4zM12 5h8v4h-8zM6 13h5v6H6zM13 12h7v4h-7zM14 18h4v3h-4z"/>',
    '<path d="M12 20c-3-4-3-10 0-15 3 5 3 11 0 15zM12 20c-6-1-9-6-9-10 5 1 8 5 9 10zM12 20c6-1 9-6 9-10-5 1-8 5-9 10z"/>',
    '<circle cx="7" cy="10" r="4"/><circle cx="17" cy="10" r="4"/><path d="M3 18q4.5-3 9 0t9 0"/>',
    '<circle cx="12" cy="12" r="8"/><circle cx="12" cy="12" r="2.5"/><path d="M4 4l1 1M19 3l-1 2M21 19l-2-1"/>',
    '<path d="M3 17h3v-4h3V9h3V6h3v5h3v4h3"/>',
    '<path d="M3 12c3-5 6-5 9-3 3-2 6-2 9 3-3 5-15 5-18 0z"/><path d="M6 12h12"/>',
    '<circle cx="12" cy="12" r="4"/><ellipse cx="12" cy="12" rx="10" ry="4" transform="rotate(-20 12 12)"/>',
    '<circle cx="17" cy="12" r="3"/><path d="M14 12H3M14 10l-8-3M14 14l-8 3"/>',
    '<circle cx="6" cy="12" r="3"/><circle cx="12" cy="10" r="3"/><circle cx="18" cy="12" r="3"/><path d="M5 19h14"/>',
    '<path d="M12 4l7 13H5z"/><path d="M15 11l6-3M16 13l6 1M15 15l5 4"/>',
    '<path d="M8 20V9a4 4 0 0 1 8 0v11z"/><path d="M10 18c0-4 4-4 4-8"/>',
    '<ellipse cx="12" cy="15" rx="8" ry="5"/><path d="M4 6h16"/>',
    '<path d="M3 8h18M3 12h18M3 16h18M7 4v16M12 4v16M17 4v16"/>',
    '<path d="M3 14v-4M6 17V7M9 12v0M12 19V5M15 15V9M18 18V6M21 13v-2"/>'
];

window.RastroFxArt = {
    DEV : DEV,
    PAINT : PAINT,
    EMBLEM : EMBLEM,
    hex : hex
};
})();
