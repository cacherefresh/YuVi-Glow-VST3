# YuVi-Glow-VST3
Code49 / MPD226 MIDI-mapped effect plugin (VST3 + AU + Standalone) — one codebase, three ways to run it.

## One codebase, three build outputs
This is a single JUCE/CMake project that compiles to:
- **Standalone app** — runs directly, no host needed. Linux and macOS.
- **VST3 effect plugin** — loads in any VST3 host: Mixxx (Linux/Mac/Windows), REAPER, Ableton, etc.
- **AU effect plugin** (macOS only) — loads in Logic, and is the format Serato itself likely requires (VST3 support in Serato is unverified — see `plan/14-serato-effect-workflow.md`).

No source changes are needed to target any of the three — `CMakeLists.txt`'s `FORMATS` list controls which get built.

## Documentation
- Design/architecture: [plan/](plan/) (start at `plan/00-overview.md`)
- Repo conventions for coding agents: [AGENTS.md](AGENTS.md)
- **How to actually use it as a DJ** (host-agnostic workflow): [USER_MANUAL.md](USER_MANUAL.md)
- Build/clean/run commands (Ubuntu dev loop): [DEVELOPER_NOTES.md](DEVELOPER_NOTES.md)
- Running it Standalone on Linux for development: [STANDALONE_INSTRUCTIONS.md](STANDALONE_INSTRUCTIONS.md)
- Using it in Serato DJ Pro (macOS): [DJ_INSTRUCTIONS_SERATO.md](DJ_INSTRUCTIONS_SERATO.md)
- Using it in Mixxx (Linux/Mac/Windows — testable today without a Mac) or any other VST3/AU host: [DJ_INSTRUCTIONS_MIXXX.md](DJ_INSTRUCTIONS_MIXXX.md)

## Test assets
`assets/audio/royaltyfree/` has two CC0 audio files for testing file loading/playback (not just MIDI mapping) — see that folder's `LICENSES.md`.
