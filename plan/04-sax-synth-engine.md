# Sax Synth Engine

## v1: sample-based
`juce::Synthesiser` + `SamplerSound`/custom `SynthesiserVoice`, multi-sampled saxophone across velocity layers and round-robins.

- **Legato/portamento**: when a new note overlaps a still-held note, glide instead of retrigger. Glide time comes from the Y-axis (see below), matching the confirmed spec of *vibrato + sustain/legato* on the Y axis (no distortion stage in v1).
- **Vibrato**: pitch LFO, depth driven by Y-axis position.
- **Sustain/legato blend**: Y-axis also blends how long a note rings out / how strongly consecutive notes glide into each other — low Y = short, detached notes; high Y = long, connected, legato phrasing.
- **Expression**: keyboard velocity (and aftertouch if the Code 49 sends it) maps to amplitude + a brightness filter (low-pass cutoff) for basic breath dynamics.
- **X-axis**: steps through `ScaleMapper`'s ordered note list for the current key/scale — always quantized, never raw continuous pitch, so the pad always lands in key. Step resolution (how many scale degrees the physical joystick throw covers, and starting octave) is a UI-configurable range.

## v2/stretch: physical modeling
A simple waveguide/subtractive sax model for more authentic timbre and continuous breath control. Explicitly deferred — sample-based ships first because it's far lower engineering risk and gets a usable instrument in your hands sooner.

## Sample sourcing (blocking, non-code task)
This needs real recorded/licensed audio — see [11-open-questions-assumptions.md](11-open-questions-assumptions.md) item 2. Candidates to evaluate: Versilian Studios Community Sample Library (CC0), Spitfire LABS (free, check redistribution terms), or a small self-recorded set (a handful of velocity layers across a couple octaves is enough for v1).
