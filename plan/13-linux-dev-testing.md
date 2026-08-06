# Linux Development & Testing

Added 2026-08-05 by direct request: Linux (this Ubuntu machine) is a first-class dev/test target, even though the shipping target remains macOS ([09-macos-build-targets.md](09-macos-build-targets.md)). Reason: it lets the MVP's actual logic — MIDI-learn, file loading/resampling, pad-trigger playback, gain control — get iterated on and verified locally, fast, without needing the Mac Pro for every change. **Serato itself only runs on macOS/Windows, so Serato-specific verification is still a Mac-only step** — Linux testing covers everything up to that boundary.

## One-time system setup
JUCE's GUI module needs X11 + FreeType + Fontconfig dev headers to build at all on Linux (even the internal `juceaide` codegen tool needs them). ALSA dev headers are required for audio/MIDI I/O. Run once:

```
sudo apt-get update
sudo apt-get install -y \
    libasound2-dev \
    libx11-dev libxext-dev libxrandr-dev libxrender-dev libxcomposite-dev libxcursor-dev libxinerama-dev \
    libfreetype-dev libfontconfig1-dev \
    libglu1-mesa-dev mesa-common-dev
```

This requires `sudo` — an agent working in this repo cannot run it non-interactively (no passwordless sudo on this machine), so this is a manual one-time step for a human to run.

## Building
```
cmake -S . -B build
cmake --build build --config Debug -j
```
No `-G Xcode` on Linux — default Unix Makefiles (or pass `-G Ninja` if installed) is fine. `CMAKE_OSX_*` settings in `CMakeLists.txt` are guarded behind `if(APPLE)` and don't apply here.

## Running/testing without Serato
Serato doesn't exist on Linux, so plugin-in-a-host testing here uses the **Standalone** format instead — it's in `FORMATS` in `CMakeLists.txt` specifically for this:
```
build/YuViGlow_artefacts/Debug/Standalone/YuVi Glow
```
This opens its own window with the full UI (load/stop buttons, gain slider, MIDI-learn buttons, MIDI device dropdown) and runs against real ALSA audio + MIDI devices — so a Code 49 or Akai MPD226 plugged in via USB shows up in the MIDI device dropdown exactly as it would on macOS, and MIDI-learn / pad-trigger / fader-gain can all be verified end-to-end here.

The VST3 build (`build/YuViGlow_artefacts/Debug/VST3/`) also works on Linux and can be loaded into any Linux VST3 host (e.g. REAPER for Linux) if effect-slot-specific behavior needs checking, but for day-to-day iteration the Standalone build is simpler — no host required.

## What Linux testing does NOT cover
- Serato's actual FX-slot plugin scanning/loading behavior (Mac/Windows only).
- AU format (Apple-only; not built on Linux).
- Anything specific to macOS CoreMIDI/CoreAudio device naming or behavior.

These still need a real pass on the Mac Pro before calling a release done.
