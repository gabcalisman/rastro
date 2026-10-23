/* RASTRO hero in real-time 3D (PR #3 prototype).
   An original character built in code after Gab's design: glossy black alien, neon-edged crest, gold
   headphones, cyan crescent eyes, Flare 70s outfit and a keytar. It is a real rig: hips, spine, head,
   two arms and two legs. Legs and arms are solved with two-bone IK every frame, so the feet stay planted
   on the platform and both hands stay on the instrument (exactly two hands, always).
   Rendering is light: one small canvas, a few hundred draw-call-free primitives merged per material,
   no shadows, no post-processing. Nothing here touches audio; it only reads the telemetry the stage
   already receives. If WebGL is not available, create() returns null and the painted 2.5D puppet stays. */
(function () {
  "use strict";
  const T = window.THREE;
  if (!T) { window.RastroHero3D = { create: function () { return null; } }; return; }
  const V = function (x, y, z) { return new T.Vector3(x || 0, y || 0, z || 0); };
  const clamp = function (v, a, b) { return Math.min(b, Math.max(a, v)); };
  const DOWN = V(0, -1, 0);

  // ---------- procedural textures (canvas): 70s swirl fabric and the keytar keyboard ----------
  function swirlTexture(base, a, b) {
    const c = document.createElement("canvas"); c.width = c.height = 256;
    const g = c.getContext("2d"); g.fillStyle = base; g.fillRect(0, 0, 256, 256);
    g.lineCap = "round";
    for (let i = 0; i < 9; i++) {
      g.strokeStyle = i % 3 === 0 ? b : a; g.lineWidth = 9 + (i % 4) * 4; g.globalAlpha = .85;
      g.beginPath();
      const y0 = i * 30 - 20;
      for (let x = -20; x <= 276; x += 8) { const y = y0 + Math.sin(x / 34 + i) * 18 + Math.sin(x / 13 + i * 2) * 5; if (x === -20) g.moveTo(x, y); else g.lineTo(x, y); }
      g.stroke();
    }
    const t = new T.CanvasTexture(c); t.wrapS = t.wrapT = T.RepeatWrapping; t.colorSpace = T.SRGBColorSpace; return t;
  }
  function keysTexture() {
    const c = document.createElement("canvas"); c.width = 512; c.height = 96;
    const g = c.getContext("2d"); g.fillStyle = "#14101c"; g.fillRect(0, 0, 512, 96);
    const n = 15, w = 512 / n;
    for (let i = 0; i < n; i++) { g.fillStyle = "#f7f1ff"; g.fillRect(i * w + 2, 4, w - 4, 88); }
    g.fillStyle = "#0d0a14";
    for (let i = 0; i < n - 1; i++) { const k = i % 7; if (k === 2 || k === 6) continue; g.fillRect(i * w + w * .66, 4, w * .68, 52); }
    const t = new T.CanvasTexture(c); t.colorSpace = T.SRGBColorSpace; return t;
  }
  // Glossy black only looks glossy if it has something to reflect: a tiny neon "room" for reflections.
  function neonEnvironment(renderer) {
    const s = new T.Scene(); s.background = new T.Color("#07030d");
    const add = function (color, x, y, z, w, h, r) { const m = new T.Mesh(new T.PlaneGeometry(w, h), new T.MeshBasicMaterial({ color: color, side: T.DoubleSide })); m.position.set(x, y, z); m.lookAt(0, 0, 0); if (r) m.rotateZ(r); s.add(m); };
    add("#ff9a3d", -4, 3, 3, 3, .5); add("#ff4fd8", 4, 2.5, 2, 2.6, .5); add("#2fe0ff", 3.5, 1, -4, 2.2, .4);
    add("#b06bff", -3.5, -1, -3, 3, .6); add("#fff1d6", 0, 6, 2, 4, 1.2); add("#ffb340", 0, -3, 4, 6, .4);
    const pm = new T.PMREMGenerator(renderer), env = pm.fromScene(s, .02).texture; pm.dispose(); return env;
  }

  // ---------- geometry helpers ----------
  function limb(r0, r1, len, mat) {
    const g = new T.CylinderGeometry(r0, r1, len, 18, 1); g.translate(0, -len / 2, 0);
    const m = new T.Mesh(g, mat); const cap = new T.Mesh(new T.SphereGeometry(r0 * 1.02, 14, 10), mat); m.add(cap); return m;
  }
  function taperedTube(curve, r0, r1, seg, radial) {
    const pts = curve.getSpacedPoints(seg), frames = curve.computeFrenetFrames(seg, false), pos = [], idx = [];
    for (let i = 0; i <= seg; i++) {
      const r = r0 + (r1 - r0) * Math.pow(i / seg, 1.3), N = frames.normals[i], B = frames.binormals[i];
      for (let j = 0; j <= radial; j++) { const a = j / radial * Math.PI * 2, cx = Math.cos(a), cy = Math.sin(a); pos.push(pts[i].x + r * (cx * N.x + cy * B.x), pts[i].y + r * (cx * N.y + cy * B.y), pts[i].z + r * (cx * N.z + cy * B.z)); }
    }
    for (let i = 0; i < seg; i++) for (let j = 0; j < radial; j++) { const a = i * (radial + 1) + j, b = a + radial + 1; idx.push(a, b, a + 1, b, b + 1, a + 1); }
    const g = new T.BufferGeometry(); g.setAttribute("position", new T.Float32BufferAttribute(pos, 3)); g.setIndex(idx); g.computeVertexNormals(); return g;
  }
  // Aim a mesh whose geometry hangs along -Y from its origin so it spans from -> to.
  const _d = V(), _q = new T.Quaternion();
  function aim(mesh, from, to) { mesh.position.copy(from); _d.subVectors(to, from).normalize(); mesh.quaternion.setFromUnitVectors(DOWN, _d); }
  // Two-bone IK: root R, lengths a b, target Tg, pole P -> middle joint position (written into out).
  const _t = V(), _p = V(), _u = V();
  function ik(R, a, b, Tg, P, out) {
    _t.subVectors(Tg, R); let d = _t.length(); d = clamp(d, Math.abs(a - b) + 1e-3, a + b - 1e-3); _t.normalize();
    const cosA = clamp((a * a + d * d - b * b) / (2 * a * d), -1, 1), sinA = Math.sqrt(1 - cosA * cosA);
    _p.subVectors(P, R); _u.copy(_p).addScaledVector(_t, -_p.dot(_t)); if (_u.lengthSq() < 1e-8) _u.set(0, 0, 1); _u.normalize();
    out.copy(R).addScaledVector(_t, a * cosA).addScaledVector(_u, a * sinA);
    // the end point actually reached (target clamped to reach)
    return Tg.clone().sub(R).setLength(Math.min(Tg.distanceTo(R), a + b - 1e-3)).add(R);
  }
  function spring(st, key, target, w, dt, kick) {
    let s = st[key]; if (!s) s = st[key] = { x: target, v: 0 };
    if (kick) s.v += kick;
    if (dt <= 0) { s.x = target; s.v = 0; return s.x; }
    const n = Math.ceil(dt / (1 / 120)), h = dt / n;
    for (let i = 0; i < n; i++) { s.v += (w * w * (target - s.x) - 2 * w * s.v) * h; s.x += s.v * h; }
    return s.x;
  }

  // Outfit colour schemes (the 3D hero wears its own cut; colours follow the chosen look)
  const OUTFITS = {
    disco: { jacket: ["#5a1f9a", "#ff9a3d", "#ffcf7a"], pants: ["#6a24b0", "#ff8a3d", "#ffd27a"], lapel: "#ffb340" },
    guitar: { jacket: ["#1fb7b0", "#ffb340", "#2a2140"], pants: ["#1fb7b0", "#8a4dff", "#ffb340"], lapel: "#ffb340" },
    bass: { jacket: ["#3b1466", "#d06bff", "#ff9a3d"], pants: ["#3b1466", "#ff9a3d", "#d06bff"], lapel: "#ff9a3d" },
    pads: { jacket: ["#1b1424", "#b06bff", "#2fe0ff"], pants: ["#141018", "#ffb340", "#2fe0ff"], lapel: "#2fe0ff" },
    o_velvet: { jacket: ["#a8360f", "#ff7a2e", "#ffb070"], pants: ["#f1dcb8", "#ff7a2e", "#e04a1a"], lapel: "#ff9a3d" },
    o_neonwave: { jacket: ["#d23cff", "#2fe0ff", "#ffd23f"], pants: ["#141018", "#2fe0ff", "#d23cff"], lapel: "#2fe0ff" },
    o_verdeamarela: { jacket: ["#ffd21a", "#14a84a", "#1f4fd1"], pants: ["#f3e6c8", "#1f4fd1", "#14a84a"], lapel: "#14a84a" },
    o_college: { jacket: ["#3a1466", "#ffb340", "#f3e6c8"], pants: ["#15101c", "#ff9a3d", "#3a1466"], lapel: "#ffb340" },
    o_chrome: { jacket: ["#d8d0ff", "#8a7dff", "#ffffff"], pants: ["#15101c", "#b06bff", "#2fe0ff"], lapel: "#b06bff" },
    o_flower: { jacket: ["#f2d6a0", "#b04dff", "#ff8a1f"], pants: ["#2a160c", "#ff8a1f", "#7a3a14"], lapel: "#ff8a1f" },
    o_knit: { jacket: ["#f3e6c8", "#ff8a1f", "#8a3cf0"], pants: ["#2a0f3e", "#8a3cf0", "#4a1a6a"], lapel: "#ff8a1f" },
    o_circuit: { jacket: ["#111018", "#2fe0ff", "#b06bff"], pants: ["#111018", "#2fe0ff", "#b06bff"], lapel: "#2fe0ff" },
    o_starpilot: { jacket: ["#e8641f", "#f3e6c8", "#ffd27a"], pants: ["#2a0f3e", "#ff9a3d", "#ffb340"], lapel: "#f3e6c8" }
  };

  function create(container) {
    let renderer;
    try { renderer = new T.WebGLRenderer({ antialias: true, alpha: true, powerPreference: "low-power" }); } catch (e) { return null; }
    if (!renderer.getContext()) return null;
    renderer.setClearColor(0x000000, 0);
    renderer.outputColorSpace = T.SRGBColorSpace;
    renderer.toneMapping = T.ACESFilmicToneMapping; renderer.toneMappingExposure = 1.05;
    const canvas = renderer.domElement; canvas.className = "hero3d"; container.appendChild(canvas);
    const scene = new T.Scene();
    scene.environment = neonEnvironment(renderer);
    const camera = new T.PerspectiveCamera(26, .8, .1, 40);
    camera.position.set(0, 1.25, 6.4); camera.lookAt(0, 1.02, 0);

    // lights: warm key, violet sky fill, two rim lights coloured by the sound
    scene.add(new T.HemisphereLight(0x8a6dff, 0x2a0f10, .55));
    const key = new T.DirectionalLight(0xffe2c0, 1.6); key.position.set(-2.5, 4, 4); scene.add(key);
    const rimL = new T.DirectionalLight(0x5fe3ff, 2.4); rimL.position.set(-4, 2.5, -3); scene.add(rimL);
    const rimR = new T.DirectionalLight(0xff4fd8, 2.4); rimR.position.set(4, 2, -3); scene.add(rimR);
    const wobLight = new T.DirectionalLight(0x2fe0ff, 0); wobLight.position.set(0, 1.5, -4); scene.add(wobLight);

    // materials
    const skin = new T.MeshStandardMaterial({ color: 0x0c0912, roughness: .16, metalness: .05, envMapIntensity: 1.4 });
    const shirt = new T.MeshStandardMaterial({ color: 0x0b0b10, roughness: .55 });
    const gold = new T.MeshStandardMaterial({ color: 0xffb93a, roughness: .25, metalness: .85 });
    const neonO = new T.MeshStandardMaterial({ color: 0x220800, emissive: 0xff8a2a, emissiveIntensity: 1.1, roughness: .4 });
    const neonP = new T.MeshStandardMaterial({ color: 0x1a0626, emissive: 0xc04dff, emissiveIntensity: 1.1, roughness: .4 });
    const eyeM = new T.MeshStandardMaterial({ color: 0x002a33, emissive: 0x4ff0ff, emissiveIntensity: 1.2 });
    const mouthM = new T.MeshStandardMaterial({ color: 0x2a0018, emissive: 0xff4fd8, emissiveIntensity: .9, side: T.DoubleSide });
    const jacketM = new T.MeshStandardMaterial({ roughness: .62, side: T.DoubleSide });
    const pantsM = new T.MeshStandardMaterial({ roughness: .58 });
    const lapelM = new T.MeshStandardMaterial({ color: 0xffb340, emissive: 0xff8a2a, emissiveIntensity: .25, roughness: .5 });
    const glove = new T.MeshStandardMaterial({ color: 0x0b0910, roughness: .3, metalness: .1, envMapIntensity: 1.2 });
    const soleM = new T.MeshStandardMaterial({ color: 0x331000, emissive: 0xff8a2a, emissiveIntensity: .9 });
    const keytarM = new T.MeshStandardMaterial({ color: 0xffb21f, roughness: .3, metalness: .35 });
    const keysM = new T.MeshStandardMaterial({ map: keysTexture(), roughness: .4 });
    const cyanM = new T.MeshStandardMaterial({ color: 0x002a33, emissive: 0x2fe0ff, emissiveIntensity: 1 });
    const padM = new T.MeshStandardMaterial({ color: 0x22003a, emissive: 0xb06bff, emissiveIntensity: .9 });

    // ---------- skeleton (hierarchy for body/head; limbs are solved in root space) ----------
    const root = new T.Group(); scene.add(root);
    const hips = new T.Group(); hips.position.set(0, .82, 0); root.add(hips);
    const pelvis = new T.Mesh(new T.SphereGeometry(.2, 20, 14), pantsM); pelvis.scale.set(1.05, .7, .85); hips.add(pelvis);
    const chest = new T.Group(); chest.position.set(0, .06, 0); hips.add(chest);
    // torso: black shirt + open jacket (lathe with a gap at the front), lapels, gold chain
    const torso = new T.Mesh(new T.CapsuleGeometry(.17, .2, 6, 18), shirt); torso.position.y = .2; torso.scale.set(1, 1, .82); chest.add(torso);
    const profile = [V(.19, -.02), V(.215, .08), V(.215, .22), V(.2, .33), V(.12, .4)].map(function (p) { return new T.Vector2(p.x, p.y); });
    const jacket = new T.Mesh(new T.LatheGeometry(profile, 28, Math.PI * .5 + .42, Math.PI * 2 - .84), jacketM); jacket.scale.set(1.04, 1, .86); chest.add(jacket);
    [-1, 1].forEach(function (sx) { const l = new T.Mesh(new T.BoxGeometry(.035, .3, .02), lapelM); l.position.set(sx * .07, .2, .17); l.rotation.z = sx * .28; chest.add(l); });
    const chain = new T.Mesh(new T.TorusGeometry(.085, .008, 6, 24, Math.PI), gold); chain.position.set(0, .3, .13); chain.rotation.set(Math.PI * .58, 0, Math.PI); chest.add(chain);
    const neck = new T.Group(); neck.position.set(0, .42, 0); chest.add(neck);
    const head = new T.Group(); head.position.set(0, .3, .02); neck.add(head);
    const skull = new T.Mesh(new T.SphereGeometry(.36, 40, 30), skin); skull.scale.set(1, .96, .92); head.add(skull);
    const flames = [];
    // crest: three glossy black flames curling up and back, each with a neon edge
    [[V(-.05, .28, .08), V(.02, .52, .02), V(.18, .66, -.12), V(.3, .6, -.2), .13],
     [V(.06, .3, -.06), V(.12, .5, -.14), V(.26, .55, -.3), V(.36, .44, -.34), .1],
     [V(-.16, .22, .02), V(-.24, .42, -.02), V(-.12, .58, -.1), V(-.02, .6, -.14), .09]].forEach(function (c, i) {
      // each flame hangs from a pivot at its root so physics can swing it (see "secondary motion")
      const pivot = new T.Group(); pivot.position.copy(c[0]); head.add(pivot); flames.push({ g: pivot, i: i });
      const local = c.slice(0, 4).map(function (p) { return p.clone().sub(c[0]); });
      const curve = new T.CatmullRomCurve3(local);
      pivot.add(new T.Mesh(taperedTube(curve, c[4], .012, 40, 14), skin));
      // neon edge runs along the outer (upper-back) side of the flame, starting above the skull
      const edge = new T.CatmullRomCurve3(local.slice(1, 4).map(function (p) { return p.clone().add(V(0, c[4] * .55, -c[4] * .35)); }));
      pivot.add(new T.Mesh(taperedTube(edge, c[4] * .2, .005, 30, 6), i === 1 ? neonP : neonO));
    });
    // eyes: happy closed crescents; mouth: open smile
    const eyes = [];
    [-1, 1].forEach(function (sx) {
      const e = new T.Mesh(new T.TorusGeometry(.065, .018, 8, 20, Math.PI), eyeM); e.position.set(sx * .13, .05, .31); e.rotation.z = 0; e.lookAt(V(sx * .3, .1, 2)); head.add(e); eyes.push(e);
    });
    const mouth = new T.Mesh(new T.CircleGeometry(.075, 20, Math.PI, Math.PI), mouthM); mouth.position.set(.0, -.11, .335); mouth.rotation.x = -.15; head.add(mouth);
    // headphones
    const band = new T.Mesh(new T.TorusGeometry(.385, .024, 10, 32, Math.PI), gold); band.position.y = .02; band.scale.set(1, 1.05, 1); head.add(band); // ear to ear, over the top
    [-1, 1].forEach(function (sx) {
      const cup = new T.Mesh(new T.CylinderGeometry(.13, .13, .08, 24), gold); cup.rotation.z = Math.PI / 2; cup.position.set(sx * .36, .0, .0); head.add(cup);
      const ring = new T.Mesh(new T.TorusGeometry(.085, .02, 10, 28), neonP); ring.rotation.y = Math.PI / 2; ring.position.set(sx * .405, 0, 0); head.add(ring);
      const dot = new T.Mesh(new T.CircleGeometry(.05, 20), cyanM); dot.rotation.y = sx * Math.PI / 2; dot.position.set(sx * .41, 0, 0); head.add(dot);
    });

    // limbs (one mesh per segment, posed every frame)
    const L = { thigh: .38, shin: .4, upper: .27, fore: .25 };
    const legs = [-1, 1].map(function (sx) {
      const thigh = limb(.09, .1, L.thigh, pantsM), shin = limb(.1, .17, L.shin, pantsM);
      const shoe = new T.Group();
      const upper = new T.Mesh(new T.CapsuleGeometry(.1, .17, 8, 18), glove); upper.rotation.x = Math.PI / 2; upper.position.set(0, .13, .07); upper.scale.set(1.15, 1, .95); shoe.add(upper);
      const toe = new T.Mesh(new T.SphereGeometry(.105, 18, 12), glove); toe.position.set(0, .12, .2); toe.scale.set(1.05, .8, 1); shoe.add(toe);
      const sole = new T.Mesh(new T.BoxGeometry(.24, .08, .4), soleM); sole.position.set(0, .04, .08); shoe.add(sole);
      const band = new T.Mesh(new T.BoxGeometry(.25, .025, .41), lapelM); band.position.set(0, .085, .08); shoe.add(band);
      root.add(thigh); root.add(shin); root.add(shoe);
      return { sx: sx, thigh: thigh, shin: shin, shoe: shoe, foot: V(sx * .17, 0, sx * .02), knee: V() };
    });
    const arms = [-1, 1].map(function (sx) {
      const up = limb(.075, .07, L.upper, jacketM), fo = limb(.07, .085, L.fore, jacketM);
      const hand = new T.Group();
      const palm = new T.Mesh(new T.SphereGeometry(.07, 16, 12), glove); palm.scale.set(1, .8, 1.1); hand.add(palm);
      const thumb = new T.Mesh(new T.CapsuleGeometry(.022, .05, 4, 8), glove); thumb.position.set(sx * .055, .02, .03); thumb.rotation.z = sx * .8; hand.add(thumb);
      const cuff = new T.Mesh(new T.TorusGeometry(.075, .018, 8, 20), lapelM); cuff.rotation.x = Math.PI / 2; fo.add(cuff); cuff.position.y = -L.fore + .02;
      root.add(up); root.add(fo); root.add(hand);
      return { sx: sx, up: up, fo: fo, hand: hand, elbow: V() };
    });

    // keytar strapped to the chest, neck up to the right
    const keytar = new T.Group();
    const shape = new T.Shape();
    shape.moveTo(-.08, -.12); shape.lineTo(.62, -.1); shape.lineTo(.78, -.05); shape.lineTo(1.02, .03); shape.lineTo(1.04, .09); shape.lineTo(.8, .07); shape.lineTo(.62, .1); shape.lineTo(-.02, .12);
    shape.absarc(-.08, 0, .14, Math.PI / 2, Math.PI * 1.5, false);
    const body = new T.Mesh(new T.ExtrudeGeometry(shape, { depth: .06, bevelEnabled: true, bevelThickness: .015, bevelSize: .015, bevelSegments: 3 }), keytarM); body.position.z = -.03; keytar.add(body);
    const keys = new T.Mesh(new T.PlaneGeometry(.5, .11), keysM); keys.position.set(.33, .0, .05); keytar.add(keys);
    const spk = new T.Mesh(new T.TorusGeometry(.08, .016, 10, 28), cyanM); spk.position.set(-.08, 0, .05); keytar.add(spk);
    const spkIn = new T.Mesh(new T.CircleGeometry(.055, 24), padM); spkIn.position.set(-.08, 0, .052); keytar.add(spkIn);
    const lights = [];
    for (let i = 0; i < 4; i++) { const b = new T.Mesh(new T.CircleGeometry(.018, 12), i % 2 ? padM : cyanM); b.position.set(.68 + i * .06, .02 + i * .012, .045); keytar.add(b); lights.push(b); }
    keytar.position.set(-.16, .02, .27); keytar.rotation.set(-.12, .12, .42); chest.add(keytar);
    const keyLine = [V(.12, .0, .07), V(.56, .0, .07)], neckPoint = V(.86, .085, .07);

    // ---------- physics: two coat-tail ribbons as verlet chains (gravity, inertia, fixed segment length) ----------
    const SEG = 6, SEGLEN = .075;
    const ribbons = [-1, 1].map(function (sx) {
      const anchor = V(sx * .2, -.02, -.02); // on the jacket hem, in chest space
      const pts = [], prev = [], meshes = [];
      for (let i = 0; i <= SEG; i++) { pts.push(V(sx * .2, .8 - i * SEGLEN, 0)); prev.push(pts[i].clone()); }
      for (let i = 0; i < SEG; i++) { const m = limb(.03 - i * .003, .026 - i * .003, SEGLEN, lapelM); root.add(m); meshes.push(m); }
      return { sx: sx, anchor: anchor, pts: pts, prev: prev, meshes: meshes };
    });
    function stepRibbons(dt, hipVel) {
      if (dt <= 0) return;
      const h = Math.min(dt, 1 / 30), g = -9.8 * .55, damp = .965;
      chest.updateMatrixWorld(true);
      ribbons.forEach(function (rb) {
        const a = rb.anchor.clone(); chest.localToWorld(a); root.worldToLocal(a);
        rb.pts[0].copy(a); rb.prev[0].copy(a);
        for (let i = 1; i <= SEG; i++) {
          const p = rb.pts[i], q = rb.prev[i], vx = (p.x - q.x) * damp, vy = (p.y - q.y) * damp, vz = (p.z - q.z) * damp;
          q.copy(p); p.x += vx; p.y += vy + g * h * h; p.z += vz - .002; // slight pull backwards so they hang behind the legs
        }
        for (let it = 0; it < 4; it++)
          for (let i = 1; i <= SEG; i++) {
            const a0 = rb.pts[i - 1], b0 = rb.pts[i], d = b0.clone().sub(a0), len = d.length() || 1e-6;
            b0.copy(a0).addScaledVector(d, SEGLEN / len);
            if (b0.y < .02) b0.y = .02; // the floor
          }
        for (let i = 0; i < SEG; i++) aim(rb.meshes[i], rb.pts[i], rb.pts[i + 1]);
      });
    }
    // ---------- the groove loop (Gab's 6-pose reference sheet), one pose per beat, blended ----------
    // hx hip side, dp knee dip, yw body turn, lb lean (+ forward), ht head tilt, kt keytar tilt,
    // lf/rf [side, forward, lift] for each foot, up = free arm raised
    const GROOVE = [
      { hx: 0, dp: .03, yw: .12, lb: -.12, ht: -.08, kt: .16, lf: [0, 0, 0], rf: [.02, .04, .07], up: 0 },   // 1 lean back, toe tap
      { hx: -.08, dp: .07, yw: -.2, lb: .02, ht: .06, kt: 0, lf: [-.1, 0, 0], rf: [0, 0, 0], up: 0 },        // 2 slide left
      { hx: 0, dp: .04, yw: .55, lb: 0, ht: .1, kt: .06, lf: [0, -.04, .03], rf: [0, .06, 0], up: 0 },        // 3 turn
      { hx: .05, dp: .12, yw: .1, lb: .18, ht: .14, kt: -.06, lf: [0, 0, 0], rf: [.02, .1, 0], up: 0 },       // 4 lean in, knees bent
      { hx: .09, dp: .06, yw: -.35, lb: .04, ht: -.04, kt: .04, lf: [0, 0, 0], rf: [.1, 0, .04], up: 0 },     // 5 slide around
      { hx: 0, dp: .02, yw: .05, lb: -.18, ht: -.2, kt: .25, lf: [0, 0, 0], rf: [0, 0, 0], up: 1 }            // 6 accent pose
    ];
    const ease3 = function (x) { return x * x * (3 - 2 * x); };
    function groovePose(b) {
      const n = Math.floor(b), f = ease3(b - n), A = GROOVE[((n % 6) + 6) % 6], B = GROOVE[(((n + 1) % 6) + 6) % 6], o = {};
      for (const k in A) o[k] = Array.isArray(A[k]) ? A[k].map(function (v, i) { return v + (B[k][i] - v) * f; }) : A[k] + (B[k] - A[k]) * f;
      return o;
    }

    // soft contact shadow on the platform
    const sh = document.createElement("canvas"); sh.width = sh.height = 64; const sg = sh.getContext("2d"), grd = sg.createRadialGradient(32, 32, 2, 32, 32, 32);
    grd.addColorStop(0, "rgba(0,0,0,.65)"); grd.addColorStop(1, "rgba(0,0,0,0)"); sg.fillStyle = grd; sg.fillRect(0, 0, 64, 64);
    const shadow = new T.Mesh(new T.PlaneGeometry(1.1, .5), new T.MeshBasicMaterial({ map: new T.CanvasTexture(sh), transparent: true, depthWrite: false }));
    shadow.rotation.x = -Math.PI / 2; shadow.position.y = .003; root.add(shadow);

    let outfitKey = "";
    function setOutfit(k) {
      const o = OUTFITS[k] || OUTFITS.disco; if (k === outfitKey) return; outfitKey = k;
      if (jacketM.map) jacketM.map.dispose(); if (pantsM.map) pantsM.map.dispose();
      jacketM.map = swirlTexture(o.jacket[0], o.jacket[1], o.jacket[2]); jacketM.map.repeat.set(2, 1); jacketM.needsUpdate = true;
      pantsM.map = swirlTexture(o.pants[0], o.pants[1], o.pants[2]); pantsM.map.repeat.set(1, 1.5); pantsM.needsUpdate = true;
      lapelM.color.set(o.lapel); lapelM.emissive.set(o.lapel);
    }
    setOutfit("disco");

    const st = {}, R = { canvas: canvas, ok: true, t: 0, blinkAt: 2, blink: 0, accent: 0, press: 0, hitKick: 0, lastBeat: -1 };
    let W = 0, H = 0, pr = Math.min(2, window.devicePixelRatio || 1), crushBucket = -1;
    R.setSize = function (w, h) {
      w = Math.max(16, Math.round(w)); h = Math.max(16, Math.round(h)); if (w === W && h === H) return; W = w; H = h;
      renderer.setPixelRatio(pr); renderer.setSize(w, h, false); canvas.style.width = w + "px"; canvas.style.height = h + "px";
      camera.aspect = w / h; camera.updateProjectionMatrix();
    };
    // where the feet (world origin) land in the canvas, in CSS pixels
    R.feetPixel = function () { const p = V(0, 0, 0).project(camera); return [(p.x + 1) / 2 * W, (1 - p.y) / 2 * H]; };
    // canvas height that makes the hero `px` pixels tall from the floor to the top of the crest
    R.heightFor = function (px) { const vis = 2 * camera.position.distanceTo(V(0, 1.02, 0)) * Math.tan(camera.fov * Math.PI / 360); return px * vis / 2.15; };
    R.setOutfit = setOutfit;

    const tmp = V(), tmp2 = V(), pole = V(), reach = V();
    R.update = function (s, dt) {
      R.t += dt;
      const amp = s.amp === undefined ? 1 : s.amp, alive = clamp((s.alive || 0) * amp, 0, 1), I = clamp((s.intensity === undefined ? .8 : s.intensity), 0, 1.3) * alive;
      const beats = s.beats || 0, ph = beats - Math.floor(beats), style = s.style || "groove";
      const speed = style === "half" ? .5 : 1, b2 = beats * speed;
      // note accents
      if (s.hit) { const v = (s.vel || .7) * Math.max(.3, amp); R.hitKick = 1; R.press = 1; if (v > .78 || style === "disco" && Math.random() < .3) R.accent = 1; spring(st, "nod", 0, 1, 0, 0); st.nod.v += 3.2 * v; st.dip.v += 1.6 * v; }
      R.hitKick = Math.max(0, R.hitKick - dt * 3); R.press = Math.max(0, R.press - dt * 7); R.accent = Math.max(0, R.accent - dt * 1.4);
      // ---- body motion: the groove loop poses blended beat by beat, plus a bounce on every beat ----
      const G = groovePose(b2), bounce = Math.pow(.5 + .5 * Math.cos(2 * Math.PI * b2), 2), sway = Math.sin(b2 * Math.PI);
      const styleK = style === "disco" ? 1.15 : style === "funk" ? 1.1 : style === "half" ? .8 : 1;
      // idle life: breathing, slow weight shifts and looking around when there is no sound
      const idle = 1 - clamp(alive * 2, 0, 1), breathe = Math.sin(R.t * 1.6) * (.006 + .008 * idle);
      const idleShift = Math.sin(R.t * .35) * .03 * idle, idleLook = Math.sin(R.t * .23) * .35 * idle + Math.sin(R.t * .71) * .1 * idle;
      const hipX = spring(st, "hx", G.hx * I * styleK + idleShift, 9, dt);
      const dip = spring(st, "dip", (G.dp + .04 * bounce) * I * styleK + .02 * R.hitKick, 14, dt);
      const yaw = spring(st, "yaw", G.yw * I + (s.look || 0) * .28 + idleLook * .4, 6, dt);
      const lean = spring(st, "lean", G.lb * I, 8, dt);
      hips.position.set(hipX, .82 - dip + breathe * .5, 0);
      hips.rotation.set(0, yaw, -sway * .05 * I);
      chest.rotation.set(.05 + lean + dip * .6, -yaw * .4, sway * .08 * I);
      chest.scale.set(1 + breathe * .6, 1 + breathe, 1 + breathe * .6);
      const nod = spring(st, "nod", .12 * I * Math.cos(2 * Math.PI * b2 - .7) + G.ht * I, 10, dt);
      neck.rotation.set(nod * .5, (s.look || 0) * .25 + idleLook * .5, -sway * .07 * I);
      head.rotation.set(nod * .6 - .04 - lean * .4, (s.look || 0) * .2 + yaw * .15 + idleLook * .5, Math.sin(b2 * Math.PI + .6) * .1 * I);
      keytar.rotation.set(-.12, .12, .42 + spring(st, "kt", G.kt * I, 9, dt));
      // ---- feet from the pose (side, forward, lift); IK bends the knees, the hips stay over the feet ----
      hips.updateMatrixWorld(true);
      legs.forEach(function (lg) {
        const F = lg.sx < 0 ? G.lf : G.rf;
        const fx = spring(st, "fx" + lg.sx, lg.sx * .17 + F[0] * I, 12, dt), fz = spring(st, "fz" + lg.sx, lg.sx * .02 + F[1] * I, 12, dt), fy = spring(st, "fy" + lg.sx, F[2] * I, 16, dt);
        lg.foot.set(fx, Math.max(0, fy), fz);
        const hipJ = tmp.set(lg.sx * .1, -.05, 0); hips.localToWorld(hipJ); root.worldToLocal(hipJ);
        const ankle = tmp2.copy(lg.foot).add(V(0, .1, 0));
        pole.copy(hipJ).add(V(lg.sx * .12, -.3, 1));
        const end = ik(hipJ, L.thigh, L.shin, ankle, pole, lg.knee);
        aim(lg.thigh, hipJ, lg.knee); aim(lg.shin, lg.knee, end);
        lg.shoe.position.set(end.x, end.y - .1, end.z); lg.shoe.rotation.set(fy * 2.2, lg.sx * .12 + yaw * .5, 0);
      });
      R.poseUp = G.up * I;
      // ---- arms: one hand plays the keys (follows pitch, presses on notes), the other holds the neck
      //      and throws up a point on accents. Always exactly two hands. ----
      keytar.updateMatrixWorld(true); chest.updateMatrixWorld(true);
      const pitch = clamp(s.pitch === undefined ? .5 : s.pitch, 0, 1);
      arms.forEach(function (a) {
        const sh0 = tmp.set(a.sx * .23, .33, .0); chest.localToWorld(sh0); root.worldToLocal(sh0);
        let target;
        if (a.sx < 0) {
          const kx = spring(st, "kx", keyLine[0].x + (keyLine[1].x - keyLine[0].x) * pitch, 10, dt);
          target = V(kx, .0, .1 - .025 * R.press * (s.playing ? 1 : 0)); keytar.localToWorld(target); root.worldToLocal(target);
          target.y += .02 * Math.sin(R.t * 9) * (s.playing ? 1 : 0) * alive;
        } else {
          const n = neckPoint.clone(); keytar.localToWorld(n); root.worldToLocal(n);
          const up = V(.52, 1.92, .25), k = spring(st, "acc", Math.max(R.accent > .2 && alive > .2 ? 1 : 0, R.poseUp || 0), 7, dt);
          target = n.lerp(up, clamp(k, 0, 1));
        }
        pole.copy(sh0).add(V(a.sx * .6, -.6, -.5));
        const end = ik(sh0, L.upper, L.fore, target, pole, a.elbow);
        aim(a.up, sh0, a.elbow); aim(a.fo, a.elbow, end);
        a.hand.position.copy(end); a.hand.quaternion.copy(a.fo.quaternion);
      });
      // ---- secondary motion (physics): crest flames lag behind head movement, ribbons swing with gravity ----
      head.updateMatrixWorld(true);
      const hp = V(); head.getWorldPosition(hp);
      if (!R.hp) { R.hp = hp.clone(); R.hv = V(); }
      const hv = dt > 0 ? hp.clone().sub(R.hp).divideScalar(dt) : V(), acc = dt > 0 ? hv.clone().sub(R.hv).divideScalar(dt) : V();
      R.hp.copy(hp); R.hv.copy(hv);
      flames.forEach(function (fl, i) {
        const wF = 9 + i * 2;
        const rx = spring(st, "flx" + i, clamp(acc.y * .012 + hv.z * .05, -.35, .35), wF, dt), rz = spring(st, "flz" + i, clamp(-acc.x * .012 - head.rotation.z * .6, -.4, .4), wF, dt);
        fl.g.rotation.set(rx + Math.sin(R.t * 2.1 + i) * .02 * (1 - idle * .5), 0, rz);
      });
      stepRibbons(dt, hv);
      // ---- face: blink, eyes flash on notes, smile opens on accents ----
      if (R.t > R.blinkAt) { R.blink = 1; R.blinkAt = R.t + 2.5 + Math.random() * 3; }
      R.blink = Math.max(0, R.blink - dt * 8);
      const eyeS = 1 - Math.sin(Math.min(1, R.blink) * Math.PI) * .85;
      eyes.forEach(function (e) { e.scale.set(1, eyeS, 1); });
      eyeM.emissiveIntensity = 1.1 + R.hitKick * 1.2 * Math.max(.3, amp);
      mouth.scale.set(1, .7 + R.accent * .7 + R.hitKick * .25, 1);
      // ---- light: rim lights take the sound's colour; neon edges pulse with the envelope ----
      const col = new T.Color(s.glow || "#5fe3ff"), ga = clamp(s.glowAmt || 0, 0, 1);
      rimL.color.copy(col); rimR.color.copy(col).lerp(new T.Color("#ff4fd8"), .35);
      rimL.intensity = 1.8 + ga * 3 + (s.bloom || 0) * 2; rimR.intensity = 1.6 + ga * 2.5;
      neonO.emissiveIntensity = neonP.emissiveIntensity = .9 + ga * 1.1; soleM.emissiveIntensity = .7 + ga + R.hitKick * .6;
      cyanM.emissiveIntensity = .9 + R.press * 1.2;
      lights.forEach(function (b, i) { b.visible = !s.playing || ((Math.floor(R.t * 8) + i) % 3 !== 0); });
      const wob = clamp(s.wob || 0, 0, 1); wobLight.intensity = wob * 4; wobLight.position.x = Math.sin(R.t * 3) * 4;
      shadow.scale.set(1 + hipX * 1.5, 1, 1); shadow.position.x = hipX * .6;
      // CRUSH: render at a lower resolution (real pixels), only while the effect is audible
      const cb = s.crush > .25 ? Math.min(4, 1 + Math.floor(s.crush * 4)) : 0;
      if (cb !== crushBucket) { crushBucket = cb; renderer.setPixelRatio(cb ? pr / (cb * 2) : pr); renderer.setSize(W, H, false); canvas.style.imageRendering = cb ? "pixelated" : ""; }
    };
    R.render = function () { renderer.render(scene, camera); };
    R.dispose = function () { renderer.dispose(); canvas.remove(); };
    R.update({ alive: 0 }, 0);
    return R;
  }
  window.RastroHero3D = { create: create, OUTFITS: OUTFITS };
})();
