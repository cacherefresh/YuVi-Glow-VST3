# Developer Notes

Quick command reference for building/running this repo locally on Ubuntu. For the fuller writeup (why Linux is a supported dev target, what it doesn't cover, macOS specifics) see [plan/13-linux-dev-testing.md](plan/13-linux-dev-testing.md) and [plan/09-macos-build-targets.md](plan/09-macos-build-targets.md). Repo conventions/ground rules live in [AGENTS.md](AGENTS.md).

## One-time setup (Ubuntu)
JUCE's GUI module needs these dev headers to build at all on Linux — requires `sudo`, so it's a manual step:
```
sudo apt-get update
sudo apt-get install -y \
    libasound2-dev \
    libx11-dev libxext-dev libxrandr-dev libxrender-dev libxcomposite-dev libxcursor-dev libxinerama-dev \
    libfreetype-dev libfontconfig1-dev \
    libglu1-mesa-dev mesa-common-dev
```

## Build
```
cmake -S . -B build
cmake --build build --config Debug -j$(nproc)
```
First configure fetches JUCE (pinned version, see `CMakeLists.txt`) via `FetchContent` — needs network access, takes a minute or two. Subsequent builds are incremental.

## Clean
CMake here doesn't use a top-level `Makefile`, so plain `make clean` won't work from the repo root. Either:
```
cmake --build build --target clean   # clears build artifacts, keeps the fetched JUCE checkout (faster re-build)
```
or, for a fully clean slate (also re-fetches JUCE — slower, but rules out stale-cache issues):
```
rm -rf build
cmake -S . -B build
cmake --build build --config Debug -j$(nproc)
```

## Run locally (Standalone — no host needed)
Serato and DAWs aren't available on Linux, so day-to-day testing uses the Standalone build:
```
build/YuViGlow_artefacts/Debug/Standalone/YuVi\ Glow
```
It opens its own window against real ALSA audio/MIDI, so a Code 49 or MPD226 plugged in via USB shows up exactly as it would on macOS.

If running from an automated/background context where the shell doesn't already have a display attached, be explicit:
```
DISPLAY=:0 build/YuViGlow_artefacts/Debug/Standalone/YuVi\ Glow &
```

**Killing a running instance**: don't use `pkill -f "YuVi Glow"` verbatim in a wrapped/scripted shell — if the pattern text appears in the wrapping command's own invocation (common with agent/CI shells that echo the full command line), it can self-match and kill the wrapper instead of (or as well as) the app. Use a bracket-obfuscated pattern instead, which still matches the real process but not the literal command text:
```
pkill -f "[Y]uVi Glow"
```

## Run locally (VST3, if you need a real host)
The VST3 build also works on Linux for effect-slot-specific checks:
```
build/YuViGlow_artefacts/Debug/VST3/
```
`cmake --build` auto-installs it to `~/.vst3/YuVi Glow.vst3` — load it in any Linux VST3 host (e.g. REAPER for Linux). For everyday iteration, Standalone is simpler.

## What this doesn't cover
Real Serato loading, AU format, and macOS CoreMIDI/CoreAudio-specific behavior are all Mac-only — see `plan/09-macos-build-targets.md` for that side.
