# Project Licensing

**Status: [UNRESOLVED]** — `LICENSE` and `CREDITS.md` are written and in place, so this isn't just planning. But one real item is still open: a JUCE forum post (not JUCE's own authoritative terms) suggested AAX/AUv3/VST2/iOS specifically might not be cleanly coverable under AGPL. None of those are formats this project actually builds (it builds classic AU, VST3, and Standalone — not AUv3), so this doesn't block anything currently shipping, but it's still unverified against JUCE's actual license page (https://juce.com/legal/juce-8-licence/) rather than confirmed.

Plan + decision record, 2026-08-08. Covers the actual `LICENSE` and `CREDITS.md` files added to the repo root in this same pass.

## The finding that decided this
While researching a beat-detection library for [16-beat-detection-and-loop-pads.md](16-beat-detection-and-loop-pads.md), it became clear the library choice was almost secondary to a bigger fact: **this project already depends on JUCE**, and JUCE itself is dual-licensed — **AGPLv3, or a paid monthly commercial JUCE license** for closed-source distribution (confirmed via JUCE's own site and forum, `https://juce.com/legal/juce-8-licence/`). There's no way to distribute this plugin without landing on one side of that fork, independent of anything else this project does.

Given the explicit direction to go FOSS as much as possible: **this project is licensed AGPL-3.0**.

## Structure of `LICENSE`
1. **Preamble** — the values statement from `CHAOS_Control.lic` (Robert Lee Coffman / Cache Refresh, referenced from [github.com/cacherefresh/LICENSES](https://github.com/cacherefresh/LICENSES)), explicitly marked as a non-binding statement of intent/values sitting above the actual operative legal terms — it isn't itself a software license (no copyright grant, no permissions/restrictions, no warranty disclaimer), so it's presented as context for *why* this project is licensed the way it is, not as the legal terms themselves.
2. **Operative terms**: the standard GNU AGPL-3.0 text. Verify the copy in this repo against the canonical source (`https://www.gnu.org/licenses/agpl-3.0.txt`) before relying on it for anything consequential — reproduced here from a well-established standard text, but this is a legal document and worth an independent diff, not just trusting a copy-paste.
3. Pointer to `CREDITS.md` for third-party attribution.

## `CREDITS.md`
Every third-party work this project currently uses, per the explicit "give credit, very important" direction:
- **JUCE** (juce-framework/JUCE) — AGPLv3/commercial dual license.
- **libsonare** (libraz/libsonare) — Apache-2.0, planned dependency per `plan/issues/16`, not yet integrated.
- **Two CC0 test audio files** from Wikimedia Commons — already documented in `assets/audio/royaltyfree/LICENSES.md`, cross-referenced from `CREDITS.md` rather than duplicated.
- **The `CHAOS_Control.lic` preamble** itself, credited to its source.

## Open risk, not resolved here — needs your direct verification with JUCE
A JUCE forum post (not JUCE's own authoritative legal text) states that **for AAX, AUv3, VST2, and iOS app formats specifically, the AGPL path doesn't apply even for open-source projects** — implying those specific formats might require a paid commercial JUCE license regardless of this project's own license. This project currently targets **VST3, AU, and Standalone** — AU here likely means the older/non-"v3" Audio Unit format (confirm which one JUCE's build actually produces), and VST3 isn't explicitly named in that caveat, but this needs a direct check against `https://juce.com/legal/juce-8-licence/` (the authoritative source) before treating "AGPL, fully FOSS, no commercial license needed" as settled for every format this project builds. Flagging clearly rather than silently assuming the forum post is complete/current.

## What this means going forward
- New dependencies should be evaluated for AGPL-3.0 compatibility (permissive licenses like Apache-2.0/MIT/BSD are always fine; GPL-family is compatible; anything with a "no derivatives" or "non-commercial only" clause is not).
- Every new third-party dependency or asset gets added to `CREDITS.md` at the time it's added — not batched up later.
