# YuVi Glow — Overview

A JUCE-based, macOS-only performance instrument + DJ companion for the M-Audio Code 49.

## Core capabilities
1. **Key detection** of a loaded song (chroma + Krumhansl-Kessler correlation), with manual override.
2. **Sax-style synth engine**, quantized to the detected key, played via the Code 49's X/Y joystick and 49-key keyboard.
3. **Full Code 49 MIDI surface support**: 49 keys, 16 pads, 9 faders, X/Y joystick — all routed through a MIDI-learn layer, not hardcoded CC numbers.
4. **Audio file loading + automatic 4-stem separation** (vocals / drums / bass / other).
5. **16-pad cue system**: bottom 8 = jump-to-cue-and-play-to-end, top 8 = start+length loop region, both visualized on a waveform.
6. **4-fader stem mixer** (of the 9 faders).
7. **Two delivery forms**: a standalone macOS app first (for live Serato routing via virtual audio), then VST3 + AU instrument plugins for DAW use.

## Why standalone-first (not "VST3 inside Serato")
Serato DJ Pro's plugin architecture only hosts VST2/AU as **audio effects** on a deck that already has a signal — it has no concept of a MIDI-triggered instrument (note-on/off in, synthesized audio out). A VST3 instrument cannot be loaded inside Serato DJ Pro, regardless of how well it's built. See [08-serato-integration.md](08-serato-integration.md) for the actual signal path we're using instead.

## Non-goals for v1
- Windows support (macOS-only, per request).
- Getting a VST3 *instrument* to load inside Serato itself (not possible with Serato's current plugin model).
- Streaming/internet radio features.

## Reading order
[01-architecture.md](01-architecture.md) → [02-midi-mapping-code49.md](02-midi-mapping-code49.md) → [03-key-detection.md](03-key-detection.md) → [04-sax-synth-engine.md](04-sax-synth-engine.md) → [05-stem-separation.md](05-stem-separation.md) → [06-sampler-cue-pads.md](06-sampler-cue-pads.md) → [07-mixer.md](07-mixer.md) → [08-serato-integration.md](08-serato-integration.md) → [09-macos-build-targets.md](09-macos-build-targets.md) → [10-roadmap.md](10-roadmap.md) → [11-open-questions-assumptions.md](11-open-questions-assumptions.md)
