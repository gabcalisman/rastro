# RASTRO PLAY visual editor port

This branch is based on the Codex audio-engine line (`prepare-mac-testing`). It ports the PR #2 web editor, its screen flow, visual assets, factory sample browser, patch views, wardrobe, themes, and game progression into the existing RASTRO processor.

## Audio compatibility boundary

The processor, sampler, synth, capture path, patch search, and DSP rack remain from the Codex base. PR #2's `rackParallel`, Bloom diffusion, and Wobble width controls are intentionally not included: those change sound behavior and are outside this visual-only port. The rack remains serial, and its existing six modules and parameter IDs are retained. UI elements bind only to parameters that exist in the Codex engine.

The browser editor is enabled by default for macOS builds. Linux and Windows keep the native JUCE editor unless `RASTRO_WEB_UI=ON` is explicitly requested. No audio processing is moved to the browser or UI thread.

## NOVA effect lights

NOVA has six small, dark-outlined cartoon bulbs, one per existing rack module:

| Module | Light color |
| --- | --- |
| GHOST · Delay | Violet |
| CRUMBLE · Stutter | Amber |
| BLOOM · Reverb | Pink |
| WOBBLE · Chorus | Cyan |
| DUST · Noise | Gold |
| CRUSH · Bitcrusher | Electric violet |

Brightness and size follow the processor's per-module measured wet-versus-dry activity. Bypassed modules stay dim. The lights are presentation-only and do not feed back into audio. NOVA artwork and the six light elements are drawn in the UI layer; no images are animated by allocating or processing in the audio callback.

## State

The editor appearance JSON is stored in plugin state. Character and room selection use append-only visual parameters, preserving older sessions and leaving DSP settings unchanged. UI MIDI-note telemetry uses fixed atomic bitsets and only reports state to the browser editor.
