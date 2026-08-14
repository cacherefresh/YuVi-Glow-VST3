# MVP v0 — Scope (supersedes Phase 1 for now)

**Status: [COMPLETED]** — everything described in this doc is built and has been verified against real hardware (MIDI-learn, unified mapping editor, per-device presets, pad/knob/fader visualization, build targets). Newer features layer on top of this (`plan/issues/16`, `plan/issues/18`) without changing anything here.

Reprioritized 2026-08-05 based on direct instruction: build the smallest real, loadable-in-Serato thing first, before key detection or the sax engine.

## Why this MVP doesn't hit the Serato-instrument wall
[08-serato-integration.md](08-serato-integration.md) established that Serato can't host a MIDI-*instrument* plugin. This MVP isn't one: it's an **audio-effect-shaped VST3** (audio in, audio out — exactly what Serato's plugin slot expects) that additionally:
- Opens its **own direct CoreMIDI connection** to the Code 49, independent of whatever Serato does with the hardware's MIDI. macOS CoreMIDI allows multiple simultaneous listeners on one MIDI source, so both Serato and this plugin can read the Code 49 at once without conflict.
- Mixes in playback of a locally-loaded sample on top of whatever audio Serato feeds it.

This is the same trick that makes the rest of the roadmap viable later — it's the reason `MidiRouter`/MIDI-learn was designed as a direct-device-listener from the start, not something bolted on.

