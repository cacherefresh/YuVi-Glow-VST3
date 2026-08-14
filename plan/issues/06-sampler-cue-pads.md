# 16-Pad Cue System

**Status: [PLANNED]** — this specific design (draggable waveform markers, per-pad user-assigned cue positions, top-8/bottom-8 split) was never built. What actually exists instead is a different, simpler approach: pads 0-3 auto-populate as beat-aligned loop regions once BPM is known, and every other pad plays the whole file from the start — see `plan/issues/16-beat-detection-and-loop-pads.md`.

4×4 pad grid split top/bottom into two distinct behaviors.

## Bottom 8 pads — Cue + play-to-end
Each pad = a user-assignable timestamp in the loaded song. Pressing jumps playback to that point and plays through **to the end of the song** (or until another pad/stop is pressed). Assignment:
- Press pad while playing → captures current playhead position, or
- Drag a marker directly on the waveform.

## Top 8 pads — Loop region
Each pad has **(a) a start time** and **(b) a length**. Pressing plays that region and loops it continuously. Assignment: set start by playhead-capture or drag, set length by a second drag/handle on the waveform, or numeric entry.
- **Open question**: momentary (loop while held) vs latched (press to start, press again to stop). Default assumption: **latched**, matching typical Serato/Traktor cue-loop UX. Confirm during Phase 2 — see [11](11-open-questions-assumptions.md).

## Waveform view
`WaveformView` (JUCE `AudioThumbnail`-backed) renders the loaded song with all 16 markers overlaid — 8 cue diamonds (bottom pads) + 8 loop-region brackets (top pads) — visually distinguishing assigned vs unassigned pads, plus a live playhead. Markers are draggable to re-assign.

## Playback source
Pads trigger playback of the **full original mix by default** (not an individual stem) — most DJ-intuitive default. Could later add a per-pad or global toggle to instead play the currently-soloed stem, once the stem mixer ([07](07-mixer.md)) exists.

## Royalty-free starter content
Repo ships a small set of royalty-free demo tracks/samples so pads/waveform/key-detection are demoable out of the box (see [11](11-open-questions-assumptions.md) item 2 for sourcing). Users can load their own audio file instead at any time — same cue/loop assignment flow applies to any loaded file, and to a user's own instrument samples for the sax engine.
