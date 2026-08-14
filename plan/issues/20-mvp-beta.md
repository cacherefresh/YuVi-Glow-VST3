# 20 — MVP-BETA (Standalone / VST / Serato)

**Status: [DEVELOPED]** — the feature set below is coded and verified on Linux Standalone. VST-in-a-host and Serato-on-Mac verification are still outstanding — see the three-target checklist below.

## Context
User-defined MVP baseline, explicitly named as three separate verification targets covering every way this plugin runs:
- **MVP-BETA-STANDALONE** — this Linux dev machine, the Standalone build.
- **MVP-BETA-VST** — the VST3 build loaded into a real VST3 host (REAPER, Ableton, etc.), or the **LV2** build loaded into Mixxx on Linux specifically (Mixxx has no VST3 support — see the correction below).
- **MVP-BETA-SERATO** — the AU/VST3 build loaded into Serato on the Mac (`plan/issues/19-serato-mac-demo.md` is the operational doc for this).

Same codebase, same feature set, same behavior expected on all three — nothing in this spec is platform- or host-specific.

## Exact feature set
1. **Load Audio File** — native file picker, loads an mp3/wav from disk (or a mounted USB drive — no different from any other disk path to the OS/JUCE file picker, no special handling needed). Already existed; unchanged.
2. **Pad 1 (bottom-left)** — plays the loaded file from the start, all the way through. Pressing again any time (even mid-playback) restarts from the top. Still true today, though the mechanism changed: `triggerOrStopPadLoop()` is gone and pad 1 is simply the cue-1 pad, cue 1 being the track start (`plan/issues/22`).
3. **Pad 2 (directly right of pad 1) — momentary** — plays from the start only while physically held down; releasing it stops playback immediately. Implemented as `YuViGlowAudioProcessor::momentaryPlayPadIndex` (= 1, 0-indexed), checked before the loop-region fallback in both the note-on and note-off branches of `processIncomingMidi()`.

   ⚠️ **Superseded by [22-mixer-pitch-and-cue-pads.md](22-mixer-pitch-and-cue-pads.md).** Momentary is no longer a one-pad special case — pads 9-16 are the hold-to-play twins of pads 1-8. Pad 2 is now the latching cue-2 pad (8 seconds in) and **pad 10** is its momentary twin. `momentaryPlayPadIndex` and its special-casing are gone from the code.
4. **STOP button** — a master off switch: stops whatever's currently playing, regardless of which pad (or the old dedicated trigger note, if still bound from a saved preset) started it. Always displays the literal text **STOP** — it's a momentary action button, not a toggle, and never changes label or shows any other state. (This was already how `stopPlayback()`/the button worked — no behavior change, just confirmed and the label capitalized to match.)
5. **Playing/Stopped indicator** — the status text next to STOP shows **🔊 Playing...** (U+1F50A, speaker with sound waves) while `isPlaying()` is true, plain **Stopped** otherwise. Cosmetic only, no functional change.

## What did NOT change
- File loading itself (already worked).
- The STOP button's actual stop logic (`stopPlayback()` was already an unconditional master stop — this pass just confirmed that and fixed the label).
- ~~Pads 1, 3, 4's loop-region behavior once BPM is set (`plan/issues/16`) — untouched, still layered on top of the pad-1-style default the same way it always was. Only pad 2 is now permanently exempt from it.~~ **No longer true**: `plan/issues/22` removed beat-aligned loop regions from the pads entirely. All 16 pads are cue pads now.

## Files touched
- `Source/PluginProcessor.h` — new `momentaryPlayPadIndex` constant + doc comment.
- `Source/PluginProcessor.cpp` — `processIncomingMidi()`'s note-on/note-off branches special-case pad 2.
- `Source/PluginEditor.h` — STOP button label capitalized.
- `Source/PluginEditor.cpp` — Playing status text gets the speaker icon.
- `USER_MANUAL.md` — "Play" section rewritten around the two fixed pad roles.

