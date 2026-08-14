# Waveform Display + Per-Pad Cue Points

**Status: [PLANNED]**

Plan only, 2026-08-06 — no code written yet. This is the next real feature after the mapping editor: show the loaded file's waveform, and let each of the 16 already-MIDI-mapped pads jump playback to its own position in that file (instead of every pad restarting from 0), with a visible marker showing where each pad's cue point is — including a highlight when that pad is actually hit, per the request ("where the cue points are spliced when you hit the pad").

This is the near-term slice of the fuller 16-pad cue/loop vision already in [06-sampler-cue-pads.md](06-sampler-cue-pads.md) (bottom 8 = cue-to-end, top 8 = start+length loop) — that doc's loop-region half (top 8 pads) is still out of scope here; this plan only covers the cue-point half.

## What changes conceptually
Right now there are two separate pad concepts that this feature would merge:
- The single **Trigger Pad** (`armLearnTriggerPad()`), one learned note, always plays the loaded file from sample 0.
- The **16-pad grid** (`padBank`), already MIDI-mapped via the Edit MIDI Mapping system, currently just a touch-visualization display with no playback effect.

**Proposed**: retire the single Trigger Pad as a separate concept. Every pad in the 16-pad grid that has both (a) a MIDI-learned note and (b) an assigned cue position becomes a trigger in its own right — pressing it jumps playback to its cue position and plays to the end of the file (same "plays to completion" behavior as today's Trigger Pad, just per-pad instead of one global pad). This is a meaningful simplification once the 16-pad infrastructure already exists — flagging it explicitly since it changes existing behavior rather than purely adding to it, worth confirming before building.

## New data needed
- Each of the 16 pad slots gets an associated **cue position** (a sample index into the currently loaded file) alongside its existing MIDI note assignment. Default 0 (start of file) until explicitly set — matches current single-trigger-pad behavior until you start assigning real cue points.
- Cue positions are properties of the **loaded file**, not the **physical controller** — meaningfully different from the note/CC assignments, which are controller properties. This matters for where they get saved (see Open Questions below) — they should NOT go into the device-keyed mapping presets (`plan/issues/12`'s "Save as Default for Device"), since a cue point at 1:23 into one song is meaningless for a different song.

## New UI: `WaveformComponent`
A new component (same pattern as `PadGridComponent`/`ControlPanelComponent`) using JUCE's built-in `juce::AudioThumbnail` + `juce::AudioThumbnailCache` (fed by the existing `formatManager`) to render the loaded file's waveform.
- **Playhead**: a vertical line at the current playback position, moving live during playback.
- **16 cue markers**: small labeled markers along the timeline at each pad's cue position (only for pads that have one assigned — unassigned pads show nothing on the waveform). Reuse the pad grid's existing discovery-order numbering (pad 1-16) for the labels so they visually correspond to the physical grid.
- **Hit highlight**: when a pad is pressed, its marker briefly flashes/grows — reusing the same two-curve (fast flash + slow afterglow) timing approach already built for the pad grid's touch visualization, applied to the marker instead of a grid square.

## Assigning cue points — proposed UX
Mirrors the existing MIDI-learn UX pattern rather than inventing a new one:
- A **"Set Cue Points"** toggle button, parallel to "Edit MIDI Mapping."
- While active: click a point on the waveform to position a pending-cue marker, then click the pad (in the grid) you want to bind it to — or press the physical pad if it's already MIDI-mapped, whichever is more natural once actually building it.
- Click "Set Cue Points" again to exit, same as the mapping editor.

Alternative considered: capture the current playhead position live while the file is playing (press a modifier + the pad). Noted as a possible addition, not the primary flow — click-on-waveform is more precise for exact placement, live-capture is faster mid-listen. Could support both; deciding at build time isn't blocking this plan.

## Playback behavior (unchanged from today, just per-pad now)
Pressing a pad with both a note and a cue point assigned: jump to that cue position, play to the end of file, same as the current single Trigger Pad. Stop button still stops whatever's currently playing regardless of which pad triggered it. Looping (the "top 8 pads" half of `plan/issues/06`) stays explicitly out of scope for this pass.

## Open questions
1. **Confirm the Trigger-Pad-retirement idea above** before building — it changes existing behavior (the dedicated Learn Trigger Pad button/flow would go away) rather than being purely additive.
2. **Where do cue positions get saved?** Not the device presets (see above). Options: (a) don't persist at all yet — session-only, matching the current MVP's general lack of per-file persistence; (b) a small per-file sidecar (keyed by file hash or path, similar to the future stem-separation cache described in `plan/issues/01-architecture.md`). Leaning (a) for this pass, (b) later once file-specific caching exists for other reasons anyway.
3. **Zoom/scroll on the waveform** for precise cue placement on longer files, or is a fixed full-file view good enough for now? Leaning "fixed view is fine for v1" — the two royalty-free test files (`assets/audio/royaltyfree/`) are short enough (15s and ~2min) that this isn't a blocker yet, but will matter once real full-length songs are used.
