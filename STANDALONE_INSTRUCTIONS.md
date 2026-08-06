# Running YuVi Glow Standalone on Linux

This is the **development** path, not the DJ-facing one — it's how you run and test YuVi Glow directly on Linux without any DJ software involved, since neither Serato nor (easily) other hosts are always convenient mid-development. For actually using it in a DJ set, see [DJ_INSTRUCTIONS_SERATO.md](DJ_INSTRUCTIONS_SERATO.md) (Mac) or [DJ_INSTRUCTIONS_MIXXX.md](DJ_INSTRUCTIONS_MIXXX.md) (works on Linux today).

## Why Standalone
The Standalone build is the same plugin, just packaged as its own app instead of something a host loads — no VST3/AU host needed at all. It opens real ALSA audio/MIDI, so a Code 49 or MPD226 plugged in via USB behaves identically to how it would inside a host. This is the fastest loop for iterating on the plugin itself.

## One-time setup
See [DEVELOPER_NOTES.md](DEVELOPER_NOTES.md) for the required `apt-get install` of JUCE's Linux build dependencies (X11/FreeType/Fontconfig/ALSA headers) — needed once per machine.

## The dev wrapper: `scripts/linux_dev_env.sh`
Building and launching by hand means remembering several gotchas (a `DISPLAY` that isn't always inherited, a `pkill` pattern that can accidentally kill its own wrapping shell — see the script's comments for why). `scripts/linux_dev_env.sh` handles all of it:
```
./scripts/linux_dev_env.sh          # builds Debug, kills any previous instance, launches it
./scripts/linux_dev_env.sh Release  # same, but Release config
```
It prints the running process ID(s) and where its log is (`/tmp/yuviglow_dev.log`) when it's done.

## Manual equivalent (if you don't want the wrapper)
```
cmake -S . -B build
cmake --build build --config Debug -j$(nproc)
pkill -f "[Y]uVi Glow" 2>/dev/null || true   # note the brackets — see script comments
DISPLAY=:0 build/YuViGlow_artefacts/Debug/Standalone/YuVi\ Glow &
```

## What this does and doesn't verify
Standalone testing covers the plugin's own logic completely — MIDI-learn, the mapping editor, file loading/playback, gain control — all real, all on real hardware. What it can't verify is host-specific behavior (how a specific DAW's FX-slot UI presents the plugin, whether audio actually routes correctly from a host into it). For that, see the Mixxx doc — it's the closest thing to a real DJ-software test available on Linux.