## Three-target verification checklist
- [x] **MVP-BETA-STANDALONE** — rebuilt clean on Linux; visually confirmed STOP label and 🔊 Playing text render correctly (screenshot, forced-playback debug harness, reverted after). Pad 1/pad 2 behavior implemented and reasoned through against the existing, already-tested `triggerPlayback()`/`stopPlayback()` primitives, but **not yet physically verified by actually pressing pad 1 and pad 2 on real hardware** — worth doing before calling this fully done, per this project's standing "hands-on hardware verification is required, not optional" lesson (`plan/issues/11` items 10/13).

  **Closed out 2026-08-14 by `plan/issues/22`'s verification pass**, which did exercise the real learned bindings over ALSA against the connected MPD226 — latch, hold-to-play, release ownership, and the STOP/Playing indicator all confirmed live rather than reasoned about. That pass covers the pad roles as they exist *now* (cue pads), not the pad-1/pad-2 arrangement described above, which it superseded.
- [ ] **MVP-BETA-VST** — **LV2-in-Mixxx path dropped, 2026-08-13**, per explicit user decision: its window won't render in this sandbox (same issue as `AudioPluginHost`'s), blocking any real verification here, and Serato — the actually-shipping target — loads VST3/AU directly, no LV2 involved at all. LV2 build itself stays in `CMakeLists.txt` (harmless, still builds clean) but is no longer the active verification path. Superseded by two separate, now-primary efforts: **MVP-BETA-SERATO** (below — the real target) and **MVP-BETA-MIXXX** (`plan/issues/21-mvp-beta-mixxx.md` — a deliberately separate, standalone-audio-routing-based track using Mixxx as a live-capture/DJ-mixing testbed, not LV2 plugin-hosting).
- [ ] **MVP-BETA-SERATO** — primary focus as of 2026-08-13 (explicit user direction). Not yet done, Mac-only, tracked in `plan/issues/19-serato-mac-demo.md` / `DJ_DEVELOPER_ONETIME_BUILD_SERATO.md`. **Now gated on [22-mixer-pitch-and-cue-pads.md](22-mixer-pitch-and-cue-pads.md)** (explicit user direction, 2026-08-14): the mixer chain, pitch fader and cue pads land *before* the Mac demo, not after. `22` is built and verified on Linux; two hard macOS build blockers found and fixed in the same pass — see `plan/issues/19`.
- [ ] **MVP-BETA-MIXXX** — new, separate track (not subordinate to MVP-BETA-SERATO — explicit user call), see `plan/issues/21-mvp-beta-mixxx.md`.

## Correction: this repo's docs previously claimed Mixxx hosts VST3 — false
Found while working the MVP-BETA-VST checklist item above. `DJ_INSTRUCTIONS_MIXXX.md`, `README.md`, `AGENTS.md`, and `plan/issues/12`/`plan/issues/14` all asserted "Mixxx hosts VST3 directly," going back to when `DJ_INSTRUCTIONS_MIXXX.md` was first written (2026-08-06) — never actually verified at the time, just assumed. All corrected in this pass to say LV2 instead, with an explicit note on each so it's clear this isn't just stale wording but a real factual error that got caught.

## Side quest, resolved: browser-based verification (YuVi-Rays-DVS)
Explored, then dropped — full writeup in `plan/feature-requests/rfc001-yuvi-rays-dvs-midi-integration.md`. Short version: a browser can't host a native VST3 at all (hard sandboxing boundary), Web MIDI remote-control was a viable lighter alternative (mapping-sync isn't actually a new problem — DVS would just be another MIDI-learn-able device, same as any hardware controller), but decided to drop it and rely on native verification paths instead. Also created the `plan/feature-requests/` convention (vs. `plan/issues/`) and a `SUBAGENTS-EXTERNAL/` gitignored folder for any future cross-repo handoff prompts, both now documented in `AGENTS.md`.
