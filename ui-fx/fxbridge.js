/* RASTRO FX bridge: talks to the C++ editor through JUCE's native integration.
   Without JUCE (browser preview) a small simulated engine answers instead, clearly labelled.
   Values travel as plain parameter values (the host's units): dB for gains, 0..1 for module knobs,
   integers for choices/positions, 0/1 for switches. */
(function() {
"use strict";
const juce = window.__JUCE__ && window.__JUCE__.backend;
const listeners = {};
function on(event, fn) { (listeners[event] = listeners[event] || []).push(fn); }
function fire(event, data)
{
    (listeners[event] || []).forEach(function(fn) { fn(data); });
}

if (juce)
{
    let nextId = 1;
    const pending = new Map();
    juce.addEventListener("__juce__complete", function(e) {
        const p = pending.get(e.promiseId);
        if (p)
        {
            pending.delete(e.promiseId);
            p(e.result);
        }
    });
    ["tick", "params", "state"].forEach(function(
        ev) { juce.addEventListener(ev, function(data) { fire(ev, data); }); });
    window.RastroFx = {
        simulated : false,
        on : on,
        call : function(name) {
            const args = Array.prototype.slice.call(arguments, 1);
            return new Promise(function(resolve) {
                const id = nextId++;
                pending.set(id, resolve);
                juce.emitEvent("__juce__invoke", {name : name, params : args, resultId : id});
            });
        }
    };
    return;
}

/* ------------------------------------------------------------------ */
/* Simulated engine for browser previews. NOT the plugin's DSP.        */
/* ------------------------------------------------------------------ */
// Mirror of Source/FxDsp.h `modules` (names, controls, ranges) for previews only.
const MODULES = [
    ["GHOST", "PLAY filtered echoes", [["Time", 0, 1, .5, ""], ["Feedback", 0, 1, .35, ""], ["Tone", 0, 1, .6, ""], ["Trail", 0, 1, .5, ""]]],
    ["CRUMBLE", "PLAY fragment repeater", [["Rate", 0, 1, .5, ""], ["Variation", 0, 1, .3, ""], ["Mutation", 0, 1, .3, ""]]],
    ["BLOOM", "PLAY diffuse space", [["Size", 0, 1, .6, ""], ["Tone", 0, 1, .6, ""], ["Decay", 0, 1, .5, ""], ["Trail", 0, 1, .5, ""]]],
    ["WOBBLE", "PLAY wandering tape", [["Depth", 0, 1, .25, ""], ["Rate", .15, 3, .3, "Hz"]]],
    ["DUST", "PLAY source-following texture", [["Amount", 0, 1, .15, ""], ["Character", 0, 1, .5, ""]]],
    ["CRUSH", "PLAY sample and bit reduction", [["Amount", 0, 1, .3, ""]]],
    ["MOUTH", "Vowel resonators / formant", [["Vowel A E I O U", 0, 4, 0, ""], ["Formant shift", .5, 2, .5, "x", 1], ["Resonance", 1, 12, .4, "Q"]]],
    ["ORBIT", "Through-space flanger", [["Rate", .03, 8, .3, "Hz", 1], ["Depth", .1, 6, .5, "ms"], ["Feedback", -.85, .85, .65, ""], ["Centre", .2, 8, .4, "ms"]]],
    ["TRAIL", "Clean feedback echo", [["Time", 20, 1800, .45, "ms", 1], ["Feedback", 0, .9, .4, ""], ["Tone", 300, 18000, .7, "Hz", 1], ["Colour", 0, 1, 0, ""]]],
    ["CHOIR", "Three-voice chorus", [["Rate", .05, 5, .4, "Hz", 1], ["Depth", .1, 10, .4, "ms"], ["Delay", 8, 35, .4, "ms"], ["Spread", 0, 1, .7, ""]]],
    ["PRISM", "Pitch echo / crystal", [["Pitch", -12, 24, 2 / 3, "st"], ["Time", 40, 1500, .45, "ms", 1], ["Feedback", 0, .82, .35, ""], ["Tone", 500, 16000, .75, "Hz", 1]]],
    ["EMBER", "Drive / tube / tape", [["Drive", 1, 24, .25, "x", 1], ["Tone", 300, 18000, .7, "Hz", 1], ["Bias", -.4, .4, .5, ""], ["Drive / Tube / Tape", 0, 2, 0, "mode"]]],
    ["SQUASH", "Elastic saturation + compression", [["Drive", 1, 12, .2, "x", 1], ["Threshold", -36, 0, .6, "dB"], ["Ratio", 1, 16, .3, ":1"], ["Release", 10, 1000, .5, "ms", 1]]],
    ["SIEVE", "Resonant LP / BP / HP", [["Cutoff", 30, 18000, .65, "Hz", 1], ["Resonance", .5, 8, .2, "Q"], ["Low / Band / High", 0, 2, 0, "mode"]]],
    ["SWING", "Editable eight-point panner", [["Rate", .02, 10, .4, "Hz", 1], ["Depth", 0, 1, 1, ""], ["Slew", 1, 250, .25, "ms", 1], ["Phase", 0, 1, 0, ""]]]
  ].map(function (m) {
    return { name: m[0], description: m[1], controls: m[2].map(function (c) { return { name: c[0], low: c[1], high: c[2], initial: c[3], unit: c[4], log: !!c[5] }; }) };
  });
const FACTORY = [
    "INIT / Clear wire", "Ghost postcard", "Crumbling bloom", "Dusty cassette", "Talking moon",
    "Orbital choir", "Crystal staircase", "Velvet furnace", "Rubber postcard", "Swing machine"
];
const P = {};
function add(id, min, max, def, step)
{
    P[id] = {id : id, v : def, min : min, max : max, def : def, step : step || 0};
}
add("fxInputGain", -24, 24, 0);
add("fxOutputGain", -24, 24, 0);
add("fxBypass", 0, 1, 0, 1);
add("fxDryWet", 0, 1, 1);
add("fxRouting", 0, 1, 0, 1);
const targets = [ "Input Gain", "Output Gain", "Global Dry/Wet" ];
MODULES.forEach(function(m, i) {
    add("fxM" + i + "On", 0, 1, 0, 1);
    add("fxM" + i + "Mix", 0, 1, .5);
    add("fxM" + i + "Position", 0, 14, i, 1);
    add("fxM" + i + "InRack", 0, 1, 0, 1);
    m.controls.forEach(function(c, k) {
        add("fxM" + i + "P" + k, 0, 1, c.initial);
        targets.push(m.name + " / " + c.name);
    });
    targets.push(m.name + " / Mix");
});
for (let i = 0; i < 8; i++)
{
    add("fxPan" + i, 0, 1, [ 0, .2, .5, .8, 1, .8, .5, .2 ][i]);
    targets.push("SWING / Point " + (i + 1));
}
["X", "Y", "Lfo", "Env"].forEach(function(p) {
    add("fx" + p + "Target", 0, targets.length, 0, 1);
    add("fx" + p + "Depth", -1, 1, p === "X" || p === "Y" ? .5 : 0);
});
add("fxX", 0, 1, .5);
add("fxY", 0, 1, .5);
add("fxLfoRate", .05, 20, 1);
add("fxLfoShape", 0, 3, 0, 1);
add("fxEnvAttack", 1, 200, 10);
add("fxEnvRelease", 10, 2000, 200);
add("fxEnvSensitivity", -24, 24, 0);
// A demo chain so the preview is not empty.
[[ 11, .7 ], [ 8, .35 ], [ 7, .4 ], [ 2, .3 ], [ 14, 1 ]].forEach(function(e, n) {
    P["fxM" + e[0] + "InRack"].v = 1;
    P["fxM" + e[0] + "On"].v = 1;
    P["fxM" + e[0] + "Mix"].v = e[1];
});
P.fxM11P0.v = .55;
P.fxM11P3.v = .5;
// demo order: EMBER, TRAIL, ORBIT, BLOOM, SWING first
[11, 8, 7, 2, 14].forEach(function(m, n) { P["fxM" + m + "Position"].v = n; });
let next = 5;
for (let i = 0; i < 15; i++)
    if ([ 11, 8, 7, 2, 14 ].indexOf(i) < 0)
        P["fxM" + i + "Position"].v = next++;
P.fxLfoTarget.v = targets.indexOf("ORBIT / Depth") + 1;
P.fxLfoDepth.v = .35;
P.fxLfoRate.v = .6;
P.fxXTarget.v = targets.indexOf("EMBER / Drive") + 1;
P.fxYTarget.v = targets.indexOf("TRAIL / Feedback") + 1;
let bank = 0, preset = "Neon furnace (prévia)", program = 0;
const changed = {};
function setv(id, v)
{
    if (!P[id])
        return;
    P[id].v = Math.min(P[id].max, Math.max(P[id].min, v));
    changed[id] = P[id].v;
}
let t0 = performance.now(), lfoPhase = 0, env = 0;
setInterval(function() {
    const now = performance.now(), t = (now - t0) / 1000;
    // a "song": kick-ish pulses plus a sustained bed
    const beat = (t * 2) % 1, hit = Math.exp(-beat * 6), bed = .25 + .1 * Math.sin(t * .7);
    const input = Math.min(1, hit * .7 + bed);
    const sens = Math.pow(10, P.fxEnvSensitivity.v / 20);
    const target = Math.min(1, input * sens);
    env += (target - env) * (target > env ? .5 : .08);
    lfoPhase = (lfoPhase + P.fxLfoRate.v / 30) % 1;
    const s = P.fxLfoShape.v;
    let w = Math.sin(lfoPhase * 6.283);
    if (s === 1)
        w = 1 - 4 * Math.abs(lfoPhase - .5);
    if (s === 2)
        w = 2 * lfoPhase - 1;
    if (s === 3)
        w = lfoPhase < .5 ? 1 : -1;
    const act = MODULES.map(function(
        m,
        i) { return P["fxM" + i + "On"].v * P["fxM" + i + "InRack"].v * P["fxM" + i + "Mix"].v; });
    const bypassed = P.fxBypass.v > .5;
    const scope = [];
    for (let i = 0; i < 256; i++)
    {
        const x = i / 256;
        scope.push(
            (Math.sin(x * 6.283 * 3 + t * 9) * .6 + Math.sin(x * 6.283 * 11 + t * 23) * .25) *
            input * .8);
    }
    fire("tick", { scope: scope, in: input * .8, out: bypassed ? input * .8 : Math.min(1.2, input * (.75 + .1 * act.reduce(function (a, b) { return a + b; }, 0))), lfo: w, env: env, act: act, bank: bank, preset: preset, program: program });
    if (Object.keys(changed).length)
    {
        fire("params", Object.assign({}, changed));
        for (const k in changed)
            delete changed[k];
    }
}, 1000 / 30);
const api = {
    ready : function() {
        return {
            params : Object.values(P).map(function(p) { return Object.assign({}, p); }),
            modules : MODULES,
            targets : targets,
            factory : FACTORY,
            program : program,
            preset : preset,
            bank : bank,
            latency : 46,
            version : "preview"
        };
    },
    setParam : function(id, v) {
        if (v !== undefined && v !== null)
            setv(id, v);
        return true;
    },
    loadFactory : function(i) {
        program = i;
        preset = FACTORY[i];
        return true;
    },
    compare : function() {
        bank = 1 - bank;
        return bank;
    },
    copyCompare : function() { return true; },
    setPresetName : function(n) {
        preset = n;
        return true;
    },
    savePreset : function() { return {ok : true, name : preset}; },
    openPreset : function() { return {ok : false, name : ""}; }
};
window.RastroFx = {
    simulated : true,
    on : on,
    call : function(name) {
        const args = Array.prototype.slice.call(arguments, 1);
        return Promise.resolve(api[name] ? api[name].apply(null, args) : null);
    }
};
})();
