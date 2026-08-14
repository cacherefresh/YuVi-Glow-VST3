# YuVi Glow — One-Time Linux Build for Mixxx (Developer Steps)

**This is the developer/build doc — building the plugin and getting it into Mixxx.** For actually using it once it's loaded (loading tracks, mixing, triggering pads), see [DJ_INSTRUCTIONS_MIXXX.md](DJ_INSTRUCTIONS_MIXXX.md) instead. Same split as the Mac/Serato docs: this one is a one-time (or per-code-change) setup step, that one is what you touch every session.

**Status**: the LV2 build itself is verified — builds clean, installs to the right place, produces a well-formed plugin bundle (manifest inspected directly). Actually seeing "YuVi Glow" appear and load inside Mixxx's own Effects UI has **not** been visually confirmed yet — a rendering issue in the dev sandbox blocked that specific check (Mixxx's window won't screen-capture here, even in `--safe-mode`). Confirm it yourself the first time through; the steps below should get you there.

## Why LV2, not VST3
Mixxx has **no VST/VST3 support at all** — confirmed by inspecting the installed binary directly (zero "vst2"/"vst3" strings in it), not just assumed. It does host **LV2** plugins for real (it links `liblilv`, the standard LV2 hosting library). So YuVi Glow builds an LV2 target specifically for Mixxx — see `plan/issues/20-mvp-beta.md` for the full story of how this was figured out.

## Building it
Nothing new here beyond the normal Linux dev loop — LV2 builds automatically alongside VST3/Standalone whenever you build on Linux, no extra flag needed:
```bash
bash scripts/linux_dev_env.sh
```
(Or see [DEVELOPER_NOTES.md](DEVELOPER_NOTES.md) for the plain `cmake`/`cmake --build` commands if you'd rather not use the wrapper script.) This also auto-installs the LV2 bundle via `COPY_PLUGIN_AFTER_BUILD`:
```
~/.lv2/YuVi Glow.lv2/
```

## Verify the bundle before touching Mixxx
```bash
ls ~/.lv2/"YuVi Glow.lv2"/
```
Should show `manifest.ttl`, `dsp.ttl`, `ui.ttl`, and `libYuVi Glow.so`. If you want to eyeball that it's well-formed, `manifest.ttl` and `dsp.ttl` are plain-text Turtle — readable directly, no special tool needed. There's no `lv2info`/`lv2ls` validator installed in this environment; if you have one, it's a faster sanity check than opening Mixxx each time.

## Launching Mixxx
```bash
mixxx
```
A few things you might hit on first launch (all encountered getting this far in this session — probably specific to this sandboxed dev environment, may not apply to your normal desktop session at all):

- **First-run "Choose music library directory" dialog**: pick anything (e.g. `~/Music`) and click Open. One-time only.
- **A `symbol lookup error` mentioning `/snap/core20/...libpthread.so.0`**: this happened when launching from a shell that had VS Code's Snap environment variables leaking into it (`SNAP_*`, `XDG_DATA_DIRS`, `LOCPATH` pointing into `/snap/code/...`). If you hit this, launch from a plain terminal instead of one spawned inside/by a Snap app, or strip those specific env vars first:
  ```bash
  env -u SNAP -u SNAP_NAME -u SNAP_REVISION -u SNAP_ARCH -u SNAP_LIBRARY_PATH -u SNAP_COMMON -u SNAP_DATA \
      -u SNAP_USER_DATA -u SNAP_USER_COMMON -u SNAP_REAL_HOME -u SNAP_EUID -u SNAP_UID -u SNAP_INSTANCE_NAME \
      -u SNAP_CONTEXT -u SNAP_COOKIE -u SNAP_VERSION -u SNAP_LAUNCHER_ARCH_TRIPLET -u XDG_DATA_DIRS \
      -u XDG_DATA_HOME -u LOCPATH -u GTK_PATH -u GTK_EXE_PREFIX \
      mixxx
  ```

## Getting YuVi Glow showing up in Mixxx's effects
1. Open Mixxx's **Effects** preferences (gear/Preferences → Effects, or the Effects panel's own settings).
2. Let it (re)scan for effects if it doesn't pick up new ones automatically — it should find "YuVi Glow" alongside its built-in native effects, sourced from `~/.lv2/`.
3. If it's not listed: confirm the bundle is actually in `~/.lv2/` (see above), restart Mixxx (it scans for effects at launch), and check Mixxx's own log/console output for any LV2-loading error mentioning "YuVi Glow."

## Choosing where to put it
YuVi Glow is an *effect* — it gets assigned to whichever channel (Deck or Sampler) you choose in Mixxx's Effects panel, it doesn't create its own separate channel automatically. Two setups both work — pick based on what you're testing:
- **On one of your two mixing decks** (e.g. Deck 1): that deck's live track passes through YuVi Glow, gain-controlled by its "VST MASTER Gain," with your pad-triggered sample mixed in on top. You crossfade Deck 1↔2 exactly as normal.
- **On a dedicated 3rd/4th deck or a Sampler slot**, left empty (no track loaded — silence flowing through it): keeps YuVi Glow fully independent of your two actual mixing decks, closer to a "third channel just for this." Enable additional decks in Mixxx's Preferences → Decks if you only have 2 active by default.

Either way, once assigned, open YuVi Glow's own plugin window from the effect slot — same controls as the Standalone/VST3 builds (Load Audio File, MIDI Input dropdown, MIDI Controller Settings, VST MASTER Gain, pad grid).

## Troubleshooting
- **Effects panel doesn't show an LV2 section at all / no native effects either**: something's more broadly wrong with Mixxx's effects engine, not LV2-specific — check Mixxx's own log output for startup errors.
- **YuVi Glow shows up but won't load / errors on load**: capture the exact error text rather than guessing — this is genuinely the first time this plugin has been loaded into Mixxx by anyone, so there's no prior troubleshooting history to draw on.
- **No sound at all once loaded**: check Mixxx's own Preferences → Sound Hardware setup first (output device selected, not muted) before assuming it's a YuVi Glow problem — same class of check as any other effect.
- **MPD226 not showing up in YuVi Glow's own MIDI Input dropdown**: make sure it's not exclusively claimed by Mixxx's own Controller preferences in a way that blocks other apps from also opening it — YuVi Glow opens its own direct MIDI connection independent of the host, same as it does in Serato, but some OS/driver combinations only allow one exclusive listener per device.

## Rebuilding after a code change
Same as always:
```bash
bash scripts/linux_dev_env.sh
```
Restart Mixxx afterward if it was already running — like any host, it won't pick up a plugin binary that changed underneath it mid-session.
