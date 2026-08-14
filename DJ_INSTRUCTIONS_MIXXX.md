# YuVi Glow — Using It in Mixxx

**Status: DRAFT**, matches the current build (`plan/issues/12-mvp-v0.md`). No coding knowledge needed for anything below.

## What's Mixxx?
[Mixxx](https://mixxx.org) is a free, open-source DJ application — similar idea to Serato, but free and available on Linux, Mac, and Windows. Unlike Serato, Mixxx directly supports loading VST3 plugins as effects, which makes it the easiest way to try YuVi Glow without needing a Mac at all.

## What you need
- A computer running Mixxx (Linux, Mac, or Windows).
- An M-Audio Code 49, Akai MPD226, or honestly any class-compliant MIDI controller.

## Installing Mixxx (Ubuntu/Linux)
```
sudo apt-get install mixxx
```
(Or download an installer from [mixxx.org](https://mixxx.org) for Mac/Windows/other Linux distros.)

## Getting YuVi Glow's plugin installed
Build it from this repo (see [DEVELOPER_NOTES.md](DEVELOPER_NOTES.md)) — the build automatically installs the VST3 to the standard location Mixxx scans:
```
cmake -S . -B build
cmake --build build --config Debug -j$(nproc)
```
This installs to `~/.vst3/YuVi Glow.vst3` on Linux (or the equivalent per-platform path elsewhere).

## Loading it in Mixxx
1. Open Mixxx → **Effects** panel.
2. If this is the first time, go to Mixxx's plugin/effects preferences and make sure VST3 scanning is enabled, then let it rescan — it should pick up `~/.vst3/YuVi Glow.vst3` automatically.
3. Assign YuVi Glow to an effect unit on the deck you want to use it on.

That deck now has YuVi Glow active. Play a track on it like normal.

## Setting it up
Same as the Serato version — see [DJ_INSTRUCTIONS_SERATO.md](DJ_INSTRUCTIONS_SERATO.md)'s "Setting it up" and "Playing it live" sections, the plugin's own window works identically no matter which app is hosting it:
1. Open the plugin's own window from the effect slot.
2. Pick your MIDI controller from the dropdown.
3. Load a sound file to trigger.
4. Click the sliders icon (top-right) to expand the MIDI Controller Settings section, then click "Edit MIDI Mapping" to capture your pads/knobs/faders (touch each one, or click to target a specific one) — the section stays expanded while you do this.
5. "Save as Default for Device" so you never have to redo it.

## One thing to check
Same note as the Serato doc: if your controller is *also* mapped inside Mixxx's own MIDI controller preferences (for deck/loop/cue control), a pad press could trigger both Mixxx's own mapping and YuVi Glow's. Check Mixxx's Controller preferences if that happens.

---

## Setting up other DAWs / hosts

YuVi Glow is a standard VST3 (and AU on macOS) plugin — anything that hosts VST3/AU effects can load it. The core workflow is identical everywhere: insert it as an effect on a channel, open its own window, pick your MIDI controller, load a sound, map your pads/knobs/faders.

- **REAPER** (Linux/Mac/Windows): Insert on a track via the FX chain (`fx` button on the track) → Add → search "YuVi Glow" under VST3.
- **Ableton Live** (Mac/Windows): Drag YuVi Glow from the VST3 plugin browser onto a track.
- **Logic Pro** (Mac): Use the AU build — insert via a channel strip's plugin slot, under Audio Units.
- **Anything else**: if it can host third-party VST3 or AU effects, the same steps apply — the plugin doesn't know or care which host it's running in.
