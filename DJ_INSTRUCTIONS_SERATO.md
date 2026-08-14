# YuVi Glow — Using It in Serato DJ Pro

**Status: DRAFT**, matches the current build (`plan/issues/12-mvp-v0.md`). Exact menu wording in Serato may differ slightly by version — this will get more precise once verified on a Mac. No coding knowledge needed for anything below.

## What you're getting
YuVi Glow is a small plugin that sits on one of your Serato decks as an **effect** — the same category as a reverb or filter — and adds two things to it:
- A sound of your choosing (any mp3/wav you load into it) that plays whenever you hit a pad on your MPD226 or Code 49.
- A volume control for that deck's own sound, with a one-click "lock to normal volume" safety so you can't accidentally boost or cut it.

## What you need
- A Mac running Serato DJ Pro (DJ Lite should also work — this doesn't depend on Serato's paid features).
- An M-Audio Code 49 or Akai MPD226 (or honestly, any class-compliant MIDI controller — see "Using a different controller" below).
- No extra drivers or virtual audio cables needed — this plugs directly into Serato's own effects system.

## Getting the plugin onto your Mac
There's no installer yet — the plugin has to be built from source once per machine (or per code update). That's a separate, developer-facing process: see [DJ_DEVELOPER_ONETIME_BUILD_SERATO.md](DJ_DEVELOPER_ONETIME_BUILD_SERATO.md) for the exact steps. Come back here once that's done and both plugin files are showing up in Serato's FX panel.

## Loading it in Serato
1. Open Serato DJ Pro's FX panel for the deck you want to add this to.
2. Look for YuVi Glow in the list of available plugins — **it may only appear as one of the two formats** (VST3 or AU/"Component"), not necessarily both, depending on what your Serato version supports. Pick whichever one shows up.
3. Assign it to that deck's FX slot.

That deck now has YuVi Glow sitting on it. Play a track on that deck like you normally would — nothing changes about your regular mixing.

## Setting it up
1. Open the plugin's own window (Serato should have a button to expand/edit the FX plugin — look for it on the FX slot).
2. **Pick your controller**: use the MIDI Input dropdown at the top. If only one controller is plugged in, it connects automatically.
3. **Load a sound**: click "Load Audio File...", pick any mp3/wav. This is completely separate from whatever track is playing on the Serato deck — think of it as a one-shot sample you're adding on top.
4. **Map your pads/knobs/faders**: click the sliders icon (top-right of the plugin window) to expand the MIDI Controller Settings section, then click "Edit MIDI Mapping" — every pad/knob/fader turns red. The section stays expanded, so leave it there and start touching the pads/knobs/faders you want to use (in any order); they'll turn green as they're captured. Click "Edit MIDI Mapping" again when you're done, then click the sliders icon again to collapse the section.
5. **Save it so you never have to redo this**: click "Save as Default for Device." Next time — even after restarting Serato or your Mac — just click "Load Default for Device" and everything's back.

## Playing it live
- Hit your mapped pad 1 (bottom-left) → your loaded sound plays over the deck's own audio, all the way through. Hit it again any time to restart from the top.
- Pad 2 (right next to it) is momentary instead — it only plays while you're physically holding it down, and stops the instant you let go.
- Click **STOP** (in the plugin window) any time to immediately cut off whatever's playing, no matter which pad started it — it's a master off switch.
- The "VST MASTER Gain" slider controls how loud the *deck's own sound* is as it passes through the plugin — leave it centered for normal volume. Toggle "Lock @ 0dB" if you want to guarantee the deck's volume can never be accidentally changed by a bumped fader.

## One thing to check: does Serato already have your controller mapped?
Serato lets you assign MIDI controllers to its own functions (cue points, loops, etc.) in its own MIDI setup screen. If your MPD226/Code 49 is *also* assigned there, pressing a pad might do two things at once — trigger YuVi Glow **and** whatever Serato has that pad set to. Check Serato's own MIDI setup (Setup → MIDI) and decide whether you want the controller dedicated to YuVi Glow or doing double duty.

## Using a different controller
Nothing here is locked to the Code 49 or MPD226 specifically — the "Edit MIDI Mapping" step (in the MIDI Controller Settings section) works with any MIDI controller. If you're using something else, just skip straight to that step.

## Also available: Mixxx (free, and works today on more than just Mac)
If you want to test this whole setup without needing a Mac, see [DJ_INSTRUCTIONS_MIXXX.md](DJ_INSTRUCTIONS_MIXXX.md) — Mixxx is a free DJ app that runs on Linux too, and hosts YuVi Glow as an effect via its LV2 build (not VST3 — Mixxx doesn't support that format at all).
