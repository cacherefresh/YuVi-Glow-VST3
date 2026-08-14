#pragma once

#include <JuceHeader.h>

// Stub for the general app-settings panel (plan/issues/18) — contents aren't
// defined yet. Exists so the header bar's Settings icon isn't dead-looking;
// fill in as real settings get added.
class AppSettingsPanel : public juce::Component
{
public:
    AppSettingsPanel()
    {
        addAndMakeVisible (titleLabel);
        titleLabel.setText ("Settings", juce::dontSendNotification);
        titleLabel.setFont (juce::Font (juce::FontOptions (16.0f, juce::Font::bold)));

        addAndMakeVisible (placeholderLabel);
        placeholderLabel.setText ("More settings coming soon.", juce::dontSendNotification);
        placeholderLabel.setJustificationType (juce::Justification::centredLeft);
    }

    void paint (juce::Graphics& g) override
    {
        g.setColour (juce::Colour (0xff1c1a17).withAlpha (0.97f));
        g.fillRoundedRectangle (getLocalBounds().toFloat(), 6.0f);
        g.setColour (juce::Colours::yellow.withAlpha (0.4f));
        g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (0.5f), 6.0f, 1.5f);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced (12);
        titleLabel.setBounds (area.removeFromTop (24));
        area.removeFromTop (10);
        placeholderLabel.setBounds (area.removeFromTop (20));
    }

private:
    juce::Label titleLabel;
    juce::Label placeholderLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AppSettingsPanel)
};
