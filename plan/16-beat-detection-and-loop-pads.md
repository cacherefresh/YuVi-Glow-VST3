# Beat Detection + Auto-Sliced Loop Pads

Plan only, 2026-08-08 — no code written yet. Extends [15-waveform-cue-points.md](15-waveform-cue-points.md): the first 4 pads (slots 0-3, bottom-left row) auto-populate as beat-aligned loop regions when a file is loaded — pad 1 = beat 1→2, pad 2 = beat 2→3, pad 3 = beat 3→4, pad 4 = beat 4→5 — sourced from three tempo inputs (auto-detect, manual entry, tap tempo), all confirmed with the user before writing this.

## Tempo source #1: auto-detect via `libsonare`
Researched the open-source landscape before picking anything, since a copyleft dependency would have real licensing consequences for the whole project:
- The mature, well-tested beat-tracking libraries (BTrack, aubio, Gist — all from serious MIR research groups) are **GPL v3**. Linking one in and distributing the plugin would legally require the entire codebase to also be GPL.
- **[libsonare](https://github.com/libraz/libsonare)** — Apache-2.0, zero runtime dependencies, native C++17, single maintainer but actively developed — has exactly what's needed and no copyleft consequence. **User's choice**, made explicitly with the licensing tradeoff in view.

Real API (confirmed via its docs, not guessed):
```cpp
#include <sonare.h>
float detect_bpm (const float* samples, size_t length, int sampleRate);
std::vector<float> detect_beats (const float* samples, size_t length, int sampleRate); // timestamps in seconds
```
We already load the full file into a mono-or-stereo `juce::AudioBuffer<float>` in `loadAudioFile()` — feed a mono-summed copy into `detect_beats()` (batch, one-shot per file load, off the audio thread) to get **real beat timestamps**, not just a BPM number. This is more accurate than assuming beat 1 = sample 0 and multiplying by `60/BPM`, since it accounts for any lead-in silence/pickup before the actual downbeat.

### Loose coupling (explicit requirement)
User: *"make this loosely coupled as in, if a better one comes along or this becomes unmaintained, we can easily swap to a different approach."* Design:
```cpp
// TempoDetector.h — the only thing the rest of the codebase depends on
class TempoDetector
{
public:
    virtual ~TempoDetector() = default;
    virtual std::vector<double> detectBeatTimestamps (const juce::AudioBuffer<float>& buffer, double sampleRate) = 0;
};

// LibsonareTempoDetector.h/.cpp — the only file that includes <sonare.h>
class LibsonareTempoDetector : public TempoDetector { ... };
```
`PluginProcessor` holds a `std::unique_ptr<TempoDetector>`, constructed once with `std::make_unique<LibsonareTempoDetector>()`. Swapping libraries later means writing one new small file and changing one line — nothing else in the codebase (UI, pad logic, presets) touches `libsonare` directly.

### Build integration
`libsonare`'s docs don't show a `FetchContent` example, but it has its own `CMakeLists.txt`, so the same pattern as JUCE should work: `FetchContent_Declare` + `FetchContent_MakeAvailable`, pinned to a specific tag once one's picked (their docs don't recommend a specific version — same "pin, don't track main" approach we already use for JUCE, e.g. our `8.0.15` pin). C++17 requirement is compatible with our C++20 build.

## Tempo source #2: manual entry
A BPM text field in the UI. When used, beat positions are computed (not detected): beat 1 = sample 0 (assumption, flagged — most tracks used for pad-chopping in a live set start close to the downbeat, but this can be wrong for tracks with a long intro), beat duration = `60 / BPM` seconds, beats 2-5 computed by simple multiplication.

## Tempo source #3: tap tempo, including MPD226 hardware button
Standard tap-tempo algorithm: each tap records a timestamp; BPM = `60 / average(last N inter-tap intervals)` (N=4 is a common default). Same beat-position computation as manual entry (beat 1 = sample 0 assumption) once a stable BPM emerges from taps.

**Tap tempo is a new MIDI-learnable binding**, same pattern as the trigger pad / gain fader / pad-knob-fader banks already built — a "Learn Tap Tempo" capture (or folded into the existing "Edit MIDI Mapping" system as a fifth bank). The user specifically wants the **MPD226's own hardware tap-tempo button** (visible in earlier `aconnect` output as part of the "MPD226 Remote" port, exact note/CC still unverified — needs a real hardware capture same as everything else in this project, not guessed) bound to this **by default**, meaning: included in the device-keyed mapping presets (`plan/12`'s "Save as Default for Device") alongside pads/knobs/faders, so it's captured once and restored automatically.

## Computing the 4 pad loop regions
Given a beat-timestamp source (real detected array from libsonare, or computed from manual/tap BPM):
- Pad 0 (physical pad 1): loop `[beat[0], beat[1])`
- Pad 1 (physical pad 2): loop `[beat[1], beat[2])`
- Pad 2 (physical pad 3): loop `[beat[2], beat[3])`
- Pad 3 (physical pad 4): loop `[beat[3], beat[4])`

## New playback mode needed: bounded loop region
Today's playback (`triggerPlayback()`) only does "play from a fixed start to the end of the file." This feature needs a second mode: "loop continuously between `[start, end)` until stopped." Proposed:
- Extend the pad data model: each pad slot gets an optional loop region (`start`, `end` in samples) in addition to its existing MIDI note assignment — mirrors `plan/15`'s cue-position idea, just bounded instead of open-ended.
- Pads 0-3 specifically get **auto-populated** with beat-aligned loop regions when a file loads (from whichever tempo source is freshest) — but only if not already manually set, so a deliberate user edit is never silently clobbered by a later re-detection.
- Loop behavior while held: proposed **latched** (press to start looping, press again to stop) — matches the existing assumption already on record in `plan/11` item 4 for the general loop-pad concept, carried over here rather than re-litigated.
- Pads 4-15 are unaffected — they keep today's simple trigger-to-end behavior (or whatever `plan/15`'s cue-point system lands on for them).

## Open items to confirm before/while building
1. **Exact `libsonare` CMake target name and a pinned version tag** — needs inspecting the actual `CMakeLists.txt` at implementation time (docs didn't show this).
2. **MPD226's real tap-tempo button MIDI signal** — unverified, needs a live `aseqdump` capture same as every other control on this project, not assumed.
3. Whether auto-population should re-run every time a *new* tempo estimate arrives (e.g. you tap a corrected tempo after auto-detect got it wrong) or only ever populate once per file load — leaning "re-run and overwrite the auto-populated pads (not manually-edited ones) every time a new estimate arrives," so correcting the tempo actually fixes the pads, but this needs the "was this pad auto-set or manually touched" flag mentioned above to work safely.
