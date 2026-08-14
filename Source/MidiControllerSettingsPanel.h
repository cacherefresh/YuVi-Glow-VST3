#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

// Everything about *configuring* the controller mapping, pulled out of the
// main row flow (plan/issues/18) into one collapsible block, shown/hidden as a
// unit via HeaderBarComponent's MIDI-controller-settings icon (a real on/off
// toggle, not a menu). Deliberately excludes the MIDI Input device dropdown
// and the Save/Load Default for Device buttons — those stay always-visible
// on the main screen.
//
// Inline, not a floating overlay (rev. 2 — the first version floated over
// the rest of the window and ended up covering other controls, which is
// exactly the clutter this was supposed to fix). PluginEditor gives this
// component a zero-height slot when collapsed and `contentHeight` when
// expanded, reflowing everything below it — see PluginEditor::resized().
// Still needs to stay fully interactive rather than modal, though, since
// turning on "Edit MIDI Mapping" here has to be immediately followed by
// clicking pads/knobs/faders elsewhere in the still-visible window.
class MidiControllerSettingsPanel : public juce::Component,
                                     private juce::Timer
{
public:
    // Exact sum of the row heights/gaps resized() lays out below — kept as
    // one named constant so PluginEditor (which decides whether to reserve
    // this much space, based on whether the section is toggled open) has a
    // single source of truth instead of a second hardcoded guess.
    static constexpr int contentHeight = 24 + 10 + 28 + 12 + 28 + 8 + 36 + 12 + 28 + 8 + 20;

    explicit MidiControllerSettingsPanel (YuViGlowAudioProcessor& p) : processor (p)
    {
        addAndMakeVisible (titleLabel);
        titleLabel.setText ("MIDI Controller Settings", juce::dontSendNotification);
        titleLabel.setFont (juce::Font (juce::FontOptions (16.0f, juce::Font::bold)));

        addAndMakeVisible (learnGainButton);
        learnGainButton.onClick = [this] { processor.armLearnGainFader(); };

        addAndMakeVisible (gainLearnedLabel);
        gainLearnedLabel.setJustificationType (juce::Justification::centredLeft);

        addAndMakeVisible (editMappingButton);
        editMappingButton.setClickingTogglesState (true);
        editMappingButton.setColour (juce::TextButton::buttonOnColourId, juce::Colours::orange.withAlpha (0.7f));
        editMappingButton.onClick = [this]
        {
            processor.setEditingMappings (editMappingButton.getToggleState());
        };

        addAndMakeVisible (resetAllMappingsButton);
        resetAllMappingsButton.onClick = [this] { processor.resetAllMappings(); };

        addAndMakeVisible (mappingStatusLabel);
        mappingStatusLabel.setJustificationType (juce::Justification::centredLeft);
        mappingStatusLabel.setFont (juce::Font (juce::FontOptions (13.0f)));

        addAndMakeVisible (learnTapTempoButton);
        learnTapTempoButton.onClick = [this] { processor.armLearnTapTempo(); };

        addAndMakeVisible (tempoStatusLabel);
        tempoStatusLabel.setJustificationType (juce::Justification::centredLeft);

        startTimerHz (15);
    }

    // No paint() override — plain inline rows, no boxed background/border,
    // matching the rest of the main screen (plan/issues/18 rev. 2: this used to be
    // a floating boxed overlay, which both looked out of place and could
    // overlap other controls; now it's just a collapsible block of rows in
    // PluginEditor's normal top-to-bottom flow).

    void resized() override
    {
        auto area = getLocalBounds();
        auto row = [&area] (int h) { return area.removeFromTop (h); };

        titleLabel.setBounds (row (24));
        area.removeFromTop (10);

        auto gainLearnRow = row (28);
        learnGainButton.setBounds (gainLearnRow.removeFromLeft (260));
        gainLearnRow.removeFromLeft (8);
        gainLearnedLabel.setBounds (gainLearnRow);

        area.removeFromTop (12);

        auto mappingButtonRow = row (28);
        editMappingButton.setBounds (mappingButtonRow.removeFromLeft (150));
        mappingButtonRow.removeFromLeft (8);
        resetAllMappingsButton.setBounds (mappingButtonRow.removeFromLeft (150));

        area.removeFromTop (8);

        mappingStatusLabel.setBounds (row (36));

        area.removeFromTop (12);

        auto tapTempoRow = row (28);
        learnTapTempoButton.setBounds (tapTempoRow.removeFromLeft (150));

        area.removeFromTop (8);

        tempoStatusLabel.setBounds (row (20));
    }

private:
    void timerCallback() override
    {
        gainLearnedLabel.setText (processor.isLearningGain() ? "Waiting for fader move..."
                                                                 : processor.getGainFaderDescription(),
                                   juce::dontSendNotification);

        editMappingButton.setToggleState (processor.isEditingMappings(), juce::dontSendNotification);

        if (! processor.isEditingMappings())
        {
            mappingStatusLabel.setText ("Click \"Edit MIDI Mapping\" to (re)capture pads/knobs/faders",
                                         juce::dontSendNotification);
        }
        else
        {
            const int learningSlot = processor.getLearningPadSlot();
            const int learningKnob = processor.getLearningKnobSlot();
            const int learningFader = processor.getLearningFaderSlot();

            if (learningSlot >= 0)
            {
                mappingStatusLabel.setText ("Armed: pad " + juce::String (learningSlot + 1) + " of 16 — press it now",
                                             juce::dontSendNotification);
            }
            else if (learningKnob >= 0)
            {
                mappingStatusLabel.setText ("Armed: knob " + juce::String (learningKnob + 1) + " of 4 — turn it now",
                                             juce::dontSendNotification);
            }
            else if (learningFader >= 0)
            {
                mappingStatusLabel.setText ("Armed: fader " + juce::String (learningFader + 1) + " of 4 — move it now",
                                             juce::dontSendNotification);
            }
            else
            {
                mappingStatusLabel.setText ("Editing: red = unassigned, green = assigned. Click one, "
                                             "or just touch any unassigned pad/knob/fader to fill it in.",
                                             juce::dontSendNotification);
            }
        }

        tempoStatusLabel.setText (processor.isLearningTapTempo() ? "Waiting for tap tempo button press..."
                                                                    : "Tap tempo control: " + processor.getTapTempoDescription(),
                                   juce::dontSendNotification);
    }

    YuViGlowAudioProcessor& processor;

    juce::Label titleLabel;

    juce::TextButton learnGainButton { "Assign Knob to master VST Gain" };
    juce::Label gainLearnedLabel;

    juce::TextButton editMappingButton { "Edit MIDI Mapping" };
    juce::TextButton resetAllMappingsButton { "Reset All Mappings" };
    juce::Label mappingStatusLabel;

    juce::TextButton learnTapTempoButton { "Learn Tap Tempo" };
    juce::Label tempoStatusLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiControllerSettingsPanel)
};
