/* RASTRO FX — web interface. Every control writes a real host parameter through the bridge;
   everything that moves is driven by real telemetry (input/output peaks, LFO, follower, module
   mix). */
(function() {
"use strict";
const R = window.RastroFx, ART = window.RastroFxArt, VIZ = window.RastroViz;
const DEV = ART.DEV, hex = ART.hex;
const NS = "http://www.w3.org/2000/svg";
const $ = function(s, r) { return (r || document).querySelector(s); };
const $$ = function(s,
                    r) { return Array.prototype.slice.call((r || document).querySelectorAll(s)); };
const clamp = function(v, a, b) { return v < a ? a : v > b ? b : v; };
const W = 1440, H = 900;

const P = {}, watchers = {}, busy = {};
let MODS = [], TARGETS = [], FACTORY = [];
const T = {
    in : 0,
    out : 0,
    lfo : 0,
    env : 0,
    act : [],
    bank : 0,
    preset : "",
    program : 0
};
let S = 1; // UI scale

/* ================= parameters ================= */
function v(id) { return P[id] ? P[id].v : 0; }
function watch(id, fn)
{
    (watchers[id] = watchers[id] || []).push(fn);
    if (P[id])
        fn(P[id].v);
}
function notify(id)
{
    (watchers[id] || []).forEach(function(fn) { fn(P[id].v); });
}
function setParam(id, value, phase)
{
    const p = P[id];
    if (!p)
        return;
    if (value !== null && value !== undefined)
    {
        value = clamp(value, p.min, p.max);
        if (p.step)
            value = Math.round(value);
        p.v = value;
        notify(id);
    }
    R.call("setParam", id, value === undefined ? null : value, phase || 0);
}
function gesture(id, value)
{
    setParam(id, null, 1);
    setParam(id, value);
    setParam(id, null, 2);
}
R.on("params", function(changes) {
    for (const id in changes)
        if (P[id] && !busy[id])
        {
            P[id].v = changes[id];
            notify(id);
        }
    if (Object.keys(changes).some(function(k) { return /^fxM\d+(Position|InRack)$/.test(k); }))
        layoutChain();
});
R.on("tick", function(t) { Object.assign(T, t); });

/* ================= formatting ================= */
function physical(m, k, n)
{
    const c = MODS[m].controls[k];
    n = clamp(n, 0, 1);
    return c.log ? c.low * Math.pow(c.high / c.low, n) : c.low + n * (c.high - c.low);
}
function fmtControl(m, k, n)
{
    const c = MODS[m].controls[k], x = physical(m, k, n), u = c.unit;
    if (u === "Hz")
        return x >= 1000 ? (x / 1000).toFixed(x >= 10000 ? 1 : 2) + " kHz"
               : x >= 20 ? Math.round(x) + " Hz"
                         : x.toFixed(2) + " Hz";
    if (u === "ms")
        return x >= 1000 ? (x / 1000).toFixed(2) + " s"
                         : (x < 10 ? x.toFixed(1) : Math.round(x)) + " ms";
    if (u === "x")
        return "×" + x.toFixed(x < 10 ? 1 : 0);
    if (u === "dB")
        return x.toFixed(1) + " dB";
    if (u === "st")
        return (x > 0 ? "+" : "") + Math.round(x) + " st";
    if (u === ":1")
        return x.toFixed(1) + ":1";
    if (u === "Q")
        return "Q " + x.toFixed(1);
    if (m === 6 && k === 0)
    {
        const i = Math.min(4, Math.round(x));
        return "AEIOU"[i];
    }
    if (c.low < 0)
        return (x > 0 ? "+" : "") + Math.round(x * 100) + "%";
    return Math.round(x * 100) + "%";
}
const pct = function(x) { return Math.round(x * 100) + "%"; };
const db = function(x) { return (x > 0 ? "+" : "") + x.toFixed(1) + " dB"; };
const bip = function(x) { return (x > 0 ? "+" : "") + Math.round(x * 100) + "%"; };
const msf = function(
    x) { return x >= 1000 ? (x / 1000).toFixed(2) + " s" : Math.round(x) + " ms"; };
const hzf = function(x) { return x < 1 ? x.toFixed(2) + " Hz" : x.toFixed(1) + " Hz"; };

/* ================= knobs ================= */
function knobSvg(size)
{
    const c = size / 2, r = size / 2 - 4, rf = r - 6;
    return '<svg width="' + size + '" height="' + size + '" viewBox="0 0 ' + size + ' ' + size +
           '">' +
           '<circle class="k-track" cx="' + c + '" cy="' + c + '" r="' + r +
           '" stroke-dasharray="' + (r * Math.PI * 1.5) + ' 999" transform="rotate(135 ' + c + ' ' +
           c + ')"/>' +
           '<circle class="k-arc" cx="' + c + '" cy="' + c + '" r="' + r +
           '" transform="rotate(135 ' + c + ' ' + c + ')" stroke-dasharray="0 999"/>' +
           '<circle class="k-shadow" cx="' + c + '" cy="' + (c + 2) + '" r="' + rf + '"/>' +
           '<circle class="k-face" cx="' + c + '" cy="' + c + '" r="' + rf + '"/>' +
           '<circle class="k-gloss" cx="' + c + '" cy="' + c + '" r="' + (rf - 1.5) + '"/>' +
           '<g class="k-ptr"><line x1="' + c + '" y1="' + (c - rf * .3) + '" x2="' + c + '" y2="' +
           (c - rf + 3.5) + '"/></g></svg>';
}
// opts: size, label, color, skin, toT(plain)->0..1, fromT(t)->plain, format(plain), bipolar
function knob(parent, id, opts)
{
    opts = opts || {};
    const size = opts.size || 44, r = size / 2 - 4, circ = r * Math.PI * 1.5, c = size / 2;
    const el = document.createElement("div");
    el.className =
        "knob" + (opts.skin ? " sk-" + opts.skin : "") + (opts.cls ? " " + opts.cls : "");
    if (opts.color)
        el.style.setProperty("--kc", opts.color);
    el.innerHTML = (opts.label ? '<div class="k-label">' + opts.label + '</div>' : '') +
                   '<span class="k-dial">' + knobSvg(size) + '</span><div class="k-value"></div>';
    el.title = opts.title || opts.label || "";
    parent.appendChild(el);
    const arc = $(".k-arc", el), ptr = $(".k-ptr", el), val = $(".k-value", el);
    const p = function() { return P[id]; };
    const toT = opts.toT || function(x) {
        const q = p();
        return q ? (x - q.min) / (q.max - q.min) : 0;
    };
    const fromT = opts.fromT || function(t) {
        const q = p();
        return q.min + clamp(t, 0, 1) * (q.max - q.min);
    };
    function draw(x)
    {
        const t = clamp(toT(x), 0, 1);
        if (opts.bipolar)
        {
            const mid = circ / 2, len = Math.abs(t - .5) * circ;
            arc.setAttribute("stroke-dasharray",
                             "0 " + (t < .5 ? mid - len : mid) + " " + len + " 999");
        }
        else
            arc.setAttribute("stroke-dasharray", (t * circ) + " 999");
        ptr.setAttribute("transform", "rotate(" + (-135 + t * 270) + " " + c + " " + c + ")");
        val.textContent = (opts.format || pct)(x);
    }
    watch(id, draw);
    let startY = 0, startT = 0, moved = false;
    el.addEventListener("pointerdown", function(e) {
        if (!p())
            return;
        e.preventDefault();
        e.stopPropagation();
        el.setPointerCapture(e.pointerId);
        startY = e.clientY;
        startT = toT(v(id));
        busy[id] = true;
        moved = false;
        el.classList.add("dragging");
        setParam(id, null, 1);
    });
    el.addEventListener("pointermove", function(e) {
        if (!busy[id] || !el.hasPointerCapture(e.pointerId))
            return;
        moved = true;
        setParam(id, fromT(startT + (startY - e.clientY) / S * (e.shiftKey ? .0012 : .0055)));
    });
    function end()
    {
        if (!busy[id])
            return;
        busy[id] = false;
        el.classList.remove("dragging");
        setParam(id, null, 2);
    }
    el.addEventListener("pointerup", end);
    el.addEventListener("pointercancel", end);
    el.addEventListener("dblclick", function() {
        if (p())
            gesture(id, p().def);
    });
    el.addEventListener("wheel", function(e) {
        if (!p())
            return;
        e.preventDefault();
        gesture(id, fromT(toT(v(id)) - e.deltaY * .0008));
    }, {passive : false});
    return el;
}
function logMap(lo, hi)
{
    return {
        toT : function(x) { return Math.log(Math.max(x, lo) / lo) / Math.log(hi / lo); },
        fromT : function(t) { return lo * Math.pow(hi / lo, clamp(t, 0, 1)); }
    };
}

/* ================= devices ================= */
const devices = [];
function chainOrder()
{
    const ids = MODS.map(function(_, i) { return i; });
    ids.sort(function(a, b) {
        const d = v("fxM" + a + "Position") - v("fxM" + b + "Position");
        return d || a - b;
    });
    return ids;
}
function inChain()
{
    return chainOrder().filter(function(m) { return v("fxM" + m + "InRack") > .5; });
}

function buildDevice(m)
{
    const d = DEV[m], mod = MODS[m];
    const el = document.createElement("div");
    el.className = "device mat-" + d.mat;
    el.dataset.m = m;
    el.style.setProperty("--c", d.c);
    el.innerHTML =
        '<div class="dv-top"><i class="screw"></i><span class="dv-grip" title="Arraste para mudar a ordem"><b></b><b></b><b></b></span><button class="dv-x" title="Tirar da cadeia">×</button><i class="screw"></i></div>' +
        '<div class="dv-plate"><div class="dv-name ft-' + d.font + '">' + mod.name +
        '</div><div class="dv-sub">' + d.sub + '</div></div>' +
        '<div class="dv-screen"><canvas></canvas><i class="dv-glass"></i></div>' +
        '<div class="dv-knobs"></div>' +
        '<div class="dv-bottom"><div class="dv-mix"></div><div class="dv-foot"><span class="led"></span><button class="foot" title="Liga/desliga (bypass do aparelho)"><i></i></button><span class="dv-state">ON</span></div></div>';
    const knobs = $(".dv-knobs", el);
    mod.controls.forEach(function(c, k) {
        const id = "fxM" + m + "P" + k;
        if (c.unit === "mode")
        {
            const box = document.createElement("div");
            box.className = "dv-mode";
            box.innerHTML =
                '<div class="k-label">' + d.labels[k] + '</div>' +
                d.modes
                    .map(function(name,
                                  i) { return '<button data-i="' + i + '">' + name + '</button>'; })
                    .join("");
            knobs.appendChild(box);
            $$("button", box).forEach(function(b) {
                b.addEventListener("click", function() { gesture(id, Number(b.dataset.i) / 2); });
            });
            watch(id, function(x) {
                $$("button", box).forEach(function(b) {
                    const i = Number(b.dataset.i), w = clamp(1 - Math.abs(x * 2 - i), 0, 1);
                    b.classList.toggle("on", w > .5);
                    b.style.setProperty("--w", w.toFixed(2));
                });
            });
            return;
        }
        knob(knobs, id, {
            size : 42,
            label : d.labels[k],
            color : d.c,
            skin : d.knob,
            bipolar : c.low < 0 && c.high > 0,
            format : function(n) { return fmtControl(m, k, n); }
        });
    });
    if (mod.controls.length === 1)
        knobs.classList.add("one");
    knob($(".dv-mix", el), "fxM" + m + "Mix",
         {size : 58, label : "MIX", color : d.c, skin : d.knob, cls : "mix"});
    const foot = $(".foot", el), state = $(".dv-state", el);
    foot.addEventListener(
        "click", function() { gesture("fxM" + m + "On", v("fxM" + m + "On") > .5 ? 0 : 1); });
    watch("fxM" + m + "On", function(x) {
        el.classList.toggle("off", x < .5);
        state.textContent = x > .5 ? "ON" : "OFF";
    });
    $(".dv-x", el).addEventListener("click", function() {
        gesture("fxM" + m + "InRack", 0);
        layoutChain();
    });
    // SWING: the screen is the pattern editor.
    if (m === 14)
    {
        const scr = $(".dv-screen", el);
        let painting = false;
        const paint = function(e) {
            const r = scr.getBoundingClientRect(),
                  i = clamp(Math.floor((e.clientX - r.left) / r.width * 8), 0, 7);
            const y = clamp(1 - (e.clientY - r.top) / r.height, 0, 1);
            setParam("fxPan" + i, y);
        };
        scr.classList.add("editable");
        scr.title = "Desenhe o movimento do pan";
        scr.addEventListener("pointerdown", function(e) {
            e.stopPropagation();
            scr.setPointerCapture(e.pointerId);
            painting = true;
            for (let i = 0; i < 8; i++)
                setParam("fxPan" + i, null, 1);
            paint(e);
        });
        scr.addEventListener("pointermove", function(e) {
            if (painting)
                paint(e);
        });
        const stop = function() {
            if (!painting)
                return;
            painting = false;
            for (let i = 0; i < 8; i++)
                setParam("fxPan" + i, null, 2);
        };
        scr.addEventListener("pointerup", stop);
        scr.addEventListener("pointercancel", stop);
    }
    devices[m] = {el : el, cv : $("canvas", el), energy : 0};
    attachDeviceDrag(el, m);
    return el;
}

function layoutChain()
{
    const host = $("#devices"), chain = inChain();
    devices.forEach(function(d) {
        if (d.el.parentNode)
            d.el.remove();
    });
    chain.forEach(function(m) { host.appendChild(devices[m].el); });
    $("#emptyChain").hidden = chain.length > 0;
    $("#chainScroll").classList.toggle("empty", chain.length === 0);
    $$(".tile").forEach(function(
        t) { t.classList.toggle("in", chain.indexOf(Number(t.dataset.m)) >= 0); });
    requestAnimationFrame(drawCable);
}
// Commit a new chain order: inserted modules first (in the given order), then the rest.
function commitOrder(chain)
{
    const rest = chainOrder().filter(function(m) { return chain.indexOf(m) < 0; });
    chain.concat(rest).forEach(function(m, pos) {
        const id = "fxM" + m + "Position";
        if (v(id) !== pos)
            gesture(id, pos);
    });
    layoutChain();
}
function insertModule(m, at)
{
    const chain = inChain().filter(function(x) { return x !== m; });
    chain.splice(at === undefined ? chain.length : clamp(at, 0, chain.length), 0, m);
    gesture("fxM" + m + "InRack", 1);
    if (v("fxM" + m + "On") < .5)
        gesture("fxM" + m + "On", 1);
    commitOrder(chain);
    toast(MODS[m].name + " entrou na cadeia");
}

/* ---- drag & drop (pointer based: reliable in WKWebView) ---- */
let drag = null;
function dropIndex(clientX)
{
    const els = $$("#devices .device").filter(function(e) { return !drag || e !== drag.el; });
    for (let i = 0; i < els.length; i++)
    {
        const r = els[i].getBoundingClientRect();
        if (clientX < r.left + r.width / 2)
            return i;
    }
    return els.length;
}
function showMarker(i)
{
    const marker = $("#dropMarker") || (function() {
                       const d = document.createElement("div");
                       d.id = "dropMarker";
                       $("#chain").appendChild(d);
                       return d;
                   })();
    const els = $$("#devices .device").filter(function(e) { return !drag || e !== drag.el; });
    const chainR = $("#chain").getBoundingClientRect();
    let x;
    if (!els.length)
        x = ($("#devices").getBoundingClientRect().left - chainR.left) / S + 10;
    else if (i < els.length)
        x = (els[i].getBoundingClientRect().left - chainR.left) / S - 10;
    else
        x = (els[els.length - 1].getBoundingClientRect().right - chainR.left) / S + 8;
    marker.style.left = x + "px";
    marker.hidden = false;
}
function hideMarker()
{
    const m = $("#dropMarker");
    if (m)
        m.hidden = true;
}
function overChain(e)
{
    const r = $("#chainPanel").getBoundingClientRect();
    return e.clientX > r.left && e.clientX < r.right && e.clientY > r.top && e.clientY < r.bottom;
}
function attachDeviceDrag(el, m)
{
    const grip = $(".dv-top", el);
    grip.addEventListener("pointerdown", function(e) {
        if (e.target.closest(".dv-x"))
            return;
        e.preventDefault();
        grip.setPointerCapture(e.pointerId);
        drag = {el : el, m : m, x0 : e.clientX, y0 : e.clientY, moved : false};
    });
    grip.addEventListener("pointermove", function(e) {
        if (!drag || drag.el !== el)
            return;
        const dx = (e.clientX - drag.x0) / S, dy = (e.clientY - drag.y0) / S;
        if (!drag.moved && Math.hypot(dx, dy) < 4)
            return;
        drag.moved = true;
        el.classList.add("lifted");
        el.style.transform =
            "translate(" + dx + "px," + dy * .25 + "px) rotate(" + clamp(dx * .02, -3, 3) + "deg)";
        showMarker(dropIndex(e.clientX));
    });
    const end = function(e) {
        if (!drag || drag.el !== el)
            return;
        const d = drag;
        drag = null;
        el.classList.remove("lifted");
        el.style.transform = "";
        hideMarker();
        if (!d.moved)
            return;
        const chain = inChain().filter(function(x) { return x !== m; });
        const els = $$("#devices .device").filter(function(x) { return x !== el; });
        let i = els.length;
        for (let k = 0; k < els.length; k++)
        {
            const r = els[k].getBoundingClientRect();
            if (e.clientX < r.left + r.width / 2)
            {
                i = k;
                break;
            }
        }
        chain.splice(i, 0, m);
        commitOrder(chain);
    };
    grip.addEventListener("pointerup", end);
    grip.addEventListener("pointercancel", end);
}

/* ================= shelf ================= */
function buildShelf()
{
    const shelf = $("#shelf");
    MODS.forEach(function(mod, m) {
        const d = DEV[m], t = document.createElement("button");
        t.className = "tile";
        t.dataset.m = m;
        t.style.setProperty("--c", d.c);
        t.innerHTML = '<svg class="tl-ic" viewBox="0 0 24 24">' + ART.EMBLEM[m] +
                      '</svg><span class="tl-name">' + mod.name + '</span><span class="tl-sub">' +
                      d.sub + '</span><i class="tl-dot"></i>';
        t.title = d.tag;
        shelf.appendChild(t);
        let st = null, ghost = null;
        t.addEventListener("pointerdown", function(e) {
            e.preventDefault();
            t.setPointerCapture(e.pointerId);
            st = {x : e.clientX, y : e.clientY, moved : false};
        });
        t.addEventListener("pointermove", function(e) {
            if (!st)
                return;
            if (!st.moved && Math.hypot(e.clientX - st.x, e.clientY - st.y) < 5 * S)
                return;
            if (!st.moved)
            {
                st.moved = true;
                ghost = t.cloneNode(true);
                ghost.classList.add("tile-ghost");
                $("#app").appendChild(ghost);
            }
            const a = $("#app").getBoundingClientRect();
            ghost.style.left = ((e.clientX - a.left) / S - 60) + "px";
            ghost.style.top = ((e.clientY - a.top) / S - 28) + "px";
            if (overChain(e))
                showMarker(dropIndex(e.clientX));
            else
                hideMarker();
        });
        const end = function(e) {
            if (!st)
                return;
            const moved = st.moved;
            st = null;
            hideMarker();
            if (ghost)
            {
                ghost.remove();
                ghost = null;
            }
            if (!moved)
            {
                if (v("fxM" + m + "InRack") > .5)
                {
                    gesture("fxM" + m + "InRack", 0);
                    layoutChain();
                    toast(mod.name + " saiu da cadeia");
                }
                else
                    insertModule(m);
                return;
            }
            if (overChain(e))
                insertModule(m, dropIndex(e.clientX));
        };
        t.addEventListener("pointerup", end);
        t.addEventListener("pointercancel", end);
    });
}

/* ================= cable ================= */
function drawCable()
{
    const svg = $("#cable"), chain = $("#chain"), cr = chain.getBoundingClientRect();
    const wdt = chain.scrollWidth, y = 214;
    svg.setAttribute("width", wdt);
    svg.setAttribute("height", 484);
    svg.setAttribute("viewBox", "0 0 " + wdt + " 484");
    const par = v("fxRouting") > .5, els = $$("#devices .device");
    let html =
        '<defs><linearGradient id="cbg" gradientUnits="userSpaceOnUse" x1="0" y1="0" x2="' + wdt +
        '" y2="0"><stop offset="0" stop-color="#2fe0ff"/><stop offset=".5" stop-color="#b06bff"/><stop offset="1" stop-color="#ff4fd8"/></linearGradient></defs>';
    const xIn = 40, xOut = wdt - 40;
    if (!par || els.length < 2)
    {
        html += '<path class="cb-base" d="M' + xIn + ' ' + y + ' H' + xOut +
                '"/><path class="cb-flow" d="M' + xIn + ' ' + y + ' H' + xOut + '"/>';
    }
    else
    {
        const top = 10, bot = 476;
        html += '<path class="cb-base" d="M' + xIn + ' ' + y + ' V' + top + ' H' + xOut + ' V' + y +
                '"/><path class="cb-base" d="M' + xIn + ' ' + y + ' V' + bot + ' H' + xOut + ' V' +
                y + '"/>';
        els.forEach(function(e) {
            const r = e.getBoundingClientRect(),
                  x = (r.left + r.width / 2 - cr.left) / S + chain.scrollLeft * 0;
            html += '<path class="cb-tap" d="M' + (x - 30) + ' ' + top + ' V' + (top + 14) + ' M' +
                    (x + 30) + ' ' + bot + ' V' + (bot - 14) + '"/>';
        });
        html += '<path class="cb-flow" d="M' + xIn + ' ' + y + ' V' + top + ' H' + xOut + ' V' + y +
                '"/><path class="cb-flow" d="M' + xIn + ' ' + y + ' V' + bot + ' H' + xOut + ' V' +
                y + '"/>';
    }
    svg.innerHTML = html;
    $("#chainHint").textContent =
        par ? "PARALELO · cada aparelho recebe o sinal limpo e as saídas se somam"
            : "SÉRIE · arraste os aparelhos para mudar a ordem · arraste da prateleira para adicionar";
    $("#chainHint").classList.toggle("par", par);
}

/* ================= stage: the light producer (shared with RASTRO PLAY) =================
   Drawn from the real output scope; colours come from the modules really in use, input
   transients play the desk and fire the rings (with violet echoes when GHOST is on). */
let viz = null, slowIn = 0, lastHit = 0, padCount = 0;
const FIG = {
    hair : "puff",
    hat : "none",
    phones : "studio",
    glasses : "none",
    desk : "synth",
    style : "neon",
    colour : "fx"
};
// Rings take the colour of the module that is doing the most right now.
function ringColour(fx)
{
    let best = -1, bv = .05;
    fx.forEach(function(a, m) {
        if (a > bv)
        {
            bv = a;
            best = m;
        }
    });
    const c = best < 0 ? "#8cc8ff" : DEV[best].c, n = parseInt(c.slice(1), 16);
    return [ n >> 16 & 255, n >> 8 & 255, n & 255 ];
}
function drawStage(dt, now)
{
    if (!viz || !VIZ)
        return;
    const bypassed = v("fxBypass") > .5, input = T.in || 0;
    slowIn += (input - slowIn) * .08;
    let hit = false;
    if (!bypassed && input > .04 && input > slowIn * 1.45 && now - lastHit > 140)
    {
        hit = true;
        lastHit = now;
        padCount++;
    }
    const fx = MODS.map(function(_, m) {
        const live = v("fxM" + m + "InRack") > .5 && v("fxM" + m + "On") > .5 && !bypassed;
        return live ? clamp(v("fxM" + m + "Mix") * (.35 + input * 1.3), 0, 1) : 0;
    });
    const lfoOn = v("fxLfoTarget") > 0 ? Math.abs(v("fxLfoDepth")) : 0;
    viz.draw({
        scope : bypassed ? (T.scope || []).map(function(x) { return x * .5; }) : T.scope,
        meter : T.out || 0,
        env : clamp(slowIn * 2, 0, 1),
        fx : fx,
        react : 1,
        hit : hit,
        vel : clamp(input * 1.6, .3, 1),
        noteName : "",
        chord : "",
        noteCol : ringColour(fx),
        fig : FIG,
        bottomPad : 6,
        beat : now / 1000 * 2,
        pitch : 0,
        pad : padCount,
        echo : .25,
        cut : 1,
        lfo : T.lfo || 0,
        lfoDepth : lfoOn,
        wobRate : v("fxM3P1") * 4,
        names : MODS.map(function(m) { return m.name; })
    },
             dt);
    const chain = inChain(), tag = $("#stageText"), dotEl = $("#stageDot");
    if (bypassed)
    {
        tag.textContent = "bypass · sinal original";
        dotEl.style.background = "#7d6a99";
    }
    else if (!chain.length)
    {
        tag.textContent = "cadeia vazia · sinal limpo";
        dotEl.style.background = "#7d6a99";
    }
    else
    {
        tag.textContent = chain.map(function(m) { return MODS[m].name; }).join(" → ");
        dotEl.style.background = DEV[chain[0]].c;
    }
}

/* ================= meters ================= */
function meter(el, lin)
{
    const dbv = 20 * Math.log10(Math.max(lin, 1e-6)), t = clamp((dbv + 60) / 66, 0, 1);
    $("i", el).style.transform = "scaleY(" + t.toFixed(3) + ")";
    el.classList.toggle("clip", dbv > 0);
}

/* ================= modulation ================= */
function targetSelect(sel, id)
{
    const groups = {};
    let html = '<option value="0">— sem destino —</option>';
    html +=
        '<optgroup label="GLOBAL"><option value="1">Ganho de entrada</option><option value="2">Ganho de saída</option><option value="3">Dry/Wet global</option></optgroup>';
    TARGETS.forEach(function(name, i) {
        if (i < 3)
            return;
        const parts = name.split(" / "),
              m = MODS.findIndex(function(x) { return x.name === parts[0]; });
        if (m < 0)
            return;
        let label = parts[1];
        const k = MODS[m].controls.findIndex(function(c) { return c.name === parts[1]; });
        if (k >= 0)
            label = DEV[m].labels[k] || label;
        else if (/^Point/.test(parts[1]))
            label = "Ponto " + parts[1].split(" ")[1];
        else if (parts[1] === "Mix")
            label = "MIX";
        (groups[m] = groups[m] || [])
            .push('<option value="' + (i + 1) + '" data-m="' + m + '">' + MODS[m].name + " · " +
                  label + '</option>');
    });
    Object.keys(groups).forEach(function(m) {
        html += '<optgroup label="' + MODS[m].name + '">' + groups[m].join("") + '</optgroup>';
    });
    sel.innerHTML = html;
    sel.addEventListener("change", function() { gesture(id, Number(sel.value)); });
    watch(id, function(x) {
        sel.value = String(Math.round(x));
        const o = sel.selectedOptions[0], m = o && o.dataset.m;
        sel.style.setProperty("--tc", m !== undefined ? DEV[m].c
                                      : x > 0         ? "#ffb340"
                                                      : "rgba(176,107,255,.35)");
        sel.classList.toggle("set", x > 0);
    });
}
const xyTrail = [];
function setupXY()
{
    const pad = $("#xyPad"), star = $("#xyStar");
    let down = false;
    const move = function(e) {
        const r = pad.getBoundingClientRect();
        setParam("fxX", clamp((e.clientX - r.left) / r.width, 0, 1));
        setParam("fxY", clamp(1 - (e.clientY - r.top) / r.height, 0, 1));
    };
    pad.addEventListener("pointerdown", function(e) {
        e.preventDefault();
        pad.setPointerCapture(e.pointerId);
        down = true;
        busy.fxX = busy.fxY = true;
        setParam("fxX", null, 1);
        setParam("fxY", null, 1);
        move(e);
        pad.classList.add("active");
    });
    pad.addEventListener("pointermove", function(e) {
        if (down)
            move(e);
    });
    const end = function() {
        if (!down)
            return;
        down = false;
        busy.fxX = busy.fxY = false;
        setParam("fxX", null, 2);
        setParam("fxY", null, 2);
        pad.classList.remove("active");
    };
    pad.addEventListener("pointerup", end);
    pad.addEventListener("pointercancel", end);
    pad.addEventListener("dblclick", function() {
        gesture("fxX", .5);
        gesture("fxY", .5);
    });
    const place = function() {
        star.style.left = (v("fxX") * 100) + "%";
        star.style.top = ((1 - v("fxY")) * 100) + "%";
    };
    watch("fxX", place);
    watch("fxY", place);
    targetSelect($("#routeX select"), "fxXTarget");
    targetSelect($("#routeY select"), "fxYTarget");
    knob($("#routeX .mini-knob"), "fxXDepth",
         {size : 34, color : "#ff4fd8", bipolar : true, format : bip, title : "Quantidade X"});
    knob($("#routeY .mini-knob"), "fxYDepth",
         {size : 34, color : "#2fe0ff", bipolar : true, format : bip, title : "Quantidade Y"});
}
function drawXY()
{
    const cv = $("#xyCanvas"), w = cv.clientWidth, h = cv.clientHeight;
    if (!w)
        return;
    const dpr = Math.min(2, window.devicePixelRatio || 1) * S;
    if (cv.width !== Math.round(w * dpr))
    {
        cv.width = Math.round(w * dpr);
        cv.height = Math.round(h * dpr);
    }
    const g = cv.getContext("2d");
    g.setTransform(dpr, 0, 0, dpr, 0, 0);
    g.clearRect(0, 0, w, h);
    g.strokeStyle = "rgba(176,107,255,.12)";
    g.lineWidth = 1;
    for (let i = 1; i < 6; i++)
    {
        g.beginPath();
        g.moveTo(i * w / 6, 0);
        g.lineTo(i * w / 6, h);
        g.moveTo(0, i * h / 6);
        g.lineTo(w, i * h / 6);
        g.stroke();
    }
    xyTrail.push([ v("fxX") * w, (1 - v("fxY")) * h ]);
    if (xyTrail.length > 50)
        xyTrail.shift();
    for (let i = 1; i < xyTrail.length; i++)
    {
        g.strokeStyle = "rgba(255,179,64," + (i / xyTrail.length * .7).toFixed(3) + ")";
        g.lineWidth = 1 + i / xyTrail.length * 3;
        g.beginPath();
        g.moveTo(xyTrail[i - 1][0], xyTrail[i - 1][1]);
        g.lineTo(xyTrail[i][0], xyTrail[i][1]);
        g.stroke();
    }
}
const lfoHist = new Array(160).fill(0), envHist = new Array(160).fill(0);
function scope(cv, hist, col, bipolar, fill)
{
    const w = cv.clientWidth, h = cv.clientHeight;
    if (!w)
        return;
    const dpr = Math.min(2, window.devicePixelRatio || 1) * S;
    if (cv.width !== Math.round(w * dpr))
    {
        cv.width = Math.round(w * dpr);
        cv.height = Math.round(h * dpr);
    }
    const g = cv.getContext("2d");
    g.setTransform(dpr, 0, 0, dpr, 0, 0);
    g.clearRect(0, 0, w, h);
    g.strokeStyle = "rgba(255,255,255,.06)";
    g.beginPath();
    g.moveTo(0, bipolar ? h / 2 : h - 4);
    g.lineTo(w, bipolar ? h / 2 : h - 4);
    g.stroke();
    const yOf = function(x) { return bipolar ? h / 2 - x * (h / 2 - 6) : h - 4 - x * (h - 10); };
    g.beginPath();
    hist.forEach(function(x, i) {
        const px = i / (hist.length - 1) * w;
        i ? g.lineTo(px, yOf(x)) : g.moveTo(px, yOf(x));
    });
    if (fill)
    {
        g.lineTo(w, h);
        g.lineTo(0, h);
        g.closePath();
        const f = g.createLinearGradient(0, 0, 0, h);
        f.addColorStop(0, hex(col, .45));
        f.addColorStop(1, hex(col, 0));
        g.fillStyle = f;
        g.fill();
        g.beginPath();
        hist.forEach(function(x, i) {
            const px = i / (hist.length - 1) * w;
            i ? g.lineTo(px, yOf(x)) : g.moveTo(px, yOf(x));
        });
    }
    g.strokeStyle = col;
    g.lineWidth = 2;
    g.shadowColor = col;
    g.shadowBlur = 10;
    g.stroke();
    g.shadowBlur = 0;
    const last = hist[hist.length - 1];
    g.fillStyle = "#fff";
    g.beginPath();
    g.arc(w - 3, yOf(last), 3.5, 0, Math.PI * 2);
    g.fill();
}
function setupMod()
{
    targetSelect($("#routeLfo select"), "fxLfoTarget");
    targetSelect($("#routeEnv select"), "fxEnvTarget");
    const lr = logMap(.05, 20);
    knob($("#lfoKnobs"), "fxLfoRate", {
        size : 38,
        label : "VELOC.",
        color : "#2fe0ff",
        skin : "neon",
        toT : lr.toT,
        fromT : lr.fromT,
        format : hzf
    });
    knob($("#lfoKnobs"), "fxLfoDepth", {
        size : 38,
        label : "QUANTIDADE",
        color : "#2fe0ff",
        skin : "neon",
        bipolar : true,
        format : bip
    });
    $$("#lfoShapes button").forEach(function(b) {
        b.addEventListener("click", function() { gesture("fxLfoShape", Number(b.dataset.s)); });
    });
    watch("fxLfoShape", function(x) {
        $$("#lfoShapes button").forEach(function(b) {
            b.classList.toggle("on", Number(b.dataset.s) === Math.round(x));
        });
    });
    const ar = logMap(1, 200), rr = logMap(10, 2000);
    knob($("#envKnobs"), "fxEnvSensitivity",
         {size : 38, label : "SENSIB.", color : "#ffb340", skin : "neon", format : db});
    knob($("#envKnobs"), "fxEnvAttack", {
        size : 38,
        label : "ATAQUE",
        color : "#ffb340",
        skin : "neon",
        toT : ar.toT,
        fromT : ar.fromT,
        format : msf
    });
    knob($("#envKnobs"), "fxEnvRelease", {
        size : 38,
        label : "SOLTURA",
        color : "#ffb340",
        skin : "neon",
        toT : rr.toT,
        fromT : rr.fromT,
        format : msf
    });
    knob($("#envKnobs"), "fxEnvDepth", {
        size : 38,
        label : "QUANT.",
        color : "#ffb340",
        skin : "neon",
        bipolar : true,
        format : bip
    });
}

/* ================= header ================= */
function setupHeader()
{
    $$("#routing button").forEach(function(b) {
        b.addEventListener("click", function() { gesture("fxRouting", Number(b.dataset.r)); });
    });
    watch("fxRouting", function(x) {
        $$("#routing button").forEach(function(b) {
            b.classList.toggle("on", Number(b.dataset.r) === Math.round(x));
            b.setAttribute("aria-checked", Number(b.dataset.r) === Math.round(x));
        });
        $("#routing").classList.toggle("par", x > .5);
        requestAnimationFrame(drawCable);
    });
    const menu = $("#presetMenu");
    menu.innerHTML = FACTORY
                         .map(function(n, i) {
                             return '<button role="option" data-i="' + i + '"><span>' +
                                    String(i).padStart(2, "0") + '</span>' + n + '</button>';
                         })
                         .join("");
    $("#presetOpen").addEventListener("click", function(e) {
        e.stopPropagation();
        menu.hidden = !menu.hidden;
    });
    document.addEventListener("pointerdown", function(e) {
        if (!e.target.closest("#presetBar"))
            menu.hidden = true;
    });
    $$("button", menu).forEach(function(b) {
        b.addEventListener("click", function() {
            loadFactory(Number(b.dataset.i));
            menu.hidden = true;
        });
    });
    $("#presetPrev").addEventListener("click", function() {
        loadFactory((T.program + FACTORY.length - 1) % FACTORY.length);
    });
    $("#presetNext")
        .addEventListener("click", function() { loadFactory((T.program + 1) % FACTORY.length); });
    $$("#abBox .ab").forEach(function(b) {
        b.addEventListener("click", function() {
            if (Number(b.dataset.slot) !== T.bank)
                R.call("compare").then(refreshAll);
        });
    });
    $("#abCopy").addEventListener("click", function() {
        R.call("copyCompare")
            .then(function() { toast("Ajuste copiado para " + (T.bank ? "A" : "B")); });
    });
    $("#savePreset").addEventListener("click", function() {
        R.call("savePreset").then(function(r) {
            if (r && r.ok)
                toast("Preset salvo: " + r.name);
        });
    });
    $("#openPreset").addEventListener("click", function() {
        R.call("openPreset").then(function(r) {
            if (r && r.ok)
            {
                toast("Preset aberto: " + r.name);
                refreshAll();
            }
            else if (r && r.message)
                toast(r.message);
        });
    });
    const bp = $("#bypass");
    bp.addEventListener("click", function() { gesture("fxBypass", v("fxBypass") > .5 ? 0 : 1); });
    watch("fxBypass", function(x) {
        bp.classList.toggle("on", x > .5);
        bp.setAttribute("aria-pressed", x > .5);
        $("#app").classList.toggle("bypassed", x > .5);
    });
}
function loadFactory(i)
{
    R.call("loadFactory", i).then(function() {
        T.program = i;
        refreshAll();
        toast(FACTORY[i]);
    });
}
function refreshAll() { setTimeout(layoutChain, 80); }

function setupMaster()
{
    knob($("#mkInput"), "fxInputGain", {
        size : 50,
        label : "ENTRADA",
        color : "#2fe0ff",
        skin : "neon",
        bipolar : true,
        format : db
    });
    knob($("#mkMix"), "fxDryWet",
         {size : 62, label : "DRY / WET", color : "#ffb340", skin : "amp", cls : "mix"});
    knob($("#mkOutput"), "fxOutputGain", {
        size : 50,
        label : "SAÍDA",
        color : "#ff4fd8",
        skin : "neon",
        bipolar : true,
        format : db
    });
}

/* ================= misc ================= */
let toastTimer = 0;
function toast(text)
{
    const t = $("#toast");
    t.textContent = text;
    t.hidden = false;
    t.classList.add("show");
    clearTimeout(toastTimer);
    toastTimer = setTimeout(function() {
        t.classList.remove("show");
        setTimeout(function() { t.hidden = true; }, 300);
    }, 1800);
}
function fit()
{
    S = Math.min(window.innerWidth / W, window.innerHeight / H);
    const app = $("#app");
    app.style.transform = "translate(" + Math.max(0, (window.innerWidth - W * S) / 2) + "px," +
                          Math.max(0, (window.innerHeight - H * S) / 2) + "px) scale(" + S + ")";
}
function knobDefs()
{
    const d = document.createElementNS(NS, "svg");
    d.setAttribute("width", 0);
    d.setAttribute("height", 0);
    d.style.position = "absolute";
    const rad = function(id, a, b, c) {
        return '<radialGradient id="' + id +
               '" cx=".38" cy=".3" r=".85"><stop offset="0" stop-color="' + a +
               '"/><stop offset=".55" stop-color="' + b + '"/><stop offset="1" stop-color="' + c +
               '"/></radialGradient>';
    };
    d.innerHTML =
        '<defs>' + rad("kPurple", "#d9b6ff", "#8a3cf0", "#3b1173") +
        rad("kAmber", "#ffd27a", "#e88a12", "#7a3a05") +
        rad("kPearl", "#ffffff", "#f3e3ff", "#c9a8d8") +
        rad("kTeal", "#9ff0ee", "#138a8f", "#0b4e57") +
        rad("kCream", "#fffaf0", "#f1dfbd", "#c9a77a") +
        rad("kRubber", "#55505e", "#2a2730", "#121016") +
        rad("kRed", "#ff9a7a", "#e5341f", "#7d1206") +
        rad("kAmp", "#ffc27a", "#f07a1f", "#a8460c") +
        rad("kIce", "#d6fbff", "#3fc8f0", "#0b4a7a") +
        rad("kBakelite", "#7a4a35", "#3b1d14", "#1b0b06") +
        rad("kNeon", "#2e2540", "#1a1426", "#0c0814") +
        '<linearGradient id="kChrome" x1="0" y1="0" x2="1" y2="1"><stop offset="0" stop-color="#ffffff"/><stop offset=".35" stop-color="#b9bdc7"/><stop offset=".55" stop-color="#eef0f4"/><stop offset="1" stop-color="#6e7380"/></linearGradient>' +
        '<radialGradient id="kGloss" cx=".35" cy=".25" r=".6"><stop offset="0" stop-color="#fff" stop-opacity=".45"/><stop offset="1" stop-color="#fff" stop-opacity="0"/></radialGradient>' +
        '</defs>';
    document.body.appendChild(d);
}

/* ================= animation ================= */
let last = performance.now();
function frame(now)
{
    const dt = Math.min(.05, (now - last) / 1000);
    last = now;
    const t = now / 1000;
    meter($("#inMeter"), T.in || 0);
    meter($("#outMeter"), T.out || 0);
    lfoHist.push(T.lfo || 0);
    lfoHist.shift();
    envHist.push(T.env || 0);
    envHist.shift();
    scope($("#lfoCanvas"), lfoHist, "#2fe0ff", true, false);
    scope($("#envCanvas"), envHist, "#ffb340", false, true);
    drawXY();
    drawStage(dt, now);
    const bypassed = v("fxBypass") > .5;
    inChain().forEach(function(m) {
        const d = devices[m];
        if (!d)
            return;
        const on = v("fxM" + m + "On") > .5 && !bypassed ? 1 : 0;
        d.energy += ((T.in || 0) * (on ? 1 : .2) - d.energy) * .25;
        const cv = d.cv, w = cv.clientWidth, h = cv.clientHeight;
        if (!w)
            return;
        const dpr = Math.min(2, window.devicePixelRatio || 1) * S;
        if (cv.width !== Math.round(w * dpr))
        {
            cv.width = Math.round(w * dpr);
            cv.height = Math.round(h * dpr);
        }
        const g = cv.getContext("2d");
        g.setTransform(dpr, 0, 0, dpr, 0, 0);
        g.clearRect(0, 0, w, h);
        g.save();
        const mod = MODS[m],
              n = mod.controls.map(function(_, k) { return v("fxM" + m + "P" + k); });
        const s = {
            n : n,
            p : n.map(function(x, k) { return physical(m, k, x); }),
            mix : v("fxM" + m + "Mix"),
            e : clamp(d.energy * 1.4, 0, 1),
            on : on
        };
        if (m === 14)
            s.pan = [ 0, 1, 2, 3, 4, 5, 6, 7 ].map(function(i) { return v("fxPan" + i); });
        try
        {
            ART.PAINT[m](g, w, h, t, s, DEV[m].c);
        }
        catch (err)
        { /* a display must never break the UI */
        }
        g.restore();
        d.el.style.setProperty("--e", (s.e * on).toFixed(3));
    });
    $("#chain").style.setProperty("--flow", (1.6 - clamp(T.in || 0, 0, 1)).toFixed(2) + "s");
    requestAnimationFrame(frame);
}

/* ================= boot ================= */
function boot(state)
{
    state.params.forEach(function(p) { P[p.id] = p; });
    MODS = state.modules;
    TARGETS = state.targets;
    FACTORY = state.factory;
    T.program = state.program;
    T.preset = state.preset;
    T.bank = state.bank;
    knobDefs();
    MODS.forEach(function(_, m) { buildDevice(m); });
    buildShelf();
    setupHeader();
    setupMaster();
    setupXY();
    setupMod();
    layoutChain();
    if (VIZ)
        viz = VIZ.create($("#stage"));
    setInterval(function() {
        $("#presetName").textContent = T.preset || "—";
        $("#presetSub").textContent =
            FACTORY[T.program] === T.preset
                ? "Preset de fábrica · " + String(T.program).padStart(2, "0")
                : "Preset do usuário";
        $$("#abBox .ab")
            .forEach(function(b) { b.classList.toggle("on", Number(b.dataset.slot) === T.bank); });
        $("#abCopyTo").textContent = "→ " + (T.bank ? "A" : "B");
    }, 150);
    $("#chainScroll").addEventListener("scroll", function() { requestAnimationFrame(drawCable); });
    if (R.simulated)
    {
        $("#sim").hidden = false;
        setTimeout(function() { $("#sim").classList.add("fade"); }, 4000);
    }
    requestAnimationFrame(frame);
}
window.addEventListener("resize", function() {
    fit();
    requestAnimationFrame(drawCable);
});
fit();
R.call("ready").then(boot);
})();
