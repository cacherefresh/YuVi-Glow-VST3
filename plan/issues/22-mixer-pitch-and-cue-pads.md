# 22 — Mixer Chain, Pitch Adjust, Cue Points + Momentary Pad Rows

**Status: [DEVELOPED]** — built and verified end-to-end on Linux Standalone against the real Akai MPD226 (see "Verification" below). Outstanding: the same pass on macOS/Serato, which is `plan/issues/19`'s job.

**Gates [20-mvp-beta.md](20-mvp-beta.md)'s MVP-BETA-SERATO checklist item** — explicit user direction: this lands *before* the Mac build/demo, not after. See "MVP critical path" at the bottom for the reduced subset that actually has to work on the day.

## Context
The MVP-BETA feature set in [20-mvp-beta.md](20-mvp-beta.md) treats the plugin as one loaded file plus a couple of trigger pads. This doc turns it into something closer to a real DJ instrument: a **two-stage gain structure** (input trim + master output) modelled on a mixer channel, a **turntable-style pitch fader**, and a **cue-point pad system** where the bottom two pad rows latch and the top two rows are momentary twins of the same cue points.

The gain-structure framing is the user's, and the reasoning is forward-looking rather than cosmetic:

> think of it like a mixer that has both gain and output into the dj equipment. [...] this will be important later if for example we want to route the gain volume of the mp3 to headphones or to a live drummer's ears, and then an additional channel of the MASTER volume to dj equipment as it won't have its own fader on the dj equipment if it's a 2 channel mixer

That last point is the real constraint: on a 2-channel DJ mixer both channels are already spoken for by the decks, so YuVi Glow's own output has no physical fader anywhere in the signal path. It has to carry its own master level control, separate from input trim, or it can't be balanced against the decks at all.

## Decisions (from user Q&A this session)

1. **Pitch Adjust is varispeed, not time-stretch.** −8% plays slower *and* lower, +8% faster *and* higher — exactly what a turntable/CDJ pitch fader does. Implemented by advancing the playback read position at a fractional rate with linear interpolation. Chosen over pitch-locked time-stretching specifically because the latter needs a phase-vocoder/WSOLA dependency (new license check per the AGPL ground rule in `AGENTS.md`, plus real DSP work) and would not be ready before the Serato demo. Key-lock stays available as a later addition; nothing here forecloses it.
2. **The two new row checkboxes are per-row hold-to-play toggles**, not radio buttons and not read-only labels. Checked = that row's four pads are momentary; unchecked = that row latches like the bottom two rows. Both default to checked, so pads 9-16 are hold-to-play out of the box. Despite the user's "radio check box" wording, the behaviour chosen is a plain independent toggle per row — the two rows are not mutually exclusive.
3. **Eight cue points, and the beat-aligned loop regions come off pads 1-4 entirely.** Cue 1 = track start, cue 2 = 8 seconds in, cues 3-8 empty until captured. One rule per pad, nothing that silently means a different thing depending on whether a BPM happens to be set. This is a deliberate, user-approved retirement of part of [16-beat-detection-and-loop-pads.md](16-beat-detection-and-loop-pads.md) — see "What this retires" below.
4. **Two-stage gain, and fader 1 is *not* Pitch Adjust.** The original request put Pitch Adj on fader 1; the user corrected this when answering the master-gain question. Final assignment is in the signal chain below. Pitch Adjust moves to **fader 2**.

## Signal chain

```
  Serato deck audio ─→ [ KNOB 1: INPUT GAIN ] ─┐
     (host audio in)      trim, pre-mix         │
                                                ├─→ [ FADER 1: MASTER OUT ] ─→ to DJ equipment
  loaded mp3/wav ────→ [ FADER 2: PITCH ADJ ] ─┘
     (cue-point playback)   −8% … +8% varispeed
```

| Control  | Role                              | Lock resets to      |
|----------|-----------------------------------|---------------------|
| Knob 1   | Input gain (trim on host audio)   | middle = 0 dB       |
| Knob 2-4 | Unassigned                        | —                   |
| Fader 1  | Master output                     | middle = 0 dB       |
| Fader 2  | Pitch Adjust (−8% … +8%)          | middle = 0.00%      |
| Fader 3  | Unassigned                        | middle              |
| Fader 4  | Unassigned                        | middle              |

Knob 1 is the **existing** gain control, not a new one — the processor already applies a learned input gain to host audio (`smoothedInputGain`, `gainCcNumber`). What changes is that it becomes knob-bank slot 0 rather than a separately-learned "gain fader" CC, and its existing **Lock @ 0dB** button now also snaps the on-screen knob to its middle position instead of only clamping the parameter. The legacy `gainCcNumber` binding stays readable when loading older device presets so existing saved mappings don't break.

Fader 1 (master output) is genuinely new: it applies to the **summed** signal — trimmed host audio plus cue-point playback — as the last thing before the plugin's output buffer.

## Pitch Adjust

