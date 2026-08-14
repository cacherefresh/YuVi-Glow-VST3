# YuVi Glow — User Manual

How to actually use YuVi Glow as a DJ once it's loaded and running — host-agnostic, covers what's true whether you're in Serato ([DJ_INSTRUCTIONS_SERATO.md](DJ_INSTRUCTIONS_SERATO.md)), Mixxx ([DJ_INSTRUCTIONS_MIXXX.md](DJ_INSTRUCTIONS_MIXXX.md)), or running it Standalone for testing ([STANDALONE_INSTRUCTIONS.md](STANDALONE_INSTRUCTIONS.md)). **Status: covers the MVP** (`plan/issues/12-mvp-v0.md`), **the mapping-menu reorg** (`plan/issues/18-header-bar-and-midi-settings-menu.md`) **and the mixer/pitch/cue-pad work** (`plan/issues/22-mixer-pitch-and-cue-pads.md`). The waveform display (`plan/issues/15-waveform-cue-points.md`) is still planned-but-unbuilt.

## The mental model
Think of YuVi Glow as **one channel of a mixer that also plays its own track**, sitting on top of whatever audio is already flowing through it (a Serato/Mixxx deck, or nothing if Standalone):

```
  your deck's audio ──→ [ KNOB 1: INPUT GAIN ] ─┐
                          trim it coming in      │
                                                 ├──→ [ FADER 1: MASTER OUT ] ──→ your DJ gear
  the file you load ──→ [ FADER 2: PITCH ADJ ] ──┘
     (played by the pads)   −8% … +8%
```

Three things to hold onto:
1. **The pads play the loaded file from cue points** — bottom two rows latch, top two rows are hold-to-play twins of the same cue points.
2. **Two separate volume stages.** Knob 1 trims what's coming *in*; fader 1 sets what goes *out*. They're separate on purpose: on a 2-channel DJ mixer both channels are taken by your decks, so YuVi Glow's output has no physical fader anywhere — fader 1 *is* its level control.
3. **Fader 2 is a pitch fader**, same as the one on a turntable or CDJ.

See `plan/issues/10-roadmap.md` for what's still coming (key detection, a sax voice, stem separation).

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
Click the **sliders icon** in the top-right of the window (next to the general Settings gear icon) to expand the **MIDI Controller Settings** section. It opens inline, pushing everything below it down — it's not a popup, so you can click things in it and then go touch pads/knobs/faders in the main grid below without it closing on you.

Inside it, click **"Edit MIDI Mapping"**. Every pad, knob, and fader in the on-screen grid turns **red** (unassigned). Now just touch the physical controls you want to use, in any order:
- Hit pads → they turn **green** as each one gets captured.
- Turn knobs, move faders → same thing. Turning one knob for a while only claims that one slot, even though it sends lots of MIDI messages while you turn it — you don't need to be careful about this.
- Want to fix just one? Click its square/knob/fader on screen first (it turns **yellow**, waiting), then touch the physical control you actually want there — this steals it from wherever else it was mapped.

Click **"Edit MIDI Mapping"** again (still in that section) when you're done. Everything goes back to showing live activity instead of assignment state. Click the sliders icon again to collapse the section.

The same section also has **"Assign Knob to master VST Gain"**, **"Reset All Mappings"**, and **"Learn Tap Tempo"** (binds your controller's own tap-tempo button, if it has one, to YuVi Glow's BPM/tap-tempo controls on the main screen — full walkthrough of those not yet written up here, tracked in `plan/issues/16-beat-detection-and-loop-pads.md`).

### 5. Save it so you never redo this
Click **"Save as Default for Device"**. From now on — even after restarting — just click **"Load Default for Device"** when you plug that same controller back in, and every mapping comes back instantly.

### 6. Play
The 16 pads split into two halves that share the same eight cue points:

```
  [x]  pads 13-16    hold-to-play  →  cue 5, 6, 7, 8
  [x]  pads  9-12    hold-to-play  →  cue 1, 2, 3, 4
       pads  5-8     press once    →  cue 5, 6, 7, 8
       pads  1-4     press once    →  cue 1, 2, 3, 4
```

