# 21 — MVP-BETA-MIXXX

**Status: [PLANNED]** — architecture worked out and confirmed feasible (with one real adjustment from how it was first proposed), nothing built yet. Explicitly a **separate track from MVP-BETA-SERATO**, not a prototyping step subordinate to it — the user's own call, even though the core new capability (live audio capture) would likely end up useful for Serato too eventually.

## Context
Goal: use Mixxx on this Linux machine to build a **live audio capture + beat-slice + pad-trigger** workflow — record incoming audio (e.g. a live instrument, Deck 3's own playback, a mic), slice it at beat boundaries using YuVi Glow's already-built tempo detection, and trigger the slices live via the MPD226's pads — then land the result back in Mixxx on a 4th deck with full, real vinyl/scratch control (stop = pause, backspin = rewind).

The first version of this idea (Deck 3 muted → YuVi Glow effect → output straight back into Deck 4, all live) turned out to hit two real Mixxx/DJ-software constraints — not vague concerns, specific and confirmed:
1. **Mixxx processes effects *after* the deck fader**, quote from the manual: "Effects are processed after the deck faders and crossfader." Muting Deck 3's fader would cut the signal before it ever reached an effect — nothing to capture.
2. **An effect's processed output always returns to the same channel it came from** — there's no send/return mechanism to route Deck 3 through an effect and get the result on Deck 4. Confirmed by an open, unresolved Mixxx GitHub issue (#13185) asking for exactly this kind of flexible pre-fader/cross-channel routing — still unbuilt upstream.
3. **Vinyl/DVS scratch control fundamentally requires a track already loaded from disk** — something seekable, with a past to scratch backward into. A live incoming audio stream has no "backward." This isn't a Mixxx limitation specifically; no DJ software can apply real scratch semantics to a live stream. Confirmed with the user this is an acceptable adjustment, not a blocker: capture, then export to a file, then load that file onto Deck 4 like any normal track — at which point Mixxx's own mature, already-working vinyl-control support applies to it for real, because it's no longer live.

## Architecture (confirmed achievable)

```
Deck 3 (Mixxx)                    YuVi Glow Standalone           Deck 4 (Mixxx)
─────────────────                 ─────────────────────         ───────────────
gain: healthy level        →      audio IN (already exists,      capture exported
  ("just before yellow")          currently muted by default     → WAV file, loaded
channel fader: muted/down         for feedback safety — needs    manually onto
  (kept out of Mixxx's own        unmuting + correct device      Deck 4 like any
  master mix)                     selection)                     other track
                                          │
                    "external mixer      ▼ pad-triggered
                     mode" routes        playback of
                     Deck 3 to its       captured/sliced
                     own soundcard       segments (new
                     output → an OS      capture feature,
                     loopback device                    → full native Mixxx
                                                            vinyl/scratch
                                                            control from here
                                                            (mouse-driven —
                                                            no DVS hardware
                                                            on this machine)
```

1. **Deck 3 → YuVi Glow Standalone (live, real-time)**: Mixxx's "external mixer mode" (a real, existing feature — outputs each deck to its own separate soundcard output) points Deck 3's output at a virtual/loopback audio device instead of real hardware. YuVi Glow Standalone's audio input — already exists, it's the "Audio input muted to avoid feedback loop" bar visible at the top of every Standalone screenshot this whole session, just currently defaulted off — reads from that same loopback device.
2. **Capture, inside YuVi Glow (new feature, not built)**: record the incoming audio into a buffer for N beats (using the already-built BPM/tempo detection from `plan/issues/16` to know where the beats are), then let the pads (MPD226) trigger playback of slices within that captured buffer — the same pad-trigger mechanism that already exists for *loaded files*, extended to also work on *captured* audio.
3. **Export, inside YuVi Glow (new feature, not built)**: a manual, deliberate action (not automatic/continuous — confirmed with the user, simpler to build and reason about first) that prints the captured buffer to a WAV file on disk.
4. **File → Deck 4 (existing Mixxx functionality, zero new code)**: the user loads that exported file onto Deck 4 through Mixxx's own normal track browser, same as any track. From that point, Deck 4 has full real vinyl-control — tested here via Mixxx's mouse-driven virtual scratch on the waveform (no DVS timecode-vinyl hardware on this machine), stop/pause and backspin/rewind all working for real because it's a genuinely loaded, seekable file.

## What's explicitly NOT being pursued
Real-time scratch/vinyl control applied directly to a live audio stream — confirmed fundamentally not possible (no DJ software can do this), not just hard. The capture-then-export-then-load pattern above is the deliberate, confirmed replacement for that idea, not a compromise nobody signed off on.

## New work required
- **Live audio capture buffer** in `PluginProcessor` — record incoming audio (the existing audio-in bus, already wired for the pass-through-gain feature) into an internal buffer for a defined duration, likely beat-count-driven using the existing `TempoDetector`/BPM state from `plan/issues/16`.
- **Pad-trigger playback of captured slices** — extends the existing pad-trigger system (`triggerCuePad()` / `startPlaybackFromSample()`, see `plan/issues/22`) to operate on the captured buffer, not just the file loaded via "Load Audio File." Note this changed after this doc was written: `triggerOrStopPadLoop()` no longer exists, and pads address cue points rather than beat-aligned loop regions.
- **Export-to-file action** — a new UI action (button, or a new pad role — TBD) that writes the current captured buffer to a WAV file at a known location.
- **OS-level audio routing setup** (not code — environment/config): an ALSA loopback device (`snd-aloop`) or a PipeWire virtual device, so Mixxx's Deck 3 output can actually reach YuVi Glow Standalone's input. Not yet set up on this machine — real prerequisite, not yet started.
- **Mixxx-side configuration**: enabling external mixer mode, pointing Deck 3's output at the loopback device, unmuting/configuring YuVi Glow Standalone's audio input. Not yet done.

## Open questions (not yet resolved)
- Exact UI for triggering capture start/stop and export — new button? A dedicated pad role, similar to how pad 2 became the momentary-play exception (`plan/issues/20`)? Not decided.
- Where captured/exported files land on disk, and whether they need cleanup/management over a session (could accumulate).
- Whether captured-buffer pad-triggering should reuse the *exact* same pad-role logic as loaded-file triggering (pad 1 = whole-capture restart-on-repress, pad 2 = momentary, pads 1/3/4 = beat-region once BPM known) or needs its own distinct behavior — leaning toward reusing the existing model rather than inventing a second one, but not confirmed.

## Verification plan (once built)
1. Set up the OS-level loopback device; confirm with basic tools (e.g. `arecord`/`aplay` or `pw-link`) that audio genuinely flows through it before involving Mixxx or YuVi Glow at all.
2. Configure Mixxx: Deck 3 → external mixer mode → loopback output. Play a known test track (e.g. one of the CC0 files in `assets/audio/royaltyfree/`) on Deck 3, confirm via the loopback's own monitoring that signal is present.
3. Point YuVi Glow Standalone's audio input at the loopback, unmute it, confirm (via a VU-meter-style check or just listening) that Deck 3's audio is actually arriving inside YuVi Glow.
4. Once capture/export exist: capture N beats from Deck 3, export, load the resulting file onto Deck 4, confirm playback, confirm pause-on-stop and backspin-rewind behave like any normal loaded Mixxx track.