- **Range** −8.00% to +8.00%, matching the standard DJ pitch-fader range.
- **Display**: a read-only field labelled **Pitch Adj** sitting next to the existing Tap Tempo control, showing a signed two-decimal percentage — `+0.00%`, `-3.25%`, `+8.00%`.
- **Mapping**: fader 2's normalised 0..1 CC position maps linearly, bottom = −8%, top = +8%.
- **Centre detent**: a physical fader cannot reliably land on exactly 0.00% — MIDI CC 64 of 0-127 is `+0.06%`, not zero. Any resulting percentage within ±0.1% snaps to exactly 0.00%, which makes the two CC steps nearest centre both read as true zero. The lock button guarantees it absolutely.
- **Audio thread**: playback position becomes fractional (`double` rather than `int`) and advances by `1.0 + pct/100` output samples per sample, linearly interpolated between neighbouring samples. No allocation, no locks — satisfies the audio-thread discipline rule in `AGENTS.md`.
- **Scope**: affects the plugin's own file playback only. Host audio passing through is untouched — YuVi Glow is an effect in Serato's chain and must not resample the deck.

## Cue points

Eight cue points, owned by the currently-loaded file and reset whenever a new file loads.

| Cue | Set when                          | Value                                    |
|-----|-----------------------------------|------------------------------------------|
| 1   | On file load, always              | sample 0 (track start)                   |
| 2   | On file load, always              | 8 seconds in                             |
| 3-8 | First press of the pad while playing | wherever the playhead is at that moment |

Behaviour of a cue pad:
- **Cue is set** → playback jumps to that position and plays.
- **Cue is unset and the track is playing** → capture the current playhead into that cue. Playback is *not* interrupted; the pad is now armed for next time. Pressing it again plays from the captured point.
- **Cue is unset and nothing is playing** → no-op. There is no playhead to capture, and silently aliasing it to "play from the start" would make an empty pad indistinguishable from cue 1.

Two edge cases, both handled rather than left to chance:
- A track shorter than 8 seconds gets cue 2 clamped to the final sample rather than pointing past the end.
- Cue points are per-track state, so they live with the loaded file and are **not** written into the per-device mapping presets (`mpd226.xml` / `code49.xml` / `custom-*.xml`) — those describe a controller, not a song.

## Pad layout

The grid is already indexed pad 0 = bottom-left, filling left-to-right then upward (`PadGridComponent::paint`). In the user's 1-based numbering:

```
  [x]   pads 13-16   row 4   momentary  → cue 5, 6, 7, 8
  [x]   pads  9-12   row 3   momentary  → cue 1, 2, 3, 4
        pads  5-8    row 2   latch      → cue 5, 6, 7, 8
        pads  1-4    row 1   latch      → cue 1, 2, 3, 4
```

Pad *N* (1-8) latches: press once, plays from cue *N* through to the end. Pad *N+8* is its momentary twin: **same cue point**, but plays only while physically held and stops the instant it is released. So pad 1 and pad 9 both start at the beginning of the track — pad 1 plays on, pad 9 plays only while held.

The two checkboxes sit to the left of rows 3 and 4 and flip that row between momentary and latch.

## What this retires

Both of these are intentional removals, agreed with the user, not oversights:

- **`momentaryPlayPadIndex` (pad 2 as the momentary pad)** from [20-mvp-beta.md](20-mvp-beta.md) item 3. Pad 2 becomes the latching cue-2 pad; its momentary role moves to pad 10, which is now the general rule rather than a one-pad special case. The constant and its special-casing in `processIncomingMidi()` come out.
- **Beat-aligned loop regions on pads 1-4** from [16-beat-detection-and-loop-pads.md](16-beat-detection-and-loop-pads.md). Pads 1-4 are cue pads now. Tempo detection, manual BPM entry and tap tempo all stay — BPM is still detected, displayed and useful.

  The plan when this doc was written was to leave the loop-region machinery in the processor unbound. That turned out to be the wrong call once implemented: with no pad able to reach it, `loopActive` / `loopStartSample` / `loopEndSample` / `activeLoopPadIndex` / `padLoopStartSample` / `padLoopEndSample` / `hasPadLoopRegion()` / `applyBeatGridToPads()` were all unreachable dead code sitting in the audio callback, and the wrapping loop-playback branch actively complicated the varispeed rewrite it wrapped. They were removed instead. Nothing about the beat grid is lost that anything could still read.

  **This removal exposed a real pre-existing bug, now fixed**: `applyBeatGridToPads()` was the *only* consumer of auto-detect's output, and it never wrote `currentBpm` — so libsonare auto-detection ran on every file load and its result never reached the UI at all. The BPM box only ever populated from manual entry or tap tempo. Auto-detect now derives BPM by averaging the detected inter-beat intervals and stores it, which is verified working (129.1 BPM on the repo's own test clip).

## Assumptions flagged rather than silently taken

- **8 seconds for cue 2 is a fixed literal**, taken directly from the request. Not configurable, not derived from BPM. If it should be "8 beats" rather than "8 seconds" once a BPM is known, that is a follow-up, not this doc.
- **Unassigned cue pads while stopped do nothing.** Reasoned above; the alternative (aliasing to track start) was rejected. Worth confirming against real hardware feel during the demo.
- **Master output and input gain are both plain linear gains** with middle = unity (0 dB), matching how the existing gain control already behaves. No fader taper curve, no dB-scaled law — that is a refinement, not MVP.
- **Nothing here is controller-specific.** All five controls bind through the existing `MidiLearnBank` slots, so the same code works with the Code 49, the MPD226, or anything class-compliant — per the standing ground rule in `AGENTS.md`.