- **Bottom two rows (pads 1-8) latch.** Hit one → the file plays from that pad's cue point, all the way through, mixed on top of whatever else is playing. Hit it again any time to restart from that cue.
- **Top two rows (pads 9-16) are the same cue points, hold-to-play.** Pad 9 is pad 1's twin, pad 10 is pad 2's, and so on. They play only **while you're physically holding them down** and stop the instant you release. The two checkboxes to the left of those rows turn hold-to-play off per row if you'd rather they latch.
- **Where the cue points are**: cue 1 is always the very start of the track and cue 2 is 8 seconds in — both set automatically the moment you load a file. Cues 3-8 start empty.
- **Setting your own cue points**: while the track is playing, hit an empty pad (3-8, or their twins) — that marks the spot. Playback keeps going; nothing jumps. Hit that same pad again and it plays from the point you marked. Hitting an empty pad while nothing is playing does nothing at all, because there's no playhead to mark.
- **STOP** (on screen) is a master off switch — click it any time to immediately cut off whatever's playing, no matter which pad started it.
- The status text next to STOP shows 🔊 **Playing...** while something's audible, or **Stopped** otherwise.
- Loading a new file resets all eight cue points. They belong to the track, not the controller, so they aren't saved into your device default.

### 7. Set your levels and pitch
**Input Gain (Knob 1)** trims whatever audio is flowing *through* the plugin — your deck's own sound, if hosted in Serato/Mixxx — before it gets mixed with the file you're playing. Centered = normal (0dB/unity). Toggle **"Lock @ 0dB (50%)"** to guarantee it can't move, whether from an on-screen drag or a bumped physical knob. Locking also snaps the on-screen knob back to centre.

**Master Output (Fader 1)** is the level of everything leaving the plugin — trimmed deck audio *and* the file you're playing, together. This is the one to reach for when balancing YuVi Glow against your decks, especially on a 2-channel mixer where it has no channel fader of its own.

**Pitch Adj (Fader 2)** speeds the loaded file up or down by up to ±8%, exactly like the pitch fader on a turntable — the pitch moves with the speed, it isn't key-locked. The current value shows next to Tap Tempo as a signed percentage (`+0.00%`, `-3.25%`). Bottom of the fader is −8%, top is +8%, centre is 0.00%. It only affects the file YuVi Glow is playing; your deck's audio passes through untouched.

**Fader locks.** Each of the four faders has a checkbox underneath it. Ticking one snaps that fader back to neutral and holds it there — unity for master output, 0.00% for pitch, centre for the two spare faders — and makes it ignore the physical fader entirely, so a knock mid-set can't undo it. Faders 3 and 4 aren't assigned to anything yet.

## Things worth knowing
- **The mapping is per-controller, not per-song.** Once you've saved a default for your MPD226, it applies every time that MPD226 is connected, regardless of what file is loaded.
- **Only one file is loaded at a time.** Loading a new one replaces whatever was there.
- **If your controller is also mapped inside Serato/Mixxx's own MIDI settings**, a pad press could trigger both that app's own function *and* YuVi Glow at the same time — check your host's own MIDI/controller preferences if that happens and decide whether you want the controller dedicated to YuVi Glow.
- **Cue points belong to the track, mapping belongs to the controller.** Swap in a different song and your pad-to-MIDI mapping is untouched, but all eight cue points reset (cue 1 to the new track's start, cue 2 to 8 seconds in, the rest empty). Cue points are not saved to disk.

## What's coming next (not built yet)
- A visible waveform of the loaded file, with each pad's trigger point marked on it and highlighted when hit (`plan/issues/15-waveform-cue-points.md`).
- Key lock (pitch-preserving tempo change) as an alternative to the current turntable-style varispeed pitch fader.
- Something useful bound to faders 3 and 4, and knobs 2-4.
- The full vision beyond that — key-aware sax playing, stem separation, a 4-stem mixer — is tracked in `plan/issues/10-roadmap.md`.
