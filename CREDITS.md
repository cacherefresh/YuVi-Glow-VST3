# Credits

Every third-party work this project uses, updated at the time each one is added — not batched up later. See `LICENSE` for this project's own license (AGPL-3.0) and `plan/issues/17-project-licensing.md` for the reasoning behind it.

## Framework

**[JUCE](https://github.com/juce-framework/JUCE)** — the C++ audio application framework this entire plugin is built on (audio callback, plugin formats, MIDI I/O, GUI, file I/O). Dual-licensed AGPLv3 / commercial. This project uses it under AGPLv3 — see `plan/issues/17-project-licensing.md`. Pinned to tag `8.0.15` in `CMakeLists.txt`.

**JUCE's bundled MP3 decoder** (`JUCE_USE_MP3AUDIOFORMAT`, enabled in `CMakeLists.txt`) — part of the JUCE module above, but called out separately because JUCE's own header carries an explicit disclaimer on it: "NOT guaranteed to be free from infringements of 3rd-party intellectual property... at your own risk." Enabled anyway because MP3 loading is an explicit project requirement (see `assets/WARNING_NON_FREE/`) and the core MP3 patents (Fraunhofer/Technicolor) expired industry-wide around 2017, making real-world risk low today — but flagged here rather than silently assumed, per this file's own "don't fabricate licensing" purpose.

## Integrated dependency

**[libsonare](https://github.com/libraz/libsonare)** by libraz — Apache-2.0. BPM/beat detection, see `plan/issues/16-beat-detection-and-loop-pads.md`. Pinned to tag `v1.6.0` in `CMakeLists.txt`; only `sonare_core`/`sonare_rt` are linked (most of libsonare's other subsystems are disabled at build time — unrelated to this project's actual use of it).

## Audio assets

Two CC0 (public domain) test/dev audio files — full attribution already in [`assets/audio/royaltyfree/LICENSES.md`](assets/audio/royaltyfree/LICENSES.md), summarized here:
- **"Alien Spaceship Atmosphere"** by Kevin MacLeod, via Wikimedia Commons / freepd.com.
- **"War sounds"** by Dragout, via Freesound.org / Wikimedia Commons.

## Values preamble

The preamble in `LICENSE` is adapted from **`CHAOS_Control.lic`** by Robert Lee Coffman (YuVi / Cache Refresh) — [github.com/cacherefresh/LICENSES](https://github.com/cacherefresh/LICENSES).
