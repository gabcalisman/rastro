# RASTRO by Gab Calisman

A native MIDI sample instrument and reorderable texture rack. First working prototype, C++17 / JUCE 8 / CMake. Portuguese (Brazil) is the default; English is selectable and saved with the session.

## Play

1. Load a WAV or AIFF using **CARREGAR SOM / LOAD SOUND**.
2. Play MIDI notes or the onscreen keyboard. MIDI note 60 plays at the original pitch with the default root of 60. Root and trims apply to newly triggered voices.
3. Enable **Repetir / Loop** for looping while a note is held. With it off, playback stops at the trim end. Note-off releases in either mode (30 ms). Velocity controls amplitude and the adjustable low-pass response.
4. Adjust the three effects. Drag a card's heading onto another card to swap their order. **Active / Bypassed** compares a module; **×** removes it; **+ Module** restores it. One instance of each type is available.
5. **Dry / rack** preserves a direct comparison with the sampler signal. At 0%, the rack is dry; output gain and the protective soft saturation still apply.
6. Learn a MIDI CC for PRESENCE, TRAIL or MUTATION by clicking **Learn CC**, then sending a controller message. Click again to cancel. Mappings persist; MIDI is received on all channels.
7. Save the host session/preset to store the original sample bytes, parameters, rack order/presence/bypass, language and CC mappings. The original file is no longer required.

### Effects and macros

- **GHOST:** interpolated, filtered feedback delay. Division selects 1/32, 1/16, 1/8, 1/4 or 1/2 note. Feedback, tone and mix work independently.
- **CRUMBLE:** a simple windowed fragment repeater, not a full granular synthesizer. Rate spans 1/8–1/128 note; Fragment changes captured length; mix blends the repeated sound. MUTATION offsets fragment selection.
- **BLOOM:** a simple four-line diffuse stereo tail, not a convolution or premium algorithmic reverb. Size changes delay lengths, tone filters the return, decay controls bounded feedback, and mix blends the tail.
- **PRESENCE:** gentle output gain scaling. **TRAIL:** scales GHOST feedback and BLOOM decay. **MUTATION:** controls fragment variation and MUTATE intensity.
- **MUTATE:** bounded changes to selected time, feedback, tone, rate, fragment, size and decay settings; sample, trim, root and output level are preserved. **Reset effects** restores effect and macro defaults; sample and rack order remain intact.
- Host BPM is used when available; otherwise Manual tempo applies. Tempo sync is interval-based; it does not align fragments to the DAW bar/transport phase.

## Build

CMake 3.22+, a C++17 compiler, Git, and JUCE 8 are required. CMake uses an installed JUCE 8 package or fetches the pinned JUCE **8.0.6** tag. Initial fetching requires network access. JUCE and bundled SDK licensing apply; choose the appropriate JUCE license before distributing a product.

### Linux

On a normal Debian/Ubuntu development machine:

```sh
sudo apt-get update
sudo apt-get install cmake ninja-build g++ git libasound2-dev libx11-dev libxext-dev \
  libxinerama-dev libxrandr-dev libxcursor-dev libfreetype-dev libfontconfig-dev \
  libgl1-mesa-dev libpng-dev libjpeg-dev
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build -j2
ctest --test-dir build --output-on-failure
```

Built formats: **VST3 instrument** and **Standalone**. Outputs are in `build/Rastro_artefacts/Release/`. Copy the complete `VST3/RASTRO.vst3` bundle into `~/.vst3/` and rescan your Linux DAW. Standalone requires a working ALSA audio device; enable the desired MIDI input through its audio/MIDI settings.

### Download a Mac build from GitHub Actions (no compiler needed)

The **macOS build** workflow in `.github/workflows/macos-build.yml` runs for PRs into `main`, including draft PRs. It builds AU, VST3 and Standalone using Xcode on `macos-15-intel`, explicitly targeting **both Apple silicon (`arm64`) and Intel (`x86_64`)**. Each bundle's two architectures are checked with `lipo`. The deployment target is macOS 12.0; your version of Logic Pro may require a newer macOS. The runner executes the portable DSP tests natively on Intel; packaging arm64 code does not prove arm64 execution works.

