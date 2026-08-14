# WARNING: non-free test assets — do not commit

**Everything placed in this folder is assumed non-free / not confirmed-licensed, and is git-ignored on purpose.** This README is the one exception — it's the only file in this folder that's actually tracked in git, which is what keeps the folder (and this warning) present across every clone even though nothing else here is.

## Why this folder exists
Testing YuVi Glow's file-loading and playback needs real song audio, not just the two CC0 dev clips in `assets/audio/royaltyfree/`. Files dropped here (e.g. `.mp3`s "possibly given for development testing") may be real, copyrighted songs whose licensing hasn't been checked and can't be assumed free — so they're for **local use only**, never committed, shipped, or redistributed.

## The rule
- **Nothing in this folder except this README may ever be committed.** `.gitignore` enforces this at the repo root:
  ```
  assets/WARNING_NON_FREE/*
  !assets/WARNING_NON_FREE/README_WARNING_NON_FREE.md
  ```
- This pattern covers flat files directly in this folder. If files ever get organized into subfolders here, the `.gitignore` pattern would need updating too — git can't un-ignore a file inside an already-ignored subdirectory without an extra rule for that subdirectory.
- Consistent with this project's existing ground rules in `AGENTS.md`: **"Don't commit large binaries"** and **"Don't fabricate licensing"** — this folder is the non-free counterpart to `assets/audio/royaltyfree/`, which only holds verified-CC0 files.

If a file placed here turns out to be genuinely free-licensed and worth keeping for real, move it into `assets/audio/royaltyfree/` instead, with its license verified and recorded in that folder's `LICENSES.md` — not left in this one.
