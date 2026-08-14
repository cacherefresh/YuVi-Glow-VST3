# YuVi Glow — Using It in Mixxx

**Status: DRAFT**, matches the current build (`plan/issues/12-mvp-v0.md`, `plan/issues/20-mvp-beta.md`). No coding knowledge needed for anything below. The LV2 build itself is verified (builds clean, produces a well-formed plugin bundle, Mixxx's installed binary directly links the LV2 host library it needs) — actually seeing it appear and load inside Mixxx's own Effects UI has **not** been visually confirmed yet (a rendering issue in the dev sandbox blocked that specific check) — worth confirming yourself the first time you try this.

## What's Mixxx?
[Mixxx](https://mixxx.org) is a free, open-source DJ application — similar idea to Serato, but free and available on Linux, Mac, and Windows. **Correction, 2026-08-13**: this doc previously said Mixxx hosts VST3 directly — checked properly (inspecting the actual installed binary, not just assuming) and that's wrong. Mixxx has **no VST/VST3 support at all**; it only hosts its own native effects and **LV2** plugins. YuVi Glow now builds an LV2 target specifically so Mixxx can load it directly (Linux only for now) — that's what this doc actually walks through below.

## What you need
- A computer running Mixxx **on Linux** — this doc is specifically the LV2 path (see "What's Mixxx?" above for why). Mixxx on Mac/Windows can't load YuVi Glow at all today (no LV2 build for those platforms yet); use the "other DAWs" section at the bottom instead (REAPER/Ableton/Logic via VST3/AU), or [DJ_INSTRUCTIONS_SERATO.md](DJ_INSTRUCTIONS_SERATO.md) on a Mac.
- An M-Audio Code 49, Akai MPD226, or honestly any class-compliant MIDI controller.

## Getting YuVi Glow's plugin installed
There's no installer — the plugin has to be built from source once per machine (or per code update). That's a separate, developer-facing process: see [DJ_DEVELOPER_ONETIME_BUILD_MIXXX.md](DJ_DEVELOPER_ONETIME_BUILD_MIXXX.md) for the exact steps. Come back here once "YuVi Glow" is actually showing up in Mixxx's Effects panel.

## Load two songs and mix them (plain Mixxx, no YuVi Glow yet)
If you're new to Mixxx itself, this is the bare minimum to get two tracks playing and crossfading — skip ahead if you already know your way around it.
1. Open the **Library** panel (left side) and browse to a folder with some tracks — or drag files in directly from your file manager.
2. Drag a track onto **Deck 1** (or double-click it with Deck 1 selected/loaded). Drag a second track onto **Deck 2**.
3. Click each deck's **Play** button (▶) to start them. Use each deck's own volume fader to get levels roughly even.
4. Use the **crossfader** (the horizontal slider at the bottom-center) to blend between Deck 1 and Deck 2 — full left = only Deck 1 audible, full right = only Deck 2, center = both.

That's the whole "two decks, mixing" loop — everything below layers YuVi Glow on top of it.

## Loading YuVi Glow in Mixxx
1. Open Mixxx's **Effects** panel/preferences.
2. "YuVi Glow" should be listed alongside Mixxx's built-in native effects (sourced from `~/.lv2/` — see the developer doc if it's not there).
3. **Decide where to put it** — this changes what it actually does to your mix:
   - **On Deck 1 or Deck 2** (one of the two decks you're actively mixing): that deck's live track passes through YuVi Glow — gain-controlled by its own "Input Gain (Knob 1)" — and whatever sample you trigger on YuVi Glow's pads gets mixed in on top of *that deck's* audio. You keep crossfading Deck 1↔2 exactly as above; YuVi Glow just rides along on whichever one you picked.
   - **On a 3rd/4th deck, or a Sampler slot**, left empty (no track loaded on it — silence flowing through): keeps YuVi Glow fully independent of Deck 1/2. Use that channel's own fader to bring YuVi Glow's triggered sample in and out on its own, alongside your normal Deck 1↔2 crossfade. If you only have 2 decks enabled, turn on more in Preferences → Decks first.

Either way, once assigned, open YuVi Glow's own plugin window from the effect slot — that's the same window/controls as the Standalone build, described below.

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
