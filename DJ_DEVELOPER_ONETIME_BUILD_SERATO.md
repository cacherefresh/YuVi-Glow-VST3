# YuVi Glow — One-Time Mac Build for Serato (Developer Steps)

**This is the developer/build doc — compiling the code on your Mac.** For actually using the plugin once it's loaded into Serato (day-to-day, no coding), see [DJ_INSTRUCTIONS_SERATO.md](DJ_INSTRUCTIONS_SERATO.md) instead. Keep the two separate: this one is a one-time (or per-code-change) setup step; that one is what you touch every session.

**Status**: this is the **first time this codebase has ever been compiled on macOS** — everything so far has been built and tested on Linux (Standalone + VST3 only). AU has never been built at all until you run the steps below. Expect this first pass to surface at least one real surprise; the troubleshooting section at the bottom covers the likeliest ones.

## What you'll end up with
Three things built from one codebase, installed to their standard macOS locations:
- **Standalone app** — for the smoke test below, not for the actual demo.
- **VST3** — `~/Library/Audio/Plug-Ins/VST3/YuVi Glow.vst3`
- **AU (Audio Unit)** — `~/Library/Audio/Plug-Ins/Components/YuVi Glow.component`

Serato may only actually list one of VST3/AU depending on your installed version — that's checked directly, live, in the Serato section below rather than assumed here.

## Prerequisites
- **Xcode Command Line Tools** (not full Xcode.app — much smaller, faster install):
  ```bash
  xcode-select --install
  ```