## Verification

Done on this Linux box against the real Akai MPD226, driven over ALSA by addressing the plugin's own sequencer client directly (its input ports advertise `WRITE` but not `SUBS_WRITE`, so `aplaymidi` can't subscribe to them — a ~40-line direct-addressing sender was used instead). Pad and CC numbers came from the live mapping, so these are the real learned bindings, not synthetic ones.

| Behaviour | Result |
|---|---|
| Pad 1 press + release — latches, keeps playing past note-off | ✅ `🔊 Playing...` |
| Stray pad 9 release while pad 1 owns playback — must not cut it | ✅ still playing |
| Pad 9 held — takes over playback | ✅ playing |
| Pad 9 released — stops immediately | ✅ `Stopped` |
| Pad 3 (empty cue) while stopped — no-op | ✅ stayed `Stopped` |
| Pad 2 → cue 2 (8s in) | ✅ playing |
| Pad 3 (empty cue) while playing — captures, must not interrupt | ✅ still playing |
| Pad 3 again, cue now set — plays from the captured point | ✅ playing |
| Pitch fader CC 0 / 64 / 96 / 127 | ✅ `-8.00%` / `+0.00%` / `+4.09%` / `+8.00%` |
| Centre detent (CC 64 raw = +0.06%) | ✅ reads `+0.00%` |
| Auto-detect BPM reaches the UI | ✅ 129.1 |
| `.ogg` loads through the file picker | ✅ (see the filter fix below) |

CC 96 landing on exactly `+4.09%` matches `(96/127 − 0.5) × 16` to the digit, so the mapping is linear across the whole range rather than just correct at the endpoints.

**Not covered by this pass**: audible confirmation of the varispeed *sound* (the readout and the read-rate maths are verified, a human ear on the pitch shift is not), the master-output and input-gain audio stages beyond their parameters moving, and anything at all on macOS/Serato.

## Unrelated bug found and fixed in the same pass
The file picker's filter was `"*.wav;*.mp3"` while **both** of the repo's own CC0 test clips in `assets/audio/royaltyfree/` are `.ogg` — so the smoke-test steps in `DJ_DEVELOPER_ONETIME_BUILD_SERATO.md`, which tell you to load one of them, could not be followed. JUCE decodes Ogg Vorbis fine via `registerBasicFormats()`; only the filter string hid them. Widened to `*.wav;*.mp3;*.ogg;*.flac;*.aiff;*.aif`, matching what this build actually decodes.

## MVP critical path

For the Serato demo specifically, the subset that must work:
- **Pad 1** plays the loaded track from the beginning (already working today).
- **Pad 9** plays from the beginning only while held, stopping immediately on release.

Everything else in this doc — cues 3-8, the capture behaviour, pitch adjust, the master fader, the locks — is wanted, but the demo does not fall over if one of them misbehaves. Build and verify the two pads above first.

## Files touched
- `Source/PluginProcessor.h` / `.cpp` — cue-point array, fractional playback position + linear interpolation, master output gain, input gain moved to knob bank, per-row momentary flags, fader lock state, BPM derived from detected beats; removal of `momentaryPlayPadIndex` and the whole pad→loop-region path.
- `Source/PadGridComponent.h` — two row checkboxes in a new left gutter, and the grid shifted right to make room.
- `Source/ControlPanelComponent.h` — lock checkboxes under each of the four faders; locked faders drawn dimmed.
- `Source/PluginEditor.h` / `.cpp` — Master Output row, Pitch Adj readout beside Tap Tempo, gain label renamed, file-picker filter widened.
- `Source/ToggleCheckbox.h` — **new**, from the `/simplify` pass. Both components had independently written out the same two non-obvious checkbox details (JUCE's tick box is sized off the button's *height* and inset 4px from the left, so square bounds clip it into a "C"; and the state has to be polled from the processor rather than assumed from the last click). One of them carried a comment pointing at the other file to explain a magic number, which is the signal that it wanted extracting.

## `/simplify` pass
Run per `AGENTS.md` after the feature was working. Two things fixed:
- **Efficiency, audio thread**: the varispeed loop called `getReadPointer()`/`getWritePointer()` once per output sample *per channel*. Hoisted to `getArrayOfReadPointers()`/`getArrayOfWritePointers()` before the loop.
- **Reuse**: extracted `Source/ToggleCheckbox.h` as above.

Re-verified after both changes — pad 1 latch, pad 9 hold/release, and playback at −8% and +8% all behave identically to the pre-cleanup run.
- `USER_MANUAL.md`, `DJ_INSTRUCTIONS_SERATO.md`, `DJ_DEVELOPER_ONETIME_BUILD_SERATO.md` — pad roles and the new controls.
- `plan/00-OPEN_ISSUES.md`, `plan/issues/16`, `plan/issues/20` — status and cross-reference updates.