**A workflow definition is not proof of a successful build.** Check the actual run for the commit you want. These are local-test bundles: ad-hoc signed, not Developer ID signed or notarized. Automated checks do not include `auval` or Logic Pro playback.

1. Sign into GitHub with access to this private repository. Open [draft PR #1](https://github.com/gabcalisman/rastro/pull/1), choose **Checks**, and open its **macOS build** run. Alternatively open the repository's [Actions page](https://github.com/gabcalisman/rastro/actions/workflows/macos-build.yml) and choose the run for `prepare-mac-testing`. Keep the PR in draft; merging is not required to get an artifact.
2. Wait for the build job to finish with a **green success** status. Open the run's summary and scroll to **Artifacts**. Download **RASTRO-macOS-universal**. If the run failed, inspect the first failed step; do not assume a ZIP was produced. If Actions is disabled or GitHub requires approval, a repository owner must enable/approve the workflow in GitHub. No developer secrets or signing certificate are required by this workflow.
3. In Finder, open the downloaded artifact ZIP. Inside is **`RASTRO-macOS-universal.zip`** plus a SHA-256 file. Open that **inner ZIP** too. This extra ZIP preserves the app/plugin permissions and symlinks that artifact downloads otherwise lose. Keep the complete `.component`, `.app` and `.vst3` bundles intact.
4. Open the extracted `RASTRO-macOS-universal` folder. Check `BUILD-INFO.txt` for the source commit, actual bundle architectures, toolchain and testing limits. Artifacts expire after **14 days**. Re-run the workflow from its run page if GitHub offers that control and you have access; manual **Run workflow** is available once this workflow exists on the default branch.
5. Quit Logic Pro. In Finder choose **Go → Go to Folder**, enter `~/Library/Audio/Plug-Ins/Components/`, and copy **`AU/RASTRO.component`** there. If the folder is absent, create it using the Terminal command below. Move any older RASTRO bundle to a backup outside the plugin folders first. Logic uses the AU `.component`; it cannot load the VST3 bundle.
6. Optionally copy **`Standalone/RASTRO.app`** into your user `~/Applications` folder. The VST3 bundle is optional for another compatible Mac host; place it in `~/Library/Audio/Plug-Ins/VST3/`. You do not need Xcode/CMake just to download and install these bundles.

For exact copy commands, open Terminal, type `cd ` (including the space), drag the **extracted inner `RASTRO-macOS-universal` folder** into Terminal, press Return, then run:

```sh
mkdir -p "$HOME/Library/Audio/Plug-Ins/Components" "$HOME/Applications"
ditto "AU/RASTRO.component" "$HOME/Library/Audio/Plug-Ins/Components/RASTRO.component"
ditto "Standalone/RASTRO.app" "$HOME/Applications/RASTRO.app"
# Optional, for a VST3 host (not Logic):
mkdir -p "$HOME/Library/Audio/Plug-Ins/VST3"
ditto "VST3/RASTRO.vst3" "$HOME/Library/Audio/Plug-Ins/VST3/RASTRO.vst3"
```

The AU is now expected at **`~/Library/Audio/Plug-Ins/Components/RASTRO.component`** and the app at **`~/Applications/RASTRO.app`**. Downloaded unsigned/notarized test builds may be blocked by macOS security checks even with a valid ad-hoc signature. Follow macOS's explicit approval flow only for a build/commit you trust (for the app, look in System Settings → Privacy & Security if it was blocked). Do not disable Gatekeeper globally. If the AU cannot be approved/validated, use the local source-build steps below or share the exact error for diagnosis.

Validate the installed AU in Terminal:

```sh
auval -v aumu Rstr GbCa
```

Look for **`AU VALIDATION SUCCEEDED`**, then open Logic Pro → Settings → Plug-in Manager and rescan RASTRO if necessary. Add it to a **Software Instrument** track through **AU Instruments → Gab Calisman → RASTRO → Stereo**, load a WAV/AIFF, and start at low listening volume. Follow the detailed Logic test checklist below for MIDI, note-off, effects, tempo, automation, project save/reopen and offline bounce. Those tests, Apple-silicon execution, physical audio/MIDI, and downloaded-bundle security behavior still require testing on your own Mac. **No Logic Pro playback test is claimed by the workflow.**

### macOS / Logic Pro: first build and test

**These are instructions to run on your Mac, not a report of a successful Mac test.** This chat workspace is Linux. The separate GitHub Actions macOS workflow can build Mac bundles; check its actual run status for build evidence. Neither that workflow nor this workspace runs Logic Pro. Linux `.so` files and the Linux Standalone executable cannot be installed on a Mac.

#### 1. Review and publish the source from Codex first

The prototype was created in the Codex workspace on branch `work`. A GitHub connection does **not** automatically commit or push edits. At the repository inspection for this guide on October 21, 2026, the workspace still had uncommitted changes, local HEAD was `9546a84` (`Initial commit`), GitHub `main`'s README contained only `# rastro`, and GitHub `main` had no `CMakeLists.txt`. This is a dated observation, not a live repository status indicator.

1. In this Codex task, open the **changes/diff view** (the label varies by Codex client). Review the README change **and the new files**: `CMakeLists.txt`, `Source/`, `tests/`, `.gitignore` and `.clang-format`. Newly created files must be included, not just the tracked README diff. There are no new product features in this Mac preparation update.
2. If your client offers **Create PR**, use it to publish a new branch to `gabcalisman/rastro`, with `main` as the base. Open the resulting GitHub PR and check its **Files changed** tab includes the source and tests. Creating a PR publishes a branch; it does not update `main` until you merge it.
3. If there is no publishing control, send Codex this explicit instruction after reviewing: **“Commit all RASTRO prototype source, tests, configuration and the updated README on a new branch, push it to gabcalisman/rastro, and open a draft pull request against main. Do not merge it.”** Codex will need working repository write access. Inspect the resulting PR before marking it ready and merging.
4. After merging, check GitHub `main` contains `CMakeLists.txt` and `Source/Processor.cpp`. Only then follow the `main` clone instructions below. To test before merging, check out the PR's actual branch name on your Mac instead.

If you prefer a terminal in this **existing Codex workspace**, the equivalent Git operations are:

```sh
cd /workspace/rastro
git status --short --branch
git diff -- README.md
# Read the new files listed by status in Codex's file/diff view as well.
git switch -c prepare-mac-testing
git add README.md CMakeLists.txt Source tests .gitignore .clang-format
git diff --cached --stat
git diff --cached --check
git diff --cached
# Continue only after reviewing the staged contents:
git commit -m "Add RASTRO prototype and Mac testing guide"
git push -u origin prepare-mac-testing
```

Use an unused branch name if that name already exists. Git must already have your author identity and repository authentication configured; do not paste tokens into commands. After a successful push, open <https://github.com/gabcalisman/rastro/compare/main...prepare-mac-testing> to create the PR. Review it and merge when ready. The steps above are instructions; this documentation update does not itself commit, push or merge anything.

#### 2. Install the Mac prerequisites

You need a Mac with a macOS version supported by your installed Xcode and Logic Pro, enough disk space for Xcode/build files, and an internet connection for the first JUCE download.

1. Install **full Xcode** from the Mac App Store. Open it once, accept the license, and let it finish installing its components. The commands below use the **Xcode generator**, so Command Line Tools alone are insufficient.
2. Open **Terminal** (Applications → Utilities → Terminal). Select the full Xcode installation and finish its setup:

   ```sh
   sudo xcode-select --switch /Applications/Xcode.app/Contents/Developer
   sudo xcodebuild -runFirstLaunch
   xcodebuild -version
   git --version
   ```

   `sudo` asks for your Mac login password; Terminal does not display the characters as you type. If Xcode is installed elsewhere, substitute its actual path. `xcodebuild -version` should print an Xcode version, not a “requires Xcode” error.
3. Install **CMake 3.22 or newer**. If you already use [Homebrew](https://brew.sh), run:

   ```sh
   brew install cmake
   cmake --version
   ```

   If you do not use Homebrew, get the macOS CMake application from <https://cmake.org/download/> and place it in `/Applications`. In each Terminal session, make its commands available with:

   ```sh
   export PATH="/Applications/CMake.app/Contents/bin:$PATH"
   cmake --version
   ```

4. Install/open Logic Pro for the later host test. You do **not** need a separate JUCE download, VST SDK installation, or paid font. CMake fetches JUCE **8.0.6** if that version is not installed. JUCE licensing still applies to distribution.

#### 3. Get the published project onto your Mac

After the PR has been merged into `main`, run in your Mac Terminal:

```sh
mkdir -p "$HOME/Developer"
cd "$HOME/Developer"
git clone https://github.com/gabcalisman/rastro.git
cd rastro
git switch main
git pull --ff-only
test -f CMakeLists.txt && test -f Source/Processor.cpp && echo "Prototype source is present"
```

This repository is private: authenticate with your GitHub account using an existing credential manager/GitHub CLI setup or use GitHub Desktop to clone it, then open that clone in Terminal. A normal GitHub account password is not a Git HTTPS password. Do not share authentication tokens in this chat.

If you have already cloned it, open that folder and run `git status` before pulling; preserve any local edits. If testing an unmerged PR, replace `git switch main` with `git switch THE_ACTUAL_PR_BRANCH` after `git fetch origin`. Run the remaining commands from the folder containing `CMakeLists.txt`. If the final source check prints nothing, you have the old README-only revision: finish publishing or select the correct branch before building.

#### 4. Configure and build the Mac plugins

For the simplest first test, build for the architecture of this Terminal session:

```sh
uname -m
cmake -S . -B build-mac -G Xcode -DCMAKE_OSX_ARCHITECTURES="$(uname -m)"
cmake --build build-mac --config Release --target Rastro_AU Rastro_VST3 Rastro_Standalone --parallel 2
```

`uname -m` normally prints `arm64` on Apple silicon or `x86_64` on an Intel Mac. On Apple silicon, use a native Terminal (disable its “Open using Rosetta” setting) for a native arm64 build and run Logic natively too. Allow the initial JUCE download and compilation to finish. A failed command is not a completed build; keep the first compiler error for diagnosis.

Expected outputs, relative to the project folder:

| Format | Build output | Where to test it |
| --- | --- | --- |
| AU instrument | `build-mac/Rastro_artefacts/Release/AU/RASTRO.component` | Logic Pro |
| VST3 instrument | `build-mac/Rastro_artefacts/Release/VST3/RASTRO.vst3` | A Mac DAW/host that supports VST3 |
| Standalone | `build-mac/Rastro_artefacts/Release/Standalone/RASTRO.app` | Open as a Mac application |

**Logic Pro loads AU, not VST3.** A `.component`, `.vst3` or `.app` is a bundle; copy the entire bundle, not a file from inside it. CMake has automatic plugin copying disabled, so building alone does not install the plugins.

Optional: build both Intel and Apple silicon architectures into universal bundles using a **separate build folder**:

```sh
cmake -S . -B build-mac-universal -G Xcode -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"
cmake --build build-mac-universal --config Release --target Rastro_AU Rastro_VST3 Rastro_Standalone --parallel 2
```

For that build, replace `build-mac` with `build-mac-universal` in the commands below. Verify architectures with `lipo -archs build-mac/Rastro_artefacts/Release/AU/RASTRO.component/Contents/MacOS/RASTRO`. This guide does not claim that either architecture has been built here.

#### 5. Run the available checks on your Mac

The portable DSP test is included in the normal configuration:

```sh
cmake --build build-mac --config Release --target rastro_tests --parallel 2
ctest --test-dir build-mac -C Release --output-on-failure
```

For additional processor/state/editor checks, reconfigure with the integration tests enabled:

```sh
cmake -S . -B build-mac -DRASTRO_INTEGRATION_TESTS=ON
cmake --build build-mac --config Release --target rastro_tests rastro_integration --parallel 2
ctest --test-dir build-mac -C Release --output-on-failure
```

The expected suite count is one without integration tests and two with them; success means all enabled tests pass. Run the editor checks in a logged-in graphical Mac session. Passing these tests is separate from passing AU validation and actually using Logic Pro.

#### 6. Install the bundles for your Mac user

Quit Logic Pro and any other audio host before installing or replacing a plugin. From the project folder:

```sh
mkdir -p "$HOME/Library/Audio/Plug-Ins/Components"
mkdir -p "$HOME/Library/Audio/Plug-Ins/VST3"
ditto "build-mac/Rastro_artefacts/Release/AU/RASTRO.component" \
  "$HOME/Library/Audio/Plug-Ins/Components/RASTRO.component"
ditto "build-mac/Rastro_artefacts/Release/VST3/RASTRO.vst3" \
  "$HOME/Library/Audio/Plug-Ins/VST3/RASTRO.vst3"
```

The expected installed paths are **`~/Library/Audio/Plug-Ins/Components/RASTRO.component`** and **`~/Library/Audio/Plug-Ins/VST3/RASTRO.vst3`**. Here `~` means your user home folder; these instructions install only for your account and require no `sudo`. In Finder, choose Go → Go to Folder and enter either parent folder to inspect it. If replacing an older test build, move the old bundle to a backup folder outside the plugin folders first to avoid merging stale bundle contents. Avoid keeping duplicate copies in both your user Library and `/Library/Audio/Plug-Ins/`.

For a local test, if code-signature verification fails, ad-hoc sign **your locally built installed copies**, then verify them:

```sh
codesign --force --sign - "$HOME/Library/Audio/Plug-Ins/Components/RASTRO.component"
codesign --force --sign - "$HOME/Library/Audio/Plug-Ins/VST3/RASTRO.vst3"
codesign --verify --verbose=2 "$HOME/Library/Audio/Plug-Ins/Components/RASTRO.component"
codesign --verify --verbose=2 "$HOME/Library/Audio/Plug-Ins/VST3/RASTRO.vst3"
```

Ad-hoc signing is for local testing; it is not Developer ID signing or Apple notarization. Distribution signing/notarization is not configured. Do not disable Gatekeeper or other system security settings to make a failed build load.

#### 7. Validate the AU and test in Logic Pro

First, in Terminal, validate the installed AU:

```sh
auval -v aumu Rstr GbCa
```

The identifiers match the CMake configuration: instrument type `aumu`, subtype `Rstr`, manufacturer `GbCa` (case-sensitive). Look for **`AU VALIDATION SUCCEEDED`**. If it fails or cannot find the AU, retain the output; do not treat installation as validation success.

1. Open Logic Pro. Go to **Logic Pro → Settings → Plug-in Manager** (older versions call Settings “Preferences”). Find **RASTRO**, manufacturer **Gab Calisman**. If needed, select it and use **Reset & Rescan Selection**. Confirm its validation status before proceeding.
2. Create an empty project and a **Software Instrument** track. If the Library opens with another instrument, open the channel strip's **Instrument** slot and choose **AU Instruments → Gab Calisman → RASTRO → Stereo**. Do not look for it in an Audio FX slot. If your Logic version also offers Mono, test that after the Stereo test.
3. Start with low monitor/headphone volume. Load a short, known-good WAV or AIFF. Check the waveform appears. Play the plugin's onscreen keyboard, then Logic's Musical Typing (**Command-K**) or a MIDI keyboard with the track selected/record-enabled. Confirm MIDI activity and sound. Root note **60** means the original sample pitch; octave naming can differ between Logic and the plugin.
4. Release keys and check notes stop. Test one-shot and Loop, root note, start/end trim, velocity/filter amount, chords, and output level. Root/trim changes apply to newly triggered notes.
5. Compare each effect's active and bypassed sound; adjust every control. Drag card headers to reorder, remove/re-add a module, compare Dry/rack, use MUTATE and Reset effects. Confirm MUTATE does not replace the sample or jump the output level. Try CC learn if you have a controller.
6. Change the Logic project tempo and confirm the plugin's host BPM display and repeat timing follow it. Test the manual fallback in Standalone, where there is no host BPM. Timing follows BPM, not bar position.
7. Switch Portuguese/English, resize the window, and record a short MIDI region. Automate a macro using Logic's automation controls and play it back. Play, stop, seek, and repeat to check for stuck notes or glitches.
8. Save the Logic project. Quit and reopen Logic, then reopen the project. Confirm the sample, trims/root, effect values/order/bypass/removal, CC assignments and language return. To check sample embedding, after saving and quitting temporarily rename/move **your test source file**, reopen the project and check it still plays; restore the source file afterwards.
9. Bounce a short passage offline and listen to the resulting file. Compare it with playback and confirm the effect tail is not cut off. The declared host tail is 12 seconds; extreme settings can ring longer. Repeat at your usual buffer size and, if useful, 44.1/48 kHz.

Record your macOS/Logic/Xcode versions, Mac chip, plugin architecture, sample rate, buffer size, AU validation output, and any reproduction steps. Hardware MIDI, audible output, AU scanning, project recall and bounce are all **pending Mac tests**, even though Linux checks have passed.

#### If something fails

- **CMake/Xcode not found:** revisit prerequisites; `xcode-select -p` should point inside full Xcode, and `cmake --version` must work in this Terminal.
- **Wrong generator or architecture cache:** use a new build folder; do not mix Linux build files or an old generator into the Mac build folder.
- **JUCE download fails:** check internet/repository access and retry configuration. The first build needs the pinned JUCE source; copied Linux binaries will not help.
- **Plugin not listed:** verify the `.component` path and architecture, quit/reopen Logic, run `auval`, and rescan just RASTRO. If a newly installed AU is still undiscovered, log out/in or restart your Mac, then rescan. Do not delete all Audio Unit caches as a first step.
- **Validation/signature failure:** check the first `auval`/`codesign` error, confirm native/universal architecture matches your Logic process, and rebuild/reinstall. Do not simply force-enable a failing AU.
- **No sound:** use a Software Instrument track, select it for MIDI input, load a sample, check output/trim values, test the onscreen keyboard, and verify Logic's audio device settings.
- **Uninstall:** quit hosts, then move only the installed `RASTRO.component` and `RASTRO.vst3` bundles out of the two user plugin folders. Keep the source/build folder if you plan to rebuild.


### Tests

Portable DSP checks can run without JUCE:

```sh
cmake -S . -B build-core -DRASTRO_CORE_ONLY=ON
cmake --build build-core
ctest --test-dir build-core --output-on-failure
```

Optional processor/editor integration checks require JUCE and a display (Xvfb works on Linux):

```sh
cmake -S . -B build -DRASTRO_INTEGRATION_TESTS=ON
cmake --build build -j2
xvfb-run -a ctest --test-dir build --output-on-failure
```

`rastro_tests` checks playback, release, effect audibility/stability, extreme trims, voice stealing and four sample rates, with allocation trapping in the DSP path. `rastro_integration` generates WAV/AIFF fixtures, checks mono/stereo import/rendering, sample-accurate MIDI onset, host tempo, state roundtrip, CC learn, invalid input, mutation preservation and editor snapshots at minimum/maximum sizes. Processor audio processing also runs under an allocation trap. It writes `editor-preview.png` into the test working directory.

## Structure and realtime behavior

- `Source/Engine.h`: JUCE-independent sample source representation, 16-voice sampler and effects. Buffers allocate only in prepare; audio rendering performs no file I/O, locking or allocation. Level, filter amount, mixes, effect controls and bypass use smoothing. Order changes fade out/in over 5 ms.
- `Source/Processor.*`: host/MIDI adapter, parameters, tempo, file decoder and embedded state. Keyboard events use a fixed SPSC queue; host parameter notifications from CC changes are deferred off the audio thread. With the editor closed, CC changes affect audio immediately and are flushed to parameters when state is saved or the editor reopens.
- `Source/Editor.*`: resizable JUCE vector interface, waveform/trim display, draggable cards, CC learn and keyboard.
- `Source/Strings.h`: centralized pt-BR / English UI text, tooltips, errors and About/help content. Module names remain unchanged.
- `tests/`: portable engine tests and optional JUCE integration tests.

The loader publishes immutable samples through an atomic pointer. Old sample buffers remain owned until processor destruction, so no sample can be freed on the audio thread or while a voice/editor reads it. Import is limited to **32 MB encoded, 120 seconds, 16 million frames**, with a **256 MB decoded-audio budget per instance lifetime**. Reopening the instance releases that budget. This simple retention strategy is deliberate for this prototype; a background retirement queue is a future improvement. Loading and state restoration decode on their caller's non-audio thread; the audio callback never decodes files. Hosts must use the normal non-realtime state lifecycle.

## Known limitations

- Prototype DSP: linear interpolation can alias at high pitches; no high-quality resampling, sustain pedal, MPE, per-channel voice ownership or voice-steal crossfade. Overlapping same-pitch notes release together. Loop edges fade briefly rather than using an overlapping crossfade; very short loops can sound quiet.
- One sample and one module of each type. Imported audio uses the first two channels. Source replacement stops active voices. No preset browser, undo system or standalone preset-file UI; use host state/presets. Restore errors retain the prior state.
- Effect timing follows BPM but not bar phase. Delay time/size movement can produce intentional pitch smearing. BLOOM is a basic diffuse tail; effect bypass keeps internal buffers warm, so old tails may reappear when re-enabled. The declared host tail is 12 seconds; extreme feedback can ring longer.
- File decoding runs outside the callback but can briefly occupy the UI/state caller. Session states embed base64 source audio and can be large.
- CC learn uses the first incoming CC, on any channel. Host automation notification for learned CC is deferred to the editor/state thread; closed-editor recording of those CC gestures as host automation is not implemented. Hardware MIDI, physical audio output and DAW workflow require hands-on testing.
- The minimum editor size is 900 × 860. The sound trail follows a single most-recent voice position; there is no per-voice visualization or animation preference.
- Video-derived audio and a separate audio-effect plugin are deliberately not implemented. Source decoding, engine and host/editor layers are separate to support those later.

## Verification in this cloud session

Built on Debian 13 x86_64 with GCC 14.2, CMake 3.31.6 and JUCE 8.0.6. Since system package installation was unavailable, Debian development packages were downloaded and extracted into workspace-local build tooling; no privileged installation was needed.

- VST3 and Standalone Release builds completed.
- CTest: **2/2 suites passed** (portable DSP and processor/editor integration).
- Engine checks passed under AddressSanitizer and UndefinedBehaviorSanitizer. Leak detection was disabled because LeakSanitizer is incompatible with the sandbox's tracing environment.
- **pluginval 1.0.3, strictness 5, seed 42: exit 0**, including VST3 scan/open, editor, processing, state, automation and bus checks. The separate Steinberg validator was not configured; pluginval's auval placeholder on Linux is not an AU validation.
- Portuguese and English editors were rendered with Xvfb; visual issues found in the snapshots were fixed. Minimum and maximum window sizes were exercised.
- Standalone launched under Xvfb and remained running until the smoke test's 4-second timeout. ALSA reported `/dev/snd/seq` unavailable; hardware MIDI and audible output were not tested.
- **No macOS build, AU validation or Logic Pro test was performed.**

To repeat external validation after installing [pluginval](https://github.com/Tracktion/pluginval):

```sh
pluginval --strictness-level 5 --random-seed 42 --validate build/Rastro_artefacts/Release/VST3/RASTRO.vst3
```

### Mac preparation follow-up (October 21, 2026)

This follow-up changed only the README. The CMake source was inspected: VST3 and Standalone are enabled generally; AU is appended under `if(APPLE)`, and `IS_SYNTH TRUE`, `NEEDS_MIDI_INPUT TRUE`, manufacturer `GbCa` and subtype `Rstr` are configured. Linux CMake configuration succeeded again, the existing Release build was up to date, CTest passed **2/2 suites**, and `git diff --check` passed. No Apple compiler/SDK or Logic Pro was available. macOS AU/VST3 compilation, installation, signing/architecture compatibility, `auval`, and all Logic Pro listening/recall/bounce tests remain unverified. The publication instructions above do not themselves publish the workspace.
