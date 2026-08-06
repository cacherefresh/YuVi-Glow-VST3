# Serato Integration

## The constraint
Serato DJ Pro hosts VST2/AU plugins strictly as **audio effects** applied to a deck's existing signal. It has no MIDI-instrument hosting concept — there is no way to load a plugin that takes MIDI note-on/off and generates audio from nothing. This is true regardless of plugin format (VST3 wouldn't help even if Serato supported it, which it currently doesn't — Serato supports VST2/AU only).

## Current approach (superseded the plan below — see `plan/12-mvp-v0.md`)
The MVP pivoted 2026-08-05 to an **audio-effect-shaped VST3/AU** — audio in, audio out, `IS_SYNTH FALSE` — which is exactly what Serato's FX slot expects, so it loads directly inside Serato with no virtual-audio routing needed. It still gets hardware control (Code 49 / MPD226) by opening its own direct MIDI connection, independent of the host. See `plan/12-mvp-v0.md` for the full feature set and `plan/14-serato-effect-workflow.md` for the actual on-machine steps to insert it into a Serato deck's FX slot and use it there.

One caveat carried over from the original concern below and still unverified: **Serato has historically hosted VST2/AU only, not VST3** — the AU build may be the one that actually shows up in Serato's plugin list, not the VST3 build, even though this repo is VST3-named. Confirm on the Mac (`plan/14-serato-effect-workflow.md` step 0).

## Superseded plan: standalone app + virtual audio routing
Kept for reference — this was the original plan before the MVP pivot above, and remains the right approach *if* a future phase needs a true MIDI-triggered instrument (the sax engine, [04-sax-synth-engine.md](04-sax-synth-engine.md)) that Serato genuinely cannot host as an FX-slot plugin no matter the format:
1. YuVi Glow runs as a standalone macOS app, entirely separate from Serato. It opens its own CoreMIDI input and CoreAudio output.
2. Its audio output is routed to a **virtual audio driver** — [BlackHole](https://existential.audio/blackhole/) (free, open source, 2-channel is enough).
3. In **Audio MIDI Setup** (macOS built-in utility), create a **Multi-Output Device** or **Aggregate Device** combining your real audio interface with BlackHole, so you can still hear your normal Serato output while YuVi Glow's output is simultaneously available as a virtual input.
4. In **Serato DJ Pro**, select the BlackHole input as the source for a spare channel/deck — the performance mixes live into your Serato set.

Full step-by-step for a non-technical DJ lives in [DJ_INSTRUCTIONS.md](../DJ_INSTRUCTIONS.md) — **that file is currently stale**, still describing this superseded standalone+BlackHole path (and the not-yet-built sax engine/key detection/stem separation) rather than the actual MVP's FX-slot approach. Needs a rewrite once the MVP's on-Serato workflow (`plan/14-serato-effect-workflow.md`) is verified on the Mac — not done yet, flagged here rather than silently left inconsistent.

## Later: VST3 + AU for DAW use (unrelated to Serato)
Regardless of which of the above is current, the same engine can also ship as a VST3 + AU instrument for use in a real DAW (Ableton, Logic, Pro Tools) — for production/writing, not for loading inside Serato.

## What you need to get
- **BlackHole** — free, no purchase needed.
- **Serato DJ Pro** (not DJ Lite) if you want to use its FX/plugin features elsewhere in your set — not required for the YuVi Glow routing itself, which just looks like a line input to Serato.
- **Apple Developer Program ($99/yr)** — only needed if you plan to distribute the app/plugin to other people (macOS notarization/Gatekeeper). Not required to build and run on your own Mac. See [09](09-macos-build-targets.md).
