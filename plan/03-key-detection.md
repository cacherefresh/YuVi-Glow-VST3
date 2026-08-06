# Key Detection

## Approach
1. Extract a 12-bin chroma (pitch-class) vector per analysis frame via `juce::dsp::FFT`, averaged across the full track.
2. Correlate the averaged chroma vector against the 24 Krumhansl-Kessler key profiles (12 major + 12 minor).
3. Pick the best-correlating key as the detection result; report a confidence score (normalized correlation margin between best and second-best candidate).
4. Surface both the detected key and confidence in `KeyDisplay`, with a manual override dropdown — algorithmic key detection is typically ~70–85% accurate on full mixes, and being wrong live is worse than a two-second manual correction.

## BPM
Needed independently for the loop pads ([06](06-sampler-cue-pads.md)). Standard onset-detection + autocorrelation tempo estimate, with manual tap-tempo/override in the UI — same reasoning as above: DJs correct tempo faster than they trust an algorithm.

## Scale mode
Detected key gives a tonic + major/minor. `ScaleMapper` additionally lets the user pick a scale *mode* to actually improvise in (major, natural minor, dorian, mixolydian, blues, pentatonic) — sax playing in strict natural-minor/major only sounds correct sometimes; blues/dorian are common jazz-sax defaults worth having on day one.

## Processing timing
Runs on a background thread as soon as a file is loaded; UI shows "analyzing…" and the sax engine defaults to chromatic (unquantized) until a key is available, then switches to quantized playback.
