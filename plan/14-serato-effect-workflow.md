# Using YuVi Glow as a Serato FX-Slot Effect

Plan only, 2026-08-06 — no code changes needed for this. Everything described here (file loading, MPD226 mapping, input gain) already exists as of `plan/12-mvp-v0.md`; what's missing is verified, on-machine deployment into Serato itself, which is Mac-only and can't be done from this Linux sandbox.

**Update, same day**: the "can't verify FX-slot hosting from Linux" gap below is now partially closed — Mixxx is a real, free, cross-platform DJ app that hosts VST3 effects directly and runs on Linux, so the *general* "does this plugin work correctly when a real DJ app hosts it as an FX-slot effect, feeds it deck audio, and the plugin also listens to a MIDI controller directly" question can be tested via Mixxx today. See `DJ_INSTRUCTIONS_MIXXX.md`. What Mixxx *can't* answer is anything Serato-specific (its exact plugin format support, its exact FX-panel UI, its own MIDI-mapping interplay) — those still need the Mac.

## Step 0 (do this first, before anything else): confirm which format Serato actually lists
`plan/08-serato-integration.md` flags this from earlier planning and it's still unverified: **Serato DJ Pro has historically hosted VST2/AU only, not VST3** — despite this repo's name. If that's still true on your installed Serato version, the **AU build** is what goes in the FX slot, not the VST3 build. `CMakeLists.txt` already builds both on macOS for exactly this reason (`FORMATS` includes `AU` only `if(APPLE)`), so nothing needs to change here — just don't assume VST3 is the one to pick.

Action: on the Mac, open Serato's FX/plugin panel and see which plugin formats it actually enumerates. This determines which artifact you load in step 2.

## Step 1: build on macOS
```
cmake -S . -B build -G Xcode
cmake --build build --config Release
```
(`Release` for actual use; `Debug` is fine for testing.) `COPY_PLUGIN_AFTER_BUILD TRUE` in `CMakeLists.txt` makes JUCE auto-install both formats to their standard macOS locations:
- VST3: `~/Library/Audio/Plug-Ins/VST3/YuVi Glow.vst3`
- AU: `~/Library/Audio/Plug-Ins/Components/YuVi Glow.component`

(Unverified from this sandbox — confirm these are populated after building; if Serato doesn't see the AU, it may need an AU cache rescan, e.g. via `auval` or a Serato-side "rescan plugins" action.)

## Step 2: insert into a Serato deck's FX slot
General Serato DJ Pro flow (exact wording may differ by version — verify on your machine, don't take this as gospel):
1. Open Serato's FX panel for the deck you want to route into.
2. Select a plugin-hosted FX slot type (vs. a built-in Serato FX) and choose YuVi Glow from the list — AU or VST3, whichever step 0 showed as available.
3. The deck's live audio now flows through the plugin. This is the same "audio in → audio out" shape Serato expects from any FX — nothing YuVi-Glow-specific about the insertion step itself.

## Step 3: what "loads a track" actually means here (confirmed with you 2026-08-06)
Two different things, and only one is in scope:
- **In scope, already built**: the plugin's own "Load Audio File..." button loads an arbitrary mp3/wav from disk into the plugin's *own* sample buffer, entirely independent of whatever Serato has cued on that deck. The MPD226's mapped trigger pad plays *that* file to completion; the actual Serato deck signal passes through scaled by Input Gain (or forced to unity if gain-locked) and sums with it.
- **Out of scope**: reading/knowing what track Serato itself has loaded on the deck. Serato doesn't expose a file path, metadata, or seek access to FX-slot plugins — only a live PCM signal. Not pursuing this; would require unsupported reverse-engineering of Serato's internals.

So in practice: load your Serato deck track as normal in Serato, and separately use the plugin's own file picker to load whatever you want the MPD226 pad to trigger (could be totally unrelated content — a vocal stab, a drop, anything).

## Step 4: MPD226 alongside Serato — check for double-mapping
YuVi Glow opens its **own direct MIDI connection** to the MPD226 (via the plugin's MIDI device dropdown), independent of the host — this already works and doesn't change when running inside Serato's FX slot, since CoreMIDI allows multiple simultaneous listeners on one MIDI source.

**New consideration once actually inside Serato**: check Serato's own MIDI Setup panel (Setup → MIDI, or equivalent) to see if the MPD226 is *also* assigned there to native Serato functions (cue points, loops, etc.). If it is, physical pad presses would trigger both Serato's native mapping *and* YuVi Glow's mapping simultaneously — not a conflict exactly (no crash, no exclusive-lock issue), but potentially confusing double-behavior. Decide deliberately: either leave the MPD226 unassigned in Serato's native MIDI setup so it's dedicated to YuVi Glow, or intentionally dual-purpose it.

## Step 5: reaching the plugin's own editor window inside Serato
Serato's FX slot UI should offer a way to open/expand the plugin's native editor (typical for any VST/AU host) — that's where the Load Audio File button, MIDI device dropdown, Edit MIDI Mapping toggle, Input Gain slider/lock, and the pad/knob/fader grid all live, same as the Standalone build we've been testing on Linux. Untested: whether Serato's window chrome handles the plugin's current fixed 540x700 size gracefully, or whether it needs to be resizable/scrollable inside Serato's own FX panel. Flag if this looks cramped once you're there — it's a quick JUCE-side fix if so.

## Acceptance checklist (run once on the Mac)
- [ ] Serato's FX panel shows YuVi Glow as an available plugin (confirms step 0's format question).
- [ ] Inserting it into a deck's FX slot doesn't error and audio passes through when a track plays on that deck.
- [ ] Plugin's own editor window opens and all controls are reachable (Load File, Stop, Input Gain + lock, MIDI device dropdown, Edit MIDI Mapping, pad grid, control panel, Save/Load Default).
- [ ] MPD226 selectable in the plugin's MIDI device dropdown while Serato is running (confirms no exclusive-lock conflict with Serato's own MIDI handling).
- [ ] "Load Default for Device" restores a previously-captured MPD226 mapping (if one was saved on this Mac before) — or run "Edit MIDI Mapping" fresh if not.
- [ ] Load a file via the plugin's own picker, trigger it via the mapped pad, confirm it plays to completion mixed with the deck's live audio.
- [ ] Toggle Input Gain lock, confirm the deck signal is forced to unity and the physical fader stops affecting it.
