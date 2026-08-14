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
2. **Pad 1 (bottom-left)** — plays the loaded file from the start, all the way through. Pressing again any time (even mid-playback) restarts from the top. This is the *default* behavior for every mapped pad except pad 2, so it needs no special-case code beyond what already existed (`triggerOrStopPadLoop()`'s existing fallback to `triggerPlayback()` for any pad without a loop region).
3. **Pad 2 (directly right of pad 1) — NEW, momentary** — plays from the start only while physically held down; releasing it stops playback immediately. This is a real, permanent exception to pad 1's default — it never switches to loop-region behavior even once BPM is set (unlike pads 1, 3, 4). Implemented as `YuViGlowAudioProcessor::momentaryPlayPadIndex` (= 1, 0-indexed) checked first, before the loop-region fallback, in both the note-on and note-off branches of `processIncomingMidi()`.
4. **STOP button** — a master off switch: stops whatever's currently playing, regardless of which pad (or the old dedicated trigger note, if still bound from a saved preset) started it. Always displays the literal text **STOP** — it's a momentary action button, not a toggle, and never changes label or shows any other state. (This was already how `stopPlayback()`/the button worked — no behavior change, just confirmed and the label capitalized to match.)
5. **Playing/Stopped indicator** — the status text next to STOP shows **🔊 Playing...** (U+1F50A, speaker with sound waves) while `isPlaying()` is true, plain **Stopped** otherwise. Cosmetic only, no functional change.

## What did NOT change
- File loading itself (already worked).
- The STOP button's actual stop logic (`stopPlayback()` was already an unconditional master stop — this pass just confirmed that and fixed the label).
- Pads 1, 3, 4's loop-region behavior once BPM is set (`plan/issues/16`) — untouched, still layered on top of the pad-1-style default the same way it always was. Only pad 2 is now permanently exempt from it.

## Files touched
- `Source/PluginProcessor.h` — new `momentaryPlayPadIndex` constant + doc comment.
- `Source/PluginProcessor.cpp` — `processIncomingMidi()`'s note-on/note-off branches special-case pad 2.
- `Source/PluginEditor.h` — STOP button label capitalized.
- `Source/PluginEditor.cpp` — Playing status text gets the speaker icon.
- `USER_MANUAL.md` — "Play" section rewritten around the two fixed pad roles.

## Three-target verification checklist
- [x] **MVP-BETA-STANDALONE** — rebuilt clean on Linux; visually confirmed STOP label and 🔊 Playing text render correctly (screenshot, forced-playback debug harness, reverted after). Pad 1/pad 2 behavior implemented and reasoned through against the existing, already-tested `triggerPlayback()`/`stopPlayback()` primitives, but **not yet physically verified by actually pressing pad 1 and pad 2 on real hardware** — worth doing before calling this fully done, per this project's standing "hands-on hardware verification is required, not optional" lesson (`plan/issues/11` items 10/13).
- [ ] **MVP-BETA-VST** — **LV2-in-Mixxx path dropped, 2026-08-13**, per explicit user decision: its window won't render in this sandbox (same issue as `AudioPluginHost`'s), blocking any real verification here, and Serato — the actually-shipping target — loads VST3/AU directly, no LV2 involved at all. LV2 build itself stays in `CMakeLists.txt` (harmless, still builds clean) but is no longer the active verification path. Superseded by two separate, now-primary efforts: **MVP-BETA-SERATO** (below — the real target) and **MVP-BETA-MIXXX** (`plan/issues/21-mvp-beta-mixxx.md` — a deliberately separate, standalone-audio-routing-based track using Mixxx as a live-capture/DJ-mixing testbed, not LV2 plugin-hosting).
- [ ] **MVP-BETA-SERATO** — primary focus as of 2026-08-13 (explicit user direction). Not yet done, Mac-only, tracked in `plan/issues/19-serato-mac-demo.md` / `DJ_DEVELOPER_ONETIME_BUILD_SERATO.md`.
- [ ] **MVP-BETA-MIXXX** — new, separate track (not subordinate to MVP-BETA-SERATO — explicit user call), see `plan/issues/21-mvp-beta-mixxx.md`.

## Correction: this repo's docs previously claimed Mixxx hosts VST3 — false
Found while working the MVP-BETA-VST checklist item above. `DJ_INSTRUCTIONS_MIXXX.md`, `README.md`, `AGENTS.md`, and `plan/issues/12`/`plan/issues/14` all asserted "Mixxx hosts VST3 directly," going back to when `DJ_INSTRUCTIONS_MIXXX.md` was first written (2026-08-06) — never actually verified at the time, just assumed. All corrected in this pass to say LV2 instead, with an explicit note on each so it's clear this isn't just stale wording but a real factual error that got caught.

## Side quest, resolved: browser-based verification (YuVi-Rays-DVS)
Explored, then dropped — full writeup in `plan/feature-requests/rfc001-yuvi-rays-dvs-midi-integration.md`. Short version: a browser can't host a native VST3 at all (hard sandboxing boundary), Web MIDI remote-control was a viable lighter alternative (mapping-sync isn't actually a new problem — DVS would just be another MIDI-learn-able device, same as any hardware controller), but decided to drop it and rely on native verification paths instead. Also created the `plan/feature-requests/` convention (vs. `plan/issues/`) and a `SUBAGENTS-EXTERNAL/` gitignored folder for any future cross-repo handoff prompts, both now documented in `AGENTS.md`.
