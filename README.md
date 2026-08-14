# YuVi-Glow-VST3
Code49 / MPD226 MIDI-mapped effect plugin (VST3 + AU + LV2 + Standalone) — one codebase, four build outputs.

## One codebase, four build outputs
This is a single JUCE/CMake project that compiles to:
- **Standalone app** — runs directly, no host needed. Linux and macOS.
- **VST3 effect plugin** — loads in most VST3 hosts: REAPER, Ableton, etc. (**not** Mixxx — see below.)
- **AU effect plugin** (macOS only) — loads in Logic, and is the format Serato itself likely requires (VST3 support in Serato is unverified — see `plan/issues/14-serato-effect-workflow.md`).
- **LV2 effect plugin** (Linux only) — the format Mixxx actually hosts. Mixxx has **no VST/VST3 support** (verified by inspecting the installed binary — a claim this repo's own docs got wrong for a while, now corrected); it links `liblilv` and hosts LV2 plugins instead, which is why this target exists.

No source changes are needed to target any of these — `CMakeLists.txt`'s `FORMATS` list controls which get built.

## Documentation
- Design/architecture: [plan/](plan/) (start at `plan/00-overview.md`; what's still outstanding is tracked in `plan/00-OPEN_ISSUES.md`)
- Repo conventions for coding agents: [AGENTS.md](AGENTS.md)
- **How to actually use it as a DJ** (host-agnostic workflow): [USER_MANUAL.md](USER_MANUAL.md)
- Build/clean/run commands (Ubuntu dev loop): [DEVELOPER_NOTES.md](DEVELOPER_NOTES.md)
- Running it Standalone on Linux for development: [STANDALONE_INSTRUCTIONS.md](STANDALONE_INSTRUCTIONS.md)
- Using it in Serato DJ Pro (macOS): [DJ_INSTRUCTIONS_SERATO.md](DJ_INSTRUCTIONS_SERATO.md)
- One-time Mac build steps to get it into Serato in the first place: [DJ_DEVELOPER_ONETIME_BUILD_SERATO.md](DJ_DEVELOPER_ONETIME_BUILD_SERATO.md)
- Using it in Mixxx (Linux, via the LV2 build — testable today without a Mac) or any other VST3/AU host: [DJ_INSTRUCTIONS_MIXXX.md](DJ_INSTRUCTIONS_MIXXX.md)
- One-time Linux build steps to get it into Mixxx in the first place: [DJ_DEVELOPER_ONETIME_BUILD_MIXXX.md](DJ_DEVELOPER_ONETIME_BUILD_MIXXX.md)

## Test assets
`assets/audio/royaltyfree/` has two CC0 audio files for testing file loading/playback (not just MIDI mapping) — see that folder's `LICENSES.md`.

## License
[AGPL-3.0](LICENSE) — chosen because this project depends on JUCE, which requires AGPLv3 or a paid commercial license (see `plan/issues/17-project-licensing.md` for the full reasoning). Third-party attribution for everything this project uses: [CREDITS.md](CREDITS.md).
