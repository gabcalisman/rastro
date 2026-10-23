/* RASTRO art — original vector illustrations built as rigs (separate parts) so they can react
   to the sound. Provisional art: the final illustrations will replace these parts one by one. */
(function () {
  "use strict";
  const OL = 'class="ol"';

  /* ---------------- stage scene ---------------- */
  function bulbs() {
    let s = "", i;
    for (i = 0; i < 13; i++) {
      const x = 20 + i * 37, y = 26 + Math.sin(i / 12 * Math.PI) * 16;
      s += '<line x1="' + x + '" y1="' + (y - 6) + '" x2="' + x + '" y2="' + y + '" class="wire"/>' +
           '<circle class="bulb" data-i="' + i + '" cx="' + x + '" cy="' + (y + 5) + '" r="5.2"/>';
    }
    return '<path class="wire" d="M0 18 Q240 66 480 18" fill="none"/>' + s;
  }
  function city() {
    const b = [[0, 230], [40, 205], [70, 222], [96, 180], [130, 214], [160, 192], [196, 168], [226, 200], [258, 174], [290, 210], [320, 186], [352, 204], [384, 176], [420, 214], [452, 196], [480, 230]];
    let d = "M0 330 L0 " + b[0][1], w = "";
    for (let i = 1; i < b.length; i++) { d += " L" + b[i][0] + " " + b[i - 1][1] + " L" + b[i][0] + " " + b[i][1]; }
    d += " L480 330 Z";
    for (let i = 0; i < 46; i++) {
      const x = 12 + (i * 53) % 456, y = 200 + (i * 37) % 70;
      w += '<rect class="win" data-i="' + i + '" x="' + x + '" y="' + y + '" width="5" height="7" rx="1"/>';
    }
    return '<path class="city" d="' + d + '"/>' + w;
  }
  function records(x, y, n) {
    let s = "";
    const cols = ["rec1", "rec2", "rec3", "rec4"];
    for (let i = 0; i < n; i++) s += '<rect class="' + cols[i % 4] + '" ' + OL + ' x="' + (x + i * 9) + '" y="' + (y - 34 - (i % 3) * 3) + '" width="9" height="' + (34 + (i % 3) * 3) + '" rx="1.5"/>';
    return s;
  }
  function leaf(x, y, r, s, cls) {
    return '<g class="' + (cls || "") + '" style="transform-origin:' + x + 'px ' + y + 'px"><g class="leaf" transform="translate(' + x + ' ' + y + ') rotate(' + r + ') scale(' + s + ')">' +
      '<path class="leafA" ' + OL + ' d="M0 0 C 18 -40 70 -62 118 -50 C 104 -8 58 22 0 0 Z"/>' +
      '<path class="vein" d="M2 0 C 36 -14 74 -30 112 -48 M38 -14 l10 14 M62 -26 l10 15 M84 -36 l8 12 M40 -16 l-4 -14 M66 -28 l-4 -15"/></g></g>';
  }
  function monstera(x, y, r, s) {
    return '<g class="leaf sway2" transform="translate(' + x + ' ' + y + ') rotate(' + r + ') scale(' + s + ')">' +
      '<path class="leafB" ' + OL + ' d="M0 0 C -10 -50 30 -96 80 -100 C 132 -102 160 -60 150 -18 L 126 -30 L 136 -2 L 104 -20 L 106 10 L 74 -8 L 68 22 L 44 0 L 30 20 Z"/>' +
      '<path class="vein" d="M4 -4 C 40 -40 90 -70 140 -64"/></g>';
  }
  function scene() {
    return '<svg viewBox="0 0 480 452" preserveAspectRatio="xMidYMid slice" xmlns="http://www.w3.org/2000/svg" aria-hidden="true">' +
      '<defs>' +
      '<linearGradient id="gSky" x1="0" y1="0" x2="0" y2="1"><stop offset="0" class="sky1"/><stop offset=".55" class="sky2"/><stop offset="1" class="sky3"/></linearGradient>' +
      '<radialGradient id="gGlow"><stop offset="0" class="glow0"/><stop offset="1" class="glow1"/></radialGradient>' +
      '<radialGradient id="gSpot" cx=".5" cy=".5" r=".5"><stop offset="0" class="spot0"/><stop offset="1" class="spot1"/></radialGradient>' +
      '<linearGradient id="gLamp" x1="0" y1="0" x2="0" y2="1"><stop offset="0" class="lamp0"/><stop offset="1" class="lamp1"/></linearGradient>' +
      '<clipPath id="cWin"><circle cx="240" cy="186" r="140"/></clipPath>' +
      '<clipPath id="cLamp"><path d="M-14 -70 Q0 -84 14 -70 L22 0 L-22 0 Z"/></clipPath>' +
      '</defs>' +
      // window view
      '<g clip-path="url(#cWin)"><g id="pxSky"><rect x="-40" y="0" width="560" height="360" fill="url(#gSky)"/>' +
      '<g id="stars">' + Array.from({ length: 34 }, function (_, i) { return '<circle class="star" data-i="' + i + '" cx="' + ((i * 97) % 470 + 5) + '" cy="' + ((i * 61) % 150 + 50) + '" r="' + (i % 3 ? 1.1 : 1.8) + '"/>'; }).join("") + '</g>' +
      '<circle id="sun" class="sun" cx="240" cy="200" r="58"/><g id="sunBands"><rect x="170" y="196" width="140" height="5" class="band"/><rect x="170" y="210" width="140" height="7" class="band"/><rect x="170" y="226" width="140" height="9" class="band"/></g></g>' +
      '<g id="pxCity">' + city() + '</g></g>' +
      // wall with round window hole
      '<path class="wall" fill-rule="evenodd" d="M-20 -20 H500 V470 H-20 Z M240 46 A140 140 0 1 0 240.1 46 Z"/>' +
      '<rect class="wall2" x="-20" y="318" width="520" height="40"/><path class="trim" d="M-20 318 H500"/>' +
      '<circle ' + OL + ' class="frame" cx="240" cy="186" r="140" fill="none" stroke-width="9"/>' +
      '<path class="mullion" d="M240 46 V326 M100 186 H380"/>' +
      '<circle ' + OL + ' cx="240" cy="186" r="140" fill="none"/>' +
      bulbs() +
      // hanging plant
      '<g id="hangPlant" class="sway" style="transform-origin:58px 0px"><path class="wire" d="M58 0 V58"/>' + leaf(58, 82, 150, .5) + leaf(58, 82, 210, .45) + leaf(58, 82, 100, .42) +
      '<path ' + OL + ' class="pot" d="M40 60 H76 L70 88 H46 Z"/></g>' +
      // shelf with records + tape deck
      '<g id="shelf"><rect ' + OL + ' class="wood" x="10" y="150" width="104" height="10" rx="3"/>' + records(18, 150, 8) +
      '<rect ' + OL + ' class="wood" x="10" y="232" width="104" height="10" rx="3"/>' +
      '<g id="deck"><rect ' + OL + ' class="deck" x="18" y="190" width="88" height="42" rx="6"/><rect class="deckWin" x="26" y="196" width="72" height="22" rx="4"/>' +
      '<g class="reel" id="reel1" transform="translate(44 207)"><circle ' + OL + ' class="reelC" r="9"/><path class="spoke" d="M0 -8 V8 M-8 0 H8"/></g>' +
      '<g class="reel" id="reel2" transform="translate(80 207)"><circle ' + OL + ' class="reelC" r="9"/><path class="spoke" d="M0 -8 V8 M-8 0 H8"/></g>' +
      '<rect class="vu" id="vu" x="30" y="222" width="0" height="5" rx="2"/></g></g>' +
      // neon sign right
      '<g id="neon" transform="translate(392 96) rotate(-6)"><rect ' + OL + ' class="neonBox" x="-44" y="-30" width="92" height="56" rx="10"/>' +
      '<text class="neonText" x="2" y="10" text-anchor="middle">play</text></g>' +
      // lava lamp
      '<g id="lava" transform="translate(420 312)"><path ' + OL + ' class="lampBase" d="M-20 0 L20 0 L14 -14 L-14 -14 Z"/>' +
      '<g transform="translate(0 -14)"><path ' + OL + ' fill="url(#gLamp)" d="M-14 -70 Q0 -84 14 -70 L22 0 L-22 0 Z"/>' +
      '<g clip-path="url(#cLamp)"><ellipse id="blob1" class="blob" cx="-4" cy="-20" rx="9" ry="11"/><ellipse id="blob2" class="blob" cx="6" cy="-46" rx="7" ry="9"/><ellipse id="blob3" class="blob" cx="-2" cy="-62" rx="5" ry="6"/></g>' +
      '<path ' + OL + ' class="lampBase" d="M-12 -70 Q0 -82 12 -70 L8 -86 L-8 -86 Z"/></g></g>' +
      // floor
      '<path class="floor" d="M-20 352 H500 V470 H-20 Z"/><path class="floorLine" d="M-20 352 H500"/>' +
      '<ellipse id="rug" class="rug" ' + OL + ' cx="240" cy="410" rx="190" ry="34"/><ellipse class="rug2" cx="240" cy="410" rx="150" ry="24"/><ellipse class="rug3" cx="240" cy="410" rx="104" ry="15"/>' +
      '<ellipse id="spot" cx="240" cy="408" rx="120" ry="26" fill="url(#gSpot)"/>' +
      // speakers
      speaker(18, 278, "L") + speaker(392, 270, "R") +
      // crate
      '<g><rect ' + OL + ' class="wood" x="96" y="352" width="66" height="44" rx="4"/>' + records(102, 360, 5) + '</g>' +
      '<g id="ghosts"></g><g id="charLayer"></g>' +
      '<g id="notesP"></g><g id="dustP"></g>' +
      // foreground leaves
      '<g id="fg">' + monstera(-34, 470, -18, .95) + monstera(512, 474, 196, .9) + leaf(-20, 300, -24, .8, "sway") + leaf(500, 330, 200, .75, "sway") + '</g>' +
      '<rect id="darkVeil" x="0" y="0" width="480" height="452" fill="#1a0c10" opacity="0" pointer-events="none"/>' +
      '<rect id="bloomVeil" class="bloomVeil" x="0" y="0" width="480" height="452"/>' +
      '</svg>';
  }
  function speaker(x, y, side) {
    return '<g id="spk' + side + '" transform="translate(' + x + ' ' + y + ')"><rect ' + OL + ' class="spk" x="0" y="0" width="70" height="120" rx="8"/>' +
      '<g transform="translate(35 38)"><circle ' + OL + ' class="cone" r="22"/><circle id="cone' + side + '1" class="coneIn" r="11"/></g>' +
      '<g transform="translate(35 90)"><circle ' + OL + ' class="cone" r="15"/><circle id="cone' + side + '2" class="coneIn" r="7"/></g></g>';
  }

  /* ---------------- characters (rigged) ---------------- */
  // Every character exposes: root, hips, legL, legR, torso, armL (playing hand), armR, head, hair,
  // eyes (blink), pupils, mouth (open), instrument. Anchors for accessories in "anchors".
  function nova() {
    return {
      anchors: { head: [0, -268], headW: 104, chest: [0, -176], back: [0, -190], top: [0, -326] },
      svg:
        '<g id="c-root">' +
        '<g id="c-legL"><path ' + OL + ' class="pants" d="M-34 -124 L-8 -124 L-26 -18 L-56 -18 Z"/><path class="stripe" d="M-24 -118 L-44 -22"/>' +
        '<path ' + OL + ' class="shoe" d="M-70 -4 C -70 -20 -54 -24 -30 -20 C -20 -18 -18 -6 -22 0 L -66 0 C -70 0 -70 -2 -70 -4 Z"/><path class="sole" d="M-70 -3 H-22"/></g>' +
        '<g id="c-legR"><path ' + OL + ' class="pants" d="M8 -124 L34 -124 L56 -18 L26 -18 Z"/><path class="stripe" d="M24 -118 L44 -22"/>' +
        '<path ' + OL + ' class="shoe" d="M70 -4 C 70 -20 54 -24 30 -20 C 20 -18 18 -6 22 0 L 66 0 C 70 0 70 -2 70 -4 Z"/><path class="sole" d="M70 -3 H22"/></g>' +
        '<g id="c-torso"><path ' + OL + ' class="hood" d="M-44 -200 Q-54 -160 -42 -118 L42 -118 Q54 -160 44 -200 Q0 -214 -44 -200 Z"/>' +
        '<path class="trimO" d="M0 -204 V-120"/><path ' + OL + ' class="hoodBack" d="M-30 -206 Q0 -222 30 -206 Q0 -196 -30 -206 Z"/>' +
        '<path class="string" d="M-10 -202 V-176 M10 -202 V-180"/><path ' + OL + ' class="pocket" d="M-26 -150 H26 L20 -128 H-20 Z"/></g>' +
        '<g id="c-armR"><path ' + OL + ' class="hood" d="M38 -196 C 64 -190 76 -178 80 -160 L 64 -152 C 58 -168 50 -176 34 -178 Z"/><circle ' + OL + ' class="skin" cx="78" cy="-152" r="10"/></g>' +
        '<g id="c-inst" transform="translate(4 -150) rotate(-24)"><rect ' + OL + ' class="kt" x="-84" y="-16" width="112" height="34" rx="12"/>' +
        '<rect class="ktKeys" x="-72" y="-8" width="80" height="18" rx="3"/>' + Array.from({ length: 9 }, function (_, i) { return '<path class="ktLine" d="M' + (-63 + i * 9) + ' -8 V10"/>'; }).join("") +
        Array.from({ length: 6 }, function (_, i) { return '<rect class="ktBlack" x="' + (-66 + [0, 1, 3, 4, 5, 7][i] * 9) + '" y="-8" width="5" height="10"/>'; }).join("") +
        '<rect ' + OL + ' class="ktNeck" x="26" y="-7" width="76" height="14" rx="5"/><rect ' + OL + ' class="kt" x="98" y="-11" width="24" height="22" rx="6"/>' +
        '<circle class="ktDot" cx="-78" cy="0" r="3"/><circle class="ktDot" cx="16" cy="9" r="2.5"/></g>' +
        '<g id="c-armL"><path ' + OL + ' class="hood" d="M-40 -194 C -62 -184 -66 -164 -54 -144 L -40 -150 C -46 -164 -42 -174 -30 -180 Z"/><circle id="c-hand" ' + OL + ' class="skin" cx="-46" cy="-142" r="10"/></g>' +
        '<g id="c-head">' +
        '<path ' + OL + ' class="skin" d="M-46 -262 L-74 -288 L-50 -248 Z"/><path ' + OL + ' class="skin" d="M46 -262 L74 -288 L50 -248 Z"/>' +
        '<path ' + OL + ' class="skin" d="M-50 -268 C -52 -312 52 -312 50 -268 C 48 -238 22 -220 0 -218 C -22 -220 -48 -238 -50 -268 Z"/>' +
        '<path class="skinShade" d="M-44 -246 C -30 -226 30 -226 44 -246 C 30 -232 -30 -232 -44 -246 Z"/>' +
        '<circle class="spotS" cx="-24" cy="-296" r="4"/><circle class="spotS" cx="-12" cy="-302" r="2.6"/><circle class="spotS" cx="28" cy="-294" r="3"/>' +
        '<g id="c-eyes"><g transform="translate(-19 -264) rotate(10)"><ellipse ' + OL + ' class="eye" rx="14" ry="10"/><ellipse class="pupil c-pupil" rx="4" ry="8"/><path class="lid" d="M-15 -2 Q0 -14 15 -2 L15 -12 L-15 -12 Z"/></g>' +
        '<g transform="translate(19 -264) rotate(-10)"><ellipse ' + OL + ' class="eye" rx="14" ry="10"/><ellipse class="pupil c-pupil" rx="4" ry="8"/><path class="lid" d="M-15 -2 Q0 -14 15 -2 L15 -12 L-15 -12 Z"/></g></g>' +
        '<g id="c-mouth"><path class="mouthLine" d="M-14 -238 Q2 -228 16 -240"/><path class="tooth" d="M6 -235 L9 -229 L12 -236 Z"/></g>' +
        '<ellipse id="c-mouthOpen" class="mouthOpen" cx="1" cy="-236" rx="9" ry="0"/>' +
        '<g id="c-hair"><path ' + OL + ' class="hair" d="M-56 -274 L-70 -300 L-44 -296 L-54 -330 L-22 -310 L-16 -344 L4 -314 L24 -342 L28 -308 L56 -326 L46 -294 L70 -298 L52 -268 C 34 -292 -30 -292 -56 -274 Z"/>' +
        '<path class="hairShine" d="M-30 -304 L-18 -318 M12 -316 L20 -330"/></g>' +
        '<path ' + OL + ' class="gold" d="M-62 -246 l3 6 6 1 -5 4 2 6 -6 -3 -6 3 2 -6 -5 -4 6 -1 Z"/>' +
        '</g></g>'
    };
  }
  function fur(cx, cy, r, n, amp, seed) {
    let d = "";
    for (let i = 0; i <= n; i++) {
      const a = i / n * Math.PI * 2, rr = r + (i % 2 ? amp : -amp * .2) + Math.sin(i * 7.3 + seed) * amp * .4;
      const x = cx + Math.cos(a) * rr, y = cy + Math.sin(a) * rr * 1.04;
      d += (i ? " L" : "M") + x.toFixed(1) + " " + y.toFixed(1);
    }
    return d + " Z";
  }
  function fuzz() {
    return {
      anchors: { head: [0, -196], headW: 150, chest: [0, -120], back: [0, -150], top: [0, -268] },
      svg:
        '<g id="c-root">' +
        '<g id="c-legL"><path ' + OL + ' class="pants" d="M-40 -64 L-12 -64 L-16 -16 L-46 -16 Z"/><path ' + OL + ' class="shoe" d="M-66 -2 C -66 -20 -46 -24 -20 -20 C -10 -16 -12 -4 -14 0 L -62 0 Z"/><path class="sole" d="M-66 -3 H-14"/></g>' +
        '<g id="c-legR"><path ' + OL + ' class="pants" d="M12 -64 L40 -64 L46 -16 L16 -16 Z"/><path ' + OL + ' class="shoe" d="M66 -2 C 66 -20 46 -24 20 -20 C 10 -16 12 -4 14 0 L 62 0 Z"/><path class="sole" d="M66 -3 H14"/></g>' +
        '<g id="c-torso"><path ' + OL + ' class="furB" d="' + fur(0, -150, 92, 64, 9, 1) + '"/><path class="furShade" d="' + fur(0, -120, 70, 40, 5, 3) + '"/>' +
        '<path ' + OL + ' class="tee" d="M-56 -112 Q0 -96 56 -112 L50 -70 Q0 -60 -50 -70 Z"/><path class="teeStar" d="M0 -98 l4 8 9 1 -7 6 2 9 -8 -5 -8 5 2 -9 -7 -6 9 -1 Z"/></g>' +
        '<g id="c-armR"><path ' + OL + ' class="furB" d="M70 -140 C 98 -134 104 -112 96 -96 L 80 -102 C 82 -114 76 -122 64 -124 Z"/></g>' +
        '<g id="c-inst" transform="translate(0 -96) rotate(-20)"><rect ' + OL + ' class="kt" x="-84" y="-15" width="112" height="32" rx="12"/>' +
        '<rect class="ktKeys" x="-72" y="-7" width="80" height="16" rx="3"/>' + Array.from({ length: 9 }, function (_, i) { return '<path class="ktLine" d="M' + (-63 + i * 9) + ' -7 V9"/>'; }).join("") +
        '<rect ' + OL + ' class="ktNeck" x="26" y="-7" width="72" height="14" rx="5"/><rect ' + OL + ' class="kt" x="94" y="-11" width="24" height="22" rx="6"/></g>' +
        '<g id="c-armL"><path ' + OL + ' class="furB" d="M-70 -140 C -96 -128 -96 -104 -80 -90 L -66 -98 C -74 -110 -72 -120 -60 -126 Z"/><circle id="c-hand" ' + OL + ' class="furB" cx="-72" cy="-92" r="11"/></g>' +
        '<g id="c-head">' +
        '<g id="c-eyes"><g transform="translate(0 -176)"><circle ' + OL + ' class="eye" r="30"/><circle class="iris" r="16"/><circle class="pupil c-pupil" r="8"/><circle class="shine" cx="-6" cy="-8" r="4"/><path class="lid lidF" d="M-31 -2 A31 31 0 0 1 31 -2 L31 -34 L-31 -34 Z"/></g></g>' +
        '<g id="c-mouth"><path ' + OL + ' class="mouthDark" d="M-40 -132 Q0 -104 40 -132 Q0 -122 -40 -132 Z"/><path class="toothW" d="M-26 -129 l6 9 6 -7 M14 -127 l6 7 6 -9"/></g>' +
        '<ellipse id="c-mouthOpen" class="mouthOpen" cx="0" cy="-124" rx="26" ry="0"/>' +
        '<g id="c-hair"><path ' + OL + ' class="beanie" d="M-64 -214 C -64 -268 64 -268 64 -214 Z"/><rect ' + OL + ' class="beanieRim" x="-68" y="-222" width="136" height="16" rx="8"/>' +
        '<circle ' + OL + ' class="badge" cx="34" cy="-240" r="10"/><path class="badgeFace" d="M29 -242 h0.1 M39 -242 h0.1 M29 -237 q5 5 10 0"/>' +
        '<path ' + OL + ' class="phones" d="M-80 -196 C -80 -270 80 -270 80 -196" fill="none" stroke-width="6"/><rect ' + OL + ' class="phonesCup" x="-92" y="-206" width="20" height="34" rx="8"/><rect ' + OL + ' class="phonesCup" x="72" y="-206" width="20" height="34" rx="8"/></g>' +
        '</g></g>'
    };
  }

  /* ---------------- accessories (unlockables) ---------------- */
  const items = [
    { id: "shades", name: "Óculos Sunset", level: 2 },
    { id: "chain", name: "Corrente de Ouro", level: 3 },
    { id: "crown", name: "Coroa do Beat", level: 4 },
    { id: "stars", name: "Estrelinhas", level: 5 },
    { id: "halo", name: "Auréola de Vinil", level: 6 },
    { id: "cape", name: "Capa de Fita", level: 7 },
    { id: "cap", name: "Boné Neon", level: 8 },
    { id: "wings", name: "Asas de Cassete", level: 9 }
  ];
  function accessory(id, a) {
    const hx = a.head[0], hy = a.head[1], w = a.headW, cx = a.chest[0], cy = a.chest[1], bx = a.back[0], by = a.back[1], tx = a.top[0], ty = a.top[1];
    switch (id) {
      case "shades": return '<g transform="translate(' + hx + ' ' + (hy + 4) + ')"><circle ' + OL + ' class="shade" cx="-' + (w * .2) + '" cy="0" r="' + (w * .15) + '"/><circle ' + OL + ' class="shade" cx="' + (w * .2) + '" cy="0" r="' + (w * .15) + '"/><path class="ol" d="M-' + (w * .06) + ' -2 H' + (w * .06) + '"/><path class="glint" d="M-' + (w * .27) + ' -6 l6 -4"/></g>';
      case "chain": return '<path class="goldLine" d="M' + (cx - 26) + ' ' + (cy - 26) + ' Q' + cx + ' ' + (cy + 8) + ' ' + (cx + 26) + ' ' + (cy - 26) + '" fill="none"/><circle ' + OL + ' class="gold" cx="' + cx + '" cy="' + (cy - 6) + '" r="8"/>';
      case "crown": return '<path ' + OL + ' class="gold" transform="translate(' + tx + ' ' + (ty + 6) + ')" d="M-26 0 L-30 -30 L-14 -14 L0 -36 L14 -14 L30 -30 L26 0 Z"/>';
      case "stars": return '<g class="orbit" transform="translate(' + hx + ' ' + hy + ')">' + [0, 120, 240].map(function (r) { return '<path ' + OL + ' class="gold" transform="rotate(' + r + ') translate(' + (w * .75) + ' 0) scale(.8)" d="M0 -10 l3 7 8 1 -6 5 2 8 -7 -4 -7 4 2 -8 -6 -5 8 -1 Z"/>'; }).join("") + '</g>';
      case "halo": return '<g transform="translate(' + hx + ' ' + (hy - 6) + ')"><circle ' + OL + ' class="vinylH" r="' + (w * .72) + '"/><circle class="vinylG" r="' + (w * .55) + '"/><circle class="vinylL" r="' + (w * .14) + '"/></g>';
      case "cape": return '<path ' + OL + ' class="cape" d="M' + (bx - 40) + ' ' + by + ' Q' + bx + ' ' + (by - 14) + ' ' + (bx + 40) + ' ' + by + ' L' + (bx + 64) + ' ' + (by + 130) + ' Q' + bx + ' ' + (by + 150) + ' ' + (bx - 64) + ' ' + (by + 130) + ' Z"/>';
      case "cap": return '<g transform="translate(' + tx + ' ' + (ty + 18) + ')"><path ' + OL + ' class="capC" d="M-40 6 C -40 -34 40 -34 40 6 Z"/><path ' + OL + ' class="capC" d="M20 2 H72 Q72 12 20 12 Z"/><circle class="capDot" cx="0" cy="-12" r="5"/></g>';
      case "wings": return '<g transform="translate(' + bx + ' ' + by + ')"><path ' + OL + ' class="wing" d="M-30 0 C -110 -60 -150 10 -120 40 C -96 26 -70 30 -30 20 Z"/><path ' + OL + ' class="wing" d="M30 0 C 110 -60 150 10 120 40 C 96 26 70 30 30 20 Z"/></g>';
    }
    return "";
  }
  const behind = { halo: 1, cape: 1, wings: 1 };

  /* ---------------- module icons ---------------- */
  function icon(m) {
    const s = '<svg viewBox="0 0 56 52" class="card-art" aria-hidden="true">';
    switch (m) {
      case 0: return s + '<g class="ic-trail"><path class="ghostT" d="M14 44 V22 a14 14 0 0 1 28 0 V44 l-5 -4 -5 4 -4 -4 -5 4 -4 -4 Z"/></g><path ' + OL + ' class="ghostB" d="M14 44 V22 a14 14 0 0 1 28 0 V44 l-5 -4 -5 4 -4 -4 -5 4 -4 -4 Z"/><circle class="eyeD" cx="23" cy="24" r="2.6"/><circle class="eyeD" cx="33" cy="24" r="2.6"/></svg>';
      case 1: return s + '<circle ' + OL + ' class="cookie" cx="26" cy="27" r="15"/><g class="crumbs"><rect ' + OL + ' class="cookie" x="42" y="12" width="7" height="7" rx="2"/><rect ' + OL + ' class="cookie" x="44" y="30" width="6" height="6" rx="2"/><rect ' + OL + ' class="cookie" x="38" y="42" width="5" height="5" rx="1"/></g><circle class="chip" cx="21" cy="23" r="2.4"/><circle class="chip" cx="30" cy="31" r="2.4"/><circle class="chip" cx="27" cy="20" r="1.8"/></svg>';
      case 2: return s + '<g transform="translate(28 26)"><g class="petals">' + [0, 60, 120, 180, 240, 300].map(function (r) { return '<ellipse ' + OL + ' class="petal" transform="rotate(' + r + ') translate(0 -11)" rx="6" ry="10"/>'; }).join("") + '<circle ' + OL + ' class="core" r="7"/></g></g></svg>';
      case 3: return s + '<path ' + OL + ' class="jelly" d="M8 40 C 8 18 48 18 48 40 Z"/><path class="wave1" d="M6 46 q6 -6 11 0 t11 0 t11 0 t11 0"/><circle class="eyeD" cx="22" cy="30" r="2.3"/><circle class="eyeD" cx="34" cy="30" r="2.3"/></svg>';
      case 4: return s + '<g class="spin"><circle ' + OL + ' class="vinyl" cx="28" cy="26" r="19"/><circle class="groove" cx="28" cy="26" r="13"/><circle class="label" cx="28" cy="26" r="6"/></g><g class="specks"><circle cx="8" cy="10" r="1.6"/><circle cx="48" cy="8" r="1.3"/><circle cx="50" cy="44" r="1.7"/><circle cx="6" cy="42" r="1.2"/></g></svg>';
      case 5: return s + '<g class="px">' + [[12, 14], [20, 14], [28, 14], [12, 22], [20, 22], [28, 22], [36, 22], [20, 30], [28, 30], [36, 30], [28, 38], [36, 38]].map(function (p, i) { return '<rect ' + OL + ' class="pxr pxr' + (i % 3) + '" x="' + p[0] + '" y="' + p[1] + '" width="8" height="8"/>'; }).join("") + '</g></svg>';
    }
    return s + '</svg>';
  }

  /* ---------------- app-level leaves (decor in safe zones only) ---------------- */
  function cornerLeaves() {
    const L = function (x, y, r, s, k) { return '<svg style="left:' + x + 'px;top:' + y + 'px" width="10" height="10" viewBox="0 0 10 10"><g class="sway' + (k || "") + '">' + leaf(0, 0, r, s) + '</g></svg>'; };
    const M = function (x, y, r, s) { return '<svg style="left:' + x + 'px;top:' + y + 'px" width="10" height="10" viewBox="0 0 10 10">' + monstera(0, 0, r, s) + '</svg>'; };
    return M(-6, 790, -44, .75) + L(-4, 540, -30, .55, "2") + L(1206, 530, 210, .55) + M(1208, 792, 228, .8) + L(-6, 92, 20, .45, "2") + L(1206, 86, 160, .45);
  }

  /* ---------------- painted art (from the concept illustrations) ---------------- */
  // Painted characters: cut out from the approved paintings, split at the neck so the head can move.
  // Image px (height 760) -> stage units with scale k, origin at the feet.
  function painted(file, w, headH, pivot, head, top, inst, style) {
    return function () {
      const k = .395, W = w * k, H = 760 * k, ox = -W / 2, oy = -H;
      const X = function (x) { return (ox + x * k).toFixed(1); }, Y = function (y) { return (oy + y * k).toFixed(1); };
      let fx = "";
      if (inst.pads) { // 4x4 pad grid: each note lights one pad
        const p = inst.pads;
        fx = '<g id="c-fx" transform="rotate(' + p.rot + ' ' + X(p.x) + ' ' + Y(p.y) + ')">';
        for (let r = 0; r < 4; r++) for (let c = 0; c < 4; c++) fx += '<rect class="padLit" data-i="' + (r * 4 + c) + '" x="' + (ox + (p.x + (c - 2) * p.cell) * k + 1).toFixed(1) + '" y="' + (oy + (p.y + (r - 2) * p.cell) * k + 1).toFixed(1) + '" width="' + (p.cell * k - 2).toFixed(1) + '" height="' + (p.cell * k - 2).toFixed(1) + '" rx="1.5" fill="#ffc46b" opacity="0"/>';
        fx += '</g>';
      } else { // keyboard glow along the keys
        const g = inst.keys;
        fx = '<g id="c-fx" transform="rotate(' + g.rot + ' ' + X(g.x) + ' ' + Y(g.y) + ')"><ellipse class="keyGlow" cx="' + X(g.x) + '" cy="' + Y(g.y) + '" rx="' + (g.rx * k).toFixed(1) + '" ry="' + (g.ry * k).toFixed(1) + '" fill="url(#gKeys)" opacity="0"/>' +
          '<ellipse class="keySpot" cx="' + X(g.x) + '" cy="' + Y(g.y) + '" rx="' + (12 * k).toFixed(1) + '" ry="' + (g.ry * k * .7).toFixed(1) + '" fill="#fff6d0" opacity="0"/></g>';
      }
      return {
        painted: true, style: style, inst: inst,
        pivot: [ox + pivot[0] * k, oy + pivot[1] * k],
        anchors: { head: [ox + head[0] * k, oy + head[1] * k], headW: head[2] * k, chest: [ox + pivot[0] * k, oy + (pivot[1] + 50) * k], back: [ox + pivot[0] * k, oy + (pivot[1] + 35) * k], top: [ox + top[0] * k, oy + top[1] * k] },
        svg: '<defs><radialGradient id="gKeys"><stop offset="0" stop-color="#fff2c0" stop-opacity=".95"/><stop offset=".55" stop-color="#ffb347" stop-opacity=".5"/><stop offset="1" stop-color="#ff7a3d" stop-opacity="0"/></radialGradient></defs>' +
          '<g id="c-root"><g id="c-torso"><image href="' + file + '_body.png" x="' + ox.toFixed(1) + '" y="' + oy.toFixed(1) + '" width="' + W.toFixed(1) + '" height="' + H.toFixed(1) + '"/>' + fx + '</g>' +
          '<g id="c-head"><image href="' + file + '_head.png" x="' + ox.toFixed(1) + '" y="' + oy.toFixed(1) + '" width="' + W.toFixed(1) + '" height="' + (headH * k).toFixed(1) + '"/></g></g>'
      };
    };
  }
  // Each character plays its own way. bounce/sway/nod: dance amounts; hit: how a note lands
  // (dip = squash down, lean = tilt back, hop = quick jump, nodHit = head hit); jitter adds nervous energy.
  const STYLE = {
    fuzz: { bounce: 1.25, sway: .8, nod: 1.6, dip: 1.2, lean: .2, hop: 0, nodHit: 1.6, jitter: 0 },
    soul: { bounce: .45, sway: 2.2, nod: .9, dip: .4, lean: .3, hop: 0, nodHit: .6, jitter: 0, smooth: true },
    lua: { bounce: .8, sway: .6, nod: 1, dip: .3, lean: .2, hop: 1.4, nodHit: .8, jitter: .6 },
    nova: { bounce: .7, sway: 1, nod: 1.1, dip: .5, lean: 1.6, hop: 0, nodHit: 1, jitter: 0 },
    fuzz2: { bounce: 1, sway: 1.2, nod: 1.3, dip: .9, lean: .4, hop: 0, nodHit: 1.3, jitter: 0 },
    sapo: { bounce: .9, sway: .7, nod: 1.4, dip: 1.5, lean: 0, hop: 0, nodHit: 1.1, jitter: 0, fingers: true }
  };
  const fuzzArt = painted("fuzz", 482, 260, [220, 238], [190, 120, 260], [185, 14], { keys: { x: 215, y: 368, rx: 120, ry: 34, rot: -27 } }, STYLE.fuzz);
  const soulArt = painted("soul", 403, 270, [200, 232], [200, 130, 250], [200, 10], { keys: { x: 225, y: 352, rx: 140, ry: 30, rot: -20 } }, STYLE.soul);
  const luaArt = painted("lua", 397, 280, [205, 246], [200, 150, 230], [180, 14], { keys: { x: 260, y: 352, rx: 130, ry: 26, rot: -17 } }, STYLE.lua);
  const novaArt = painted("nova", 450, 290, [215, 262], [210, 170, 200], [230, 12], { keys: { x: 238, y: 400, rx: 115, ry: 30, rot: -31 } }, STYLE.nova);
  const fuzz2Art = painted("fuzz2", 379, 240, [185, 205], [190, 120, 230], [185, 10], { keys: { x: 190, y: 330, rx: 120, ry: 30, rot: -21 } }, STYLE.fuzz2);
  const sapoArt = painted("sapo", 398, 215, [180, 186], [170, 110, 200], [170, 6], { pads: { x: 163, y: 333, cell: 21, rot: 20 } }, STYLE.sapo);
  // Painted rooms: the painting plus glows placed over its objects, so they react to the sound.
  const ROOMS = {
    neon: { img: "room_neon.jpg", w: 1584, h: 993, tint: "#9c7bff", spot: "#bffcff", spotEdge: "#39e3d2", veil: "#0a0418",
      win: [225, 185, 430, 265], screen: [1370, 405, 128, 88, "#39e3d2"], bed: [250, 722, 760, 34], strip: "M585 38 L690 178 L690 420",
      lamp: [785, 100, 150], globe: [1160, 562, 120], spk: [[127, 583, 70], [250, 488, 48], [676, 478, 40], [932, 478, 34], [215, 872, 86]] },
    sunset: { img: "room_sunset.jpg", w: 1585, h: 992, tint: "#ffb066", spot: "#ffe2a8", spotEdge: "#ff9a4a", veil: "#1a0806",
      win: [265, 70, 415, 320], screen: [1150, 280, 110, 70, "#ffcf7a"], bed: [770, 600, 520, 30], strip: "",
      lamp: [1405, 205, 140], globe: [265, 450, 90], lamp2: [868, 425, 70], spk: [[290, 610, 52], [737, 590, 42], [1300, 300, 40], [1120, 305, 34], [1495, 705, 60]] },
    lofi: { img: "room_lofi.jpg", w: 1586, h: 992, tint: "#ffb066", spot: "#ffd7a0", spotEdge: "#ff8a4c", veil: "#07040f",
      win: [810, 40, 370, 350], screen: [845, 445, 95, 40, "#ffcf7a"], bed: [300, 690, 520, 26], strip: "",
      lamp: [1095, 180, 120], globe: [370, 385, 110], lamp2: null, spk: [[1210, 365, 46], [1295, 368, 58], [1420, 770, 80], [175, 860, 70]] },
    orbital: { img: "room_orbital.jpg", w: 1672, h: 941, tint: "#ffb066", spot: "#ffe2b0", spotEdge: "#ff8a4c", veil: "#120a14",
      win: [650, 30, 580, 380], screen: [1355, 370, 120, 60, "#ff9ad5"], bed: [80, 640, 380, 30], strip: "M597 135 L597 250",
      lamp: [270, 305, 100], globe: [1630, 105, 85], lamp2: [1105, 385, 60], spk: [[1335, 365, 50], [1622, 365, 55], [432, 75, 34], [1560, 640, 40]] },
    lunar: { img: "room_lunar.jpg", w: 1585, h: 992, tint: "#ff9a4a", spot: "#ffd7a0", spotEdge: "#c05cff", veil: "#07040f",
      win: [650, 85, 440, 440], screen: [420, 345, 120, 90, "#ffcf7a"], bed: [520, 700, 600, 40], strip: "",
      lamp: [555, 400, 90], globe: [1385, 445, 95], lamp2: [262, 190, 80], spk: [[345, 585, 58], [315, 488, 34], [660, 560, 40], [1500, 545, 46]] }
  };
  function roomOffset(kind) { const c = ROOMS[kind]; return c ? -(c.w * 488 / c.h - 480) / 2 : 0; }
  function scenePainted(kind) {
    const c = ROOMS[kind];
    const glow = function (id, g) { return g ? '<circle id="' + id + '" cx="' + g[0] + '" cy="' + g[1] + '" r="' + g[2] + '" fill="url(#gWarm)" opacity="0"/>' : ""; };
    return '<svg viewBox="0 0 480 452" preserveAspectRatio="xMidYMid slice" xmlns="http://www.w3.org/2000/svg" aria-hidden="true">' +
      '<defs><radialGradient id="gWarm"><stop offset="0" stop-color="#ffd38a" stop-opacity=".95"/><stop offset=".45" stop-color="#ff9a4a" stop-opacity=".45"/><stop offset="1" stop-color="#ff7a3d" stop-opacity="0"/></radialGradient>' +
      '<radialGradient id="gCyan"><stop offset="0" stop-color="' + c.spot + '" stop-opacity=".95"/><stop offset=".5" stop-color="' + c.spotEdge + '" stop-opacity=".4"/><stop offset="1" stop-color="' + c.spotEdge + '" stop-opacity="0"/></radialGradient>' +
      '<radialGradient id="gSpot" cx=".5" cy=".5" r=".5"><stop offset="0" stop-color="' + c.spot + '" stop-opacity=".55"/><stop offset="1" stop-color="' + c.spotEdge + '" stop-opacity="0"/></radialGradient>' +
      '<radialGradient id="gShadow" cx=".5" cy=".5" r=".5"><stop offset="0" stop-color="#0a0418" stop-opacity=".7"/><stop offset="1" stop-color="#0a0418" stop-opacity="0"/></radialGradient></defs>' +
      '<g id="room" transform="translate(' + roomOffset(kind).toFixed(1) + ' -18) scale(' + (488 / c.h).toFixed(4) + ')">' +
      '<image href="' + c.img + '" x="0" y="0" width="' + c.w + '" height="' + c.h + '"/>' +
      '<rect id="winGlow" x="' + c.win[0] + '" y="' + c.win[1] + '" width="' + c.win[2] + '" height="' + c.win[3] + '" fill="' + c.tint + '" opacity="0"/>' +
      '<rect id="arcadeGlow" x="' + c.screen[0] + '" y="' + c.screen[1] + '" width="' + c.screen[2] + '" height="' + c.screen[3] + '" rx="10" fill="' + c.screen[4] + '" opacity="0"/>' +
      '<rect id="bedGlow" x="' + c.bed[0] + '" y="' + c.bed[1] + '" width="' + c.bed[2] + '" height="' + c.bed[3] + '" rx="15" fill="url(#gWarm)" opacity="0"/>' +
      (c.strip ? '<path id="neonStrip" d="' + c.strip + '" stroke="#9ffcff" stroke-width="14" fill="none" stroke-linecap="round" opacity="0"/>' : "") +
      glow("lampGlow", c.lamp) + glow("globeGlow", c.globe) + glow("lamp2Glow", c.lamp2) +
      '<g id="spkGlows">' + c.spk.map(function (s, i) { return '<circle class="spkGlow" data-i="' + i + '" cx="' + s[0] + '" cy="' + s[1] + '" r="' + s[2] + '" fill="url(#gCyan)" opacity="0"/>'; }).join("") + '</g>' +
      '</g>' +
      '<ellipse cx="240" cy="414" rx="92" ry="16" fill="url(#gShadow)"/>' +
      '<ellipse id="spot" cx="240" cy="410" rx="130" ry="30" fill="url(#gSpot)" opacity=".3"/>' +
      '<g id="ghosts"></g><g id="charLayer"></g><g id="notesP"></g><g id="dustP"></g>' +
      '<rect id="darkVeil" x="0" y="0" width="480" height="452" fill="' + c.veil + '" opacity="0" pointer-events="none"/>' +
      '<rect id="bloomVeil" x="0" y="0" width="480" height="452" fill="' + c.tint + '" opacity="0" pointer-events="none"/>' +
      '</svg>';
  }

  // STAGE: Gab's studio with the round platform. Unlike the other rooms, everything (painting, lights, platform,
  // character, effects) lives in one "world" group in picture pixels, moved by a game-style camera in app.js.
  // Every glow sits on a real object of the painting: lamps, speakers, lava lamp, neon strips, city lights,
  // planet, the platform rings and its front light bars.
  const STAGE = { img: "room_stage.jpg", w: 1672, h: 941, feet: [836, 664], plat: [836, 664, 462, 50],
    lamps: [[165, 240, 95], [460, 276, 60], [1290, 362, 95], [1630, 266, 60], [1420, 306, 50], [316, 384, 45], [1020, 360, 40]],
    lava: [1040, 478, 75], planet: [1032, 118, 105],
    spk: [[565, 250, 58], [722, 366, 36], [1185, 280, 56], [1113, 452, 30]],
    strips: ["M388 0 V300", "M1252 112 V226", "M300 146 H452", "M1268 0 V40"],
    bars: [[712, 761, 240, 13, "o"], [457, 737, 93, 11, "c"], [1122, 739, 93, 11, "c"]],
    dots: [[366, 733], [381, 733], [396, 733], [1286, 733], [1301, 733], [1316, 733]],
    city: [700, 150, 1150, 330] };
  function sceneStage() {
    const c = STAGE, p = c.plat;
    let rnd = 7; const R = function () { rnd = (rnd * 16807) % 2147483647; return rnd / 2147483647; };
    let tw = "";
    for (let i = 0; i < 34; i++) {
      const x = c.city[0] + R() * (c.city[2] - c.city[0]), y = c.city[1] + R() * (c.city[3] - c.city[1]);
      if (Math.hypot(x - c.planet[0], y - c.planet[1]) < 90) continue;
      tw += '<rect class="twk" x="' + x.toFixed(0) + '" y="' + y.toFixed(0) + '" width="' + (3 + R() * 4).toFixed(0) + '" height="' + (4 + R() * 5).toFixed(0) + '" fill="' + ["#ffd27a", "#ff7ad9", "#9ffcff", "#ffffff"][(R() * 4) | 0] + '" opacity="0"/>';
    }
    const glow = function (cls, g, fill) { return '<circle class="' + cls + '" cx="' + g[0] + '" cy="' + g[1] + '" r="' + g[2] + '" fill="' + fill + '" opacity="0"/>'; };
    const ring = function (rx, ry, w, dash) { return '<ellipse cx="' + p[0] + '" cy="' + p[1] + '" rx="' + rx + '" ry="' + ry + '" fill="none" stroke-width="' + w + '"' + (dash ? ' stroke-dasharray="' + dash + '"' : "") + '/>'; };
    return '<svg viewBox="0 0 480 452" preserveAspectRatio="xMidYMid slice" xmlns="http://www.w3.org/2000/svg" aria-hidden="true">' +
      '<defs><radialGradient id="gWarm"><stop offset="0" stop-color="#ffd38a" stop-opacity=".95"/><stop offset=".45" stop-color="#ff9a4a" stop-opacity=".45"/><stop offset="1" stop-color="#ff7a3d" stop-opacity="0"/></radialGradient>' +
      '<radialGradient id="gPink"><stop offset="0" stop-color="#ffb6f0" stop-opacity=".9"/><stop offset=".5" stop-color="#d06bff" stop-opacity=".35"/><stop offset="1" stop-color="#8a3cf0" stop-opacity="0"/></radialGradient>' +
      '<radialGradient id="gCyan"><stop offset="0" stop-color="#d6fbff" stop-opacity=".9"/><stop offset=".5" stop-color="#39e3d2" stop-opacity=".35"/><stop offset="1" stop-color="#39e3d2" stop-opacity="0"/></radialGradient>' +
      '<radialGradient id="gGlow"><stop id="gGlow0" offset="0" stop-color="#5fe3ff" stop-opacity=".85"/><stop id="gGlow1" offset="1" stop-color="#5fe3ff" stop-opacity="0"/></radialGradient>' +
      '<linearGradient id="gCone" x1="0" y1="0" x2="0" y2="1"><stop id="gCone0" offset="0" stop-color="#5fe3ff" stop-opacity=".0"/><stop id="gCone1" offset=".25" stop-color="#5fe3ff" stop-opacity=".35"/><stop id="gCone2" offset="1" stop-color="#5fe3ff" stop-opacity="0"/></linearGradient>' +
      '<radialGradient id="gVig" cx=".5" cy=".46" r=".72"><stop offset=".62" stop-color="#05020c" stop-opacity="0"/><stop offset="1" stop-color="#05020c" stop-opacity=".7"/></radialGradient>' +
      '<clipPath id="platClip"><ellipse cx="' + p[0] + '" cy="' + p[1] + '" rx="' + (p[2] - 8) + '" ry="' + (p[3] - 4) + '"/></clipPath></defs>' +
      '<g id="world">' +
      '<image href="' + c.img + '" x="0" y="0" width="' + c.w + '" height="' + c.h + '"/>' +
      '<g id="twinkles">' + tw + '</g>' +
      glow("planetG", c.planet, "url(#gPink)") +
      '<g id="lampGs">' + c.lamps.map(function (l) { return glow("lampG", l, "url(#gWarm)"); }).join("") + '</g>' +
      glow("lavaG", c.lava, "url(#gWarm)") +
      '<g id="stripGs" fill="none" stroke-linecap="round">' + c.strips.map(function (d) { return '<path class="stripG" d="' + d + '" stroke="#ffc46b" stroke-width="10" opacity="0"/>'; }).join("") + '</g>' +
      '<g id="spkGlows">' + c.spk.map(function (s, i) { return '<circle class="spkGlow" data-i="' + i + '" cx="' + s[0] + '" cy="' + s[1] + '" r="' + s[2] + '" fill="url(#gCyan)" opacity="0"/>'; }).join("") + '</g>' +
      // light cones from the ceiling onto the platform (follow the glow colour, sway with the groove)
      '<g id="cones" opacity="0" display="none"><path id="coneA" d="M560 -20 L620 -20 L900 690 L640 690Z" fill="url(#gCone)"/><path id="coneB" d="M1050 -20 L1110 -20 L1030 690 L770 690Z" fill="url(#gCone)"/></g>' +
      '<ellipse id="platPool" cx="' + p[0] + '" cy="' + p[1] + '" rx="' + (p[2] - 30) + '" ry="' + (p[3] - 4) + '" fill="url(#gGlow)" opacity="0"/>' +
      '<g id="platRings" stroke="#5fe3ff" opacity="0">' + ring(p[2], p[3], 5, "60 26") + ring(375, 40, 3.5, "") + ring(280, 30, 3, "18 14") + ring(170, 18, 2.5, "") + '</g>' +
      '<g id="barGs">' + c.bars.map(function (b) { return '<rect class="barG" data-k="' + b[4] + '" x="' + (b[0] - 14) + '" y="' + (b[1] - 10) + '" width="' + (b[2] + 28) + '" height="' + (b[3] + 20) + '" rx="' + ((b[3] + 20) / 2) + '" fill="#ffb340" opacity="0"/>'; }).join("") + '</g>' +
      '<g id="dotGs">' + c.dots.map(function (d) { return '<circle class="dotG" cx="' + d[0] + '" cy="' + d[1] + '" r="9" fill="#fff4d6" opacity="0"/>'; }).join("") + '</g>' +
      '<g id="ghosts"></g><g id="charLayer"></g><g id="notesP"></g><g id="dustP"></g>' +
      '</g>' +
      '<g id="bokeh"></g>' +
      '<rect x="-200" y="-200" width="880" height="852" fill="url(#gVig)" pointer-events="none"/>' +
      '<rect id="darkVeil" x="-200" y="-200" width="880" height="852" fill="#07030f" opacity="0" pointer-events="none"/>' +
      '<rect id="bloomVeil" x="-200" y="-200" width="880" height="852" fill="#ff7ad9" opacity="0" pointer-events="none"/>' +
      '</svg>';
  }

  // Parts declare an outline plus their own colour class; merge them into one class attribute.
  function F(s) { return s.replace(/class="ol" class="([^"]+)"/g, 'class="ol $1"'); }
  function wrapChar(make) { return function () { const c = make(); c.svg = F(c.svg); return c; }; }
  window.RastroArt = {
    scene: function (kind) { return kind === "stage" ? sceneStage() : ROOMS[kind] ? scenePainted(kind) : F(scene()); },
    stage: STAGE,
    roomScale: function (kind) { return ROOMS[kind] ? 488 / ROOMS[kind].h : 1; },
    characters: { nova: novaArt, fuzz: fuzzArt, fuzz2: fuzz2Art, soul: soulArt, lua: luaArt, sapo: sapoArt, novaVector: wrapChar(nova), fuzzVector: wrapChar(fuzz) },
    rooms: ["sunset", "neon", "lofi", "lunar", "orbital", "stage"],
    roomNames: ["Sunset Loft", "Quarto Neon", "Lo-fi", "Base Lunar", "Orbital", "Estúdio Palco"],
    roomOffset: roomOffset,
    paintedSize: { fuzz: [482, 260], fuzz2: [379, 240], sapo: [398, 215], soul: [403, 270], lua: [397, 280], nova: [450, 290] },
    items: items,
    accessory: function (id, a) { return F(accessory(id, a)); },
    behind: behind,
    icon: function (m) { return F(icon(m)); },
    cornerLeaves: function () { return F(cornerLeaves()); }
  };
})();
