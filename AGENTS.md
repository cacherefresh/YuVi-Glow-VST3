# AGENTS.md

Instructions for any coding agent (Claude Code or otherwise) working in this repo.

## What this repo is
YuVi Glow: a JUCE-based instrument + DJ companion built primarily for the M-Audio Code 49, and equally for an Akai MPD226 (16 pads + first fader — same control surface, different brand). One codebase builds three targets — Standalone, VST3, and AU (macOS only) — so the exact same plugin runs in Serato (macOS), Mixxx (Linux/Mac/Windows), any other VST3/AU host, or standalone with no host at all. Full design context lives in [plan/](plan/) — read `plan/00-overview.md` first, then the rest in numeric order. Non-technical usage docs: [DJ_INSTRUCTIONS_SERATO.md](DJ_INSTRUCTIONS_SERATO.md), [DJ_INSTRUCTIONS_MIXXX.md](DJ_INSTRUCTIONS_MIXXX.md) (also covers other DAWs at the bottom), [STANDALONE_INSTRUCTIONS.md](STANDALONE_INSTRUCTIONS.md) (Linux dev usage).

## Ground rules
- **Both macOS and Linux are real distribution targets, not just macOS with Linux as a dev sandbox.** Serato (the original motivating use case) is Mac/Windows-only, so that path needs macOS. But Mixxx — a real, free, cross-platform DJ app — hosts the exact same VST3 on Linux, Mac, and Windows as an effect, which makes Linux a legitimate end-user target too, not merely where development happens. See `plan/13-linux-dev-testing.md` (dev loop), `plan/14-serato-effect-workflow.md` (Serato), and `DJ_INSTRUCTIONS_MIXXX.md` (Mixxx / other hosts). Don't build anything that only works on one platform without a real reason.
- **Never hardcode logic to one controller model.** All physical-control → parameter bindings go through MIDI-learn (see `plan/02-midi-mapping-code49.md`), which is what makes the same code work unmodified with the Code 49's pads/fader *or* the Akai MPD226's pads/fader *or* any other class-compliant MIDI controller — the plugin just binds whatever note/CC the selected device actually sends. Don't add a Code49-specific or MPD226-specific code path; if a future feature genuinely needs per-device defaults, put them in a config file (e.g. `config/code49-default-mapping.json`), never in code.
- **Audio thread discipline**: no allocation, no file I/O, no blocking calls (including ML inference) on the audio callback thread. Stem separation, key/BPM detection, and thumbnail generation belong on background threads (see `plan/01-architecture.md`).
- **Don't commit large binaries.** Stem-separation model weights and any bundled sample libraries must not be committed directly to git — see `plan/05-stem-separation.md` and `plan/11-open-questions-assumptions.md` for the download-on-first-launch approach and licensing constraints. Flag before adding anything over a few MB.
- **Don't fabricate licensing.** Any sample audio (sax multisamples, demo tracks) must have a real, checkable royalty-free/CC0 source recorded in `plan/11-open-questions-assumptions.md` or a `LICENSES/` file. Never invent or assume a license.
- **Serato only hosts VST3/AU as audio effects (audio in → audio out), never as MIDI instruments.** Any plugin meant to load inside Serato must have an audio input (`IS_SYNTH FALSE`), even if its main job is triggering its own sample playback — see `plan/08-serato-integration.md` and `plan/12-mvp-v0.md` for how the MVP satisfies this while still reacting to Code 49 hardware. A true MIDI-instrument target (no audio in) can only ever be the standalone app, never something loaded inside Serato.
- **Plugins get hardware control by opening their own direct MIDI connection to the device** (CoreMIDI on macOS, ALSA on Linux — both handled transparently by `juce::MidiInput`), not by relying on host-forwarded MIDI — Serato doesn't reliably forward hardware MIDI into effect plugins, but the OS MIDI layer allows multiple simultaneous listeners on one source, so the plugin can listen to the controller directly alongside Serato (or, on Linux, alongside nothing — there's no Serato there) without conflict.
- **Follow the phase order in `plan/10-roadmap.md`** (currently reprioritized by `plan/12-mvp-v0.md`). Don't jump ahead to stem separation or the sax engine before the current MVP is working end-to-end — each phase has its own approval checkpoint with the repo owner before implementation starts.

## Build
CMake + JUCE via `FetchContent` (pinned tag, see `CMakeLists.txt`). Xcode generator on macOS (`plan/09-macos-build-targets.md`), Unix Makefiles/Ninja on Linux (`plan/13-linux-dev-testing.md`). Formats: VST3 everywhere, AU macOS-only, Standalone everywhere (the Standalone build is the fast local dev-loop on Linux, since there's no Serato here to load a VST3 into). For exact commands (build/clean/run on Ubuntu) see [DEVELOPER_NOTES.md](DEVELOPER_NOTES.md) — keep it in sync with `plan/13-linux-dev-testing.md` if either changes.

**Compiling on macOS and loading in real Serato has NOT been verified as of the last update to this file** — only Linux builds have been smoke-tested. Don't report a build "done" without one of: an actual macOS compile + Serato load test, or an explicit note that it's still unverified on the real target. See `plan/12-mvp-v0.md`'s "Known limitation" section.

## Claude Code skills to use while working here
- **`/run`** — build and actually launch the plugin (or a throwaway JUCE AudioPluginHost/standalone harness) to verify a change works, not just that it compiles. Prefer this over claiming something works from reading the code.
- **`/simplify`** — run after finishing a feature, before calling it done: cleanup pass for reuse/simplification in the DSP and UI code.
- **`/security-review`** — run before any build gets distributed to another person (Phase 5 packaging, or sooner if sharing a build ad hoc).
- These are invoked via slash command or the Skill tool; don't skip straight past them just because the task feels code-only.

## Code style
- C++20.
- JUCE naming/style conventions (PascalCase classes, camelCase members, `juce::` namespace usage as JUCE itself does).
- Prefer JUCE's own DSP/utility classes (`juce::dsp::FFT`, `juce::Synthesiser`, `juce::AudioThumbnail`, etc.) over reinventing them.
- **Organize like a human would: reusable components, no giant monolithic files, but don't over-abstract.** When the same pattern gets written out 2-3+ times (it happened with pad/fader/knob MIDI-learn logic — three near-identical copies before `Source/MidiLearnBank.h` existed), extract it into its own small, concrete class, not a generic framework. When a file is accumulating genuinely distinct responsibilities (audio callback + MIDI device connection management both used to live in `PluginProcessor.cpp` before `Source/MidiDeviceManager.h/.cpp` existed), split by responsibility. Keep the processor's public API stable across such refactors — UI components shouldn't need to change just because internals got reorganized.

## Test/dev assets
`assets/audio/royaltyfree/` holds small CC0 audio files for exercising file loading/playback in tests — not a curated end-user demo pack (that's still open, `plan/11` item 8). Every file in there needs its license verified on the actual source page (not assumed) and recorded in that folder's `LICENSES.md`, per the "don't fabricate licensing" rule above.

## Scope discipline
Don't add speculative features, alternate DAW targets, or abstractions beyond what the current roadmap phase calls for. If something in `plan/` looks wrong or outdated once real hardware/code exists, update the relevant `plan/*.md` file rather than silently diverging from it.