## Exact feature set
- **VST3 (+ AU) plugin**, loadable directly in Serato DJ Pro on a deck's FX slot.
- **Load button** in the plugin UI → native file picker → loads an mp3/wav into memory.
- **Bottom-left pad of the Code 49** triggers playback of the loaded file from the start, plays to the end, then stops automatically. No looping in this MVP.
- **Stop button**, clickable in the plugin UI, stops playback immediately if triggered.
- **F1 fader (master fader) = input gain**: controls the gain applied to whatever audio Serato is feeding the plugin (the deck signal passing through), summed with the triggered sample playback. Also exposed as a host-automatable parameter and an on-screen slider, so it works with or without the hardware connected.
- **All other faders/pads/keys: unmapped.** Explicitly out of scope for this MVP.
- **MIDI-learn, not hardcoded**, for both the trigger pad and the gain fader — consistent with the `AGENTS.md` ground rule, since exact Code 49 note/CC numbers are still unverified against real hardware ([11-open-questions-assumptions.md](11-open-questions-assumptions.md) item 1). A "Learn Trigger Pad" and "Learn Gain Fader" button each arm a one-shot capture: press the button, then hit the physical pad/move the fader, and it's bound.
- **MIDI device selector** in the UI to pick which system MIDI input to listen to (so it works even before you know the Code 49's exact port name).

## Explicitly deferred (not in this MVP)
Key detection, sax synth engine, stem separation, 16-pad cue/loop system, 4-stem mixer. All still planned — see [10-roadmap.md](10-roadmap.md) — just not first. (The standalone app target itself is **not** deferred — it exists today alongside VST3/AU, see "Build targets" below.)

## Known limitation of this build pass
This plugin is developed and smoke-tested on a Linux dev machine — see [13-linux-dev-testing.md](13-linux-dev-testing.md) for the local build/test workflow (Standalone + VST3 both build on Linux). The code also targets macOS (VST3 + AU, universal binary) per [09-macos-build-targets.md](09-macos-build-targets.md), but **actual macOS compilation and real Serato loading must still happen on the Mac Pro** — Serato doesn't run on Linux, so that step is unavoidably Mac-only and hasn't been verified yet.

## Controller support
Nothing in this MVP is hardcoded to the Code 49. MIDI-learn binds whatever note/CC the currently-selected MIDI device sends, so the same build works with the **Akai MPD226** (16 pads + first fader — same control shape) with zero code changes, just re-running "Learn Trigger Pad" / "Learn Gain Fader" against that device. Both are treated as equally-supported target controllers going forward.

## 16-pad touch visualization (`PadGridComponent`)
Added 2026-08-05 during real-hardware testing against a physical MPD226. Purely a diagnostic/expressive display — not a control binding, doesn't affect audio.

- **Layout**: 4x4 grid of squares, one per pad slot, bottom-left first, filling left-to-right then upward.
- **Mapping is explicit MIDI-learn, per slot** — not auto-discovered. See "Unified mapping editor" below for how a slot actually gets bound; two earlier auto-discovery approaches (a hardcoded "36-51" note range, then "first 16 notes seen") were tried and abandoned — full history in [11-open-questions-assumptions.md](11-open-questions-assumptions.md) item 10.
- **Two-layer glow per pad** when NOT editing mappings, both driven by the same hit (velocity + timestamp), each with its own decay curve:
  - **Purple flash** (`0xff9b59ff`) — appears immediately on hit, opacity/size scale with velocity, fades linearly over ~400ms. The "did I just hit this" signal.
  - **Teal afterglow** (`0xff2dd4bf`) — appears after a ~150ms delay (so it visually trails the purple flash rather than overlapping its onset), then fades over ~1800ms — over 4x longer than the flash. The "recently active" trailing signal, drawn wider/softer and painted underneath the purple flash.
  - Both are plain radial-gradient fills (`juce::ColourGradient`, no image effects/blur needed) expanding outward from the square's center, scaled by each curve's current amount (0–1).

## Input-gain lock toggle
Added 2026-08-05. A "Lock @ 0dB (50%)" toggle next to the Input Gain slider (`setGainLocked()`): forces the parameter to its 0.5-normalized midpoint (1.0x on the 0–2.0 range = unity = 0dB) and disables the on-screen slider while locked. The audio-thread gain calc also clamps to unity directly (not just via the parameter) while locked, and incoming MIDI from the learned gain fader is ignored while locked — so the lock can't be defeated by bumping the physical fader. State persists across save/load.

## Full MPD226 control-surface visualization (`ControlPanelComponent`)
Added 2026-08-06, extending the pad-grid pattern to the rest of the MPD226's surface: 4 knobs + 4 faders, positioned to match the physical unit — faders to the right of the pads, knobs above the faders. Live position display when not editing (purple arc gauge per knob, teal fill bar per fader); no audio-parameter binding yet — that's future work for the roadmap's stem mixer ([07-mixer.md](07-mixer.md)).

## Unified mapping editor (pads + knobs + faders)
Redesigned 2026-08-06 after real-hardware testing surfaced a genuine bug in the first version (separate per-bank "Learn All" buttons that auto-advanced on *every* incoming message): continuous controllers (knobs/faders) send dozens of CC messages per physical gesture, so one knob turn instantly blew through all 4 fader/knob slots. See [11-open-questions-assumptions.md](11-open-questions-assumptions.md) item 10 for the root-cause writeup.

- **One "Edit MIDI Mapping" toggle** (`setEditingMappings()`) now governs all three banks together, replacing the old per-bank Learn-All/Reset buttons. While active:
  - Every pad/knob/fader shows a flat state colour instead of its normal performance visualization: **red** = unassigned, **yellow** = the one currently armed for learning, **green** = assigned.
  - Clicking any pad/knob/fader arms just that slot (`armLearnPadSlot()`/`armLearnKnobSlot()`/`armLearnFaderSlot()`) as an explicit target — the next matching note/CC steals it from wherever else it was bound.
  - If nothing is explicitly armed, any note/CC that doesn't already match an assignment auto-fills the first unassigned (red) slot in that bank — so the whole grid can be captured by just physically touching everything in turn, no clicking required.
  - **The bug fix**: advancing/binding only happens when a genuinely *different* note/CC number arrives. Repeated messages from an already-bound control (e.g. continuing to turn the same knob) are pure no-ops for assignment — they only update the live value shown once editing ends. This relies on a hardware guarantee (every physical control has its own fixed note/CC number) rather than an assumption about encoder direction or absolute-vs-relative value semantics.
- **"Reset All Mappings"** clears pads + knobs + faders together back to unassigned.
- While NOT editing, pads/knobs/faders revert to their normal performance visualization and clicking does nothing (no accidental relearns during a set).

## Per-device mapping presets
Added 2026-08-06. **"Save as Default for Device"** / **"Load Default for Device"** write/read the complete mapping (trigger pad, gain fader, all 16 pads, all 4 knobs, all 4 faders — everything above) to a small XML file keyed by whichever device category is currently connected, so a capture only has to happen once per physical controller rather than once per app launch:
- `~/Library/Application Support/YuViGlow/presets/mpd226.xml` (macOS path; JUCE resolves the platform-appropriate equivalent) for the MPD226, `code49.xml` for the Code 49, `custom-<sanitized name>.xml` for anything else.
- Load auto-detects from whichever device is currently connected — no picker needed, matching the "just picks the correct one" ask.
- The same full mapping is also included in the plugin's normal DAW-session save/restore (`getStateInformation`/`setStateInformation`), independent of these named presets.

## Build targets and documentation (2026-08-06)
Formalized as one-codebase-three-targets, matching what was already technically true:
- **Standalone** (Linux + macOS) — dev/test loop, see [STANDALONE_INSTRUCTIONS.md](../../STANDALONE_INSTRUCTIONS.md) and `scripts/linux_dev_env.sh`.
- **VST3** (all platforms) — loads in Mixxx (a real, free, cross-platform DJ app that hosts VST3 effects directly, unlike Serato) or any other VST3 host, see [DJ_INSTRUCTIONS_MIXXX.md](../../DJ_INSTRUCTIONS_MIXXX.md).
- **AU** (macOS only) — the format Serato likely actually requires (unverified — see [14-serato-effect-workflow.md](14-serato-effect-workflow.md) step 0), see [DJ_INSTRUCTIONS_SERATO.md](../../DJ_INSTRUCTIONS_SERATO.md) (renamed from the earlier `DJ_INSTRUCTIONS.md`, and rewritten to match this MVP instead of the superseded standalone+BlackHole plan).

Mixxx specifically matters because it closes most of the "can't verify FX-slot hosting without a Mac" gap noted in `plan/issues/14` — it's a real DJ app, on Linux, hosting the exact plugin format Serato would also load.
