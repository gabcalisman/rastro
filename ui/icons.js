/* RASTRO icon set: original line icons (24×24, stroke = currentColor) and the neon module art of the
   effect rack. Inline SVG so the plugin needs no icon font. */
(function () {
  "use strict";
  const S = function (body, extra) { return '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linecap="round" stroke-linejoin="round" aria-hidden="true"' + (extra || "") + '>' + body + '</svg>'; };
  const I = {
    wave: S('<path d="M3 12h1M6 8v8M9 5v14M12 9v6M15 6v12M18 9v6M21 12h0"/>'),
    sliders: S('<path d="M6 4v16M12 4v16M18 4v16"/><path d="M4 9h4M10 15h4M16 7h4"/>'),
    cube: S('<path d="M12 3l8 4.5v9L12 21l-8-4.5v-9z"/><path d="M4 7.5l8 4.5 8-4.5M12 12v9"/>'),
    heart: S('<path d="M12 20s-7-4.4-7-10a4 4 0 0 1 7-2.6A4 4 0 0 1 19 10c0 5.6-7 10-7 10z"/>'),
    heartFill: '<svg viewBox="0 0 24 24" aria-hidden="true"><path d="M12 20s-7-4.4-7-10a4 4 0 0 1 7-2.6A4 4 0 0 1 19 10c0 5.6-7 10-7 10z" fill="currentColor"/></svg>',
    save: S('<path d="M5 4h11l3 3v13H5z"/><path d="M8 4v5h7V4M8 20v-6h8v6"/>'),
    undo: S('<path d="M9 7L4 12l5 5"/><path d="M4 12h10a6 6 0 0 1 0 12" transform="translate(0 -6)"/>'),
    redo: S('<path d="M15 7l5 5-5 5"/><path d="M20 12H10a6 6 0 0 0 0 12" transform="translate(0 -6)"/>'),
    dice: '<svg viewBox="0 0 24 24" aria-hidden="true"><rect x="3" y="3" width="18" height="18" rx="4" fill="currentColor"/><g fill="#1a0b24"><circle cx="8" cy="8" r="1.6"/><circle cx="16" cy="8" r="1.6"/><circle cx="12" cy="12" r="1.6"/><circle cx="8" cy="16" r="1.6"/><circle cx="16" cy="16" r="1.6"/></g></svg>',
    gear: S('<circle cx="12" cy="12" r="3.2"/><path d="M12 2.5v3M12 18.5v3M2.5 12h3M18.5 12h3M5.3 5.3l2.1 2.1M16.6 16.6l2.1 2.1M5.3 18.7l2.1-2.1M16.6 7.4l2.1-2.1"/>'),
    menu: S('<path d="M4 7h16M4 12h16M4 17h10"/>'),
    play: '<svg viewBox="0 0 24 24" aria-hidden="true"><path d="M7 4.5v15l12-7.5z" fill="currentColor"/></svg>',
    pause: '<svg viewBox="0 0 24 24" aria-hidden="true"><rect x="6" y="5" width="4" height="14" rx="1" fill="currentColor"/><rect x="14" y="5" width="4" height="14" rx="1" fill="currentColor"/></svg>',
    stop: '<svg viewBox="0 0 24 24" aria-hidden="true"><rect x="6" y="6" width="12" height="12" rx="2" fill="currentColor"/></svg>',
    search: S('<circle cx="11" cy="11" r="6.5"/><path d="M16 16l4.5 4.5"/>'),
    filter: S('<path d="M4 7h16M7 12h10M10 17h4"/>'),
    left: S('<path d="M15 5l-7 7 7 7"/>'), right: S('<path d="M9 5l7 7-7 7"/>'), down: S('<path d="M6 9l6 6 6-6"/>'), up: S('<path d="M6 15l6-6 6 6"/>'),
    zoomIn: S('<circle cx="11" cy="11" r="6.5"/><path d="M16 16l4.5 4.5M8 11h6M11 8v6"/>'), zoomOut: S('<circle cx="11" cy="11" r="6.5"/><path d="M16 16l4.5 4.5M8 11h6"/>'),
    close: S('<circle cx="12" cy="12" r="9"/><path d="M6 18L18 6"/>'),
    room: S('<ellipse cx="12" cy="17" rx="8" ry="2.6"/><path d="M4 17V8l8-4 8 4v9"/><path d="M12 9v5"/>'),
    hanger: S('<path d="M12 7a2 2 0 1 1 2 2c-1 .4-2 1-2 2l9 6H3l9-6"/>'),
    spark: S('<path d="M12 3l2 6 6 2-6 2-2 6-2-6-6-2 6-2z"/>'),
    lab: S('<circle cx="6" cy="6" r="2.4"/><circle cx="18" cy="6" r="2.4"/><circle cx="12" cy="18" r="2.4"/><path d="M6 8.4c0 4 6 3 6 7.2M18 8.4c0 4-6 3-6 7.2"/>'),
    phones: S('<path d="M4 15v-3a8 8 0 0 1 16 0v3"/><rect x="3" y="14" width="4" height="6" rx="1.5"/><rect x="17" y="14" width="4" height="6" rx="1.5"/>'),
    glasses: S('<circle cx="7" cy="14" r="3.5"/><circle cx="17" cy="14" r="3.5"/><path d="M10.5 14h3M3.5 13L5 9M20.5 13L19 9"/>'),
    jacket: S('<path d="M9 3l3 3 3-3 5 3v14h-4v-8M9 3L4 6v14h4v-8"/><path d="M12 6v14"/>'),
    pants: S('<path d="M6 3h12l1 18h-5l-2-11-2 11H5z"/>'),
    shoe: S('<path d="M3 16v-5l4-1 3 3 5 1 5 2v2z"/><path d="M3 18h18"/>'),
    hat: S('<path d="M6 14a6 6 0 0 1 12 0"/><path d="M3 15h18"/><circle cx="12" cy="6" r="1.5"/>'),
    guitar: S('<path d="M14 10l5-5M17 3l4 4"/><path d="M11 9.5a3.5 3.5 0 0 0-5 1.5c-.5 1.4-2.5 1.5-3 3.5-.7 3 3.5 7.2 6.5 6.5 2-.5 2.1-2.5 3.5-3a3.5 3.5 0 0 0 1.5-5"/><circle cx="9" cy="15" r="1.5"/>'),
    keytar: S('<rect x="2" y="9" width="17" height="7" rx="2"/><path d="M5 9v4M8 9v4M11 9v4M14 9v4M19 12l3-3"/>'),
    knob: S('<circle cx="12" cy="12" r="8"/><path d="M12 12V5"/>'),
    palette: S('<path d="M12 3a9 9 0 1 0 0 18c1.5 0 2-1 1.5-2s0-2 1.5-2H18a3 3 0 0 0 3-3c0-6-4-11-9-11z"/><circle cx="7.5" cy="11" r="1.2"/><circle cx="10" cy="7" r="1.2"/><circle cx="15" cy="7.5" r="1.2"/>'),
    face: S('<circle cx="12" cy="12" r="8.5"/><path d="M8.5 10.5q1-1.5 2 0M13.5 10.5q1-1.5 2 0M8.5 14.5q3.5 3 7 0"/>'),
    flame: S('<path d="M12 3c1 4 6 5 6 11a6 6 0 0 1-12 0c0-3 2-4 2-6 2 1 2 3 2 3s2-3 2-8z"/>'),
    glow: S('<circle cx="12" cy="12" r="4"/><path d="M12 2v3M12 19v3M2 12h3M19 12h3M5 5l2 2M17 17l2 2M5 19l2-2M17 7l2-2"/>'),
    gem: S('<path d="M6 4h12l3 5-9 11L3 9z"/><path d="M3 9h18M9 4l3 16M15 4l-3 16"/>'),
    dance: S('<circle cx="13" cy="4" r="2"/><path d="M8 10l4-3 3 3 3-1M12 7l-1 6 4 3v5M11 13l-4 3-2 4"/>'),
    look: S('<rect x="4" y="3" width="16" height="18" rx="3"/><path d="M8 3v4h8V3M9 13h6"/>'),
    lock: S('<rect x="5" y="11" width="14" height="10" rx="2"/><path d="M8 11V8a4 4 0 0 1 8 0v3"/>'),
    star: '<svg viewBox="0 0 24 24" aria-hidden="true"><path d="M12 2.5l2.9 6 6.6.8-4.9 4.6 1.3 6.6L12 17.2 6.1 20.5l1.3-6.6L2.5 9.3l6.6-.8z" fill="currentColor"/></svg>',
    check: S('<path d="M5 12.5l4.5 4.5L19 7"/>'),
    plus: S('<path d="M12 5v14M5 12h14"/>'),
    power: S('<path d="M12 3v8"/><path d="M6.3 6.8a8 8 0 1 0 11.4 0"/>'),
    drag: '<svg viewBox="0 0 24 24" aria-hidden="true" fill="currentColor"><circle cx="9" cy="6" r="1.6"/><circle cx="15" cy="6" r="1.6"/><circle cx="9" cy="12" r="1.6"/><circle cx="15" cy="12" r="1.6"/><circle cx="9" cy="18" r="1.6"/><circle cx="15" cy="18" r="1.6"/></svg>',
    mic: S('<rect x="9" y="3" width="6" height="11" rx="3"/><path d="M5 11a7 7 0 0 0 14 0M12 18v3"/>'),
    rec: '<svg viewBox="0 0 24 24" aria-hidden="true"><circle cx="12" cy="12" r="7" fill="currentColor"/></svg>',
    swap: S('<path d="M4 8h14l-3-3M20 16H6l3 3"/>'),
    more: '<svg viewBox="0 0 24 24" aria-hidden="true" fill="currentColor"><circle cx="5" cy="12" r="2"/><circle cx="12" cy="12" r="2"/><circle cx="19" cy="12" r="2"/></svg>',
    music: S('<path d="M9 18V5l11-2v13"/><circle cx="6.5" cy="18" r="2.5"/><circle cx="17.5" cy="16" r="2.5"/>'),
    gift: S('<rect x="4" y="9" width="16" height="11" rx="1.5"/><path d="M3 9h18v-3H3zM12 6v14M12 6c-1-3-5-3-5-1s5 1 5 1zm0 0c1-3 5-3 5-1s-5 1-5 1z"/>'),
    folder: S('<path d="M3 6h6l2 2h10v11H3z"/>'),
    keys: S('<rect x="3" y="5" width="18" height="14" rx="2"/><path d="M7 5v9M11 5v9M15 5v9M19 5v9"/>'),
    pluck: S('<path d="M4 20c4-2 4-16 8-16s4 14 8 16"/>'),
    pad: S('<path d="M3 16c3-6 5-6 9-6s6 0 9 6"/><path d="M3 20h18"/>'),
    lead: S('<path d="M3 15l4-8 4 10 4-12 4 10 2-3"/>'),
    bass: S('<path d="M3 12c2 0 2-6 4.5-6S10 18 12 18s2.5-12 4.5-12S19 12 21 12"/>'),
    bell: S('<path d="M6 16V11a6 6 0 0 1 12 0v5l2 2H4z"/><path d="M10 20a2 2 0 0 0 4 0"/>'),
    texture: S('<circle cx="6" cy="8" r="1"/><circle cx="12" cy="6" r="1"/><circle cx="18" cy="9" r="1"/><circle cx="8" cy="14" r="1"/><circle cx="15" cy="13" r="1"/><circle cx="11" cy="19" r="1"/><circle cx="19" cy="17" r="1"/>'),
    drum: S('<ellipse cx="12" cy="8" rx="8" ry="3"/><path d="M4 8v8c0 1.7 3.6 3 8 3s8-1.3 8-3V8M3 3l6 5M21 3l-6 5"/>'),
    strings: S('<path d="M7 3v18M12 3v18M17 3v18M4 8c5 2 11 2 16 0"/>')
  };
  const Mcol = ["#b06bff", "#ffa02e", "#ff4fd8", "#2fe0ff", "#ffa02e", "#b06bff"];
  // Module emblem (left) and a small live picture of what the effect does (right).
  function modIcon(mi) {
    const c = Mcol[mi], g = 'stroke="' + c + '" fill="none" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"';
    const art = [
      '<circle cx="24" cy="24" r="15" ' + g + '/><path d="M17 22a2.5 3 0 1 0 5 0a2.5 3 0 1 0 -5 0M26 22a2.5 3 0 1 0 5 0a2.5 3 0 1 0 -5 0" fill="' + c + '"/><path d="M19 30q5 4 10 0" ' + g + '/>',
      '<path d="M24 8l8 8-8 8-8-8z M12 22l7 4-4 8-6-5z M30 24l8 2-2 9-8-3z M20 34l6 2-2 6-6-3z" ' + g + '/><circle cx="24" cy="16" r="2" fill="' + c + '"/>',
      '<path d="M24 34c-4-6-4-14 0-20 4 6 4 14 0 20zM24 34c-8-2-12-8-12-14 6 1 10 6 12 14zM24 34c8-2 12-8 12-14-6 1-10 6-12 14zM24 34c-10 0-16-4-18-8 6-2 13 1 18 8zM24 34c10 0 16-4 18-8-6-2-13 1-18 8z" ' + g + '/>',
      '<path d="M8 18q5-5 10 0t10 0 10 0M8 25q5-5 10 0t10 0 10 0M8 32q5-5 10 0t10 0 10 0" ' + g + '/>',
      Array.from({ length: 16 }, function (_, i) { const a = i / 16 * Math.PI * 2, r = i % 2 ? 9 : 14; return '<circle cx="' + (24 + Math.cos(a) * r).toFixed(1) + '" cy="' + (24 + Math.sin(a) * r).toFixed(1) + '" r="' + (i % 2 ? 1.6 : 2.2) + '" fill="' + c + '"/>'; }).join("") + '<circle cx="24" cy="24" r="4" fill="' + c + '"/>',
      [[14, 14], [22, 10], [30, 14], [18, 22], [26, 22], [34, 22], [14, 30], [22, 30], [30, 30], [22, 38]].map(function (p, i) { return '<rect x="' + p[0] + '" y="' + p[1] + '" width="6" height="6" rx="1" fill="' + c + '" opacity="' + (0.5 + (i % 3) * .25) + '"/>'; }).join("")
    ][mi];
    return '<svg class="mod-ic" viewBox="0 0 48 48" aria-hidden="true" style="filter:drop-shadow(0 0 6px ' + c + ')">' + art + '</svg>';
  }
  function modViz(mi) {
    const c = Mcol[mi], g = 'stroke="' + c + '" fill="none" stroke-width="2" stroke-linecap="round"';
    let s = "";
    if (mi === 0) s = '<path class="v-a" d="M0 22 C20 22 25 6 45 8 S70 22 100 20" ' + g + '/><path class="v-b" d="M0 24 C22 24 30 12 50 13 S75 24 100 23" ' + g + ' opacity=".45"/>';
    if (mi === 1 || mi === 4) for (let i = 0; i < 34; i++) { const x = (i * 37) % 100, y = 14 + Math.sin(i * 1.7) * 9 * (mi === 4 ? 1 : (1 - Math.abs(x - 50) / 60)); s += '<circle class="v-dot" cx="' + x + '" cy="' + y.toFixed(1) + '" r="' + (i % 3 ? 1 : 1.6) + '" fill="' + c + '"/>'; }
    if (mi === 2) s = '<g class="v-a" transform="translate(50 26)"><path d="M0 0c-4-6-4-14 0-20 4 6 4 14 0 20zM0 0c-8-2-12-8-12-14 6 1 10 6 12 14zM0 0c8-2 12-8 12-14-6 1-10 6-12 14zM0 0c-10 0-16-3-18-6 6-2 13 0 18 6zM0 0c10 0 16-3 18-6-6-2-13 0-18 6z" fill="' + c + '" opacity=".85"/></g>';
    if (mi === 3) s = '<path class="v-a" d="M0 15 Q12 0 25 15 T50 15 T75 15 T100 15" ' + g + ' stroke-width="2.6"/>';
    if (mi === 5) for (let i = 0; i < 18; i++) { const h = 4 + Math.abs(Math.sin(i * 2.3)) * 20; s += '<rect class="v-bar" x="' + (i * 5.6) + '" y="' + (28 - h).toFixed(1) + '" width="3.6" height="' + h.toFixed(1) + '" fill="' + c + '"/>'; }
    return '<svg class="mod-viz" viewBox="0 0 100 30" preserveAspectRatio="none" aria-hidden="true" style="filter:drop-shadow(0 0 4px ' + c + ')">' + s + '</svg>';
  }
  // Logo: RASTRO in the approved soft serif with the sparkle in the A, PLAY and the signature.
  function logo() {
    return '<span class="lg-word">R<span class="lg-a">A<svg class="lg-star" viewBox="0 0 24 24" aria-hidden="true"><path d="M12 1l2.4 8.6L23 12l-8.6 2.4L12 23l-2.4-8.6L1 12l8.6-2.4z"/></svg></span>STRO</span><span class="lg-side"><span class="lg-play">PLAY</span><span class="lg-by">by Gab Calisman</span></span>';
  }
  function hydrate(root) {
    (root || document).querySelectorAll("[data-ic]").forEach(function (el) { if (!el.firstChild || el.dataset.icDone !== "1") { el.insertAdjacentHTML("afterbegin", I[el.dataset.ic] || ""); el.dataset.icDone = "1"; } });
  }
  window.RastroIcons = { I: I, modIcon: modIcon, modViz: modViz, logo: logo, hydrate: hydrate, Mcol: Mcol };
})();