- **Homebrew** (if you don't already have it) — see https://brew.sh
- **CMake + Ninja** via Homebrew:
  ```bash
  brew install cmake ninja
  ```
  (Ninja gives faster parallel builds than the default Makefiles; if you'd rather skip the extra install, every command below also works with `-G "Unix Makefiles"` in place of `-G Ninja` — Makefiles ships with the Command Line Tools already, nothing extra to install.)

## Getting the code onto this Mac
However you get it here is fine — this doc doesn't assume `git clone` specifically. If you've pushed the latest work to GitHub already:
```bash
git clone https://github.com/cacherefresh/YuVi-Glow-VST3.git
cd YuVi-Glow-VST3
git checkout feature/init1   # or whichever branch has the latest work
```
If you're transferring the working tree some other way (AirDrop, rsync, a zip), just `cd` into wherever it landed and continue below. One thing to check either way: run `git status` once you're in the folder — if it's a real git checkout, this confirms you're on the branch/commit you think you are before spending time building the wrong version.

## Build — Debug (do this first)
Separate build directory per config, so Debug and Release never collide with each other on disk:
```bash
cmake -S . -B build/debug -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_OSX_ARCHITECTURES=arm64
cmake --build build/debug -j$(sysctl -n hw.ncpu)
```
`-DCMAKE_OSX_ARCHITECTURES=arm64` builds only for your M2/M3 chip instead of the universal (arm64+x86_64) binary `CMakeLists.txt` defaults to — roughly halves compile time. Drop that flag if you ever need this build to also run on an Intel Mac.

**First configure needs network access** — it fetches JUCE and libsonare via CMake `FetchContent`, which can take a few minutes depending on your connection. This is also the very first time libsonare (the BPM-detection library) has ever been compiled on macOS — if this step fails, that's the most likely place, and it's worth pasting the exact error rather than guessing at a fix blind.

Expect the first Debug build to take a while (JUCE's GUI/audio modules are substantial) — on an M2/M3 this should still be well under the multi-hour range, but don't be surprised if it's 15-30+ minutes on the very first build. Subsequent builds (after code changes) are incremental and much faster.

## Build — Release
Same pattern, separate directory:
```bash
cmake -S . -B build/release -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES=arm64
cmake --build build/release -j$(sysctl -n hw.ncpu)
```
**Important**: both Debug and Release auto-install to the *same* system plugin locations (`~/Library/Audio/Plug-Ins/VST3/` etc. — see "What you'll end up with" above). Whichever you build **most recently** is the one Serato will actually load; building one doesn't uninstall the other, it just overwrites the installed copy at that shared destination. For the actual demo: build Debug first (for the smoke test below), and if you want Release loaded into Serato afterward, rebuild Release last so it's the one sitting at the install path when you open Serato.

## Code signing (do this after every build, before loading into any host)
Local-only use needs no paid Apple Developer account — ad-hoc signing is enough (`plan/issues/09-macos-build-targets.md`). Unlike Xcode's own build system, a plain Ninja/Makefiles build does **not** auto-sign anything, and current macOS can refuse to load an unsigned AU/VST3 into a host process at all. Sign the *installed* copies (the ones actually in the standard Plug-Ins folders, not the build directory):
```bash
codesign --force --deep -s - ~/Library/Audio/Plug-Ins/VST3/"YuVi Glow.vst3"
codesign --force --deep -s - ~/Library/Audio/Plug-Ins/Components/"YuVi Glow.component"
codesign --force --deep -s - build/debug/YuViGlow_artefacts/Standalone/"YuVi Glow.app"
```
(Re-run these after every rebuild — the copy step overwrites the file, which wipes any prior signature.)

## Verify the AU independently of Serato
`auval` is Apple's own AU validator — a much faster way to catch AU-specific problems than opening Serato each time:
```bash
auval -v aufx Yvg1 Cref
```
(`Yvg1`/`Cref` are this plugin's registered subtype/manufacturer codes, from `CMakeLists.txt`'s `PLUGIN_CODE`/`PLUGIN_MANUFACTURER_CODE`.) A wall of `PASS` lines at the end means the AU is valid and registered. If it's not found at all, try `auval -a | grep -i yuvi` to confirm macOS has actually registered it — if that's empty too, the Components install didn't take; re-check the `codesign` step above and that the build actually completed without error.

## Smoke test: Standalone first, before touching Serato
This proves the Mac build works at all — JUCE, libsonare, CoreMIDI, audio — with zero Serato/AU-hosting variables in the way:
```bash
open build/debug/YuViGlow_artefacts/Standalone/"YuVi Glow.app"
```
1. **MIDI Input** dropdown should list your MPD226 — possibly as "Akai MPD226", possibly as "Custom MIDI Device (...)" if the exact CoreMIDI name string doesn't match what the code looks for. Either is fine; MIDI-learn works identically regardless of which label it gets.
2. **Load Audio File...** → pick one of the repo's own test clips, e.g. `assets/audio/royaltyfree/war-sounds.ogg` (short, CC0, zero setup — good for a quick pass/fail check before switching to your real demo song).
3. Click the **sliders icon** (top-right) to expand **MIDI Controller Settings**, click **Edit MIDI Mapping**, hit one physical pad on the MPD226 — it should turn green in the on-screen grid.
4. Click **Edit MIDI Mapping** again to exit edit mode, then press that same pad — you should hear the file play.

If all four steps work, the Mac port is solid and any remaining issues are Serato/host-specific, not a fundamental problem with the code on this platform.

## Loading it into Serato

### Step 0: confirm which format Serato actually lists — try VST3 first
Open Serato DJ Pro's FX panel and see what shows up for "YuVi Glow." **Look for VST3 first** — if it's listed, use it and test with that before touching AU at all. Reason: VST3 is the format that's been built and exercised the most already (it's what's been tested on Linux all session, and it's this repo's primary target), so it's the more likely-to-just-work path, and testing it first tells you cleanly whether an issue is "Serato-hosting-in-general" or "AU-specifically." Fall back to AU (often labeled "Component" or "AU" in Serato's list) only if VST3 doesn't show up or doesn't load. This has genuinely never been confirmed on a real Serato install for this project — don't assume either way going in.

### Step 1: insert it and open its editor
1. Assign YuVi Glow to a deck's FX slot (whichever format Serato showed you).
2. Open the plugin's own editor window from that FX slot — same window/controls as the Standalone smoke test above.
3. Confirm the **MIDI Input** dropdown still shows your MPD226 (or Custom device) here too — this proves YuVi Glow's own direct CoreMIDI connection works fine *alongside* Serato running and holding its own MIDI connections, not just in isolation.

### Step 2: load your actual demo song and map a pad
1. **Load Audio File...** → your real demo song. If it's a file whose license you haven't verified (e.g. something "possibly given for development testing"), drop it in `assets/WARNING_NON_FREE/` first per that folder's own README — keeps it off git without changing anything about how you load it into the plugin.
2. Sliders icon → **Edit MIDI Mapping** → hit the pad you want to use → **Edit MIDI Mapping** again to exit.
3. Optional but recommended if you'll be doing this again later: **Save as Default for Device**, so next time it's just **Load Default for Device**.

### Step 3: play it
- Play a track on that Serato deck as normal — nothing about your regular mixing changes.
- Hit your mapped pad → the loaded song plays from the start, all the way through, mixed on top of the deck's own audio (this is the pad-fallback-to-whole-file behavior — no BPM/tap-tempo setup needed for this).
- Full usage details (Stop button, VST MASTER Gain, gain lock, mapping more than one pad, etc.) are in [DJ_INSTRUCTIONS_SERATO.md](DJ_INSTRUCTIONS_SERATO.md) — that's the doc to keep open during the actual demo.

## Troubleshooting
- **Neither VST3 nor AU shows up in Serato at all**: confirm both are actually installed — `ls ~/Library/Audio/Plug-Ins/VST3/` and `ls ~/Library/Audio/Plug-Ins/Components/` should each show `YuVi Glow.*`. If they're missing, the build/install step above didn't complete — check the build output for errors rather than re-running blind. If they're present but Serato still doesn't see them, restart Serato (it scans for plugins at launch) or look for a "rescan plugins" option in its settings.
- **AU specifically doesn't validate / Serato shows an error for it**: run the `auval -v aufx Yvg1 Cref` command above directly — its output is far more specific than anything Serato will tell you. Re-run the `codesign` step if `auval` complains about signing.
- **Standalone won't open from Finder, shows a Gatekeeper warning**: right-click → Open (instead of double-click) the first time, or run `xattr -cr build/debug/YuViGlow_artefacts/Standalone/"YuVi Glow.app"` if it somehow picked up a quarantine flag (can happen if the code arrived via AirDrop/download rather than a local git clone).
- **MPD226 doesn't appear as "Akai MPD226"**: expected possibility, not a bug — see the smoke-test note above. It'll show as "Custom MIDI Device (...)" instead; pick it from the dropdown and MIDI-learn works exactly the same either way.
- **libsonare or JUCE fails to fetch/build**: this is genuinely the first time either has been built on macOS for this project. Capture the exact CMake/build error text — don't guess-fix blind, since this is uncharted territory for this codebase specifically.
- **Plugin editor window looks cramped inside Serato's FX panel**: flagged as a known unknown back when this was first planned (`plan/issues/14-serato-effect-workflow.md`) — the window is a fixed 540×~800px. If Serato's panel clips it, that's a quick JUCE-side resizability fix, not a fundamental problem — just note it rather than trying to force it to fit.

## Rebuilding after a code change
```bash
cmake --build build/debug -j$(sysctl -n hw.ncpu)   # or build/release
codesign --force --deep -s - ~/Library/Audio/Plug-Ins/VST3/"YuVi Glow.vst3"
codesign --force --deep -s - ~/Library/Audio/Plug-Ins/Components/"YuVi Glow.component"
```
Restart Serato afterward if it was already running — like most hosts, it won't pick up a plugin binary that changed underneath it mid-session.
