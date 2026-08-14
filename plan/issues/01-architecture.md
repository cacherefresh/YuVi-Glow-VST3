# Architecture

**Status: [PLANNED]** — describes a `Core`/`UI` module layout (`KeyDetector`, `StemSeparator`, `SaxSynthEngine`, `ScaleMapper`, `CuePadEngine`, `MidiRouter`, `WaveformView`, `XYPadView`, etc.) that was never built. The actual MVP (`plan/issues/12-mvp-v0.md`) took a simpler, different shape — see `Source/` for what really exists.

## Module layout

A shared **Core** static library used by all three build targets (standalone app, VST3, AU) so DSP logic is written once.

```
Core/
  AudioEngine        — deck playback, stem mixing, sax voice mixing → master bus, limiter
  KeyDetector         — chroma extraction + KK key-profile correlation, BPM estimate
  StemSeparator       — offline ML inference wrapper (ONNX Runtime), disk-cached per file hash
  SaxSynthEngine       — juce::Synthesiser subclass; sample-based sax voice, vibrato/legato LFOs
  ScaleMapper          — key + scale-mode → ordered note lattice used by the X-axis stepper
  CuePadEngine         — 16 pads: 8× PlayToEndCue, 8× LoopRegion
  MidiRouter           — Code 49 MIDI-learn + mapping table, persisted as JSON preset
  Mixer                — stem gain/mute/solo + sax voice gain → master

UI/
  WaveformView          — JUCE AudioThumbnail + 16 draggable cue/loop markers
  XYPadView              — mirrors hardware joystick position; mouse fallback for testing w/o hardware
  MixerStripView x4      — stem faders
  KeyDisplay              — detected key + confidence + manual override
  PadGridView             — 4×4 pad grid, cue vs loop mode indicated visually
  MidiLearnPanel           — assign/reassign any physical control to any engine parameter

Standalone/   — JUCE AudioAppComponent target; CoreAudio + CoreMIDI device selection
Plugin/       — AudioProcessor wrapping Core; JUCE emits both VST3 and AU from one wrapper
```

## Threading
- Audio thread: DSP only (synth voices, mixer, cue/loop playback). No allocation, no file I/O, no ML inference on this thread.
- Background thread pool: `StemSeparator` inference, `KeyDetector` analysis, waveform thumbnail generation. UI shows a "processing…" state on load; engine crossfades stems in once ready.
- Message thread: UI, MIDI-learn capture, preset load/save.

## Build system
CMake + JUCE (pinned via `FetchContent`), Xcode generator on macOS for code signing / notarization support. Universal binary (arm64 + x86_64), macOS 11+ (Big Sur and later).

## Config / presets
- `config/code49-default-mapping.json` — shipped default MIDI-learn mapping (see [02](02-midi-mapping-code49.md)).
- Per-song stem cache + key/BPM analysis cache keyed by file hash, stored in `~/Library/Application Support/YuViGlow/cache/`.
