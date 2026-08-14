# Credits

Every third-party work this project uses, updated at the time each one is added — not batched up later. See `LICENSE` for this project's own license (AGPL-3.0) and `plan/issues/17-project-licensing.md` for the reasoning behind it.

## Framework

**[JUCE](https://github.com/juce-framework/JUCE)** — the C++ audio application framework this entire plugin is built on (audio callback, plugin formats, MIDI I/O, GUI, file I/O). Dual-licensed AGPLv3 / commercial. This project uses it under AGPLv3 — see `plan/issues/17-project-licensing.md`. Pinned to tag `8.0.15` in `CMakeLists.txt`.

## Planned dependency (not yet integrated)

**[libsonare](https://github.com/libraz/libsonare)** by libraz — Apache-2.0. Planned for BPM/beat/downbeat detection, see `plan/issues/16-beat-detection-and-loop-pads.md`. Will be added here properly (with pinned version) once actually integrated.

## Audio assets

Two CC0 (public domain) test/dev audio files — full attribution already in [`assets/audio/royaltyfree/LICENSES.md`](assets/audio/royaltyfree/LICENSES.md), summarized here:
- **"Alien Spaceship Atmosphere"** by Kevin MacLeod, via Wikimedia Commons / freepd.com.
- **"War sounds"** by Dragout, via Freesound.org / Wikimedia Commons.

## Values preamble

The preamble in `LICENSE` is adapted from **`CHAOS_Control.lic`** by Robert Lee Coffman (YuVi / Cache Refresh) — [github.com/cacherefresh/LICENSES](https://github.com/cacherefresh/LICENSES).
