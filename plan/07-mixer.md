# Mixer

## Stem mixer (4 of 9 faders)
Faders 1–4 (default) → live gain for the 4 separated stems: vocals, drums, bass, other. Each stem also gets mute/solo, exposed in the UI (`MixerStripView`) and mappable via MIDI-learn to spare pads/knobs if available — Code 49 knob availability needs hardware confirmation, see [11](11-open-questions-assumptions.md).

## Master bus
Sum of: stem mix + sax synth voice + pad-triggered playback (full mix or soloed stem per [06](06-sampler-cue-pads.md)) → gain-staged through a limiter to prevent clipping when multiple sources combine.

## Remaining faders (5–9)
Open/configurable via MIDI-learn. Suggested defaults: sax voice level, reverb send, filter cutoff, delay send, master volume.
