# macOS Build Targets

**Status: [DEVELOPED]** — the CMake Mac-target configuration described below (VST3 + AU + Standalone, universal binary, signing notes) is real, written, and present in `CMakeLists.txt` today. What's not yet done: it has never actually been built/verified on real macOS hardware — this codebase's first-ever Mac compile is happening now, see [DJ_DEVELOPER_ONETIME_BUILD_SERATO.md](../../DJ_DEVELOPER_ONETIME_BUILD_SERATO.md) and `plan/issues/19-serato-mac-demo.md`. Note the "instrument plugin" wording below is also stale — superseded by the MVP's effect-plugin pivot, `plan/issues/12-mvp-v0.md`.

## Toolchain
- CMake + JUCE (pinned via `FetchContent`), Xcode generator.
- C++20.
- Universal binary: arm64 (Apple Silicon) + x86_64 (Intel), macOS 11 (Big Sur) minimum.
- Requires Xcode + command-line tools installed on the build machine (your Mac Pro is fine for this).

## Targets emitted
1. **Standalone app** — `JUCE AudioAppComponent`-based, own CoreAudio/CoreMIDI device selection. This is what actually routes into Serato (see [08](08-serato-integration.md)).
2. **VST3** — instrument plugin, DAW use.
3. **AU** — instrument plugin; matters specifically on Mac since Logic/MainStage/GarageBand only load AU, not VST3.

All three wrap the same `Core` static library — no logic duplicated per target.

## Signing & notarization
- Building and running locally on your own Mac: no Apple Developer account needed, ad-hoc/unsigned is fine.
- **Distributing to anyone else** (including a plugin installer or a DMG for another DJ to install): needs an Apple Developer Program membership ($99/yr) for code signing + notarization, or Gatekeeper will block it on first launch. This only becomes necessary at packaging time (Phase 5) — no need to sign up now.

## CI (optional, later)
GitHub Actions `macos-latest` runner can automate build+test once there's code to build; not a Phase 0/1 concern.
