#pragma once

#include <JuceHeader.h>

// Thin strip across the top of the editor holding two icon buttons: a
// general app-settings icon (stub panel for now, see AppSettingsPanel.h)
// and the MIDI-controller-settings icon (see MidiControllerSettingsPanel.h).
// This component only draws the bar and the two icons and forwards clicks
// via callbacks — it owns none of the panels it opens, so it has no idea
// what "settings" actually means. See plan/issues/18.
class HeaderBarComponent : public juce::Component
{
public:
    std::function<void()> onSettingsClicked;
    std::function<void()> onMidiSettingsClicked;

    HeaderBarComponent()
    {
        settingsButton.setTooltip ("Settings");
        midiSettingsButton.setTooltip ("MIDI Controller Settings");

        // The MIDI-settings icon is a real on/off toggle (plan/issues/18 rev. 2 —
        // an inline collapsible section, not a popup menu), so its pressed
        // state persists and is read back by PluginEditor after each click
        // to decide whether to show the section — see isMidiSettingsActive().
        midiSettingsButton.setClickingTogglesState (true);

        settingsButton.onClick = [this] { if (onSettingsClicked) onSettingsClicked(); };
        midiSettingsButton.onClick = [this] { if (onMidiSettingsClicked) onMidiSettingsClicked(); };
        addAndMakeVisible (settingsButton);
        addAndMakeVisible (midiSettingsButton);
    }

    bool isMidiSettingsActive() const { return midiSettingsButton.getToggleState(); }
    void setMidiSettingsActive (bool active) { midiSettingsButton.setToggleState (active, juce::dontSendNotification); }

    void paint (juce::Graphics& g) override
    {
        const auto barColour = juce::Colour (0xff2a2620); // dark, warm-neutral
        const auto accent = juce::Colours::yellow.withAlpha (0.65f);

        g.setColour (barColour);
        g.fillRect (getLocalBounds());

        // Thin accent stripe along the bottom edge — the only yellow here.
        g.setColour (accent);
        g.fillRect (getLocalBounds().removeFromBottom (2));
    }

    void resized() override
    {
        constexpr int iconSize = 26;
        constexpr int margin = 8;
        auto area = getLocalBounds().reduced (margin);
        midiSettingsButton.setBounds (area.removeFromRight (iconSize).withSizeKeepingCentre (iconSize, iconSize));
        area.removeFromRight (margin);
        settingsButton.setBounds (area.removeFromRight (iconSize).withSizeKeepingCentre (iconSize, iconSize));
    }

private:
    // Hand-drawn vector icons rather than emoji/image assets — avoids
    // missing-glyph fallback risk on minimal Linux font setups.
    struct GearIconButton : public juce::Button
    {
        GearIconButton() : juce::Button ("settings") {}

        void paintButton (juce::Graphics& g, bool isMouseOver, bool) override
        {
            const auto bounds = getLocalBounds().toFloat();
            const auto centre = bounds.getCentre();
            const float outerR = bounds.getWidth() * 0.46f;
            const float innerR = outerR * 0.52f;
            const auto colour = juce::Colours::yellow.withAlpha (isMouseOver ? 0.95f : 0.75f);

            g.setColour (colour);
            constexpr int numTeeth = 8;
            for (int i = 0; i < numTeeth; ++i)
            {
                const float angle = (float) i / (float) numTeeth * juce::MathConstants<float>::twoPi;
                juce::Path tooth;
                tooth.addRectangle (-1.6f, -outerR - 2.5f, 3.2f, 5.0f);
                tooth.applyTransform (juce::AffineTransform::rotation (angle).translated (centre));
                g.fillPath (tooth);
            }

            g.drawEllipse (juce::Rectangle<float> (outerR * 2.0f, outerR * 2.0f).withCentre (centre), 2.2f);
            g.drawEllipse (juce::Rectangle<float> (innerR * 2.0f, innerR * 2.0f).withCentre (centre), 2.0f);
        }
    };

    struct SlidersIconButton : public juce::Button
    {
        SlidersIconButton() : juce::Button ("midiSettings") {}

        void paintButton (juce::Graphics& g, bool isMouseOver, bool) override
        {
            const auto bounds = getLocalBounds().toFloat();

            if (getToggleState())
            {
                g.setColour (juce::Colours::yellow.withAlpha (0.22f));
                g.fillRoundedRectangle (bounds.expanded (4.0f), 4.0f);
            }

            const auto colour = juce::Colours::yellow.withAlpha (isMouseOver || getToggleState() ? 0.95f : 0.75f);
            g.setColour (colour);

            constexpr int numTracks = 3;
            const float trackW = bounds.getWidth() / (float) numTracks;
            const float knobPositions[numTracks] = { 0.35f, 0.65f, 0.5f };

            for (int i = 0; i < numTracks; ++i)
            {
                const float x = bounds.getX() + trackW * (i + 0.5f);
                g.drawLine (x, bounds.getY() + 2.0f, x, bounds.getBottom() - 2.0f, 2.0f);

                const float knobY = bounds.getY() + bounds.getHeight() * knobPositions[i];
                g.fillEllipse (juce::Rectangle<float> (6.0f, 6.0f).withCentre ({ x, knobY }));
            }
        }
    };

    GearIconButton settingsButton;
    SlidersIconButton midiSettingsButton;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HeaderBarComponent)
};
