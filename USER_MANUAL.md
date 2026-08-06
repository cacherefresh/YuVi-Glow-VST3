# YuVi Glow — User Manual

How to actually use YuVi Glow as a DJ once it's loaded and running — host-agnostic, covers what's true whether you're in Serato ([DJ_INSTRUCTIONS_SERATO.md](DJ_INSTRUCTIONS_SERATO.md)), Mixxx ([DJ_INSTRUCTIONS_MIXXX.md](DJ_INSTRUCTIONS_MIXXX.md)), or running it Standalone for testing ([STANDALONE_INSTRUCTIONS.md](STANDALONE_INSTRUCTIONS.md)). **Status: matches the current build** (`plan/12-mvp-v0.md`) — describes what's actually implemented today, not the planned-but-unbuilt waveform/cue-point feature (`plan/15-waveform-cue-points.md`).

## The mental model
YuVi Glow does two things at once, on top of whatever audio is already flowing through it (a Serato/Mixxx deck, or nothing if Standalone):
1. **Plays a file you load into it, on command**, triggered by a MIDI pad.
2. **Controls the volume of the pass-through audio** with a fader, and a one-click safety lock.

That's the whole instrument right now. It's small on purpose — see `plan/12-mvp-v0.md` for what's still coming (key detection, a sax voice, stem separation, the full 16-pad cue/loop system).

## Step-by-step: your first session

### 1. Plug in your controller
Connect your MPD226, Code 49, or any class-compliant MIDI controller via USB before opening the app/plugin.

### 2. Pick your MIDI device
Open the plugin's window. The **MIDI Input** dropdown lists what's detected:
- One recognized device (Code 49 or MPD226) → connects automatically.
- Nothing detected → shows `<No MIDI Device Detected>`. Check the USB connection.
- More than one device detected → pick manually, since YuVi Glow won't guess between two real controllers.

### 3. Load a sound
Click **"Load Audio File..."** and pick an mp3/wav. Two royalty-free test files ship in this repo for exactly this purpose if you don't have your own handy:
- `assets/audio/royaltyfree/alien-spaceship-atmosphere.ogg` (~2 minutes, ambient — good for testing that longer playback works)
- `assets/audio/royaltyfree/war-sounds.ogg` (15 seconds — good for a quick trigger test)

(Both CC0, see `assets/audio/royaltyfree/LICENSES.md`.) This file is completely separate from whatever's playing on your actual DJ deck — think of it as a one-shot sample sitting on top of your mix, not a replacement for it.

### 4. Map your controller
Click **"Edit MIDI Mapping"**. Every pad, knob, and fader in the on-screen grid turns **red** (unassigned). Now just touch the physical controls you want to use, in any order:
- Hit pads → they turn **green** as each one gets captured.
- Turn knobs, move faders → same thing. Turning one knob for a while only claims that one slot, even though it sends lots of MIDI messages while you turn it — you don't need to be careful about this.
- Want to fix just one? Click its square/knob/fader on screen first (it turns **yellow**, waiting), then touch the physical control you actually want there — this steals it from wherever else it was mapped.

Click **"Edit MIDI Mapping"** again when you're done. Everything goes back to showing live activity instead of assignment state.

### 5. Save it so you never redo this
Click **"Save as Default for Device"**. From now on — even after restarting — just click **"Load Default for Device"** when you plug that same controller back in, and every mapping comes back instantly.

### 6. Play
- Hit your mapped trigger pad → the loaded file plays from the start, all the way through, mixed on top of whatever else is playing.
- Click **"Stop"** (on screen) to cut it off early.
- Hit the pad again any time (even mid-playback) to restart it from the top.

### 7. Control the pass-through volume
The **Input Gain** slider controls the volume of whatever audio is flowing *through* the plugin (your deck's own sound, if hosted in Serato/Mixxx) — separate from the loaded file's volume. Centered = normal (0dB/unity).

Toggle **"Lock @ 0dB (50%)"** any time you want to guarantee that volume can't change, whether from an on-screen drag or a bumped physical fader. Useful mid-set when you don't want a stray knob touch to change your levels.

## Things worth knowing
- **The mapping is per-controller, not per-song.** Once you've saved a default for your MPD226, it applies every time that MPD226 is connected, regardless of what file is loaded.
- **Only one file is loaded at a time.** Loading a new one replaces whatever was there.
- **If your controller is also mapped inside Serato/Mixxx's own MIDI settings**, a pad press could trigger both that app's own function *and* YuVi Glow at the same time — check your host's own MIDI/controller preferences if that happens and decide whether you want the controller dedicated to YuVi Glow.
- **Nothing here is locked to one loaded file's identity.** Trigger mapping and cue behavior (once built, `plan/15`) will need re-checking whenever you swap in a different song — the pad plays "the currently loaded file from the start," not "this specific song."

## What's coming next (not built yet)
- A visible waveform of the loaded file, with each pad's trigger point marked on it and highlighted when hit (`plan/15-waveform-cue-points.md`).
- Each of the 16 pads getting its own position in the song instead of every pad restarting from 0.
- The full vision beyond that — key-aware sax playing, stem separation, a 4-stem mixer — is tracked in `plan/10-roadmap.md`.
