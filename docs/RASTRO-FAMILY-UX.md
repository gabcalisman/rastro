# RASTRO family: visual and interaction direction

## Shared identity

- Keep the same illustrated RASTRO wordmark across products. Put the product name beside
  it as a clear secondary lockup: `PLAY`, `FX`, or `CAPTURE`. Keep `by Gab Calisman`
  visible in the header at a smaller size.
- Use one visual universe: deep plum/ink surfaces, warm cream panels, neon magenta,
  cyan, orange, and violet accents, expressive outlines, small comic-style marks, and
  the approved NOVA/alien artwork. Avoid generic emoji as a substitute for the character.
- Reuse the same knob language, value readouts, toggles, typography hierarchy, module
  colors, preset browser, focus states, and scaling rules. A control should feel familiar
  in every RASTRO product.
- Let each product express its job through layout and motion. Shared identity does not
  mean copying one plugin's screen into another.

## Product jobs

| Product | First thing the user sees | Main interaction | Supporting areas |
| --- | --- | --- | --- |
| RASTRO PLAY | The instrument and its character | Play notes; shape the sampled sound | Sample, envelope, filter, rack, patches, Camarim |
| RASTRO FX | The signal path and active rack | Add, reorder, blend, and perform effects | XY modulation, LFO, envelope follower, presets, A/B |
| RASTRO Capture | Input level and record state | Arm, record, review, and save a take | Input selection, monitoring, waveform, recordings |

## RASTRO FX interaction direction

- Make the effect chain the visual center. The module bank should show which effects are
  waiting and which are in the chain. Adding one should feel like dropping a character
  into a live signal path; dragging changes order, and the chain makes series/parallel
  routing obvious.
- Keep each effect's personality in its own color and small piece of artwork. Its unique
  controls stay on the card, with advanced values available without leaving the rack.
- Treat MOD as a performance surface. The XY pad is the hero control, with X/Y targets
  and amounts legible at a glance. The marker should have a trail and respond to movement;
  assigned destinations should share their effect's color.
- Give LFO and envelope follower distinct animated displays and grouped controls. Show
  their current output, target, and amount together. Keep attack, release, sensitivity,
  waveform, rate, and destination reachable in the same MOD area.
- Keep global input/output, dry/wet, bypass, meters, routing, preset name, save/open, and
  A/B visible in a stable header. Presets recall sound and appearance together.
- Provide clear hover/focus/active/bypassed states, readable values, host automation,
  keyboard focus, and comfortable resizing. Movement should communicate audio state and
  remain subtle enough for long mixing sessions.

## Reference use

The approved RASTRO PLAY concepts establish the family cues: the first reference supplies
the dark neon/cosmic atmosphere and NOVA placement; the second supplies the comic outlines,
cream framing, and expressive module illustrations. FX should combine those cues into one
coherent screen while retaining its rack-first workflow and the PLAY/Capture products'
own purposes.

## Quality gate for future builds

- Check title lockup, `by Gab Calisman`, artwork, and text glyphs in the actual Logic UI.
- Render at the minimum, default, and large editor sizes on a Retina display; verify text,
  paths, knobs, card bounds, and scroll behavior at each scale.
- Test the visible modulation routes and the module rack with real parameter changes, not
  only static screenshots.
- Compare a build against the approved references before producing a new download ZIP.
