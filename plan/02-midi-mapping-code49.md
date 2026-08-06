# Code 49 MIDI Mapping

**Everything below is a default preset, not a hardcoded assumption.** Exact note/CC numbers depend on the Code 49's currently-loaded control preset (it ships with several, user-switchable on the hardware itself), so the engine reads all physical control input through `MidiRouter`'s learn layer. Nothing in `Core` references a raw note/CC number directly.

## Physical surface → engine mapping (default)

| Control | Count | Default engine target |
|---|---|---|
| Keyboard | 49 keys, velocity-sensitive | `SaxSynthEngine` note input, quantized through `ScaleMapper` (toggle: quantized-to-scale vs chromatic passthrough) |
| Pads | 16, 4×4 grid | `CuePadEngine` — top 8 = loop-region pads, bottom 8 = cue+play-to-end pads (see [06](06-sampler-cue-pads.md)) |
| Faders | 9, assignable CC | Faders 1–4 → stem volumes (vocals/drums/bass/other). Faders 5–9 → open, suggested defaults: sax voice level, reverb send, filter cutoff, delay send, master |
| X/Y joystick | 1 (pitch-bend + mod-wheel, or dual-CC depending on firmware mode) | X → scale-degree stepper (quantized position, not continuous pitch bend) driving `ScaleMapper`. Y → CC1-style continuous value driving vibrato depth + legato/sustain blend |

## MIDI-learn
Every mappable engine parameter can be re-bound live: right-click (or a dedicated "learn" toggle in `MidiLearnPanel`) → move the physical control → binding saved. This is the mechanism that makes the table above a *default* rather than a hard requirement — first hardware session will confirm actual note/CC numbers and lock in `config/code49-default-mapping.json`.

## Verification step (Phase 1)
Run a MIDI monitor (e.g. `Audio MIDI Setup` → `MIDI Studio` on macOS, `aseqdump`/`amidi` on Linux, or the app's own MIDI-learn UI) against the physical Code 49 to record real note/CC numbers for each control bank before finalizing the shipped default preset.

## Other controllers (e.g. Akai MPD226)
Because every binding here goes through MIDI-learn rather than a hardcoded note/CC table, this design is not actually Code-49-specific — it's written up as "Code 49 mapping" because that's the primary target device, but an **Akai MPD226** (16 pads + first fader, the same control shape) works identically: pick it in the MIDI device dropdown, learn the same controls against it. There is no separate mapping table to maintain per device; the table above is a *default suggestion*, not a requirement of the code.
