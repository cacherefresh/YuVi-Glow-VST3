# Stem Separation

**Status: [PLANNED]** — no code exists yet (no ONNX Runtime dependency, no `StemSeparator`).

## Approach
Pretrained 4-stem source separation model (Demucs `htdemucs`-family, MIT licensed) exported to ONNX, run via **ONNX Runtime** (C++) inside the app/plugin, offline per loaded file.

## Pipeline
1. Load file → resample to the model's expected sample rate.
2. Chunked inference (song split into overlapping windows to bound memory/CPU).
3. Reconstruct 4 stem buffers: vocals, drums, bass, other.
4. Cache result as `.wav` per stem in `~/Library/Application Support/YuViGlow/cache/<file-hash>/`, so reloading the same song skips re-separation entirely.
5. UI shows a progress/"analyzing…" state during first-time separation; full mix plays immediately while separation runs in the background, then the mixer ([07](07-mixer.md)) goes live once stems are ready.

## Distribution decision (needs a call before Phase 3)
Model weights are tens–hundreds of MB — **do not commit them directly to git**. Two options:
- **Download-on-first-launch** (recommended): app fetches the ONNX weights from a stable URL on first run, verifies a checksum, caches locally. Keeps the repo light, sidesteps repo-size/notarization bloat.
- **Git LFS**: keeps everything self-contained in-repo but bloats clone size and requires collaborators to have LFS set up.

Recommendation: download-on-first-launch, documented clearly in [DJ_INSTRUCTIONS_SERATO.md](../../DJ_INSTRUCTIONS_SERATO.md) as a one-time setup step (needs an internet connection the first time only). (Renamed from `DJ_INSTRUCTIONS.md` since this was written — see `plan/issues/11-open-questions-assumptions.md` item 12.)

## Performance note
CPU-only Demucs inference on a full song can take real time (seconds to low minutes depending on Mac hardware) — not viable to run live mid-set on a song you haven't preloaded. Practical DJ workflow: pre-load and pre-separate tracks before a set, same as prepping crates in Serato today.
