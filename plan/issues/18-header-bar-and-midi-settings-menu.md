# 18 — Header bar + MIDI-controller-settings menu

**Status: [COMPLETED]** — built, rebuilt, and screenshot-verified across both revisions (floating panel, then the inline collapsible replacement).

## Rev 2 (built, 2026-08-13): inline collapsible section, not a floating panel
The first build (below) implemented the MIDI Controller Settings panel as a
floating overlay positioned under the header bar. Real problem, not just a
style complaint: the floating panel **overlapped other live controls** —
confirmed via screenshot, it covered "Load Default for Device" and part of
the MIDI Input dropdown. That's the opposite of decluttering.

Fix: `MidiControllerSettingsPanel` is now laid out **inline**, in
`PluginEditor`'s normal top-to-bottom row flow, positioned right after the
MIDI Input row. The MIDI-controller-settings icon became a real on/off
toggle (`HeaderBarComponent::setClickingTogglesState(true)` on that button,
highlighted with a yellow-tinted background fill when active) instead of a
"click to open a menu" button. `PluginEditor::resized()` reserves
`MidiControllerSettingsPanel::contentHeight` of vertical space when the
toggle is on, and **zero** when it's off — so collapsing it closes the gap
completely and everything below (Save/Load Default, BPM row, pad grid)
shifts straight up, rather than leaving a stray blank row. The window stays
a fixed size always (simplest and safest given this also runs as a hosted
VST3 editor, not just Standalone) — collapsed state just leaves unused
space at the bottom rather than the window shrinking.

Also dropped the panel's boxed dark background/border entirely — it's now
plain rows with no visual container, matching the rest of the main screen,
per explicit user preference over keeping a bordered "this is a distinct
section" look.

Both icons also got real tooltips (`Button::setTooltip()` + a
`juce::TooltipWindow` member added to `PluginEditor`, required for any
`setTooltip()` call to actually render anything): "Settings" and "MIDI
Controller Settings" respectively.

The general Settings (gear) icon's stub panel is unchanged — still a small
floating overlay near the header, left as-is since it has no real content
yet to overlap anything (explicit user call: not worth inline-collapsible
treatment until it actually has contents).

## Why
The main editor window has accumulated a lot of one-off buttons for MIDI
mapping (learn buttons, edit toggle, reset) sitting inline among the
performance controls (load/stop, gain, tap tempo). This declutters the main
screen by moving everything that's about *configuring* the controller
mapping into a menu, reachable via a new icon in a new header bar, while
performance controls (file load/stop, gain, tap tempo itself, pad/knob/
fader grids) stay exactly where they are.

## Decisions (from user Q&A this session)

1. **Two icons, two menus**, both new — neither exists in our code today:
   - A general **"Settings"** icon/panel — stub only for now, no defined
     contents yet. Just a placeholder panel with a "more settings coming
     soon" message so the icon isn't dead-looking.
   - A **"MIDI Controller Settings"** icon/panel — the actual scope of this
     plan.
2. **Exact contents of the MIDI Controller Settings menu** (final scope,
   per mid-conversation correction — narrower than first proposed):
   - Learn Trigger Pad (+ its status label)
   - Learn Gain Fader (+ its status label)
   - Edit MIDI Mapping (toggle)
   - Reset All Mappings
   - Learn Tap Tempo (+ its status label)
   - **Explicitly NOT moved**: MIDI Input device selector, Save as
     Default for Device, Load Default for Device, the Tap Tempo button
     itself (registers a tap — a performance action, not a mapping
     action), BPM field. These all stay on the main screen.
3. **Menu behavior** (superseded — see "Rev 2" above): non-modal overlay
   panel. Clicking the icon toggles it open/closed; while open, it floats
   over the window but does **not** block interaction with the rest of it —
   critical because turning on "Edit MIDI Mapping" from inside the panel
   has to be followed by clicking directly on pads/knobs/faders in the main
   grid, which are not moving into the menu. Opening one panel closes the
   other, so they never overlap each other. **This overlapped real controls
   in practice and was replaced with an inline collapsible section.**
4. **Header bar style**: new — none exists today. A muted/dark bar
   spanning the top of the window, with yellow used only as an accent
   (icon color / thin stripe), not a solid bright-yellow field. Both
   icons sit top-right.
5. **Icons**: hand-drawn vector glyphs (JUCE `Path`/`Graphics` shapes), not
   emoji or external image assets — avoids missing-glyph risk on Linux
   and keeps the dependency footprint at zero. A gear/cog for general
   Settings, a sliders glyph (three vertical tracks with round caps) for
   MIDI Controller Settings.

## What moves out of the main screen, and where

| Control | Was | Now |
|---|---|---|
| Learn Trigger Pad + label | inline row | MIDI Controller Settings panel |
| Learn Gain Fader + label | inline row | MIDI Controller Settings panel |
| Edit MIDI Mapping | inline row | MIDI Controller Settings panel |
| Reset All Mappings | inline row | MIDI Controller Settings panel |
| Learn Tap Tempo + status label | tempo row | MIDI Controller Settings panel |
| mappingStatusLabel (edit-mode instructions) | own row | MIDI Controller Settings panel (pairs naturally with Edit MIDI Mapping) |

Everything else (Load/Stop, file name/status, Input Gain + lock, MIDI
Input device dropdown, Save/Load Default for Device, BPM field + Tap
Tempo button, pad grid, knob/fader panel) is untouched — same components,
same click targets, same behavior.

## New files

- `Source/HeaderBarComponent.h` — draws the header strip, owns the two
  icon buttons, exposes `onSettingsClicked` / `onMidiSettingsClicked`
  callbacks. No knowledge of what's inside either panel.
- `Source/MidiControllerSettingsPanel.h` — houses the five controls above
  plus their two status labels; identical `juce::TextButton`/`juce::Label`
  wiring to what's being removed from `PluginEditor`, just relocated.
- `Source/AppSettingsPanel.h` — stub panel, one label.

`PluginEditor` gains three new child components (header bar + two panels,
panels initially hidden) and loses the six controls listed above. No
`PluginProcessor` changes at all — this is editor-only; every method the
moved buttons call (`armLearnTriggerPad()`, `setEditingMappings()`, etc.)
already exists and is untouched.

## Sizing
Window stays fixed at 540×770 always (rev 2 decision — no dynamic resize
on toggle). Verified by screenshot that the pad grid/knob-fader panel still
fit fully on screen even with the MIDI-settings section expanded.

## Explicitly out of scope
- Contents of the general Settings panel (stub only).
- Any processor/audio-thread change.
- Persisting which panel was last open across restarts (both default closed).
