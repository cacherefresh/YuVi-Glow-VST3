# Roadmap

**Status: [PLANNED]** — Phase 1 (MVP skeleton) happened, in a different shape than described (see `plan/issues/12-mvp-v0.md`), and pieces of Phase 2 (BPM detection, loop pads) happened via `plan/issues/16` — everything else (key detection, stem separation, expressive performance polish, packaging) remains unbuilt.

## Phase 0 — Planning (this session)
Planning docs, `AGENTS.md`, `DJ_INSTRUCTIONS.md` draft, memory. **No code.**

## Phase 1 — MVP skeleton
- JUCE + CMake scaffold (Core lib + Standalone target only, plugin targets deferred).
- CoreMIDI device list, basic Code 49 MIDI-learn UI.
- Sample-based sax voice with a couple of placeholder royalty-free notes.
- Naive scale quantizer, manually-selected key (no auto key-detection yet).

## Phase 2 — Analysis + pads
- Key detection (chroma + KK profile) with manual override.
- BPM detection with tap-tempo override.
- Waveform view + cue/loop pad engine (16 pads).
- Full-mix mixer only (no stems yet).

## Phase 3 — Stem separation
- ONNX Demucs integration, download-on-first-launch model weights.
- 4-stem mixer wired to faders 1–4.

## Phase 4 — Expressive performance polish
- X/Y joystick: scale-degree stepper (X) + vibrato/legato blend (Y).
- Full default 9-fader/16-pad mapping preset finalized against real hardware.
- MIDI-learn persistence (save/load mapping presets).

## Phase 5 — Packaging
- VST3 + AU plugin targets (sharing Core with the standalone app).
- Code signing/notarization (if distributing beyond your own Mac).
- `DJ_INSTRUCTIONS.md` finalized with real screenshots/steps.
- Royalty-free sample pack + Serato routing setup verified end-to-end.

## Phase 6 — Stretch
- Physical-modeling sax voice option.
- Additional scale modes in the UI.
- Windows port, if ever wanted.

Each phase gets its own approval checkpoint before code starts — per your instruction, nothing beyond this planning phase happens without a go-ahead.
