# RFC001 — YuVi-Rays-DVS browser integration (WASM vs. Web MIDI vs. drop)

**This is a feature request, not an accepted plan item.** Lives in `plan/feature-requests/`, not `plan/issues/`, until/unless it's actually decided to build — see `AGENTS.md`'s "Feature requests vs. planned issues" section. Not linked from `plan/00-overview.md` or `plan/00-OPEN_ISSUES.md` for the same reason.

**Suggested branch name if this moves forward**: `feature-RFC001/yuvi-rays-dvs-midi-integration`

**Status: dropped, 2026-08-13** — decided against pursuing any of this (WASM or Web MIDI) for now. Native verification paths (`AudioPluginHost`, Mixxx-via-LV2, eventually Serato) already cover what this was trying to solve, without the added complexity/risk either browser option would introduce. Kept as a feature request for reference in case DVS integration becomes worth revisiting later — see "Open question" at the bottom.

## Context
There's a separate, existing web app — YuVi-Rays-DVS (`YuVi-Rays-DVS.cacherefresh.io`, code at `github.com/cacherefresh/Yuvi_Rays-DVS` or similar, `sanctuary` branch) — a single-page JavaScript app that already loads audio files from the user's computer and runs entirely in-browser. The user asked whether it could also load/host YuVi Glow (this project's VST3) directly inside its browser window, to use as a local dev test target for MVP-BETA-VST instead of (or alongside) a native host.

**Explicit constraint**: no code in the YuVi-Rays-DVS repo gets edited from this session. If this moves forward, the user will take whatever spec comes out of this RFC and apply it there themselves, on a feature branch, separately.

## Why loading a VST3 directly in a browser isn't possible
Not a permissions setting — a hard sandboxing boundary of the web platform. A `.vst3` bundle is compiled native machine code; browsers deliberately block web pages from executing arbitrary native code, full stop. No browser exposes an API for it, and this isn't something that changes with configuration.

Two real, very different things exist in that neighborhood instead:

### Option A: WebAssembly recompile
Compile YuVi Glow's own DSP/logic to WebAssembly (via Emscripten) and run it in-browser via the Web Audio `AudioWorklet` API. Real, but:
- Requires depending on an **unofficial JUCE fork** (e.g. `Dreamtonics/juce_emscripten`) for MIDI-over-WebMIDI support — mainline `juce-framework/JUCE` (what this project actually depends on, pinned to `8.0.15`) doesn't have this. Swapping our JUCE dependency for an unofficial fork is a large, risky commitment on its own.
- A genuinely new, separate build target/architecture within *this* repo, unrelated to the existing VST3/AU/Standalone builds.

### Option B: Web MIDI remote-control
YuVi-Rays-DVS sends MIDI messages (via the browser's Web MIDI API) to a natively-running instance of YuVi Glow, over a virtual MIDI port. Audio itself stays entirely native — never touches the browser's audio graph. Much lighter than Option A, and (see below) needs **zero code changes to YuVi Glow itself**.

## The mapping-sync question (the real blocker to evaluate before choosing)
Direct question asked: if YuVi-Rays-DVS's on-screen buttons send MIDI to control YuVi Glow, how do they stay in sync with YuVi Glow's MIDI mappings — which aren't fixed, they're captured live via MIDI-learn against whatever physical controller is currently connected, and can change at any time?

**Answer: this is already solved by YuVi Glow's existing architecture, with no new code needed.** YuVi Glow doesn't treat any controller as special — Code 49, MPD226, and any unrecognized ("Custom") device all go through the same MIDI-learn + per-device-keyed-preset system (`MidiDeviceManager`'s classification, `saveMappingPresetForCurrentDevice()` / `loadMappingPresetForCurrentDevice()`). A virtual MIDI port from a browser tab is, from YuVi Glow's point of view, just another MIDI device:

1. YuVi-Rays-DVS's own on-screen pad/knob/fader buttons would each send a **fixed, hardcoded** note/CC number — a convention that lives entirely on the DVS/browser side and never needs to change (e.g. "DVS pad 1 always sends note 60 on channel 1"). This is arbitrary and simple to pick, precisely because nothing on the YuVi Glow side needs to know about it in advance.
2. The first time DVS's virtual port is used, the user does an entirely ordinary MIDI-learn pass in YuVi Glow — the same "Edit MIDI Mapping" flow already built — then clicks "Save as Default for Device."
3. From then on, "Load Default for Device" auto-restores that exact mapping whenever DVS's virtual port is detected, exactly like it already does for a physical MPD226.

So "the mapping can change" isn't actually a new problem Option B introduces — it's the same problem physical controllers already have, and it's already solved. DVS is just another device as far as YuVi Glow is concerned.

## Why this matters to *this* repo specifically, if audio ever gets involved
If the scope stayed limited to Option B (MIDI control only), YuVi Glow's audio/DSP code stays exactly as it is today — one implementation, native, shared by Standalone/VST3/AU/(soon LV2). DVS would be architecturally no different from a hardware MIDI controller: it sends button-press events, nothing more.

The risk is specifically if a *future* version of this idea combined Option A (WASM) with DVS's *existing* ability to load audio files in-browser — i.e., if DVS ever ran actual playback/DSP logic on that audio itself, inside the browser, rather than just controlling a native instance. That would mean maintaining **two separate implementations of overlapping audio logic**: the native C++ version (`Source/PluginProcessor.cpp`) and a second, WASM/JS version living in DVS. Every feature change to one (e.g. today's pad-momentary-hold behavior, or the pad-2-vs-pad-1 split) would need a matching change in the other, or the two would silently drift apart in behavior. That's an ongoing double-maintenance cost, not a one-time build step — the actual reason Option A is the significantly bigger commitment of the two, beyond just the unofficial-fork issue above.

## Recommendation in this doc
Limit any near-term work to **Option B only** (Web MIDI remote-control) if this proceeds at all. Table Option A (WASM) — out of scope for now, revisit only if there's a real, separate reason to want YuVi Glow's DSP running inside a browser tab (not just "test the plugin locally," which Option B and the native `AudioPluginHost`/Mixxx-LV2 paths already cover).

## Open question — resolved
Continue with Web MIDI (Option B), still evaluate WASM (Option A) despite the fork/duplication cost, or drop this whole side-quest and rely on the native verification paths (`AudioPluginHost`, Mixxx-via-LV2, eventually Serato) instead? **Decided: drop it.** Revisit this doc if DVS integration becomes worth reconsidering later — the mapping-sync answer above still holds either way.
