#pragma once

#include <JuceHeader.h>

// Shared sizing and state-mirroring for the small unlabelled checkboxes added
// by plan/issues/22 — PadGridComponent's hold-to-play row toggles and
// ControlPanelComponent's fader locks. Extracted because both components had
// the same two non-obvious details written out independently, and one of them
// had a comment pointing at the other file to explain a magic number.
namespace yuviglow::checkbox
{
    // JUCE's ToggleButton derives its tick box from the button's *height* and
    // insets it 4px from the left edge, so square bounds clip the box's right
    // side — it renders as a "C". These are the smallest bounds that fit a
    // full box, hence wider-than-tall.
    inline constexpr int width = 26;
    inline constexpr int height = 20;

    // Mirrors processor-owned state onto a checkbox without firing onClick.
    // Both components poll rather than treating their own clicks as the only
    // source, since host state restore and preset load also move these.
    inline void mirror (juce::ToggleButton& button, bool state)
    {
        if (button.getToggleState() != state)
            button.setToggleState (state, juce::dontSendNotification);
    }
}
